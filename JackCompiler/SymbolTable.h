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
	void define(const std::string& name, const std::string& type, const Kind kind);
	unsigned int& getKindCount(Kind kind); // num vars of given kind in the current scope
	Kind kindOf(std::string& name);
	std::string typeOf(std::string& name);
	int idxOf(std::string& name);
	std::map<std::string, const STEntry>& getTable();
	~SymbolTable();
private:
	std::map<std::string, const STEntry> table;
	std::map<Kind, unsigned int> kindCount;
};

