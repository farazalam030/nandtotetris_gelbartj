#pragma once

#include <string>
#include <fstream>
#include <set>
#include <map>
#include "Shared.h"

class Parser
{
public:
	Parser(const std::string& filename);
	bool didFailOpen();
	bool hasMoreCommands(); 
	void advance();
	Command commandType();
	Command currCommandType = Command::NONE;
	std::string arg1(); 
	const int arg2();
	std::string& getCurrLine();
	void close();

	~Parser();

private:
	bool failedOpen = false;
	void removeWhitespace(std::string& line);
	std::ifstream vmFile;
	std::string currLine;
	std::set<std::string> calledFunctions;
};

