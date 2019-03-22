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

	LZOC::LZOC(OperatorPtr parentOp, Target* target, int wIn_, int whatToCount_) :
		Operator( parentOp, target), wIn(wIn_), whatToCount(whatToCount_) {
		ostringstream currLevel, currDigit, nextLevel;

		srcFileName = "LZOC";
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007)");

		ostringstream name;
		if(whatToCount==0)
			name <<"LZC_"<<wIn;
		else if(whatToCount==1)
			name <<"LOC_"<<wIn;
		else if(whatToCount==-1)
			name <<"LZOC_"<<wIn;
		else
			THROWERROR("whatToCount should be 0, 1 or -1; got " << whatToCount);

		setNameWithFreqAndUID(name.str());

		// -------- Parameter set up -----------------
		wOut = intlog2(wIn);
		p2wOut = 1<<wOut; // no need for GMP here


		addInput ("I", wIn);
		if(whatToCount==-1)
			addInput ("OZB");
		addOutput("O", wOut);

		if(whatToCount==-1)
			vhdl << tab << declare("sozb") <<" <= OZB;" << endl;

		currLevel << "level"<<wOut;
		ostringstream padStr;
		if (wIn==intpow2(wOut)) padStr<<"";
		else padStr << "& ("<<intpow2(wOut)-wIn-1 << " downto 0 => ";
		if(whatToCount==0)
			padStr <<"'1'";
		else if(whatToCount==1)
			name <<"'0'";
		else if(whatToCount==-1)
			padStr <<"not(sozb)";
		padStr <<")";

		vhdl << tab << declare(currLevel.str(),intpow2(wOut)) << "<= I" << padStr.str() <<";"<<endl; //zero padding if necessary
		//each operation is formed of a comparison followed by a multiplexing

		for (int i=wOut;i>=1;i--){
			currDigit.str(""); currDigit << "digit" << i ;
			currLevel.str(""); currLevel << "level" << i;
			nextLevel.str(""); nextLevel << "level" << i-1;
			double levelDelay = intlog(mpz_class(getTarget()->lutInputs()), intpow2(i-1)) * getTarget()->lutDelay();

			vhdl << tab <<declare(levelDelay, currDigit.str()) << "<= '1' when " << currLevel.str() << "("<<intpow2(i)-1<<" downto "<<intpow2(i-1)<<") = ";
			if(whatToCount==0)
				vhdl << zg(intpow2(i-1));
			else if(whatToCount==1)
				vhdl << og(intpow2(i-1));
			else if(whatToCount==-1)
				vhdl <<"("<<intpow2(i)-1<<" downto "<<intpow2(i-1)<<" => sozb)";
			vhdl << " else '0';"<<endl;

			if (i>1){
				double levelDelay =  getTarget()->logicDelay(6);
				vhdl << tab << declare(levelDelay, nextLevel.str(),intpow2(i-1)) << "<= "<<currLevel.str() << "("<<intpow2(i-1)-1<<" downto 0) when " << currDigit.str()<<"='1' "
					  <<"else "<<currLevel.str()<<"("<<intpow2(i)-1<<" downto "<<intpow2(i-1)<<");"<<endl;
			}
		}

		vhdl << tab << "O <= ";
		for (int i=wOut;i>=1;i--){
			currDigit.str(""); currDigit << "digit" << i ;
			vhdl << currDigit.str();
			if (i==1)
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
		int wIn,whatToCount;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn);
		UserInterface::parseInt(args, "whatToCount", &whatToCount);
		return new LZOC(parentOp, target, wIn, whatToCount);
	}



	
	void LZOC::registerFactory(){
		UserInterface::add("LZOC", // name
											 "A leading zero or one counter. The output size is computed.",
											 "ShiftersLZOCs", // category
											 "",
											 "wIn(int): input size in bits;\
                        whatToCount(int)=-1:  0 means count zeroes, 1 means count ones, -1 means add an input that defines what to count", // This string will be parsed
											 "", // no particular extra doc needed
											 LZOC::parseArguments
											 ) ;
		
	}
}
