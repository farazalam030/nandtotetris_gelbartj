#pragma once
#include "Shared.h"
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>

class JackTokenizer
{
public:
	JackTokenizer(const std::string& inputFilename);
	bool hasMoreTokens();
	void advance();
	Token tokenType();
	Keyword keyword();
	char symbol();
	std::string& identifier();
	int intVal();
	std::string& stringVal();
	int currLineNum();
	std::ofstream& getOutputFile();
	void close();
	bool didFailOpen();
	bool aborted();
	~JackTokenizer();
private:
	std::ifstream inFile;
	std::ofstream outFile;
	std::string currToken;
	std::stringstream strippedText;
	void advanceWord();
	std::string currWord;
	std::map<int, int> lineNums;
	bool abortFlag = false;
	Token currTokenType = Token::NONE;
	bool failedOpen = false;
	void stripAllComments();
	bool removeComments(std::string& line);
};

