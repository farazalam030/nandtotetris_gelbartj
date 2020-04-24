#include "CompilationEngine.h"
#include "Shared.h"
#include <iostream>

using namespace std;

static const char* nonTerminal[] = { "class", "classVarDec", "subroutineDec", "parameterList", "subroutineBody", "varDec", "statements", "letStatement",
	"ifStatement", "whileStatement", "doStatement", "returnStatement", "expression", "term", "expressionList" };

static const char* tokens[] = { "keyword", "symbol", "identifier", "int constant", "string constant", "none" };

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

CompilationEngine::CompilationEngine(JackTokenizer* tkA, std::string& outputFilename, bool useJson) : tk(tkA), jsonMode(useJson)
{
	outFile.open(outputFilename);
	if (outFile.is_open()) {
		cout << "Opened file " << outputFilename << " for detailed XML output." << endl;
	}
	else {
		cout << brightError << " opening file " << outputFilename << " for detailed output." << endl;
		failedOpen = true;
	}
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
		tk->writeCurrToken(outFile, jsonMode);
		tk->advance();
	}
}

string CompilationEngine::makeOpenTag(NonTerminal nt, bool isList) {
	string tagName = nonTerminal[static_cast<int>(nt)];
	return (jsonMode ? "\"" + tagName + "\": " + (isList ? "[" : "{") : "<" + tagName + ">") + "\n";
}

string CompilationEngine::makeCloseTag(NonTerminal nt, bool isList) {
	string tagName = nonTerminal[static_cast<int>(nt)];
	return (jsonMode ? (isList ? "]," : "},") : "</" + tagName + ">") + "\n";
}

void CompilationEngine::compileClass()
{
	outFile << makeOpenTag(NonTerminal::CLASS);
	eat(Keyword::CLASS);
	eat(Token::IDENTIFIER);
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
	outFile << makeCloseTag(NonTerminal::CLASS);
}

void CompilationEngine::checkVarDec() {
	auto varsIt = definedVars.find(tk->stringVal());
	if (varsIt == definedVars.end()) definedVars.insert(tk->stringVal());
	else cout << brightError << " at line " << tk->currPos() << ": Attempting redefinition of already defined variable \"" << tk->stringVal() << "\"" << endl;
}

void CompilationEngine::compileClassVarDec()
{
	outFile << makeOpenTag(NonTerminal::CLASS_VAR_DEC);
	eat(Token::KEYWORD);
	eatType();
	checkVarDec();
	eat(Token::IDENTIFIER);
	while (eat(',', true) == Status::OK) {
		checkVarDec();
		eat(Token::IDENTIFIER);
	}
	eat(';');
	outFile << makeCloseTag(NonTerminal::CLASS_VAR_DEC);
}

void CompilationEngine::compileSubroutineDec()
{
	outFile << makeOpenTag(NonTerminal::SUBROUTINE_DEC);
	eat(Token::KEYWORD);
	eatTypeWithVoid();
	eat(Token::IDENTIFIER);
	eat('(');
	compileParameterList();
	eat(')');
	compileSubroutineBody();
	outFile << makeCloseTag(NonTerminal::SUBROUTINE_DEC);
}

void CompilationEngine::compileParameterList()
{
	outFile << makeOpenTag(NonTerminal::PARAMETER_LIST, true);
	if (eatType(true) == Status::OK) {
		eat(Token::IDENTIFIER);
		while (eat(',', true) == Status::OK) {
			eatType();
			eat(Token::IDENTIFIER);
		}
	}
	// list can be empty
	outFile << makeCloseTag(NonTerminal::PARAMETER_LIST, true);
}

void CompilationEngine::compileSubroutineBody()
{
	outFile << makeOpenTag(NonTerminal::SUBROUTINE_BODY);
	eat('{');
	while (!tk->aborted() && tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::VAR) {
		compileVarDec();
	}
	compileStatements();
	eat('}');
	outFile << makeCloseTag(NonTerminal::SUBROUTINE_BODY);
}

void CompilationEngine::compileVarDec()
{
	outFile << makeOpenTag(NonTerminal::VAR_DEC);
	eat(Keyword::VAR);
	eatType();
	checkVarDec();
	eat(Token::IDENTIFIER);
	while (eat(',', true) == Status::OK) {
		checkVarDec();
		eat(Token::IDENTIFIER);
	}
	eat(';');
	outFile << makeCloseTag(NonTerminal::VAR_DEC);
}

