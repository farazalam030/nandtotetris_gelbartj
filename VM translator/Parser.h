#pragma once

#include <string>
#include <fstream>
#include <set>
#include <map>
#include "Shared.h"

class Parser
// Ignore all white space and comments in vm file
{
public:
	Parser(std::string filename); // open file
	bool didFailOpen();

	bool hasMoreCommands(); // more commands in current input?
	void advance(); // reads next command and makes it current command. only if hasMoreCommands()

	Command commandType(); // return C_ARITHMETIC for all arithmetic/logical commands
	Command currCommandType = Command::NONE;
	std::string arg1(); // returns first arg of current command. for C_ARITHMETIC, command itself (add, sub) is returned.
						// should not be called if current command is C_RETURN

	int arg2(); // returns second argument if current command is C_PUSH, C_POP, C_FUNCTION or C_CALL

	std::string& getCurrLine();
	void close();

	~Parser();

private:
	bool failedOpen = false;
	void removeWhitespace(std::string& line);
	std::ifstream vmFile;
	std::string currLine;
	static const std::set<std::string> arithCommands;
	static const std::map<std::string, Command> commandMap;
};

