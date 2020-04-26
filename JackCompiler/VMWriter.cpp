#include "VMWriter.h"
#include <iostream>

using namespace std;

static const char* brightError = "\x1B[91mERROR\033[0m";

static const char* segments[] = { "constant", "argument", "local", "static", "this", "that", "pointer", "temp", "none" };
static const char* commands[] = { "add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not" };

VMWriter::VMWriter(const std::string& outputFilename)
{
	outFile.open(outputFilename);
	if (outFile.is_open()) {
		cout << "Opened file " << outputFilename << " for compiler output." << endl;
	}
	else {
		cout << brightError << " opening file " << outputFilename << " for compiler output." << endl;
		failedOpen = true;
	}
}

void VMWriter::writePush(Segment segment, int index)
{
	outFile << "push " << segments[static_cast<int>(segment)] << " " << index << endl;
}

void VMWriter::writePop(Segment segment, int index)
{
	outFile << "pop " << segments[static_cast<int>(segment)] << " " << index << endl;
}

void VMWriter::writeArithmetic(Command command)
{
	if (command == Command::MULT) {
		outFile << "call Math.multiply 2" << endl;
		return;
	}
	else if (command == Command::DIV) {
		outFile << "call Math.divide 2" << endl;
		return;
	}
	outFile << commands[static_cast<int>(command)] << endl;
}

void VMWriter::writeLabel(string_view label)
{
	outFile << "label " << label << endl;
}

void VMWriter::writeGoto(string_view label)
{
	outFile << "goto " << label << endl;
}

void VMWriter::writeIf(string_view label)
{
	outFile << "if-goto " << label << endl;
}

void VMWriter::writeCall(string_view name, int nArgs)
{
	outFile << "call " << name << " " << nArgs << endl;
}

void VMWriter::writeFunction(string_view name, int nLocals)
{
	outFile << "function " << name << " " << nLocals << endl;
}

void VMWriter::writeReturn()
{
	outFile << "return" << endl;
}

bool VMWriter::didFailOpen()
{
	return failedOpen;
}

void VMWriter::close()
{
	if (outFile.is_open()) {
		cout << "Closing file." << endl;
		outFile.close();
	}
}

VMWriter::~VMWriter()
{
}
