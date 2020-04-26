#include "CodeWriter.h"
#include "Shared.h"
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <map>

using namespace std;

const set<string> CodeWriter::arithCommands = { "add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not" };

const map<string, string> CodeWriter::segmentMap = { { "local", "LCL" }, { "this", "THIS" }, { "that", "THAT"}, { "argument", "ARG" } };

const map<string, Segment> CodeWriter::segmentEnumMap = {
	{ "local", Segment::LOCAL },
	{ "this", Segment::THIS },
	{ "that", Segment::THAT },
	{ "argument", Segment::ARG },
	{ "constant", Segment::CONST },
	{ "static", Segment::STATIC },
	{ "pointer", Segment::POINTER },
	{ "temp", Segment::TEMP }
};

CodeWriter::CodeWriter(const std::string& filename): outputFilename(filename)
{
	outputFile.open(outputFilename);
	if (outputFile.is_open()) {
		cout << "Writing output to " << filename << endl;
	}
	else {
		cout << "Error occurred opening file for output." << endl;
	}
}

void CodeWriter::writeArithmetic(const std::string& command)
{
	string output = ("// " + command + "\n");
	// Get first value from stack
	output += "@SP\n"
		"AM=M-1\n";
	if (command == "neg") {
		output += "M=-M\n"
			"@SP\n"
			"M=M+1\n";
		outputFile << output;
		return;
	}
	else if (command == "not") {
		output += "M=!M\n"
			"@SP\n"
			"M=M+1\n";
		outputFile << output;
		return;
	}
	// Save first value and get second value from stack
	output += "D=M\n"
		"@SP\n"
		"AM=M-1\n";
	// Now D holds the first value (y) and M holds the second value (x)
	if (command == "add") {
		output += "M=M+D\n";
	}
	else if (command == "and") {
		output += "M=M&D\n";
	}
	else if (command == "or") {
		output += "M=M|D\n";
	}
	else {
		if (command == "sub") {
			output += "M=M-D\n"
				"@SP\n"
				"M=M+1\n";
			outputFile << output;
			return;
		}
		output += "D=M-D\n"; // store difference
		output += "M=0\n"; // start with assumption the result is false
		if (command == "eq" || command == "gt" || command == "lt") 
			output += "@TRUE_" + to_string(arithCount) + "\n";

		if (command == "eq") {
			output += "D;JEQ\n";
		}
		else if (command == "gt") {
			output += "D;JGT\n";
		}
		else if (command == "lt") {
			output += "D;JLT\n";
		}

		output += "@FALSE_" + to_string(arithCount) + "\n"
			"0;JMP\n"
			"(TRUE_" + to_string(arithCount) + ")\n"
			"@SP\n"
			"A=M\n"
			"M=-1\n" // In VM, true is represented by -1
			"(FALSE_" + to_string(arithCount) + ")\n";
		
		++arithCount;
	}
	output += "@SP\n"
		"M=M+1\n";
	outputFile << output;
}

string CodeWriter::getSegmentReference(const string& segment, int index) {
	Segment seg = segmentEnumMap.at(segment);

	switch (seg) {
	case Segment::CONST:
		return "@" + to_string(index) + "\n";
	case Segment::STATIC:
		return "@" + inputFilename.substr(0, inputFilename.find_last_of(".") + 1) + to_string(index) + "\n";
	case Segment::TEMP:
		if (TEMP_START + index > 12) {
			cout << "Invalid call to temp segment (" << to_string(TEMP_START + index) << "), exceeds bounds" << endl;
			return "";
		}
		return "@" + to_string(TEMP_START + index) + "\n";
	case Segment::POINTER:
		if (index == 0) {
			return "@THIS\n";
		}
		else if (index == 1) {
			return "@THAT\n";
		}
		else {
			cout << "Invalid call to pointer memory segment: pointer " + to_string(index) << endl;
			return "";
		}
	default:
		string output;
		if (index > 1) output += ("@" + to_string(index) + "\n"
			"D=A\n");
		output += "@" + segmentMap.find(segment)->second + "\n";
		if (index > 1) output += "A=M+D\n";
		else if (index == 1) output += "A=M+1\n";
		else output += "A=M\n";
		return output;
	}
}

