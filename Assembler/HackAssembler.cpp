// HackAssembler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Assembler.h"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv)
{
    string filename;

    if (argc <= 1) {
        cout << "Enter filename: ";
        cin >> filename;
    }
    else {
        filename = argv[1];
    }

    Assembler assembler(filename);

    return 0;
}
