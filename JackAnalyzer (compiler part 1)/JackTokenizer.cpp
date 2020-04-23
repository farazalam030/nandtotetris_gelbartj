#include "JackTokenizer.h"
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <regex>

using namespace std;

const map<string, Keyword> JackTokenizer::keywordList = {
	{"class", Keyword::CLASS},
	{"method", Keyword::METHOD},
	{"function", Keyword::FUNCTION},
	{"constructor", Keyword::CONSTRUCTOR},
	{"int", Keyword::INT},
	{"boolean", Keyword::BOOLEAN },
	{"char", Keyword::CHAR},
	{"void", Keyword::VOID},
	{"var", Keyword::VAR},
	{"static", Keyword::STATIC},
	{"field", Keyword::FIELD},
	{"let", Keyword::LET},
	{"do", Keyword::DO},
	{"if", Keyword::IF},
	{"else", Keyword::ELSE},
	{"while", Keyword::WHILE},
	{"return", Keyword::RETURN},
	{"true", Keyword::TRUE},
	{"false", Keyword::FALSE},
	{"null", Keyword::K_NULL},
	{"this", Keyword::THIS }
};

const set<char> JackTokenizer::symbols = {
	'{', '}', '(', ')', '[', ']', '.', ',', ';', '+', '-', '*', '/', '&', '|', '<', '>', '=', '~'
};

static const char * tokens[] = { "keyword", "symbol", "identifier", "integerConstant", "stringConstant" };

static const char* brightError = "\x1B[91mERROR\033[0m";

JackTokenizer::JackTokenizer(std::string& inputFilename, std::string& outputFilename)
{
	inFile.open(inputFilename);
	if (inFile.is_open()) {
		cout << "Opened file " << inputFilename << " for tokenizing." << endl;
		stripAllComments();
		outFile.open(outputFilename);
		if (outFile.is_open()) {
			cout << "Opened file " << outputFilename << " for XML output." << endl;
			outFile << "<tokens>" << endl;
		}
		else {
			cout << "Error opening file " << outputFilename << " for output." << endl;
		}
	}
	else {
		cout << "Error opening file" << inputFilename << " for input." << endl;
		failedOpen = true;
	}
}

JackTokenizer::JackTokenizer(std::string& inputFilename)
{
	inFile.open(inputFilename);
	if (inFile.is_open()) {
		cout << "Opened file " << inputFilename << " for tokenizing." << endl;
		stripAllComments();
	}
	else {
		cout << "Error opening file" << inputFilename << " for input." << endl;
		failedOpen = true;
	}
}

void JackTokenizer::stripAllComments() {
	string line;
	bool isMultiLine = false;
	int lineNum = 0;
	int writtenChars = 0;
	while (getline(inFile, line)) {
		++lineNum;
		if (isMultiLine) {
			size_t endOfComment = line.find("*/");
			if (endOfComment == string::npos) {
				continue;
			}
			else {
				line.erase(0, endOfComment + 2);
				isMultiLine = false;
			}
		}
		isMultiLine = removeComments(line);
		if (line.length() == 0) continue;
		writtenChars += line.length() + 1; // plus 1 for space
		lineNums.emplace(lineNum, writtenChars);
		strippedText << line << " "; // use spaces instead of newline
	}
	strippedText.seekg(0);
}

bool JackTokenizer::removeComments(string& line) {
	size_t slashPos = line.find("//");
	bool isMultiLine = false;
	if (slashPos == 0) { // entire line is a comment
		line.clear();
		return isMultiLine;
	}
	if (slashPos != string::npos)
		line.erase(slashPos);

	size_t longComment = line.find("/*"); // will cover doc comments as well (/**)

	if (longComment != string::npos) {
		size_t endOfComment = line.find("*/");
		if (endOfComment != string::npos) {
			line.erase(longComment, endOfComment + 2);
		}
		else {
			isMultiLine = true;
			if (longComment == 0) {
				line.clear();
				return isMultiLine;
			}
			line.erase(longComment);
		}
	}

	// Trim left whitespace
	size_t charsStart = line.find_first_not_of(" \t");
	if (charsStart != string::npos)
		line.erase(0, charsStart);
	else { // entire line is whitespace
		line.clear();
		return isMultiLine;
	}

	// Trim right whitespace
	size_t charsEnd = line.find_last_not_of(" \t");
	if (charsEnd != string::npos)
		line.erase(charsEnd + 1);

	return isMultiLine;
}

bool JackTokenizer::hasMoreTokens()
{
	return strippedText.good();
}

