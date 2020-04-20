#include "Parser.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <filesystem>

using namespace std;

const set<string> Parser::arithCommands = { "add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not" };

Parser::Parser(string& filename)
{
	vmFile.open(filename);
	if (vmFile.is_open()) {
		cout << "Opened file " << filename << endl;
	}
	else {
		cout << "Error opening file " << filename << " for parsing" << endl;
		failedOpen = true;
	}
}

bool Parser::didFailOpen()
{
	return failedOpen;
}

bool Parser::hasMoreCommands()
{
	return vmFile.good(); // file is open and has not reached eof
}

void Parser::removeComments(string& line) {
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

void Parser::advance()
{
	if (hasMoreCommands()) {
		getline(vmFile, currLine);
		// cout << "currLine before removing comments: " << currLine << endl;
		removeComments(currLine);
		// cout << "currLine after removing comments: " << currLine << endl;
		if (currLine.length() == 0) {
			// cout << "Line is now blank. Advancing." << endl;
			advance();
		}
		if (currLine.length() > 0) currCommandType = commandType();
	}
	else {
		currCommandType = Command::NONE;
		cout << "Reached end of file." << endl;
	}
}

Command Parser::commandType()
{
	string commandString = currLine.substr(0, currLine.find_first_of(" "));
	if (arithCommands.find(commandString) != arithCommands.end()) {
		// cout << "Command " << commandString << " identified as arithmetic" << endl;
		return Command::C_ARITHMETIC;
	}

	// Switch command doesn't work for strings. Inelegant but only choice for enums.
	else if (commandString == "pop") return Command::C_POP;
	else if (commandString == "push") return Command::C_PUSH;
	else if (commandString == "if-goto")  return Command::C_IF;
	else if (commandString == "goto") return Command::C_GOTO;
	else if (commandString == "function") return Command::C_FUNCTION;
	else if (commandString == "call") return Command::C_CALL;
	else if (commandString == "return") return Command::C_RETURN;
	else if (commandString == "label") return Command::C_LABEL;

	else {
		cout << "Command " << commandString << " not found." << endl;
		return Command::NONE;
	}
}

string Parser::arg1()
{
	auto firstSpace = currLine.find_first_of(" ");
	auto lastSpace = currLine.find_last_of(" ");
	if (firstSpace != string::npos) {
		return currLine.substr(firstSpace + 1, lastSpace - firstSpace - 1);
	}
	else return currLine;
}

bool is_number(const std::string& s) {
	// Note: does not work for UTF-8 strings
	return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

int Parser::arg2()
{
	if (currCommandType == Command::C_PUSH || currCommandType == Command::C_POP || currCommandType == Command::C_FUNCTION || currCommandType == Command::C_CALL) {
		string arg2String = currLine.substr(currLine.find_last_of(" ") + 1);
		if (is_number(arg2String))
			return stoi(arg2String);	
	}
	return NAN;
}

void Parser::printCurrLine() {
	cout << currLine << endl;
}

void Parser::close() {
	if (vmFile.is_open()) {
		vmFile.close();
	}
}

Parser::~Parser()
{
}
