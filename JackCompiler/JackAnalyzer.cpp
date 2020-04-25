#include "JackTokenizer.h"
#include "CompilationEngine.h"
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
    string fileOrDir;

    if (argc > 1) {
        fileOrDir = argv[1];
    }
    else {
        std::cout << "Enter .jack filename or directory: ";
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
            if (p.path().extension().string() == ".jack")
                files.push_back(p.path().string());
        }
    }
    else {
        files.push_back(fileOrDir);
    }
    
    // string fileWithDir;

    for (auto& file : files) {
        cout << "Loading " << file << endl;

        // string tokenOutput = fileWithDir.substr(0, fileWithDir.find_last_of(".")) + "T.xml";
        string parserOutput = file.substr(0, file.find_last_of(".")) + ".xml";
        JackTokenizer tokenizer(file); //  , tokenOutput);
        JackTokenizer* tokenPointer = &tokenizer;
        CompilationEngine ce(tokenPointer, parserOutput, false);

        if (ce.didFailOpen()) {
            return -1;
        }

        ce.close();
        cout << "===========" << endl;

        /*
        while (tokenizer.hasMoreTokens()) {
            // cout << "Calling advance from JackAnalyzer" << endl;
            tokenizer.advance();
            // cout << "Calling writeCurrToken from JackAnalyzer" << endl;
            tokenizer.writeCurrToken(tokenizer.getOutputFile());
        }
        tokenizer.close();
        */
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Finished analysis in " << duration.count() << "ms." << endl;

}
