#include "SymbolTable.h"

using namespace std;

SymbolTable::SymbolTable()
{
	// Can't iterate over enums so have to do this manually
	kindCount.emplace(Kind::ARG, 0);
	kindCount.emplace(Kind::FIELD, 0);
	kindCount.emplace(Kind::STATIC, 0);
	kindCount.emplace(Kind::VAR, 0);
}

void SymbolTable::reset()
{
	table.clear();
	for (auto kindEntry : kindCount) {
		kindEntry.second = 0;
	}
}

void SymbolTable::define(std::string& name, std::string& type, Kind kind)
{
	STEntry entry;
	entry.type = type;
	entry.kind = kind;
	entry.idx = kindIdx(kind)++;
	table.emplace(name, entry);
}

unsigned int& SymbolTable::kindIdx(Kind kind)
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

int SymbolTable::countOf(std::string& name)
{
	return table.at(name).idx;
}

std::map<std::string, STEntry>& SymbolTable::getTable() {
	return table;
}


SymbolTable::~SymbolTable()
{
}
