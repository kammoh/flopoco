/*
 * An adder of n integers, using a bit heap
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 * 
 * Authors : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr
 *
 * Initial software.
 * Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 
 * 2008-2011.
 * All rights reserved.
 */



#include "../Operator.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "IntAdderTree.hpp"

using namespace std;


namespace flopoco{




	
	//standalone operator
	IntAdderTree::IntAdderTree(OperatorPtr parentOp, Target* target, unsigned int wIn_, unsigned int n_, bool signedInput_):
		Operator(parentOp, target),
		wIn(wIn_),
		n(n_),
		signedInput(signedInput_)
	{
		srcFileName="IntAdderTree";
		setCopyrightString ( "Florent de Dinechin (2008-2016)" );
		ostringstream name;
		name << "IntAdderTree_" << wIn << "_" << n;
		setNameWithFreqAndUID(name.str());

		BitheapNew* bh;
		wOut = wIn+intlog2(n);
		addOutput("R", wOut);
		bh = new BitheapNew(this, wOut, signedInput); 

		for (unsigned int i=0; i<n; i++) {
			string xi=join("X",i);
			string ixi=join("iX",i);
			addInput  (join("X",i)  , wIn, true);
			vhdl << tab << declareFixPoint(ixi, signedInput, wIn-1, 0) << " <= " << (signedInput?"":"un")<< "signed(" << xi << ");" << endl; // simplest way to get signedness in the signal
			bh->addSignal(ixi);
		}


		//compress the bitheap and produce the result
		bh->startCompression();

		// Retrieve the bits we want from the bit heap
		vhdl << tab << declare("OutRes", wOut) << " <= " << 
			bh->getSumName() << ";" << endl; // This range is useful in case there was an overflow?

		vhdl << tab << "R <= OutRes;" << endl;
	}




	
	void IntAdderTree::emulate(TestCase* tc)
	{
		// get the inputs from the TestCase
		mpz_class result=0;
		for(unsigned int i=0; i<n; i++) {
			mpz_class xi = tc->getInputValue ( join("X",i) );
			if(signedInput && (xi>>(wIn-1)==1) ) {
				// TODO  
			}
			else{
				result += xi;
			}
		}
			// Don't allow overflow: the output is modulo 2^wIn
		result = result & ((mpz_class(1)<<wOut)-1);

		// complete the TestCase with this expected output
		tc->addExpectedOutput ( "R", result );
	}





	
	TestList IntAdderTree::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		return testStateList;
	}

	OperatorPtr IntAdderTree::parseArguments(OperatorPtr parentOp, Target* target, std::vector<std::string> &args)
	{
		int wIn, n;
		bool signedInput;
		UserInterface::parseInt(args, "wIn", &wIn);
		UserInterface::parseInt(args, "n", &n);
		UserInterface::parseBoolean(args, "signedInput", &signedInput);
		return new IntAdderTree(parentOp, target, wIn, n, signedInput);
	}

	void IntAdderTree::registerFactory()
	{
		UserInterface::add(
				"IntAdderTree",
				"A component adding n integers, bitheap based (as the name doesn't suggest). Output size is computed",
				"BasicInteger",
				"",
				"signedInput(bool): 0=unsigned, 1=signed; \
			  n(int): number of inputs to add;\
			  wIn(int): input size, in bits;",
				"",
				IntAdderTree::parseArguments,
				IntAdderTree::unitTest
		);
	}

	
}





