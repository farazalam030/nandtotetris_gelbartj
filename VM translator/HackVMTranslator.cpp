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
    vector<filesystem::path> files;

    if (dotLoc == string::npos) {
        isDirectory = true;
    }

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
            if (p.path().extension().string() == ".vm")
                files.push_back(p.path());
        }
    }

    string outputName = fileOrDir.substr(0, fileOrDir.find_first_of(".")) + ".asm";
    CodeWriter codeWriter(outputName);
    if (isDirectory) {
        codeWriter.writeInit();
    }
    for (auto it = files.begin(); it != files.end(); ++it) {
        if (*it == "Sys.vm") { // make sure Sys.vm is always translated first if it exists
            iter_swap(it, files.begin());
            break;
        }
    }
    for (auto& file : files) {
        std::cout << "Loading " << file << endl;
        Parser parser(file.string());
        if (parser.didFailOpen()) {
            return -1;
        }
        codeWriter.setFilename(file.filename().string());

        while (parser.hasMoreCommands()) {
            parser.advance();
            if (parser.currCommandType == Command::C_RETURN) {
                codeWriter.writeReturn();
                continue;
            }
            else if (parser.currCommandType == Command::COMMENT) {
                codeWriter.writeComment(parser.getCurrLine());
                continue;
            }
            string arg1 = std::move(parser.arg1());

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
                        // fall through
                    case Command::C_POP:
                        codeWriter.writePushPop(parser.currCommandType, arg1, arg2);
                        break;
                    case Command::C_CALL:
                        codeWriter.writeCall(arg1, arg2);
                        break;
                    case Command::C_FUNCTION:
                        codeWriter.writeFunction(arg1, arg2);
                        break;
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
