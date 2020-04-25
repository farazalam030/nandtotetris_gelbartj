#pragma once
#include "Shared.h"
#include <string>
#include <fstream>

class VMWriter
{
public:
	VMWriter(const std::string& outputFilename);
	void writePush(Segment segment, int index);
	void writePop(Segment segment, int index);
	void writeArithmetic(Command command);
	void writeLabel(const std::string& label);
	void writeGoto(const std::string& label);
	void writeIf(const std::string& label);
	void writeCall(const std::string& name, int nArgs);
	void writeFunction(const std::string& name, int nLocals);
	void writeReturn();
	bool didFailOpen();
	void close();
	~VMWriter();
private:
	std::ofstream outFile;
	bool failedOpen = false;
};

