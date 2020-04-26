#include "SymbolTable.h"
#include <iostream>

using namespace std;

static const char* kinds[] = { "STATIC", "FIELD", "ARG", "VAR" };

SymbolTable::SymbolTable()
{
	// Can't iterate over enums so have to do this manually
	kindCount.emplace(Kind::STATIC, 0);
	kindCount.emplace(Kind::FIELD, 0);
	kindCount.emplace(Kind::ARG, 0);
	kindCount.emplace(Kind::VAR, 0);
}

void SymbolTable::reset()
{
	table.clear();
	for (auto& kindEntry : kindCount) {
		kindEntry.second = 0;
		// cout << "Kind entry " << kinds[(int)kindEntry.first] << " is now " << kindCount.at(kindEntry.first) << endl;
	}
}

void SymbolTable::define(const std::string& name, const std::string& type, const Kind kind)
{
	STEntry entry;
	entry.type = type;
	entry.kind = kind;
	// cout << "getKindCount for kind " << kinds[(int)kind] << " is " << getKindCount(kind) << endl;
	entry.idx = getKindCount(kind)++;
	// cout << "getKindCount of this kind is now " << getKindCount(kind) << endl;
	cout << "Defined new variable with name " << name << ", kind " << kinds[(int)kind] << ", type " << type << ", and idx " << entry.idx << endl;
	table.emplace(name, entry);
}

unsigned int& SymbolTable::getKindCount(Kind kind)
{
	// This will actually return the highest index, not the var count. 
	// No items of kind and 1 item of kind will both return 0. Afterward, returns count - 1.
	return kindCount.at(kind);
}

Kind SymbolTable::kindOf(std::string& name)
{
	return table.at(name).kind;
}

std::string SymbolTable::typeOf(std::string& name)
{
	return table.at(name).type;
}

int SymbolTable::idxOf(std::string& name)
{
	return table.at(name).idx;
}

std::map<std::string, const STEntry>& SymbolTable::getTable() {
	return table;
}


SymbolTable::~SymbolTable()
{
}
