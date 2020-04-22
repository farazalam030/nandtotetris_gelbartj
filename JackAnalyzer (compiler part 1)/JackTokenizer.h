#pragma once
#include "Shared.h"
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

class JackTokenizer
{
public:
	JackTokenizer(std::string& inputFilename, std::string& outputFilename);
	JackTokenizer(std::string& inputFilename);
	bool hasMoreTokens();
	void advance(); // call only if hasMoreTokens(). Initially there is no current token
	Token tokenType();
	Keyword keyword(); // call only if tokenType == KEYWORD
	char symbol(); // call only if tokenType == SYMBOL
	std::string identifier(); // if tokenType == IDENTIFIER. output <, >, " and & as &lt;, &gt;, &quote; and &amp;
	int intVal(); // if tokenType == INT_CONST
	std::string stringVal(); // if tokenType == STRING_CONST. Do not include double-quotes
	void writeCurrToken(std::ofstream& destination);
	std::streampos currPos();
	std::ofstream& getOutputFile();
	void close();
	bool didFailOpen();
	~JackTokenizer();
private:
	std::ifstream inFile;
	std::ofstream outFile;
	std::string currToken;
	std::stringstream strippedText;
	void advanceWord();
	std::string currWord;
	// char currChar;
	Token currTokenType = Token::NONE;
	bool failedOpen = false;
	static const std::map<std::string, Keyword> keywordList;
	static const std::set<char> symbols;
	void stripAllComments();
	bool removeComments(std::string& line);
};

