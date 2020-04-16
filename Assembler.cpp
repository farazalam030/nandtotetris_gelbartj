#include "Assembler.h"
#include <iostream>
#include <string>
#include <sstream>
#include <regex>
#include <bitset>
#include <chrono>

using namespace std;

Assembler::Assembler(string &filename): filename(filename) {
	cout << "Trying to open filename " << this->filename << endl;
	assemblyFile.open(filename);

	if (!assemblyFile.is_open()) {
		cout << "Unable to open file" << endl;
		return;
	}

	auto start = chrono::high_resolution_clock::now();
	getLoopAddresses();
	auto stop = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << "Got loop addresses in " << duration.count() << "ms." << endl;

	start = chrono::high_resolution_clock::now();
	convertCommands();
	stop = chrono::high_resolution_clock::now();
	duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << "Converted commands in " << duration.count() << "ms." << endl;
}

void Assembler::getLoopAddresses() {
	string line;
	int instructionNum = 0;
	while (getline(assemblyFile, line)) {
		removeComments(line);
		if (line.length() == 0) continue;
		if (line.at(0) == '(') {
			line.erase(0, 1);
			line.erase(line.length() - 1); // cut off closing parens, assuming correct syntax
			symbolTable.emplace(line, instructionNum);
			continue;
		}
		codeNoCommentsLabels << line << endl;
		++instructionNum;
	}
}

void Assembler::convertCommands() {
	codeNoCommentsLabels.seekg(0);

	string newFilename = filename.substr(0, filename.length() - 4); // cut off 3-character extension
	newFilename += ".hack";
	cout << "Printing to filename " << newFilename << endl;

	string line;
	binaryFile.open(newFilename);
	while (getline(codeNoCommentsLabels, line)) {
		if (line.at(0) == '@') {
			binaryFile << convertACommand(line) << endl;
		}
		else {
			binaryFile << convertCCommand(line) << endl;
		}
	}
	binaryFile.close();
}

void Assembler::removeComments(string &line) {
	size_t slashPos = line.find_first_of("//");
	if (slashPos == 0) { // entire line is a comment
		line.clear();
		return;
	}
	if (slashPos != string::npos)
		line.erase(slashPos);

	// Trim left whitespace
	size_t charsStart = line.find_first_not_of(" \t");
	if (charsStart != string::npos)
		line.erase(0, charsStart);
	else { // entire line is whitespace
		line.clear(); 
		return; 
	}

	// Trim right whitespace
	size_t charsEnd = line.find_last_not_of(" \t");
	if (charsEnd != string::npos)
		line.erase(charsEnd + 1);
}

bool is_number(const std::string& s) {
	// Note: does not work for UTF-8 strings
	return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

string Assembler::convertACommand(string& command) {
	// Can assume passed a command starting with "@"
	command.erase(0,1);

	if (is_number(command)) {
		int binary = stoi(command);
		return bitset<16>(binary).to_string();
	}

	auto [it, madeInsertion] = symbolTable.try_emplace(command, latestFreeMem);
	if (madeInsertion) ++latestFreeMem;

	return bitset<16>(it->second).to_string();
}

string Assembler::convertCCommand(string_view command) {
	string jumpBits = "000";
	string compBits;
	string destBits = "000";
	size_t jumpStart = command.find_first_of(';');
	if (jumpStart != string::npos) {
		// Don't need out_of_range catch because spec of the program assumes no errors in input file
		jumpBits = jumpMap.at(command.substr(jumpStart + 1));
	}
	size_t equals = command.find_first_of('=');
	if (equals != string::npos) {
		destBits = destMap.at(command.substr(0, equals));
		compBits = compMap.at(command.substr(equals + 1, jumpStart));
	}
	else {
		compBits = compMap.at(command.substr(0, jumpStart));
	}

	return "111" + compBits + destBits + jumpBits;
}

Assembler::~Assembler() {
	// File stream destructor will close automatically
	// if (assemblyFile.is_open())
	//	assemblyFile.close();
}