void CompilationEngine::compileStatements()
{
	outFile << makeOpenTag(NonTerminal::STATEMENTS);
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
	outFile << makeCloseTag(NonTerminal::STATEMENTS);
}

void CompilationEngine::compileLet()
{
	outFile << makeOpenTag(NonTerminal::LET);
	eat(Keyword::LET);
	auto varsIt = definedVars.find(tk->stringVal());
	if (varsIt == definedVars.end()) {
		cout << brightError << " at line " << tk->currPos() << ": Attempting to assign value to undeclared variable \"" << tk->stringVal() << "\"" << endl;
	}
	eat(Token::IDENTIFIER);
	if (eat('[', true) == Status::OK) {
		compileExpression();
		eat(']');
	}
	eat('=');
	compileExpression();
	eat(';');
	outFile << makeCloseTag(NonTerminal::LET);
}

void CompilationEngine::compileIf()
{
	outFile << makeOpenTag(NonTerminal::IF);
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
	outFile << makeCloseTag(NonTerminal::IF);
}

void CompilationEngine::compileWhile()
{
	outFile << makeOpenTag(NonTerminal::WHILE);
	eat(Keyword::WHILE);
	eat('(');
	compileExpression();
	eat(')');
	eat('{');
	compileStatements();
	eat('}');
	outFile << makeCloseTag(NonTerminal::WHILE);
}

void CompilationEngine::compileDo()
{
	outFile << makeOpenTag(NonTerminal::DO);
	eat(Keyword::DO);
	eat(Token::IDENTIFIER);
	if (eat('.', true) == Status::OK) {
		eat(Token::IDENTIFIER);
	}
	eat('(');
	compileExpressionList();
	eat(')');
	eat(';');
	outFile << makeCloseTag(NonTerminal::DO);
}

void CompilationEngine::compileReturn()
{
	outFile << makeOpenTag(NonTerminal::RETURN);
	eat(Keyword::RETURN);
	if (eat(';', true) == Status::OK) {
		outFile << makeCloseTag(NonTerminal::RETURN);
		return;
	}
	compileExpression();
	eat(';');
	outFile << makeCloseTag(NonTerminal::RETURN);
}

void CompilationEngine::compileExpression()
{
	outFile << makeOpenTag(NonTerminal::EXPRESSION);
	compileTerm();
	while (eatOp(true) == Status::OK) {
		compileTerm();
	}
	outFile << makeCloseTag(NonTerminal::EXPRESSION);
}

bool isKeywordConst(Keyword key) {
	return (key == Keyword::TRUE || key == Keyword::FALSE || key == Keyword::K_NULL || key == Keyword::THIS);
}

void CompilationEngine::compileTerm()
{
	outFile << makeOpenTag(NonTerminal::TERM);
	if (eat(Token::IDENTIFIER, true) == Status::OK) {
		if (eat('[', true) == Status::OK) {
			compileExpression();
			eat(']');
		}
		else if (eat('(', true) == Status::OK) {
			compileExpressionList();
			eat(')');
		}
		else if (eat('.', true) == Status::OK) {
			eat(Token::IDENTIFIER);
			eat('(');
			compileExpressionList();
			eat(')');
		}
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
	else if (eat('-', true) == Status::OK || eat('~', true) == Status::OK) {
		compileTerm();
	}

	outFile << makeCloseTag(NonTerminal::TERM);
}

void CompilationEngine::compileExpressionList()
{
	outFile << makeOpenTag(NonTerminal::EXPRESSION_LIST);
	if (tk->tokenType() == Token::SYMBOL && tk->symbol() == ')') {
		outFile << makeCloseTag(NonTerminal::EXPRESSION_LIST);
		return;
	}
	compileExpression();
	while (eat(',', true) == Status::OK) {
		compileExpression();
	}
	outFile << makeCloseTag(NonTerminal::EXPRESSION_LIST);
}

void CompilationEngine::close()
{
	if (outFile.is_open()) {
		cout << "Closing file." << endl;
		outFile.close();
	}
}

bool CompilationEngine::didFailOpen()
{
	return failedOpen;
}

CompilationEngine::~CompilationEngine()
{
}
