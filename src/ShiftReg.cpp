#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "ShiftReg.hpp"

using namespace std;

namespace flopoco {

	ShiftReg::ShiftReg(Target* target, int w_, int n_)
		: Operator(target), w(w_), n(n_)
	{
		srcFileName="ShiftReg";
		setCopyrightString ( "Louis Beseme, Florent de Dinechin, Matei Istoan (2014-2016)" );
		useNumericStd_Unsigned();

		ostringstream name;
		name << "ShiftReg_"<< w_ << "_uid" << getNewUId();
		setNameWithFreqAndUID( name.str() );

		addInput("X", w, true);

		for(int i=0; i<n; i++) {
			addOutput(join("Xd", i), w, true);
		}

//		setCriticalPath(getMaxInputDelays(inputDelays));
//
//		vhdl << tab << declare("XX", w, false, Signal::registeredWithAsyncReset) << " <= X;" << endl;
//
//
//		for(int i=0; i<n; i++) {
//			vhdl << tab << join("Xd", i) << " <= XX;" << endl;
//			if (i<n-1)
//				nextCycle(false);
//		}
//		setPipelineDepth(0);
		
		vhdl << tab << declare("XX", w, false, Signal::registeredWithAsyncReset) << " <= X;" << endl;
		for(int i=0; i<n; i++) {
			vhdl << tab << "Xd" << i << " <= " << delay("XX", i, SignalDelayType::functional) << ";" << endl;
		}
	};

	ShiftReg::~ShiftReg(){

	};


	void ShiftReg::emulate(TestCase * tc){

	};

	void ShiftReg::buildStandardTestCases(TestCaseList* tcl){

	};


	OperatorPtr ShiftReg::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int w_, n_;

		UserInterface::parseStrictlyPositiveInt(args, "w", &w_);
		UserInterface::parseStrictlyPositiveInt(args, "n", &n_);

		return new ShiftReg(target, w_, n_);
	}


	void ShiftReg::registerFactory(){
		UserInterface::add("ShiftReg", // name
											 "A shift register implementation.",
											 "ShiftersLZOCs",
											 "", //seeAlso
											 "w(int): the size of the input; \
						                      n(int): the number of stages in the shift register;",
											 "",
											 ShiftReg::parseArguments
											 ) ;
	}

}
	