void CodeWriter::writePushPop(Command command, const string& segment, int index)
{
	string output;	

	if (command == Command::C_POP) {
		if (segment == "constant") {
			return; // pop constant not allowed
		}

		output += "// pop " + segment + " " + to_string(index) + "\n";
		
		if ((segment == "local" || segment == "this" || segment == "that" || segment == "argument") && index > 0) {
			output +=
				"@" + to_string(index) + "\n"
				"D=A\n"
				"@" + segmentMap.find(segment)->second + "\n"
				"D=D+M\n"  // address of where we want to store popped item now held in D
				"@SP\n"
				"AM=M-1\n"
				"D=M+D\n"  // add address to popped value, since we only have one register to work with
				"A=D-M\n"  // undo previous operation to extract original address and jump to it
				"M=D-A\n"; // subtract address from D to get popped value
		} 

		else {
			output += "@SP\n"
				"AM=M-1\n"
				"D=M\n" // last element in stack is now stored in D
				+ getSegmentReference(segment, index) + // A/M is now at referenced segment and index
				"M=D\n";
		}
	}
	else if (command == Command::C_PUSH) { 
		output += "// push " + segment + " " + to_string(index) + "\n";
		
		if (segment == "constant" && (index == 1 || index == 0)) {
			output += "@SP\n"
				"M=M+1\n"
				"A=M-1\n"
				"M=" + to_string(index) + "\n";
		}
		else {
			output += getSegmentReference(segment, index); // A/M is now at referenced segment and index

			if (segment == "constant") output += "D=A\n";
			else output += "D=M\n";
			output += "@SP\n"
				"M=M+1\n"
				"A=M-1\n"
				"M=D\n";
		}
	}

	outputFile << output;
}

void CodeWriter::setFilename(const std::string& filename)
{
	inputFilename = filename;
	funcPrefix = ""; // this was previously included in case the VM translator was expected to add a filename prefix to functions.
					 // turns out this is already done by the Jack compiler, so not necessary here. 
}

void CodeWriter::writeInit()
{
	// bootstrap code to initialize VM
	outputFile << "@256\n"
		"D=A\n"
		"@SP\n"
		"M=D\n"
		"@SysInit\n"
		"0;JMP\n";
	
	writeCallBootstrap();
	writeReturnBootstrap();

	outputFile << "(SysInit)\n";
	writeCall("Sys.init", 0);
}

void CodeWriter::writeLabel(const std::string& label)
{
	outputFile << "// label " + label + "\n"
		"(" + funcPrefix + currFunction + "$" + label + ")\n";
}

void CodeWriter::writeGoto(const std::string& label)
{
	outputFile << "// goto " + label + "\n"
		"@" + funcPrefix + currFunction + "$" + label + "\n"
		"0;JMP\n";
}

void CodeWriter::writeIf(const std::string& label)
{
	outputFile << "// if-goto " + label + "\n"
		"@SP\n" // pop latest item on stack and save in D
		"AM=M-1\n"
		"D=M\n"
		"@" + funcPrefix + currFunction + "$" + label + "\n"
		"D;JNE\n"; // jump if latest item on the stack is not false.
}

void CodeWriter::writeFunction(const std::string& functionName, int numVars)
{
	currFunction = functionName;
	outputFile << "// function " + functionName + " " + to_string(numVars) + "\n"
		"(" + funcPrefix + functionName + ")\n";

	outputFile << "@SP\n"
		"D=M\n"
		"@LCL\n" // update LCL to match SP even when 0 vars
		"M=D\n"
		"A=M\n";

	if (numVars > 0) {
		for (int i = 0; i < numVars; ++i) {
			outputFile << "M=0\n"
				"@SP\n"
				"AM=M+1\n";
		}
	}
}

