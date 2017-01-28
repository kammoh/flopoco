/*
  An operator which performes x-y and y-x in parallel for FloPoCo

  Author : Bogdan Pasca, Florent de Dinechin

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
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntDualAddSub.hpp"

using namespace std;


namespace flopoco{

	IntDualAddSub::IntDualAddSub(Target* target, int wIn, int opType):
		Operator(target), wIn_(wIn), opType_(opType)
	{
		ostringstream name;
		srcFileName="IntDualAddSub";
		setCopyrightString("Bogdan Pasca, Florent de Dinechin (2008-2010)");

		if (opType==0) {
			son_ = "yMx";
			name << "IntDualAddSub_";
		}
		else {
			son_ = "xPy";
			name << "IntDualAddSub_";
		}
		name << wIn;
		setNameWithFreqAndUID(name.str());
		useNumericStd();

		// Set up the IO signals
		addInput ("X"  , wIn_, true);
		addInput ("Y"  , wIn_, true);
		addOutput("RxMy", wIn_, 1, true);
		addOutput("R"+son_, wIn_, 1, true);

		schedule();
		double targetPeriod = 1.0/getTarget()->frequency() - getTarget()->ffDelay();
		// What is the maximum lexicographic time of our inputs?
		int maxCycle = 0;
		double maxCP = 0.0;
		for(auto i: ioList_) {
			// REPORT(DEBUG, "signal " << i->getName() <<  "  Cycle=" << i->getCycle() <<  "  criticalPath=" << i->getCriticalPath() );
			if((i->getCycle() > maxCycle)
					|| ((i->getCycle() == maxCycle) && (i->getCriticalPath() > maxCP)))	{
				maxCycle = i->getCycle();
				maxCP = i->getCriticalPath();
			}
		}
		double totalPeriod = maxCP + getTarget()->adderDelay(wIn);

		REPORT(DETAILED, "maxCycle=" << maxCycle <<  "  maxCP=" << maxCP <<  "  totalPeriod=" << totalPeriod <<  "  targetPeriod=" << targetPeriod );


		//		if(totalPeriod <= targetPeriod)		{
		if(true){ 
			vhdl << tab << declare(getTarget()->adderDelay(wIn), "tempRxMy", wIn)
					 << " <= std_logic_vector(unsigned(X) - unsigned(Y));" <<endl;
			vhdl << tab << declare(getTarget()->adderDelay(wIn), "tempR"+son_, wIn)
					 <<" <= "<< (opType_==0 ? "std_logic_vector(unsigned(Y) - unsigned(X));" : "std_logic_vector(unsigned(X) + unsigned(Y));") << endl;
			vhdl << tab << "RxMy <= tempRxMy;" << endl;
			vhdl << tab << "R" << son_ << " <= tempR" << son_ << ";" << endl;
		}

		else		{

		}
	}


#if 0
			

		if (getTarget()->isPipelined()){
			//compute the maximum input delay
			maxInputDelay = 0;
			map<string, double>::iterator iter;
			for (iter = inputDelays.begin(); iter!=inputDelays.end();++iter)
				if (iter->second > maxInputDelay)
					maxInputDelay = iter->second;

			REPORT(DETAILED, "Maximum input delay is "<<	maxInputDelay);

			double	objectivePeriod;
			objectivePeriod = 1/ getTarget()->frequency();

			REPORT(DETAILED, "Objective period is "<< objectivePeriod<<" at an objective frequency of "<<getTarget()->frequency());

			if (objectivePeriod<maxInputDelay){
				//It is the responsability of the previous components to not have a delay larger than the period
			  REPORT(INFO, "Warning, the combinatorial delay at the input of "<<this->getName()<<"is above limit");
			  maxInputDelay = objectivePeriod;
			}

			if (((objectivePeriod - maxInputDelay) - getTarget()->lutDelay())<0)	{
				bufferedInputs = 1;
				maxInputDelay=0;
				getTarget()->suggestSubaddSize(chunkSize_ ,wIn_);
				nbOfChunks = ceil(double(wIn_)/double(chunkSize_));
				cSize = new int[nbOfChunks+1];
				cSize[nbOfChunks-1]=( ((wIn_%chunkSize_)==0)?chunkSize_:wIn_-(nbOfChunks-1)*chunkSize_);
				for (int i=0;i<=nbOfChunks-2;i++)
					cSize[i]=chunkSize_;
			}
			else{
				int cS0;
				bufferedInputs=0;
				int maxInAdd;
				getTarget()->suggestSlackSubaddSize(maxInAdd, wIn_, maxInputDelay);
				//int maxInAdd = ceil(((objectivePeriod - maxInputDelay) - getTarget()->lutDelay())/getTarget()->carryPropagateDelay());
				cS0 = (maxInAdd<=wIn_?maxInAdd:wIn_);
				if ((wIn_-cS0)>0)
					{
						int newWIn = wIn_-cS0;
						getTarget()->suggestSubaddSize(chunkSize_,newWIn);
						nbOfChunks = ceil( double(newWIn)/double(chunkSize_));

						cSize = new int[nbOfChunks+1];
						cSize[0] = cS0;
						cSize[nbOfChunks]=( (( (wIn_-cSize[0])%chunkSize_)==0)?chunkSize_:(wIn_-cSize[0])-(nbOfChunks-1)*chunkSize_);
						for (int i=1;i<=nbOfChunks-1;i++)
							cSize[i]=chunkSize_;
						nbOfChunks++;
					}
				else{
					nbOfChunks=1;
					cSize = new int[1];
					cSize[0] = cS0;
				}
			}

			REPORT(DETAILED, "Buffered Inputs "<<(bufferedInputs?"yes":"no"));
			for (int i=nbOfChunks-1;i>=0;i--)
				REPORT(DETAILED, "chunk size[" <<i<<"]="<<cSize[i]);


			getOutDelayMap()["RxMy"] = getTarget()->adderDelay(cSize[nbOfChunks-1]);
			getOutDelayMap()["R"+son_] = getTarget()->adderDelay(cSize[nbOfChunks-1]);
			REPORT(DETAILED, "Last addition size is "<<cSize[nbOfChunks-1]<< " having a delay of "<<getTarget()->adderDelay(cSize[nbOfChunks-1]));

			//VHDL generation

			if(bufferedInputs)
				nextCycle();

			for (int i=0;i<nbOfChunks;i++){
				int sum=0;
				for (int j=0;j<=i;j++) sum+=cSize[j];
				vhdl << tab << declare(join("sX",i), cSize[i], true ) << " <= X" << range(sum-1, sum-cSize[i]) << ";" << endl;
				vhdl << tab << declare(join("sY",i), cSize[i], true ) << " <= Y" << range(sum-1, sum-cSize[i]) << ";" << endl;
			}
			for (int i=0;i<nbOfChunks; i++){
				// subtraction
				vhdl << tab << declare(join("xMy",i), cSize[i]+1, true) <<"  <= (\"0\" & sX"<<i<<") + (\"0\" & not(sY"<<i<<")) ";
				if(i==0) // carry
					vhdl << "+ '1';"<<endl;
				else
					vhdl << "+ " << join("xMy", i-1) << of(cSize[i-1]) << ";"<<endl;
				// addition or subtraction depending on opType
				if(opType){ //addition
					vhdl << tab << declare(join("xPy",i), cSize[i]+1, true) << " <= (\"0\" & sY"<<i<<") + (\"0\" & sX"<<i<<")";
					if(i>0)
						vhdl << "+ " << join("xPy", i-1) << of(cSize[i-1]) << ";"<<endl;
					else
						vhdl << ";" <<endl;
				}else{ // reverse subtraction
					vhdl << tab << declare(join("yMx",i), cSize[i]+1, true) <<" <= (\"0\" & sY"<<i<<") + (\"0\" & not(sX"<<i<<"))";
					if(i==0) // carry
						vhdl << "+ '1';"<<endl;
					else
						vhdl << "+ " << join("yMx", i-1) << of(cSize[i-1]) << ";"<<endl;
				}
				if (i<nbOfChunks-1)
					nextCycle();
			}

			//assign output by composing the result for x - y
			vhdl << tab << "RxMy <= ";
			for (int i=nbOfChunks-1;i>=0;i--){
				vhdl << "xMy" << i << range(cSize[i]-1,0);
				if (i==0)
					vhdl << ";" << endl;
				else
					vhdl << " & ";
			}

			//assign output by composing the result for y - x or x + y
			vhdl << tab << "R" << son_ << " <= ";
			for (int i=nbOfChunks-1;i>=0;i--){
				vhdl << son_ << i << range(cSize[i]-1,0);
				if (i==0)
					vhdl << ";" << endl;
				else
					vhdl << " & ";
			}
		}
#endif


	IntDualAddSub::~IntDualAddSub() {
	}

	void IntDualAddSub::emulate(TestCase* tc)
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		mpz_class svRxMy = svX - svY;
		tc->addExpectedOutput("RxMy", svRxMy);

		mpz_class svR2;
		if (opType_==0)
			svR2=svY-svX;
		else {
			svR2=svX+svY;
			// Don't allow overflow
			mpz_clrbit(svR2.get_mpz_t(),wIn_);
		}
		tc->addExpectedOutput("R"+son_, svR2);
	}


	void IntDualAddSub::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		tc = new TestCase(this);
		tc->addInput("X", mpz_class(0) );
		tc->addInput("Y", mpz_class(1));
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", mpz_class(0) );
		tc->addInput("Y", mpz_class(-1));
		emulate(tc);
		tcl->add(tc);
	}

	OperatorPtr IntDualAddSub::parseArguments(Target *target, vector<string> &args) {
		int wIn;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn);
		int opType;
		UserInterface::parseStrictlyPositiveInt(args, "opType", &opType);
		return new IntDualAddSub(target, wIn, opType);
	}

	void IntDualAddSub::registerFactory(){
		UserInterface::add("IntDualAddSub", // name
											 "Pipelined dual adder/subtractor",
											 "BasicInteger", // category
											 "",
											 "wIn(int): input size in bits;\
opType(int): 1=compute X-Y and X+Y, 2=compute X-Y and Y-X;",
											 "",
											 IntDualAddSub::parseArguments
											 ) ;

	}
}
