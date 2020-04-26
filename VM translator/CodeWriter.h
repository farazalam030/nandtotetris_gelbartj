#pragma once
#include <string>
#include <fstream>
#include <set>
#include <map>
#include "Shared.h"
constexpr int TEMP_START = 5;

enum class Segment { CONST = 0, ARG, LOCAL, STATIC, THIS, THAT, POINTER, TEMP, NONE };

class CodeWriter
{
public:
	CodeWriter(const std::string& filename); // open output file
	
	void writeArithmetic(const std::string& command);
	void writePushPop(Command command, const std::string& segment, int index); // command argument should be C_PUSH or C_POP only

	void setFilename(const std::string& filename);
	void writeInit();
	void writeLabel(const std::string& label);
	void writeGoto(const std::string& label);
	void writeIf(const std::string& label);
	void writeFunction(const std::string& functionName, int numVars);
	void writeCall(const std::string& functionName, int numArgs);
	void writeReturn();
	void writeComment(const std::string& comment);

	void writeReturnBootstrap();
	void writeCallBootstrap();

	void close(); // close output file

	~CodeWriter();

private:
	const std::string outputFilename;
	std::string inputFilename;
	std::string funcPrefix;
	std::string currFunction;
	std::ofstream outputFile;
	std::map<std::string, int> functionCallCount;
	static const std::set<std::string> arithCommands;
	static const std::map<std::string, std::string> segmentMap;
	int arithCount = 0;
	std::string getSegmentReference(const std::string& segment, int index);
	static const std::map<std::string, Segment> segmentEnumMap;
};

