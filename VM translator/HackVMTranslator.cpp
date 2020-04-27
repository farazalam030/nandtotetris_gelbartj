#include "Parser.h"
#include "CodeWriter.h"
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <vector>

using namespace std;

static set<string> calledFunctions = { "Sys.init", "String.intValue" };

// static const vector<string> osClasses = { "Array", "Keyboard", "Math", "Memory", "Output", "Screen", "String", "Sys" };

int main(int argc, char *argv[])
{
    string fileOrDir;

    if (argc > 1) {
        fileOrDir = argv[1];
    }
    else {
        std::cout << "Enter .vm filename or directory: ";
        std::cin >> fileOrDir;
        std::cout << endl;
    }

    auto start = chrono::high_resolution_clock::now();
    auto dotLoc = fileOrDir.find_first_of(".");
    bool isDirectory = false;
    vector<filesystem::path> files;

    if (dotLoc == string::npos) {
        isDirectory = true;
    }

    static vector<string> os = { "Array.vm", "Keyboard.vm", "Math.vm", "Memory.vm", "Output.vm", "Screen.vm", "String.vm", "Sys.vm" };

    filesystem::directory_iterator dirIt;
    try {
        dirIt = filesystem::directory_iterator(isDirectory ? fileOrDir : ".");
    }
    catch (filesystem::filesystem_error) {
        std::cout << "Invalid directory name, unable to open. Closing..." << endl;
        return -1;
    }
    for (auto& p : dirIt) {
        if (!isDirectory) {
            if (p.path().filename().string() == fileOrDir) {
                files.push_back(p.path());
                break;
            }
        }
        else {
            if (p.path().extension().string() == ".vm" && std::find(os.begin(), os.end(), p.path().filename().string()) == os.end() )
                files.push_back(p.path());
        }
    }

    string outputName = fileOrDir.substr(0, fileOrDir.find_first_of(".")) + ".asm";
    CodeWriter codeWriter(outputName);
    if (isDirectory) {
        codeWriter.writeInit();
    }

    int numFunctions = 0;
    for (auto& file : files) {
        std::cout << "Loading " << file.string() << " for dead code analysis" << endl;
        Parser parser(file.string());
        if (parser.didFailOpen()) {
            return -1;
        }
        // std::cout << "Checking for dead code..." << endl;
        while (parser.hasMoreCommands()) {
            parser.advance();
            if (parser.commandType() == Command::C_CALL) {
                // cout << "Found call to " << parser.arg1() << endl;
                calledFunctions.insert(parser.arg1());
            }
            else if (parser.commandType() == Command::C_FUNCTION) {
                ++numFunctions;
            }
        }
        parser.close();
    }
    
    int loopNums = 0;
    // std::cout << "User files loop" << endl;
    while (true) {
        ++loopNums;
        // std::cout << "~~~~~~~~~~~~~~~~~~~~" << endl << "Loop " << loopNums << endl;
        int newCalledFunctions = 0;
        for (auto& file : files) {
            // std::cout << "Loading " << file.string() << " for dead code analysis" << endl;
            Parser parser(file.string());
            if (parser.didFailOpen()) {
                return -1;
            }
            // std::cout << "Checking for dead code..." << endl;
            bool inCalledFunction = false;
            while (parser.hasMoreCommands()) {
                parser.advance();
                if (parser.commandType() == Command::C_RETURN) {
                    inCalledFunction = false;
                }
                else if (parser.commandType() == Command::C_FUNCTION) {
                    inCalledFunction = calledFunctions.find(parser.arg1()) != calledFunctions.end();
                }
                else if (parser.commandType() == Command::C_CALL && inCalledFunction) {
                    if (calledFunctions.find(parser.arg1()) == calledFunctions.end()) {
                        calledFunctions.insert(parser.arg1());
                        // std::cout << "Adding " << parser.arg1() << " to called functions list" << endl;
                        ++newCalledFunctions;
                    }
                }
            }
            parser.close();
        }
        if (newCalledFunctions == 0) break;
    }
    // std::cout << endl << "Entering OS analysis loop" << endl;
    loopNums = 0;
    
    while (true) {
        ++loopNums;
        // std::cout << "~~~~~~~~~~~~~~~~~~~~" << endl << "Loop " << loopNums << endl;
        int newCalledFunctions = 0;
        for (auto& osFile : os) {
            // std::cout << "Loading " << file.string() << " for dead code analysis" << endl;
            Parser parser(fileOrDir + "\\" + osFile);
            if (parser.didFailOpen()) {
                return -1;
            }
            // std::cout << "Checking for dead code..." << endl;
            bool inCalledFunction = false;
            while (parser.hasMoreCommands()) {
                parser.advance();
                if (parser.commandType() == Command::C_RETURN) {
                    inCalledFunction = false;
                }
                else if (parser.commandType() == Command::C_FUNCTION) {
                    if (loopNums == 1) ++numFunctions;
                    inCalledFunction = calledFunctions.find(parser.arg1()) != calledFunctions.end();
                }
                else if (parser.commandType() == Command::C_CALL && inCalledFunction) {
                    if (calledFunctions.find(parser.arg1()) == calledFunctions.end()) {
                        calledFunctions.insert(parser.arg1());
                        // std::cout << "Adding " << parser.arg1() << " to called functions list" << endl;
                        ++newCalledFunctions;
                    }
                    
                }
            }
            parser.close();
        }
        if (newCalledFunctions == 0) break;
    }
    std::cout << "Completed dead code analysis. Out of " << numFunctions << " functions, only " << calledFunctions.size() << " are called." << endl;
    
    if (isDirectory) {
        std::for_each(os.begin(), os.end(), [&](auto& file) { file = fileOrDir + "\\" + file; });
        std::for_each(files.begin(), files.end(), [](auto& file) { os.push_back(file.string()); });
    }
    else {
        os.clear();
        std::for_each(files.begin(), files.end(), [](auto& file) { os.push_back(file.string()); });
    }
    

    for (auto& file : os) {
        std::cout << "Loading " << file << " for translation" << endl;
        Parser parser(file);
        if (parser.didFailOpen()) {
            return -1;
        }
        codeWriter.setFilename(file.substr(file.find_last_of("\\") + 1));

        bool doNotAdvance = false;
        while (parser.hasMoreCommands()) {
            if (!doNotAdvance) parser.advance();
            else doNotAdvance = !doNotAdvance;
            if (parser.currCommandType == Command::C_RETURN) {
                codeWriter.writeReturn();
                continue;
            }
            else if (parser.currCommandType == Command::COMMENT) {
                codeWriter.writeComment(parser.getCurrLine());
                continue;
            }
            const string arg1 = std::move(parser.arg1());

            switch (parser.currCommandType) {
                case Command::C_ARITHMETIC:
                    codeWriter.writeArithmetic(arg1);
                    continue;
                case Command::C_GOTO:
                    codeWriter.writeGoto(arg1);
                    continue;
                case Command::C_IF:
                    codeWriter.writeIf(arg1);
                    continue;
                case Command::C_LABEL:
                    codeWriter.writeLabel(arg1);
                    continue;
            }

            int arg2 = parser.arg2();
            if (arg2 != NAN) {
                switch (parser.currCommandType) {
                    case Command::C_PUSH:
                        parser.advance();
                        if (parser.currCommandType == Command::C_POP) {
                            codeWriter.writePush(arg1, arg2, true);
                            codeWriter.writePop(parser.arg1(), parser.arg2(), true);
                        }
                        else {
                            codeWriter.writePush(arg1, arg2);
                            doNotAdvance = true;
                        }
                        break;
                    case Command::C_POP:
                        codeWriter.writePop(arg1, arg2);
                        break;
                    case Command::C_CALL:
                        codeWriter.writeCall(arg1, arg2);
                        break;
                    case Command::C_FUNCTION:
                        // cout << "Checking function " << arg1 << "..." ;
                        if (calledFunctions.find(arg1) != calledFunctions.end()) {
                            // cout << "Found!" << endl;
                            codeWriter.writeFunction(arg1, arg2);
                        }
                        else {
                            // cout << "Function " << arg1 << " not found in calledFunctions list" << endl;
                            
                            //if (arg1.substr(0, arg1.find_first_of(".")) != "Screen") {
                                while (parser.currCommandType != Command::C_RETURN) {
                                    // cout << "Skipping command " << parser.getCurrLine() << endl;
                                    parser.advance();
                               }
                            //}
                            //else {
                            //    codeWriter.writeFunction(arg1, arg2);
                            //}
                            
                        }
                        break;
                }
            }
        }
        parser.close();
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    std::cout << "Finished VM translation in " << duration.count() << "ms." << endl;

    
    codeWriter.close();

}
