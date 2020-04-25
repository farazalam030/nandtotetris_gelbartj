#pragma once
#include "Shared.h"
#include <string>
#include <map>

struct STEntry {
	std::string type; // using string instead of enum because this can be any class name
	Kind kind;
	unsigned int idx;
};

class SymbolTable
{
public:
	SymbolTable();
	void reset();
	void define(std::string& name, std::string& type, Kind kind);
	unsigned int& kindIdx(Kind kind); // num vars of given kind in the current scope
	Kind kindOf(std::string& name);
	std::string typeOf(std::string& name);
	int countOf(std::string& name);
	std::map<std::string, STEntry>& getTable();
	~SymbolTable();
private:
	std::map<std::string, STEntry> table;
	std::map<Kind, unsigned int> kindCount;
};

