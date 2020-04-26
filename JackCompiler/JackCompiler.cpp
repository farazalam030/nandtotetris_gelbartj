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

    for (auto& file : files) {
        cout << "Loading " << file << "..." << endl;

        string vmOutput = file.substr(0, file.find_last_of(".") + 1) + "vm";
        JackTokenizer tokenizer(file);
        VMWriter vm(vmOutput);
        CompilationEngine ce(tokenizer, vm);

        if (vm.didFailOpen()) {
            return -1;
        }

        vm.close();
        cout << "===========" << endl;
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Finished analysis in " << duration.count() << "ms." << endl;

}
