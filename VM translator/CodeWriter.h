#pragma once
#include <string>
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include "Shared.h"
constexpr int TEMP_START = 5;

class CodeWriter
{
public:
	CodeWriter(std::string& filename); // open output file
	
	void writeArithmetic(std::string& command);
	void writePushPop(Command command, std::string& segment, int index); // command argument should be C_PUSH or C_POP only

	void setFilename(std::string& filename);
	void writeInit();
	void writeLabel(std::string& label);
	void writeGoto(std::string& label);
	void writeIf(std::string& label);
	void writeFunction(std::string& functionName, int numVars);
	void writeCall(std::string& functionName, int numArgs);
	void writeReturn();

	void close(); // close output file

	~CodeWriter();

private:
	std::string outputFilename;
	std::string inputFilename;
	std::string funcPrefix;
	std::string currFunction;
	std::ofstream outputFile;
	std::map<std::string, int> functionCallCount;
	static const std::set<std::string> arithCommands;
	static const std::map<std::string, std::string> segmentMap;
	int arithCount = 0;
	std::string getSegmentReference(std::string& segment, int index);
};

