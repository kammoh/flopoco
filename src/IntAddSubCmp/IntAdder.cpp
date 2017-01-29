/*
An integer adder for FloPoCo

It may be pipelined to arbitrary frequency.
Also useful to derive the carry-propagate delays for the subclasses of Target

Authors:  Bogdan Pasca, Florent de Dinechin

This file is part of the FloPoCo project
developed by the Arenaire team at Ecole Normale Superieure de Lyon

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
  All rights reserved.
*/

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "IntAdder.hpp"

//#include "Attic/IntAdderClassical.hpp"
//#include "Attic/IntAdderAlternative.hpp"
//#include "Attic/IntAdderShortLatency.hpp"

using namespace std;
namespace flopoco {

	IntAdder::IntAdder ( Target* target, int wIn_, int optimizeType, bool srl, int implementation):
		IntAdder(nullptr, target, wIn_, optimizeType, srl, implementation)
	{
	}

	IntAdder::IntAdder (OperatorPtr parentOp, Target* target, int wIn_, int optimizeType, bool srl, int implementation):
		Operator (target), wIn ( wIn_ )
	{
		srcFileName="IntAdder";
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2016)" );
		ostringstream name;
		name << "IntAdder_" << wIn;
		setNameWithFreqAndUID(name.str());
		setParentOperator(parentOp);
													
		// Set up the IO signals
		addInput  ("X"  , wIn, true);
		addInput  ("Y"  , wIn, true);
		addInput  ("Cin");
		addOutput ("R"  , wIn, 1 , true);

		schedule();
		double targetPeriod = 1.0/getTarget()->frequency() - getTarget()->ffDelay();
		// What is the maximum lexicographic time of our inputs?
		int maxCycle = 0;
		double maxCP = 0.0;
		for(auto i: ioList_) {
			//REPORT(DEBUG, "signal " << i->getName() <<  "  Cycle=" << i->getCycle() <<  "  criticalPath=" << i->getCriticalPath() );
			if((i->getCycle() > maxCycle)
					|| ((i->getCycle() == maxCycle) && (i->getCriticalPath() > maxCP)))	{
				maxCycle = i->getCycle();
				maxCP = i->getCriticalPath();
			}
		}
		double totalPeriod = maxCP + getTarget()->adderDelay(wIn);

		REPORT(DETAILED, "maxCycle=" << maxCycle <<  "  maxCP=" << maxCP <<  "  totalPeriod=" << totalPeriod <<  "  targetPeriod=" << targetPeriod );

		if(totalPeriod <= targetPeriod)		{
			REPORT(DEBUG, "1 " << getTarget()->adderDelay(wIn));
			vhdl << tab << declare(getTarget()->adderDelay(wIn),"Rtmp", wIn); // just to use declare()
			REPORT(DEBUG, "2");
			vhdl << " <= X + Y + Cin;" << endl; 
			REPORT(DEBUG, "3");
			vhdl << tab << "R <= Rtmp;" << endl;

		}

		else		{
			
			// Here we split into chunks.
			double remainingSlack = targetPeriod-maxCP;
			int firstSubAdderSize = getMaxAdderSizeForPeriod(remainingSlack) - 2;
			int maxSubAdderSize = getMaxAdderSizeForPeriod(targetPeriod) - 2;

			bool loop=true;
			int subAdderSize=firstSubAdderSize;
			int previousSubAdderSize;
			int subAdderFirstBit = 0;
			int i=0; 
			while(loop) {
				REPORT(DETAILED, "Sub-adder " << i << " : first bit=" << subAdderFirstBit << ",  size=" <<  subAdderSize);
				// Cin
				if(subAdderFirstBit == 0)	{
					vhdl << tab << declare("Cin_0") << " <= Cin;" << endl;
				}else	 {
					vhdl << tab << declare(join("Cin_", i)) << " <= " << join("S_", i-1) <<	of(previousSubAdderSize) << ";" << endl;
				}
				// operands
				vhdl << tab << declare(join("X_", i), subAdderSize+1) << " <= '0' & X"	<<	range(subAdderFirstBit+subAdderSize-1, subAdderFirstBit) << ";" << endl;
				vhdl << tab << declare(join("Y_", i), subAdderSize+1) << " <= '0' & Y"	<<	range(subAdderFirstBit+subAdderSize-1, subAdderFirstBit) << ";" << endl;
				vhdl << tab << declare(getTarget()->adderDelay(subAdderSize+1), join("S_", i), subAdderSize+1)
						 << " <= X_" << i	<<	" + Y_" << i << " + Cin_" << i << ";" << endl;
				vhdl << tab << declare(join("R_", i), subAdderSize) << " <= S_" << i	<<	range(subAdderSize-1,0) << ";" << endl;
				// prepare next iteration
				i++;
				subAdderFirstBit += subAdderSize;
				previousSubAdderSize = subAdderSize;
				if (subAdderFirstBit==wIn)
					loop=false;
				else
					subAdderSize = min(wIn-subAdderFirstBit, maxSubAdderSize);
			}

			vhdl << tab << "R <= ";
			while(i>0)		{
				i--;
				vhdl <<  "R_" << i << (i==0?" ":" & ");
			}
			vhdl << ";" << endl;
		}
		REPORT(DEBUG, "Done");

	}


	int IntAdder::getMaxAdderSizeForPeriod(double targetPeriod) {
		int count = 10; // You have to add something eventually
		while(getTarget()->adderDelay(count) < targetPeriod)
			count++;
		return count-1;
	}



	/*************************************************************************/
	IntAdder::~IntAdder() {
	}

	/*************************************************************************/
	void IntAdder::emulate ( TestCase* tc ) {
		// get the inputs from the TestCase
		mpz_class svX = tc->getInputValue ( "X" );
		mpz_class svY = tc->getInputValue ( "Y" );
		mpz_class svC = tc->getInputValue ( "Cin" );

		// compute the multiple-precision output
		mpz_class svR = svX + svY + svC;
		// Don't allow overflow: the output is modulo 2^wIn
		svR = svR & ((mpz_class(1)<<wIn)-1);

		// complete the TestCase with this expected output
		tc->addExpectedOutput ( "R", svR );
	}


	OperatorPtr IntAdder::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int wIn;
		int arch;
		int optObjective;
		bool srl;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn, false);
		UserInterface::parseInt(args, "arch", &arch, false);
		UserInterface::parseInt(args, "optObjective", &optObjective, false);
		UserInterface::parseBoolean(args, "SRL", &srl, false);
		return new IntAdder(target, wIn,optObjective,srl,arch);
	}

	void IntAdder::registerFactory(){
		UserInterface::add("IntAdder", // name
											 "Integer adder. In modern VHDL, integer addition is expressed by a + and one usually needn't define an entity for it. However, this operator will be pipelined if the addition is too large to be performed at the target frequency.",
											 "BasicInteger", // category
											 "",
											 "wIn(int): input size in bits;\
					  arch(int)=-1: -1 for automatic, 0 for classical, 1 for alternative, 2 for short latency; \
					  optObjective(int)=2: 0 to optimize for logic, 1 to optimize for register, 2 to optimize for slice/ALM count; \
					  SRL(bool)=true: optimize for shift registers",
											 "",
											 IntAdder::parseArguments
											 );
		
	}


}


