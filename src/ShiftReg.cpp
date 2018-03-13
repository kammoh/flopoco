#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "ShiftReg.hpp"

using namespace std;

namespace flopoco {

	ShiftReg::ShiftReg(OperatorPtr parentOp, Target* target, int w_, int n_, bool hasReset_)
		: Operator(parentOp, target), w(w_), n(n_), hasReset(hasReset_)
	{
		srcFileName="ShiftReg";
		setCopyrightString ( "Louis Beseme, Florent de Dinechin, Matei Istoan (2014-2016)" );
		useNumericStd_Unsigned();

		ostringstream name;
		name << "ShiftReg_"<< w_;
		setNameWithFreqAndUID( name.str() );

		addInput("X", w, true);

		for(int i=1; i<=n; i++) {
			addOutput(join("Xd", i), w, true);
		}

		vhdl << tab << declare("X0" , w)  << " <= X;" << endl;
		for(int i=0; i<n; i++) {
			if(hasReset)
				addRegisteredSignalCopy(join("X", i+1), join("X", i), Signal::registeredWithAsyncReset);
			else
				addRegisteredSignalCopy(join("X", i+1), join("X", i), Signal::registeredWithoutReset);				
		}
		for(int i=1; i<=n; i++) {
			vhdl << tab << join("Xd",i)  << " <= " << join("X", i) << ";" << endl;
		}
	};

	ShiftReg::~ShiftReg(){

	};


	void ShiftReg::emulate(TestCase * tc){
		// TODO... or not
	};

	void ShiftReg::buildStandardTestCases(TestCaseList* tcl){

	};


	OperatorPtr ShiftReg::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int w_, n_;

		UserInterface::parseStrictlyPositiveInt(args, "w", &w_);
		UserInterface::parseStrictlyPositiveInt(args, "n", &n_);

		return new ShiftReg(parentOp, target, w_, n_);
	}


	void ShiftReg::registerFactory(){
		UserInterface::add("ShiftReg", // name
											 "A plain shift register implementation.",
											 "ShiftersLZOCs",
											 "", //seeAlso
											 "w(int): the size of the input; \
						                      n(int): the number of stages in the shift register;",
											 "",
											 ShiftReg::parseArguments
											 ) ;
	}

}
	
