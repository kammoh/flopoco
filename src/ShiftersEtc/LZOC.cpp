/*
  A leading zero/one counter for FloPoCo

  Author : Florent de Dinechin, Bogdan Pasca

   This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2011.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "LZOC.hpp"

using namespace std;


namespace flopoco{

	LZOC::LZOC(OperatorPtr parentOp, Target* target, int wIn_, int countType_) :
		Operator( parentOp, target), wIn(wIn_), countType(countType_) {

		srcFileName = "LZOC";
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007)");

		ostringstream name;
		if(countType==0)
			name <<"LZC_"<<wIn;
		else if(countType==1)
			name <<"LOC_"<<wIn;
		else if(countType==-1)
			name <<"LZOC_"<<wIn;
		else
			THROWERROR("countType should be 0, 1 or -1; got " << countType);

		setNameWithFreqAndUID(name.str());

		// -------- Parameter set up -----------------
		wOut = intlog2(wIn);
		p2wOut = 1<<wOut; // no need for GMP here

		
		addInput ("I", wIn);
		if(countType==-1)
			addInput ("OZB");
		addOutput("O", wOut);
		
		if(countType==-1)
			vhdl << tab << declare("sozb") <<" <= OZB;" << endl;

		vhdl << tab << declare(join("level", wOut), wIn) << " <= I;"<<endl; 
		//each operation is formed of a comparison followed by a multiplexing
		
		int currLevelSize=wIn, prevLevelSize=0;
		string currDigit, prevLevel, currLevel;
		int i=wOut-1;
		while (i>=0){
			//		while (currLevelSize > target->lutInputs+1){
			prevLevelSize = currLevelSize;
			currLevelSize = intpow2(i);
			
			currDigit = join("digit", i);
			prevLevel = join("level", i+1);
			currLevel = join("level", i);
			//			double levelDelay = intlog(mpz_class(getTarget()->lutInputs()), intpow2(i-1)) * getTarget()->lutDelay();
			//REPORT(0,currDigit);
			vhdl << tab <<declare(currDigit) << "<= '1' when " << prevLevel << range(prevLevelSize-1, prevLevelSize-intpow2(i)) << " = ";
			int bitsToCount=prevLevelSize-intpow2(i);
			if(countType==0)
				vhdl << zg(bitsToCount);
			else if(countType==1)
				vhdl << og(bitsToCount);
			else if(countType==-1)
				vhdl <<"("<<prevLevelSize-1<<" downto " << prevLevelSize-intpow2(i) << " => sozb)";
			vhdl << " else '0';"<<endl;

			if (i>0){
				double levelDelay =  getTarget()->logicDelay(6);
				vhdl << tab << declare(currLevel, currLevelSize) << "<= "<<prevLevel << range(prevLevelSize-intpow2(i)-1,0) << " & ";
				if(countType==0)
					vhdl << og(currLevelSize-(prevLevelSize-intpow2(i)));
				else if(countType==1)
					vhdl << zg(currLevelSize-(prevLevelSize-intpow2(i)));
				else if(countType==-1)
					vhdl <<"("<<currLevelSize-1<<" downto " << prevLevelSize-intpow2(i) << " => not sozb)";
				
				vhdl << " when " << currDigit << "='1' "
						 <<"else " << prevLevel << range(prevLevelSize-1, prevLevelSize-intpow2(i)) << ";"<<endl;
			}
			i--;
		}

		vhdl << tab << "O <= ";
		for (int i=wOut-1;i>=0;i--){
			currDigit = join( "digit", i) ;
			vhdl << currDigit;
			if (i==0)
				vhdl << ";"<<endl;
			else
				vhdl << " & ";
		}

	}

	LZOC::~LZOC() {}



	void LZOC::emulate(TestCase* tc)
	{
		mpz_class si   = tc->getInputValue("I");
		mpz_class sozb = tc->getInputValue("OZB");
		mpz_class so;

		int j;
		int bit = (sozb == 0) ? 0 : 1;
		for (j = 0; j < wIn; j++)
			{
				if (mpz_tstbit(si.get_mpz_t(), wIn - j - 1) != bit)
					break;
			}

		so = j;
		tc->addExpectedOutput("O", so);
	}


	OperatorPtr LZOC::parseArguments(OperatorPtr parentOp, Target *target, std::vector<std::string> &args) {
		int wIn,countType;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn);
		UserInterface::parseInt(args, "countType", &countType);
		return new LZOC(parentOp, target, wIn, countType);
	}



	
	void LZOC::registerFactory(){
		UserInterface::add("LZOC", // name
											 "A leading zero or one counter. The output size is computed.",
											 "ShiftersLZOCs", // category
											 "",
											 "wIn(int): input size in bits;\
                        countType(int)=-1:  0 means count zeroes, 1 means count ones, -1 means add an input that defines what to count", // This string will be parsed
											 "", // no particular extra doc needed
											 LZOC::parseArguments
											 ) ;
		
	}
}
