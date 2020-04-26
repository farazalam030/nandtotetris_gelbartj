#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
constexpr int FIRST_FREE_MEM = 16;

class Assembler
{
public:
	Assembler(const std::string& filename);

	void getLoopAddresses();
	void removeComments(std::string& line);
	void convertCommands();
	void convertACommand(std::string& command);
	void convertCCommand(std::string_view command);

	~Assembler();

private:
	const std::string filename;
	std::stringstream codeNoCommentsLabels;
	std::ofstream binaryFile;
	std::ifstream assemblyFile;

	int latestFreeMem = FIRST_FREE_MEM;

	std::map <std::string, int> symbolTable = {
		{ "SP", 0 },
		{ "LCL", 1 },
		{ "ARG", 2 },
		{ "THIS", 3 },
		{ "THAT", 4 },
		{ "R0", 0 },
		{ "R1", 1 },
		{ "R2", 2 },
		{ "R3", 3 },
		{ "R4", 4 },
		{ "R5", 5 },
		{ "R6", 6 },
		{ "R7", 7 },
		{ "R8", 8 },
		{ "R9", 9 },
		{ "R10", 10 },
		{ "R11", 11 },
		{ "R12", 12 },
		{ "R13", 13 },
		{ "R14", 14 },
		{ "R15", 15 },
		{ "SCREEN", 16384 },
		{ "KBD", 24576 }
	};

	static const std::map <std::string_view, std::string> jumpMap; 
	static const std::map <std::string_view, std::string> compMap; 
	static const std::map <std::string_view, std::string> destMap;

};