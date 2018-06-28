/*
  Function Table for FloPoCo

  Authors: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,

  All rights reserved.

  */


#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "../utils.hpp"

#include "FixFunctionByTable.hpp"

using namespace std;

namespace flopoco{

	FixFunctionByTable::FixFunctionByTable(OperatorPtr parentOp, Target* target, string func, bool signedIn, int lsbIn, int msbOut, int lsbOut, int logicTable)		:
		Table(parentOp, target)

	{}
#if 0

		,
		Table::wIn(-lsbIn + (signedIn?1:0)),
		wOut(msbOut-lsbOut+1),
		logicTable(logicTable)
	{

		f = new FixFunction(func, signedIn, lsbIn, msbOut, lsbOut);
		ostringstream name;
		srcFileName="FixFunctionByTable";

		name<<"FixFunctionByTable";
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Florent de Dinechin (2010-2018)");
		addHeaderComment("-- Evaluator for " +  f-> getDescription() + "\n");

	}

#endif
	FixFunctionByTable::~FixFunctionByTable() {
		free(f);
	}


	// This replaces the actual constructor and returns a Table object that is not this.
	// It works as long as the table is built using its factory interface.
	OperatorPtr FixFunctionByTable::buildTableOperator() {
#if 0
		vector<mpz_class> v;
		for(int i=0; i<(1<<wIn); i++) {
			v.push_back(function(i));	OperatorPtr FixFunctionByTable::buildTableOperator
		};
		actualFunctionTable =  new Table(getParentOp(), getTarget(), v);
		return actualFunctionTable;
#endif
	}
	
	//overloading the method from table
	mpz_class FixFunctionByTable::function(int x){
		mpz_class ru, rd;
		f->eval(mpz_class(x), rd, ru, true);
		return rd;
	}


	void FixFunctionByTable::emulate(TestCase* tc){
		f->emulate(tc, true /* correct rounding */);
	}

	OperatorPtr FixFunctionByTable::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
	{
		bool signedIn;
		int lsbIn, msbOut, lsbOut;
		string f;
		UserInterface::parseString(args, "f", &f);
		UserInterface::parseBoolean(args, "signedIn", &signedIn);
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseInt(args, "msbOut", &msbOut);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		return new FixFunctionByTable(target, f, signedIn, lsbIn, msbOut, lsbOut);
	}

	void FixFunctionByTable::registerFactory()
	{
		UserInterface::add("FixFunctionByTable", // name
											 "Evaluator of function f on [0,1) or [-1,1), depending on signedIn, using a table.",
											 "FunctionApproximation",
											 "",
											 "f(string): function to be evaluated between double-quotes, for instance \"exp(x*x)\";\
signedIn(bool): true if the function input is [-1,1), false if it is [0,1);\
lsbIn(int): weight of input LSB, for instance -8 for an 8-bit input;\
msbOut(int): weight of output MSB;\
lsbOut(int): weight of output LSB;",
											 "This operator uses a table to store function values.",
											 FixFunctionByTable::parseArguments
											 ) ;
	}
}
