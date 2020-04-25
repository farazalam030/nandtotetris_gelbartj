#include "CompilationEngine.h"
#include "Shared.h"
#include <iostream>

using namespace std;

static const char* nonTerminal[] = { "class", "classVarDec", "subroutineDec", "parameterList", "subroutineBody", "varDec", "statements", "letStatement",	"ifStatement", "whileStatement", "doStatement", "returnStatement", "expression", "term", "expressionList", "identifier", "category", "idx", "isDefinition" };

static const char* tokens[] = { "keyword", "symbol", "identifier token", "int constant", "string constant", "none" };
static const char* kinds[] = { "static", "field", "arg", "var" };

static const char* brightError = "\x1B[91mERROR\033[0m";

const map<Keyword, string> reverseKeywordList = {
	{Keyword::CLASS, "class"},
	{Keyword::METHOD, "method"},
	{Keyword::FUNCTION, "function"},
	{Keyword::CONSTRUCTOR, "constructor" },
	{Keyword::INT, "int" },
	{Keyword::BOOLEAN, "boolean" },
	{Keyword::CHAR, "char" },
	{Keyword::VOID, "void" },
	{Keyword::VAR, "var" },
	{Keyword::STATIC, "static" },
	{Keyword::FIELD, "field" },
	{Keyword::LET, "let" },
	{Keyword::DO, "do" },
	{Keyword::IF, "if" },
	{Keyword::ELSE, "else" },
	{Keyword::WHILE, "while" },
	{Keyword::RETURN, "return" },
	{Keyword::TRUE, "true" },
	{Keyword::FALSE, "false" },
	{Keyword::K_NULL, "null" },
	{Keyword::THIS, "this" }
};

const set<char> ops = { '+', '-', '*', '/', '&', '|', '<', '>', '=' };

CompilationEngine::CompilationEngine(JackTokenizer* tkA, VMWriter* vmA) : tk(tkA), vm(vmA)
{
	tk->advance();
	if (tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::CLASS)
		compileClass();
	else {
		cout << brightError << ": File does not begin with class declaration." << endl;
	}
}

