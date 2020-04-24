#include "Parser.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <map>
#include <filesystem>

using namespace std;

const set<string> Parser::arithCommands = { "add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not" };
const map<string, Command> Parser::commandMap = {
	{ "pop",      Command::C_POP },
	{ "push",     Command::C_PUSH },
	{ "if-goto",  Command::C_IF },
	{ "goto",     Command::C_GOTO },
	{ "function", Command::C_FUNCTION },
	{ "call",     Command::C_CALL },
	{ "return",   Command::C_RETURN },
	{ "label",    Command::C_LABEL },
	{ "//",       Command::COMMENT }
};

Parser::Parser(string filename)
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

void Parser::removeWhitespace(string& line) {
	// Trim left whitespace
	size_t charsStart = line.find_first_not_of(" \t");
	if (charsStart != string::npos)
		line.erase(0, charsStart);
	else { // entire line is whitespace
		line.clear();
		return;
	}

	size_t slashPos = line.find("//");
	if (slashPos != string::npos && slashPos > 0) { // remove inline comments only
		line.erase(slashPos);
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
		removeWhitespace(currLine);
		if (currLine.length() == 0) {
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
		return Command::C_ARITHMETIC;
	}

	auto foundCommand = commandMap.find(commandString);
	if (foundCommand != commandMap.end()) {
		return foundCommand->second;
	}

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

string& Parser::getCurrLine() {
	return currLine;
}

void Parser::close() {
	if (vmFile.is_open()) {
		vmFile.close();
	}
}

Parser::~Parser()
{
}
