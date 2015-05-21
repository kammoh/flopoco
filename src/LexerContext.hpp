#ifndef __LEXER_CONTEXT_HPP__
#define __LEXER_CONTEXT_HPP__

#include <iostream>
#include <vector>

#include "utils.hpp"

using namespace std;
using namespace flopoco;

class LexerContext {
public:
	typedef enum {
		unset,
		signalAssignment,
		conditionalSignalAssignment,
		selectedSignalAssignment,
		selectedSignalAssignment2,
		selectedSignalAssignment3,
		variableAssignment,
		instance,
		process,
		caseStatement
	} LexMode;

	void* scanner;
	int result;
	istream* is;
	ostream* os;
	int yyTheCycle;
	vector<pair<string, int> > theUseTable;

	string lhsName;
	vector<string> extraRhsNames;
	vector<triplet<string, string, int>> dependenceTable;

	LexMode lexingMode;

public:
	LexerContext(istream* is = &cin, ostream* os = &cout) {
		init_scanner();
		this->is = is;
		this->os = os;
		yyTheCycle=0;

		lhsName = "";
		extraRhsNames.clear();
		dependenceTable.clear();
		lexingMode = LexMode::unset;

	}

	//these methods are generated in VHDLLexer.cpp 

	void lex();

	virtual ~LexerContext() { destroy_scanner();}

protected:
	void init_scanner();

	void destroy_scanner();
};



#endif