void CodeWriter::writeCall(const std::string& functionName, int numArgs)
{
	int callCount = 1;

	auto callIt = functionCallCount.find(currFunction);

	if (callIt != functionCallCount.end()) {
		callCount = callIt->second;
		++callCount;
		callIt->second = callCount;
	}
	else {
		functionCallCount.emplace(currFunction, 1);
	}	

	// callCount tracks the number of calls made within the current function
	const string returnSymbol = funcPrefix + currFunction + "$ret." + to_string(callCount);

	// store numArgs in R13, returnSymbol in R14, and function name (address) in R15, then call bootstrap code
	outputFile << "// call " + functionName + " " + to_string(numArgs) + "\n";
	if (numArgs > 1) {
		outputFile << "@" + to_string(numArgs) + "\n"
			"D=A\n"
			"@R13\n"
			"M=D\n";
	}
	else {
		outputFile << "@R13\n"
			"M=" << to_string(numArgs) << "\n";
	}
		
	outputFile << "@" + returnSymbol + "\n"
		"D=A\n"
		"@R14\n"
		"M=D\n"
		"@" + funcPrefix + functionName + "\n"
		"D=A\n"
		"@R15\n"
		"M=D\n"
		"@__CallBootstrap__\n"
		"0;JMP\n"
		"(" + returnSymbol + ")\n";
		// return here when finished with function call
}

void CodeWriter::writeCallBootstrap() {
	const string saveVar = "D=M\n"
		"@SP\n"
		"M=M+1\n"
		"A=M-1\n"
		"M=D\n";

	outputFile << "(__CallBootstrap__)\n"
		"@SP\n" // at point function is called, SP will be pointing to return address spot. This spot - numArgs will be arg0. Which means if numArgs = 0, they will be in the same spot! This case is handled in the return function
		"A=M\n"
		"D=A\n"
		"@R13\n" // numArgs
		"D=D-M\n"
		"M=D\n" // address of arg 0 now stored in R13. can't reset as new ARG until existing value is saved
		"@R14\n" // returnSymbol
		"D=M\n"
		"@SP\n"
		"A=M\n"
		"M=D\n" // address of return spot now stored here
		"@SP\n"
		"M=M+1\n" // advance SP then begin to save frame
		"@LCL\n"
		+ saveVar +
		"@ARG\n"
		+ saveVar +
		"@THIS\n"
		+ saveVar +
		"@THAT\n"
		+ saveVar +
		"@R13\n"
		"D=M\n"
		"@ARG\n"
		"M=D\n" // address of new (callee's) arg 0 now stored in @ARG
		"@R15\n" // function name
		"A=M\n"
		"0;JMP\n";
}

void CodeWriter::writeReturnBootstrap() {
	// restore saved frame, starting with THAT
	outputFile << "(__ReturnBootstrap__)\n"
		"@LCL\n"
		"A=M-1\n"
		"D=M\n"
		"@THAT\n"
		"M=D\n"
		// restore THIS
		"@LCL\n"
		"A=M-1\n"
		"A=A-1\n"
		"D=M\n"
		"@THIS\n"
		"M=D\n"

		// save return address in R14, otherwise it will be overwritten if the function has no args.
		// return address is five spots behind callee's LCL
		"@5\n"
		"D=A\n"
		"@LCL\n"
		"A=M-D\n"
		"D=M\n"
		"@R14\n"
		"M=D\n"
		// copy last item on stack to arg 0
		"@SP\n"
		"A=M-1\n"
		"D=M\n"
		"@ARG\n"
		"A=M\n"
		"M=D\n"
		// reset stack pointer, will always be spot after arg 0 / return value
		"D=A+1\n"
		"@SP\n"
		"M=D\n"

		// restore ARG
		"@LCL\n"
		"A=M-1\n"
		"A=A-1\n"
		"A=A-1\n"
		"D=M\n"
		"@ARG\n"
		"M=D\n"
		// restore LCL
		"@4\n"
		"D=A\n"
		"@LCL\n"
		"A=M-D\n" // four spots behind callee's LCL is caller's saved LCL
		"D=M\n"
		"@LCL\n"
		"M=D\n"

		"@R14\n"
		"A=M\n"
		"0;JMP\n";
}

void CodeWriter::writeReturn()
{
	outputFile << "// return\n"
		"@__ReturnBootstrap__\n"
		"0;JMP\n";
}

void CodeWriter::writeComment(const string& comment) {
	outputFile << "// " << comment << endl; // double set of "//" will indicate comments from vm file
}

void CodeWriter::close()
{
	if (outputFile.is_open()) {
		outputFile.close();
	}
}

CodeWriter::~CodeWriter()
{
}
