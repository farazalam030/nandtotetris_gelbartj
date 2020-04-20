#include "Parser.h"
#include "CodeWriter.h"
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>

using namespace std;

int main(int argc, char *argv[])
{
    /*
    const char* commandArray[] = { "NOT_FOUND", "C_ARITHMETIC", "C_PUSH", "C_POP", "C_LABEL", "C_GOTO", "C_IF", "C_FUNCTION", "C_RETURN", "C_CALL" };
    */
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
    vector<string> files;

    if (dotLoc == string::npos) {
        isDirectory = true;
    }
    if (isDirectory) {
        filesystem::directory_iterator dirIt;
        try {
            dirIt = filesystem::directory_iterator(fileOrDir);
        }
        catch (filesystem::filesystem_error) {
            cout << "Invalid directory name, unable to open. Closing..." << endl;
            return -1;
        }
        for (auto& p : dirIt) {
            if (p.path().extension().string() == ".vm")
                files.push_back(p.path().filename().string());
        }
    }
    else {
        files.push_back(fileOrDir);
    }
    string outputName = fileOrDir.substr(0, fileOrDir.find_first_of(".")) + ".asm";
    CodeWriter codeWriter(outputName);
    if (isDirectory) {
        codeWriter.writeInit();
    }
    string fileWithDir;
    for (auto it = files.begin(); it != files.end(); ++it) {
        if (*it == "Sys.vm") { // make sure Sys.vm is always translated first if it exists
            iter_swap(it, files.begin());
            break;
        }
    }
    for (auto& file : files) {
        cout << "Loading " << file << endl;
        fileWithDir = isDirectory ? fileOrDir + "/" + file : fileOrDir; // forward slash OK even on Windows
        Parser parser(fileWithDir);
        if (parser.didFailOpen()) {
            return -1;
        }
        codeWriter.setFilename(file);

        while (parser.hasMoreCommands()) {
            parser.advance();
            if (parser.currCommandType == Command::C_RETURN) {
                codeWriter.writeReturn();
                continue;
            }
            string arg1 = parser.arg1();
            if (parser.currCommandType == Command::C_ARITHMETIC) {
                codeWriter.writeArithmetic(arg1);
                continue;
            }
            else if (parser.currCommandType == Command::C_GOTO) {
                codeWriter.writeGoto(arg1);
                continue;
            }
            else if (parser.currCommandType == Command::C_IF) {
                codeWriter.writeIf(arg1);
                continue;
            }
            else if (parser.currCommandType == Command::C_LABEL) {
                codeWriter.writeLabel(arg1);
                continue;
            }
            int arg2 = parser.arg2();
            if (arg2 != NAN) {
                if (parser.currCommandType == Command::C_POP || parser.currCommandType == Command::C_PUSH) codeWriter.writePushPop(parser.currCommandType, arg1, arg2);
                else if (parser.currCommandType == Command::C_CALL) {
                    codeWriter.writeCall(arg1, arg2);
                }
                else if (parser.currCommandType == Command::C_FUNCTION) {
                    codeWriter.writeFunction(arg1, arg2);
                }
            }
        }
        parser.close();
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Finished VM translation in " << duration.count() << "ms." << endl;

    
    codeWriter.close();

}
