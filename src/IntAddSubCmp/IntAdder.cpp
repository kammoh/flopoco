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

	IntAdder::IntAdder ( OperatorPtr parentOp, Target* target, int wIn_, int optimizeType, bool srl, int implementation):
		Operator ( target), wIn ( wIn_ )
	{
		srcFileName="IntAdder";
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2016)" );
		ostringstream name;
		name << "IntAdder_" << wIn;
		setNameWithFreqAndUID(name.str());
		double targetPeriod, totalPeriod;
		int maxCycle = 0;
		double maxCp = 0.0;

		setParentOperator(parentOp);
													
		// Set up the IO signals
		addInput  ("X"  , wIn, true);
		addInput  ("Y"  , wIn, true);
		addInput  ("Cin");
		addOutput ("R"  , wIn, 1 , true);

		schedule();

		targetPeriod = 1.0/target->frequency() - target->ffDelay();

		if((ioList_[0]->getCycle() > maxCycle)
				|| ((ioList_[0]->getCycle() == maxCycle) && (ioList_[0]->getCriticalPath() > maxCp)))
		{
			maxCycle = ioList_[0]->getCycle();
			maxCp = ioList_[0]->getCriticalPath();
		}
		for(unsigned i=1; i<ioList_.size(); i++)
			if((ioList_[i]->getCycle() > maxCycle)
					|| ((ioList_[i]->getCycle() == maxCycle) && (ioList_[i]->getCriticalPath() > maxCp)))
			{
				maxCycle = ioList_[i]->getCycle();
				maxCp = ioList_[i]->getCriticalPath();
			}

		totalPeriod = maxCp + target->adderDelay(wIn+1);

		if(totalPeriod <= targetPeriod)
		{
			vhdl << tab << " R <= X + Y + Cin;" << endl;
			REPORT(INFO, "Adder " << name.str() << " adding delay " << target->adderDelay(wIn+1));
			getSignalByName("R")->setCriticalPathContribution(target->adderDelay(wIn+1));
			scheduleSignal(getSignalByName("R"));
		}else
		{
			int maxAdderSize = getMaxAdderSizeForFreq(false);
			int chunks = wIn/maxAdderSize, lastChunkSize = 0;
			if(chunks*maxAdderSize < wIn)
			{
				chunks++;
				lastChunkSize = wIn - (chunks-1)*maxAdderSize;
			}
			REPORT(DETAILED, "chunks=" <<  chunks << " lastChunkSize=" << lastChunkSize);

			for(int i=0; i<chunks; i++)
			{
				if(i == 0)
				{
					vhdl << tab << declare("Cin_chunk_0") << " <= Cin;" << endl;
				}else
				{
					vhdl << tab << declare(0.0, join("Cin_chunk_", i)) << " <= R_chunk_" << i-1
							<<	of(maxAdderSize) << ";" << endl;
				}

				if((i == chunks-1) && (lastChunkSize != maxAdderSize))
				{
					vhdl << tab << declare(0.0, join("X_chunk_", i), lastChunkSize+1) << " <= '0' & X"
							<<	range(lastChunkSize-1+i*maxAdderSize, i*maxAdderSize) << ";" << endl;
					vhdl << tab << declare(0.0, join("Y_chunk_", i), lastChunkSize+1) << " <= '0' & Y"
							<<	range(lastChunkSize-1+i*maxAdderSize, i*maxAdderSize) << ";" << endl;
					vhdl << tab << declare(target->adderDelay(lastChunkSize+1), join("R_chunk_", i), lastChunkSize+1) << " <= X_chunk_" << i
							<<	" + Y_chunk_" << i << " + Cin_chunk_" << i << ";" << endl;
				}else
				{
					vhdl << tab << declare(0.0, join("X_chunk_", i), maxAdderSize+1) << " <= '0' & X"
							<<	range((i+1)*maxAdderSize-1, i*maxAdderSize) << ";" << endl;
					vhdl << tab << declare(0.0, join("Y_chunk_", i), maxAdderSize+1) << " <= '0' & Y"
							<<	range((i+1)*maxAdderSize-1, i*maxAdderSize) << ";" << endl;
					vhdl << tab << declare(target->adderDelay(maxAdderSize+1), join("R_chunk_", i), maxAdderSize+1) << " <= X_chunk_" << i
							<<	" + Y_chunk_" << i << " + Cin_chunk_" << i << ";" << endl;
				}

				vhdl << endl;
			}

			vhdl << tab << "R <=";
			for(int i=chunks-1; i>=0; i--)
			{
				vhdl << (i==(chunks-1)?" ":" & ") << "R_chunk_" << i;
				if((i == chunks-1) && (lastChunkSize != maxAdderSize))
					vhdl << range(lastChunkSize-1, 0);
				else
					vhdl << range(maxAdderSize-1, 0);
			}
			vhdl << ";" << endl;
		}


	}


	int IntAdder::getMaxAdderSizeForFreq(bool hasFF)
	{
		int count = 1;
		double targetPeriod = 1.0/getTarget()->frequency();

		while(getTarget()->adderDelay(count)+(hasFF?getTarget()->ffDelay():0) < targetPeriod)
			count++;
		if(getTarget()->adderDelay(count)+(hasFF?getTarget()->ffDelay():0) > targetPeriod)
			count--;

		return count;
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


	OperatorPtr IntAdder::parseArguments(Target *target, vector<string> &args) {
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


