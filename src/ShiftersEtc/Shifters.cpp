/*
  A barrel shifter for FloPoCo

  Authors: Florent de Dinechin, Bogdan Pasca

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.


*/

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "Shifters.hpp"

using namespace std;

// TODO there is a small inefficiency here, as most bits of s don't need to be copied all the way down

namespace flopoco{


	Shifter::Shifter(Target* target, int wIn, int maxShift, ShiftDirection direction) :
			Operator(target), wIn_(wIn), maxShift_(maxShift), direction_(direction)
	{
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2016)" );
		srcFileName = "Shifters";
		ostringstream name;
		if(direction_==Left) name <<"LeftShifter_";
		else                 name <<"RightShifter_";
		name<<wIn_<<"_by_max_"<<maxShift;
		setNameWithFreqAndUID(name.str());

		
		REPORT(DETAILED, " wIn="<<wIn<<" maxShift="<<maxShift<<" direction="<< (direction == Right?  "RightShifter": "LeftShifter") );

		// -------- Parameter set up -----------------
		wOut_         = wIn_ + maxShift_;
		wShiftIn_     = intlog2(maxShift_);

		addInput ("X", wIn_);
		addInput ("S", wShiftIn_);
		addOutput("R", wOut_);

		//vhdl << tab << declare(getTarget()->localWireDelay(), "level0", wIn_) << "<= X;" << endl;
		vhdl << tab << declare("level0", wIn_) << "<= X;" << endl;
		//vhdl << tab << declare(getTarget()->localWireDelay(), "ps", wShiftIn_) << "<= S;" << endl;
		vhdl << tab << declare("ps", wShiftIn_) << "<= S;" << endl;

		// Pipelining
		// The theory is that the number of shift levels that the tools should be able to pack in a row of LUT-k is ceil((k-1)/2)
		// Actually there are multiplexers in CLBs etc that can also be used, so let us keep it simple at k/2. It works on Virtex6, TODO test on Stratix.
		int levelPerLut = target->lutInputs()/2; // this is a floor so it is the same as the formula above
		REPORT(DETAILED, "Trying to pack " << levelPerLut << " levels in a row of LUT" << target->lutInputs())
		int levelInLut=0;
		double levelDelay;
		
		for(int currentLevel=0; currentLevel<wShiftIn_; currentLevel++){
			levelInLut ++;
			if (levelInLut >= levelPerLut) {
				levelDelay=target->lutDelay() + target->localWireDelay(wIn+intpow2(currentLevel+1)-1);
				levelInLut=0;
			}
			else // this level incurs no delay
				levelDelay=0;
			
			REPORT (DEBUG, "delay of level " << currentLevel <<" is " << levelDelay); 
			ostringstream currentLevelName, nextLevelName;
			currentLevelName << "level"<<currentLevel;
			nextLevelName << "level"<<currentLevel+1;
			if (direction==Right){
				vhdl << tab << declare(levelDelay, nextLevelName.str(),wIn+intpow2(currentLevel+1)-1 )
					  <<"<=  ("<<intpow2(currentLevel)-1 <<" downto 0 => '0') & "<<currentLevelName.str()<<" when ps";
				if (wShiftIn_ > 1)
					vhdl << "(" << currentLevel << ")";
				vhdl << " = '1' else "
					  << tab << currentLevelName.str() <<" & ("<<intpow2(currentLevel)-1<<" downto 0 => '0');"<<endl;
			}else{
				vhdl << tab << declare(levelDelay, nextLevelName.str(),wIn+intpow2(currentLevel+1)-1 )
					  << "<= " << currentLevelName.str() << " & ("<<intpow2(currentLevel)-1 <<" downto 0 => '0') when ps";
				if (wShiftIn_>1)
					vhdl << "(" << currentLevel<< ")";
				vhdl << "= '1' else "
					  << tab <<" ("<<intpow2(currentLevel)-1<<" downto 0 => '0') & "<< currentLevelName.str() <<";"<<endl;
			}

		}
		//update the output slack

		ostringstream lastLevelName;
		lastLevelName << "level"<<wShiftIn_;
		if (direction==Right)
			vhdl << tab << "R <= "<<lastLevelName.str()<<"("<< wIn + intpow2(wShiftIn_)-1-1 << " downto " << wIn_ + intpow2(wShiftIn_)-1 - wOut_ <<");"<<endl;
		else
			vhdl << tab << "R <= "<<lastLevelName.str()<<"("<< wOut_-1 << " downto 0);"<<endl;

	}

	Shifter::~Shifter() {
	}


	void Shifter::emulate(TestCase* tc)
	{
		mpz_class sx = tc->getInputValue("X");
		mpz_class ss = tc->getInputValue("S");
		mpz_class sr ;

		mpz_class shiftedInput = sx;
		int i;

		if (direction_==Left){
			mpz_class shiftAmount = ss;
			for (i=0;i<shiftAmount;i++)
				shiftedInput=shiftedInput*2;

			for (i= wIn_+intpow2(wShiftIn_)-1-1; i>=wOut_;i--)
				if ( mpzpow2(i) <= shiftedInput )
					shiftedInput-=mpzpow2(i);
		}else{
			mpz_class shiftAmount = maxShift_-ss;

			if (shiftAmount > 0){
				for (i=0;i<shiftAmount;i++)
					shiftedInput=shiftedInput*2;
			}else{
				for (i=0;i>shiftAmount;i--)
					shiftedInput=shiftedInput/2;
			}
		}

		sr=shiftedInput;
		tc->addExpectedOutput("R", sr);
	}



	OperatorPtr Shifter::parseArguments(Target *target, std::vector<std::string> &args) {
		int wIn, maxShift;
		bool dirArg;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn);
		UserInterface::parseStrictlyPositiveInt(args, "maxShift", &maxShift);
		UserInterface::parseBoolean(args, "dir", &dirArg);
		ShiftDirection dir = (dirArg?Shifter::Right:Shifter::Left);
		return new Shifter(target, wIn, maxShift, dir);
	}



	void Shifter::registerFactory(){
		UserInterface::add("Shifter", // name
											 "A classical barrel shifter. The output size is computed.",
											 "ShiftersLZOCs",
											 "",
											 "wIn(int): input size in bits;   maxShift(int): maximum shift distance in bits;   dir(bool): 0=left, 1=right", // This string will be parsed
											 "", // no particular extra doc needed
											 Shifter::parseArguments
											 ) ;

	}


}