Status CompilationEngine::eat(Token tokenType, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if (tk->tokenType() != tokenType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected " << tokens[static_cast<int>(tokenType)] << " and got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		if (tk->tokenType() == Token::INT_CONST)
			vm->writePush(Segment::CONSTANT, tk->intVal());
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eat(char symbol, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if (tk->symbol() != symbol) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected '" << symbol << "' and got '" << tk->stringVal() << "' instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eat(Keyword keywordType, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if (tk->keyword() != keywordType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected " << reverseKeywordList.at(keywordType) << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eatType(bool includeVoid, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if ((tk->tokenType() == Token::KEYWORD &&
		(tk->keyword() == Keyword::INT ||
			tk->keyword() == Keyword::BOOLEAN ||
			tk->keyword() == Keyword::CHAR ||
			(includeVoid ? tk->keyword() == Keyword::VOID : false)))
		|| tk->tokenType() == Token::IDENTIFIER) {
		writeTkAndAdvance();
		return Status::OK;
	}
	else {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected type" << endl;
		return Status::SYNTAX_ERROR;
	}
}

Status CompilationEngine::eatType(bool isOptional) {
	return eatType(false, isOptional);
}

Status CompilationEngine::eatTypeWithVoid(bool isOptional) {
	return eatType(true, isOptional);
}

Status CompilationEngine::eatOp(bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	auto opIt = tk->tokenType() == Token::SYMBOL ? ops.find(tk->symbol()) : ops.end();
	if (opIt == ops.end()) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected operator. Got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}


void CompilationEngine::writeTkAndAdvance() {
	if (!tk->aborted()) {
		// tk->writeCurrToken(outFile, jsonMode);
		tk->advance();
	}
}

/*
string CompilationEngine::makeOpenTag(NonTerminal nt, bool isList) {
	string tagName = nonTerminal[static_cast<int>(nt)];
	return (jsonMode ? "\"" + tagName + "\": " + (isList ? "[" : "{") : "<" + tagName + ">") + "\n";
}

string CompilationEngine::makeCloseTag(NonTerminal nt, bool isList) {
	string tagName = nonTerminal[static_cast<int>(nt)];
	return (jsonMode ? (isList ? "]," : "},") : "</" + tagName + ">") + "\n";
}
*/

void CompilationEngine::compileClass()
{
	// outFile << makeOpenTag(NonTerminal::CLASS);
	classTable.reset();
	eat(Keyword::CLASS);
	className = tk->stringVal();
	compileIdentifier(true, false);
	eat('{');
	if (tk->tokenType() == Token::KEYWORD) {
		while (!tk->aborted() && (tk->keyword() == Keyword::STATIC || tk->keyword() == Keyword::FIELD)) {
			compileClassVarDec();
		}
		while (!tk->aborted() && (tk->keyword() == Keyword::CONSTRUCTOR || tk->keyword() == Keyword::FUNCTION || tk->keyword() == Keyword::METHOD)) {
			compileSubroutineDec();
		}
	}
	eat('}');
	// outFile << makeCloseTag(NonTerminal::CLASS);
}

void CompilationEngine::checkVarDec(bool isClass) {
	auto found = (isClass ? classTable : subroutineTable).getTable().find(tk->stringVal());
	auto end = (isClass ? classTable : subroutineTable).getTable().end();
	if (found != end) {
		cout << brightError << " at line " << tk->currPos() << ": Attempting redefinition of already defined variable \"" << tk->stringVal() << "\"" << endl;
	}
}

Kind keyToKind(Keyword key) {
	switch (key) {
	case Keyword::STATIC:
		return Kind::STATIC;
	case Keyword::VAR:
		return Kind::VAR;
	case Keyword::FIELD:
		return Kind::FIELD;
	default:
		return Kind::ARG; // parameter in subroutine declaration
	}
}

Segment kindToSegment(Kind kind) {
	switch (kind) {
	case Kind::ARG:
		return Segment::ARG;
	case Kind::FIELD:
		return Segment::THIS;
	case Kind::STATIC:
		return Segment::STATIC;
	case Kind::VAR:
		return Segment::LOCAL;
	default:
		return Segment::NONE;
	}
}

void CompilationEngine::compileClassVarDec()
{
	// outFile << makeOpenTag(NonTerminal::CLASS_VAR_DEC);
	Kind kind = keyToKind(tk->keyword());
	eat(Token::KEYWORD);
	string type = tk->stringVal();
	eatType();
	checkVarDec(true);
	classTable.define(tk->stringVal(), type, kind);
	compileIdentifier(true, false);
	while (eat(',', true) == Status::OK) {
		checkVarDec(true);
		classTable.define(tk->stringVal(), type, kind);
		compileIdentifier(true, false);
	}
	eat(';');
	// outFile << makeCloseTag(NonTerminal::CLASS_VAR_DEC);
}

void CompilationEngine::compileSubroutineDec()
{
	// outFile << makeOpenTag(NonTerminal::SUBROUTINE_DEC);
	subroutineTable.reset();
	Keyword key = tk->keyword();
	eat(Token::KEYWORD);
	eatTypeWithVoid();
	string funName = tk->stringVal();
	compileIdentifier(true, true);
	eat('(');
	compileParameterList(key == Keyword::METHOD);
	eat(')');
	compileSubroutineBody(funName);
	// outFile << makeCloseTag(NonTerminal::SUBROUTINE_DEC);
}

void CompilationEngine::compileParameterList(bool isMethod)
{
	// outFile << makeOpenTag(NonTerminal::PARAMETER_LIST, true);
	if (isMethod) {
		string thisStr = "this"; // define requires string reference
		subroutineTable.define(thisStr, className, Kind::ARG);
	}
	string type = tk->stringVal();
	if (eatType(true) == Status::OK) {
		subroutineTable.define(tk->stringVal(), type, Kind::ARG);
		compileIdentifier(true, false); // parameter list args are ARG variable declarations
		while (eat(',', true) == Status::OK) {
			type = tk->stringVal();
			eatType();
			subroutineTable.define(tk->stringVal(), type, Kind::ARG);
			compileIdentifier(true, false);
		}
	}
	// else do nothing. list can be empty
	// outFile << makeCloseTag(NonTerminal::PARAMETER_LIST, true);
}

void CompilationEngine::compileSubroutineBody(string& funName)
{
	// outFile << makeOpenTag(NonTerminal::SUBROUTINE_BODY);
	int localCount = 0;
	eat('{');
	while (!tk->aborted() && tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::VAR) {
		++localCount;
		compileVarDec();
	}
	vm->writeFunction(className + "." + funName, localCount);
	compileStatements();
	eat('}');
	// outFile << makeCloseTag(NonTerminal::SUBROUTINE_BODY);
}

void CompilationEngine::compileVarDec()
{
	// outFile << makeOpenTag(NonTerminal::VAR_DEC);
	eat(Keyword::VAR);
	string type = tk->stringVal();
	eatType();
	checkVarDec(false);
	classTable.define(tk->stringVal(), type, Kind::VAR);
	compileIdentifier(true, false);
	while (eat(',', true) == Status::OK) {
		checkVarDec(false);
		classTable.define(tk->stringVal(), type, Kind::VAR);
		compileIdentifier(true, false);
	}
	eat(';');
	// outFile << makeCloseTag(NonTerminal::VAR_DEC);
}

void CompilationEngine::compileStatements()
{
	// outFile << makeOpenTag(NonTerminal::STATEMENTS);
	bool endFlag = false;
	while (!tk->aborted() && tk->tokenType() == Token::KEYWORD && !endFlag) {
		switch (tk->keyword()) {
		case Keyword::LET:
			compileLet();
			break;
		case Keyword::IF:
			compileIf();
			break;
		case Keyword::WHILE:
			compileWhile();
			break;
		case Keyword::DO:
			compileDo();
			break;
		case Keyword::RETURN:
			compileReturn();
			break;
		default:
			endFlag = true;
			break;
		}
		// cout << "Current token is " << tk->stringVal() << endl;
	}
	// outFile << makeCloseTag(NonTerminal::STATEMENTS);
}

void CompilationEngine::compileLet()
{
	// outFile << makeOpenTag(NonTerminal::LET);
	eat(Keyword::LET);
	auto classIt = classTable.getTable().find(tk->stringVal());
	auto subIt = subroutineTable.getTable().find(tk->stringVal());
	bool isSubVar = subIt != subroutineTable.getTable().end();
	if (classIt == classTable.getTable().end() && !isSubVar) {
		cout << brightError << " at line " << tk->currPos() << ": Attempting to assign value to undeclared variable \"" << tk->stringVal() << "\"" << endl;
		return; // not sure if this will break things elsewhere but need to avoid future statements in this case
	}
	compileIdentifier(false, false);
	if (eat('[', true) == Status::OK) {
		compileExpression();
		eat(']');
	}
	eat('=');
	compileExpression();
	// after returning from compileExpression, result of expression will be at the top of the stack
	vm->writePop(kindToSegment((isSubVar ? subIt : classIt)->second.kind), (isSubVar ? subIt : classIt)->second.idx);
	eat(';');
	// outFile << makeCloseTag(NonTerminal::LET);
}

void CompilationEngine::compileIf()
{
	// outFile << makeOpenTag(NonTerminal::IF);
	eat(Keyword::IF);
	eat('(');
	compileExpression();
	eat(')');
	eat('{');
	compileStatements();
	eat('}');
	if (eat(Keyword::ELSE, true) == Status::OK) {
		eat('{');
		compileStatements();
		eat('}');
	}
	// outFile << makeCloseTag(NonTerminal::IF);
}

void CompilationEngine::compileWhile()
{
	// outFile << makeOpenTag(NonTerminal::WHILE);
	eat(Keyword::WHILE);
	eat('(');
	compileExpression();
	eat(')');
	eat('{');
	compileStatements();
	eat('}');
	// outFile << makeCloseTag(NonTerminal::WHILE);
}

void CompilationEngine::compileDo()
{
	// outFile << makeOpenTag(NonTerminal::DO);
	eat(Keyword::DO);
	compileIdentifier(false, true);
	if (eat('.', true) == Status::OK) {
		compileIdentifier(false, true);
	}
	eat('(');
	compileExpressionList();
	eat(')');
	eat(';');
	// outFile << makeCloseTag(NonTerminal::DO);
}

void CompilationEngine::compileReturn()
{
	// outFile << makeOpenTag(NonTerminal::RETURN);
	eat(Keyword::RETURN);
	if (eat(';', true) == Status::OK) {
		// outFile << makeCloseTag(NonTerminal::RETURN);
		return;
	}
	compileExpression();
	eat(';');
	// outFile << makeCloseTag(NonTerminal::RETURN);
}

void CompilationEngine::compileExpression()
{
	// outFile << makeOpenTag(NonTerminal::EXPRESSION);
	compileTerm();
	while (eatOp(true) == Status::OK) {
		compileTerm();
	}
	// outFile << makeCloseTag(NonTerminal::EXPRESSION);
}

bool isKeywordConst(Keyword key) {
	return (key == Keyword::TRUE || key == Keyword::FALSE || key == Keyword::K_NULL || key == Keyword::THIS);
}

void CompilationEngine::compileTerm()
{
	// outFile << makeOpenTag(NonTerminal::TERM);
	if (tk->tokenType() == Token::IDENTIFIER) {
		compileTermIdentifier();
	}
	else if (eat(Token::INT_CONST, true) == Status::OK) {}
	else if (eat(Token::STRING_CONST, true) == Status::OK) {}
	else if (tk->tokenType() == Token::KEYWORD && isKeywordConst(tk->keyword())) {
		eat(Token::KEYWORD);
	}
	else if (eat('(', true) == Status::OK) {
		compileExpression();
		eat(')');
	}
	else if (eat('-', true) == Status::OK) {
		compileTerm();
		vm->writeArithmetic(Command::NEG);
	}
	else if (eat('~', true) == Status::OK) {
		compileTerm();
		vm->writeArithmetic(Command::NOT);
	}

	// outFile << makeCloseTag(NonTerminal::TERM);
}

void CompilationEngine::compileExpressionList()
{
	// outFile << makeOpenTag(NonTerminal::EXPRESSION_LIST);
	if (tk->tokenType() == Token::SYMBOL && tk->symbol() == ')') {
		// outFile << makeCloseTag(NonTerminal::EXPRESSION_LIST);
		return;
	}
	compileExpression();
	while (eat(',', true) == Status::OK) {
		compileExpression();
	}
	// outFile << makeCloseTag(NonTerminal::EXPRESSION_LIST);
}

void CompilationEngine::compileIdentifier(bool beingDefined, bool isSubroutine) {
	compileIdentifier(beingDefined, isSubroutine, std::nullopt);
}

void CompilationEngine::compileIdentifier(bool beingDefined, bool isSubroutine, std::optional<std::reference_wrapper<string>> savedToken)
{
	// cout << "In compileIdentifier" << endl;
	if (!savedToken) {
		// outFile << makeOpenTag(NonTerminal::IDENTIFIER);
		// tk->writeCurrToken(outFile);
	}
	string token = savedToken ? savedToken->get() : tk->stringVal();

	// cout << "Calling defined tag" << endl;
	// outFile << makeOpenTag(NonTerminal::IDENTIFIER_BEING_DEFINED);
	// outFile << beingDefined;
	// outFile << makeCloseTag(NonTerminal::IDENTIFIER_BEING_DEFINED);

	// cout << "Calling category tag" << endl;
	// outFile << makeOpenTag(NonTerminal::IDENTIFIER_CAT);

	if (isSubroutine) {
		// outFile << "subroutine";
		// outFile << makeCloseTag(NonTerminal::IDENTIFIER_CAT);
		// outFile << makeCloseTag(NonTerminal::IDENTIFIER);
		if (!savedToken) tk->advance();
		return;
	}

	// cout << "Not subroutine. Checking for variables." << endl;
	auto localVar = subroutineTable.getTable().find(token);
	auto classVar = classTable.getTable().find(token);
	if (localVar != subroutineTable.getTable().end() || classVar != classTable.getTable().end()) {
		// outFile << kinds[static_cast<int>(localVar != subroutineTable.getTable().end() ? localVar->second.kind : classVar->second.kind)];
		// outFile << makeCloseTag(NonTerminal::IDENTIFIER_CAT);
		// outFile << makeOpenTag(NonTerminal::IDENTIFIER_IDX);
		// outFile << (localVar != subroutineTable.getTable().end() ? localVar->second.idx : classVar->second.idx);
		// outFile << makeCloseTag(NonTerminal::IDENTIFIER_IDX);
	}
	else { // is class
		// cout << "Is class" << endl;
		// outFile << "class";
		// outFile << makeCloseTag(NonTerminal::IDENTIFIER_CAT);
	}
	// outFile << makeCloseTag(NonTerminal::IDENTIFIER);
	if (!savedToken) tk->advance();
}

void CompilationEngine::compileTermIdentifier() {
	//outFile << makeOpenTag(NonTerminal::IDENTIFIER);
	// tk->writeCurrToken(outFile);
	string token = tk->stringVal();
	tk->advance();
	if (tk->tokenType() == Token::SYMBOL && tk->symbol() == '[') {
		compileIdentifier(false, false, token);
		eat('[');
		compileExpression();
		eat(']');
	}
	else if (tk->tokenType() == Token::SYMBOL && tk->symbol() == '(') {
		compileIdentifier(false, true, token);
		eat('(');
		compileExpressionList();
		eat(')');
	}
	else if (tk->tokenType() == Token::SYMBOL && tk->symbol() == '.') {
		compileIdentifier(false, false, token);
		eat('.');
		compileIdentifier(false, true);
		eat('(');
		compileExpressionList();
		eat(')');
	}
	else {
		compileIdentifier(false, false, token); // regular variable
	}
}

CompilationEngine::~CompilationEngine()
{
}