int JackTokenizer::currPos() {
	for (auto it = lineNums.begin(); it != lineNums.end(); it++) {
		if (strippedText.tellg() < it->second) {
			return it->first - 1;
		}
	}
	return -1;
}

bool JackTokenizer::aborted() {
	return abortFlag;
}

void JackTokenizer::advanceWord() {
	getline(strippedText, currWord, ' ');
}

void JackTokenizer::advance()
{
	if (currWord.length() == 0) {
		advanceWord();
	}

	while (currWord == " ") {
		cout << "Skipping lone space" << endl;
		advanceWord();
	}

	if (currWord.length() > 0 && symbols.find(currWord.at(0)) != symbols.end()) {
		currTokenType = Token::SYMBOL;
		currToken = currWord.at(0);
		if (currWord.length() == 1)
			advanceWord();
		else {
			currWord.erase(0,1);
		}
		return;
	}

	regex nums("[0-9]+");
	std::smatch m;
	if (regex_search(currWord, m, nums) && m.prefix().length() == 0) { // only process "words" that start with a number
		currTokenType = Token::INT_CONST;
		currToken = m.str(); //shorthand for m[0].str()
		if (m.suffix().length() == 0) {
			advanceWord();
		}
		else currWord = m.suffix();
		return;
	}
	
	regex stringexpr("\".*\"?"); // start quote, any character, followed by optional close quote.
	string tempWord = currWord;
	if (regex_match(currWord, m, stringexpr)) {
		tempWord.erase(0, 1); // remove opening quote
		size_t closeQuotePos = tempWord.find_first_of("\""); // ok to find first of since opening quote has been removed

		while (closeQuotePos == string::npos) { // Continue until finding close quote. 
			if (!hasMoreTokens()) {
				cout << brightError << " in string literal, reached end of file without finding close quote ('\"'). Aborting." << endl;
				abortFlag = true;
				return;
			}
			advanceWord();									
			tempWord = tempWord + " " + currWord;
			closeQuotePos = tempWord.find_first_of("\"");
		}
		currTokenType = Token::STRING_CONST;
		if (closeQuotePos != tempWord.length() - 1) {
			currWord = tempWord.substr(closeQuotePos + 1);
			tempWord.erase(closeQuotePos); // exclude close quote
			currToken = tempWord;
			return;
		}
		currToken = tempWord;
		advanceWord();
		return;
	}

	currTokenType = Token::IDENTIFIER;
	size_t symbolSplit = currWord.find_first_of("{}()[].,;+-*/&|<>=~");
	currToken = currWord.substr(0, symbolSplit);
	if (symbolSplit != string::npos) {
		currWord.erase(0, symbolSplit);
	}
	else advanceWord();
	// check for keyword
	auto keywordIt = keywordList.find(currToken);
	if (keywordIt != keywordList.end()) {
		currTokenType = Token::KEYWORD;
	}
}

void JackTokenizer::writeCurrToken(ofstream& destination) {
	string tokenToWrite = currToken;
	if (tokenToWrite == "<") {
		tokenToWrite = "&lt;";
	}
	else if (tokenToWrite == ">") {
		tokenToWrite = "&gt;";
	}
	else if (tokenToWrite == "\"") {
		tokenToWrite = "&quot;";
	}
	else if (tokenToWrite == "&") {
		tokenToWrite = "&amp;";
	}
	if (currTokenType != Token::NONE) {
		destination << "<" << tokens[static_cast<int>(currTokenType)] << ">" << tokenToWrite << "</" << tokens[static_cast<int>(currTokenType)] << ">" << endl;
	}
}

ofstream& JackTokenizer::getOutputFile()
{
	return outFile;
}

Token JackTokenizer::tokenType()
{
	return currTokenType;
}

Keyword JackTokenizer::keyword()
{
	auto key = keywordList.find(currToken);
	if (key != keywordList.end()) {
		return key->second;
	}
	return Keyword::NONE;
}

char JackTokenizer::symbol()
{
	return currToken.at(0);
}

std::string JackTokenizer::identifier()
{
	return currToken;
}

int JackTokenizer::intVal()
{
	return stoi(currToken);
}

std::string JackTokenizer::stringVal()
{
	return currToken;
}

void JackTokenizer::close()
{
	if (inFile.is_open()) {
		outFile << "</tokens>" << endl;
		inFile.close();
		cout << "==============" << endl;
	}
}

bool JackTokenizer::didFailOpen()
{
	return failedOpen;
}

JackTokenizer::~JackTokenizer()
{
}
