/*
  Floating Point Adder for FloPoCo

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Authors:   Bogdan Pasca, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 2008-2010.
  All right reserved.

  */

#include "FPAddSinglePath.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include <utils.hpp>
#include <Operator.hpp>


using namespace std;

//TODO +- inf for exponent => update exception

namespace flopoco{


#define DEBUGVHDL 0


  FPAddSinglePath::FPAddSinglePath(Target* target,
				   int wE, int wF,
				   bool sub) :
		Operator(target), wE(wE), wF(wF), sub(sub) {

		srcFileName="FPAddSinglePath";

		ostringstream name;
		if(sub)
			name<<"FPSub_";
		else
			name<<"FPAdd_";

		name <<wE<<"_"<<wF<<"_uid"<<getNewUId();
		setName(name.str());

		setCopyrightString("Bogdan Pasca, Florent de Dinechin (2010)");

		sizeRightShift = intlog2(wF+3);
		REPORT(DEBUG, "sizeRightShift = " <<  sizeRightShift);
		/* Set up the IO signals */
		/* Inputs: 2b(Exception) + 1b(Sign) + wE bits (Exponent) + wF bits(Fraction) */
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addFPOutput("R", wE, wF);

		//=========================================================================|
		//                          Swap/Difference                                |
		// ========================================================================|
		// In principle the comparison could be done on the exponent only, but then the result of the effective addition could be negative,
		// so we would have to take its absolute value, which would cost the same as comparing exp+frac here.
		vhdl << tab << declare("excExpFracX",2+wE+wF) << " <= X"<<range(wE+wF+2, wE+wF+1) << " & X"<<range(wE+wF-1, 0)<<";"<<endl;
		vhdl << tab << declare("excExpFracY",2+wE+wF) << " <= Y"<<range(wE+wF+2, wE+wF+1) << " & Y"<<range(wE+wF-1, 0)<<";"<<endl;



		vhdl << tab << declare("addCmpOp1",wE+wF+3) << " <= '0'  & excExpFracX;"<<endl;
		vhdl << tab << declare("addCmpOp2",wE+wF+3) << " <= '1'  & not excExpFracY;"<<endl;
		IntAdder *cmpAdder = new IntAdder(target, wE+wF+3);
		inPortMap(cmpAdder, "X", "addCmpOp1");
		inPortMap(cmpAdder, "Y", "addCmpOp2");
		inPortMapCst(cmpAdder, "Cin", "'1'");
		outPortMap (cmpAdder, "R", "cmpRes");

		vhdl << instance(cmpAdder, "cmpAdder") << endl;
		vhdl<< tab << declare("swap")  << " <= cmpRes"<<of(wE+wF+2)<<";"<<endl;

		addComment("exponent difference");
		vhdl<< tab << declare(target->localWireDelay() + target->adderDelay(wE+1),
													"eXmeY",wE)	<< " <= (X"<<range(wE+wF-1,wF)<<") - (Y"<<range(wE+wF-1,wF)<<");"<<endl;
		vhdl<< tab << declare(target->localWireDelay() + target->adderDelay(wE+1),
													"eYmeX",wE) << " <= (Y"<<range(wE+wF-1,wF)<<") - (X"<<range(wE+wF-1,wF)<<");"<<endl;
		vhdl<<tab<<declare(target->localWireDelay() + target->lutDelay(),
											 "expDiff",wE) << " <= eXmeY when swap = '0' else eYmeX;"<<endl;


		string pmY="Y";
		if ( sub ) {
			vhdl << tab << declare(target->localWireDelay() + target->lutDelay(), "mY", wE+wF+3)
					<< " <= Y" << range(wE+wF+2,wE+wF+1) << " & not(Y"<<of(wE+wF)<<") & Y" << range(wE+wF-1,0) << ";"<<endl;
			pmY = "mY";
		}

		// depending on the value of swap, assign the corresponding values to the newX and newY signals

		addComment("input swap so that |X|>|Y|");
		vhdl<<tab<<declare(target->localWireDelay() + target->lutDelay(),
											 "newX",wE+wF+3) << " <= X when swap = '0' else "<< pmY << ";"<<endl;
		vhdl<<tab<<declare(target->localWireDelay() + target->lutDelay(),
											 "newY",wE+wF+3) << " <= " << pmY <<" when swap = '0' else X;"<<endl;

		//break down the signals
		addComment("now we decompose the inputs into their sign, exponent, fraction");
		vhdl << tab << declare("expX",wE) << "<= newX"<<range(wE+wF-1,wF)<<";"<<endl;
		vhdl << tab << declare("excX",2)  << "<= newX"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("excY",2)  << "<= newY"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("signX")   << "<= newX"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("signY")   << "<= newY"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare(target->localWireDelay() + target->lutDelay(),
													 "EffSub") << " <= signX xor signY;"<<endl;
		vhdl << tab << declare("sXsYExnXY",6) << " <= signX & signY & excX & excY;"<<endl;
		vhdl << tab << declare("sdExnXY",4) << " <= excX & excY;"<<endl;

		vhdl << tab << declare(target->localWireDelay()+ target->lutDelay(),
													 "fracY",wF+1) << " <= "<< zg(wF+1)<<" when excY=\"00\" else ('1' & newY("<<wF-1<<" downto 0));"<<endl;

		addComment("Exception management logic");
		double exnDelay;
		if (target->lutInputs()>=6)
			exnDelay = target->localWireDelay()+target->lutDelay();
		else
			exnDelay = 2*(target->localWireDelay()+target->lutDelay());
		vhdl <<tab<<"with sXsYExnXY select "<<endl;
		vhdl <<tab<<declare(exnDelay,"excRt",2) << " <= \"00\" when \"000000\"|\"010000\"|\"100000\"|\"110000\","<<endl
				 <<tab<<tab<<"\"01\" when \"000101\"|\"010101\"|\"100101\"|\"110101\"|\"000100\"|\"010100\"|\"100100\"|\"110100\"|\"000001\"|\"010001\"|\"100001\"|\"110001\","<<endl
				 <<tab<<tab<<"\"10\" when \"111010\"|\"001010\"|\"001000\"|\"011000\"|\"101000\"|\"111000\"|\"000010\"|\"010010\"|\"100010\"|\"110010\"|\"001001\"|\"011001\"|\"101001\"|\"111001\"|\"000110\"|\"010110\"|\"100110\"|\"110110\", "<<endl
				 <<tab<<tab<<"\"11\" when others;"<<endl;
		
		vhdl <<tab<<declare(target->localWireDelay() + target->lutDelay(), "signR")
				 << "<= '0' when (sXsYExnXY=\"100000\" or sXsYExnXY=\"010000\") else signX;"<<endl;


		vhdl<<tab<<declare(target->localWireDelay() + target->eqConstComparatorDelay(wE+1), "shiftedOut")
				<< " <= '1' when (expDiff >= "<<wF+2<<") else '0';"<<endl;
		//shiftVal=the number of positions that fracY must be shifted to the right

		if (wE>sizeRightShift) {
			vhdl<<tab<<declare(target->localWireDelay() + target->lutDelay(), "shiftVal",sizeRightShift)
					<< " <= expDiff("<< sizeRightShift-1<<" downto 0)"
					//<< " when shiftedOut='0' else \"" << unsignedBinary(wF+3,sizeRightShift) << "\";" << endl;  // was CONV_STD_LOGIC_VECTOR("<<wF+3<<","<<sizeRightShift<<")
					<< " when shiftedOut='0' else CONV_STD_LOGIC_VECTOR("<<wF+3<<","<<sizeRightShift<<");" << endl;
		}
		else if (wE==sizeRightShift) {
			vhdl<<tab<<declare("shiftVal", sizeRightShift) << " <= expDiff" << range(sizeRightShift-1,0) << ";" << endl ;
		}
		else 	{ //  wE< sizeRightShift
			//vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= " << zg(sizeRightShift-wE) << " & expDiff;" << endl;  // was CONV_STD_LOGIC_VECTOR(0,"<<sizeRightShift-wE <<") & expDiff;" <<	endl;
			vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= CONV_STD_LOGIC_VECTOR("<<sizeRightShift-wE <<", 0) & expDiff;" <<	endl;
		}

		// shift right the significand of new Y with as many positions as the exponent difference suggests (alignment)
		REPORT(DETAILED, "Building right shifter");
		Shifter* rightShifter = new Shifter(target,wF+1,wF+3, Shifter::Right);
		rightShifter->changeName(getName()+"_RightShifter");
		inPortMap  (rightShifter, "X", "fracY");
		inPortMap  (rightShifter, "S", "shiftVal");
		outPortMap (rightShifter, "R","shiftedFracY");
		vhdl << instance(rightShifter, "RightShifterComponent");

		vhdl<<tab<< declare(target->localWireDelay() + target->eqConstComparatorDelay(wF+1), "sticky")
				<< " <= '0' when (shiftedFracY("<<wF<<" downto 0) = " << zg(wF) << ") else '1';"<<endl;

		//pad fraction of Y [overflow][shifted frac having inplicit 1][guard][round]
		vhdl<<tab<< declare("fracYfar", wF+4)      << " <= \"0\" & shiftedFracY("<<2*wF+3<<" downto "<<wF+1<<");"<<endl;
		vhdl<<tab<< declare("EffSubVector", wF+4) << " <= ("<<wF+3<<" downto 0 => EffSub);"<<endl;
		vhdl<<tab<< declare(target->localWireDelay(wF+4) + target->lutDelay(), "fracYfarXorOp", wF+4)
				<< " <= fracYfar xor EffSubVector;"<<endl;
		//pad fraction of X [overflow][inplicit 1][fracX][guard bits]
		vhdl<<tab<< declare("fracXfar", wF+4)      << " <= \"01\" & (newX("<<wF-1<<" downto 0)) & \"00\";"<<endl;

		vhdl<<tab<< declare(target->localWireDelay()+ target->lutDelay(), "cInAddFar")
				<< " <= EffSub and not sticky;"<< endl;//TODO understand why

		//result is always positive.
		IntAdder* fracAddFar = new IntAdder(target,wF+4);
		inPortMap  (fracAddFar, "X", "fracXfar");
		inPortMap  (fracAddFar, "Y", "fracYfarXorOp");
		inPortMap  (fracAddFar, "Cin", "cInAddFar");
		outPortMap (fracAddFar, "R","fracAddResult");
		vhdl << instance(fracAddFar, "fracAdder");

		//shift in place
		vhdl << tab << declare("fracGRS",wF+5) << "<= fracAddResult & sticky; "<<endl;


		LZOCShifterSticky* lzocs = new LZOCShifterSticky(target, wF+5, wF+5, intlog2(wF+5), false, 0);
		inPortMap  (lzocs, "I", "fracGRS");
		outPortMap (lzocs, "Count","nZerosNew");
		outPortMap (lzocs, "O","shiftedFrac");
		vhdl << instance(lzocs, "LZC_component");

		//need to decide how much to add to the exponent
		/*		manageCriticalPath(target->localWireDelay() + target->adderDelay(wE+2));*/
		// 	vhdl << tab << declare("expPart",wE+2) << " <= (" << zg(wE+2-lzocs->getCountWidth(),0) <<" & nZerosNew) - 1;"<<endl;
		//update exponent

		//incremented exponent.
		// pipeline: I am assuming the two additions can be merged in a row of luts but I am not sure
		vhdl << tab << declare("extendedExpInc",wE+2) << "<= (\"00\" & expX) + '1';"<<endl;

		vhdl << tab << declare(target->localWireDelay() + target->adderDelay(wE+2),
													 "updatedExp",wE+2) << " <= extendedExpInc - (" << zg(wE+2-lzocs->getCountWidth(),0) <<" & nZerosNew);"<<endl;
		vhdl << tab << declare("eqdiffsign")<< " <= '1' when nZerosNew="<<og(lzocs->getCountWidth(),0)<<" else '0';"<<endl;


		//concatenate exponent with fraction to absorb the possible carry out
		vhdl<<tab<<declare("expFrac",wE+2+wF+1)<<"<= updatedExp & shiftedFrac"<<range(wF+3,3)<<";"<<endl;

		vhdl<<tab<<declare("stk")<<"<= shiftedFrac"<<of(1)<<" or shiftedFrac"<<of(0)<<";"<<endl;
		vhdl<<tab<<declare("rnd")<<"<= shiftedFrac"<<of(2)<<";"<<endl;
		vhdl<<tab<<declare("grd")<<"<= shiftedFrac"<<of(3)<<";"<<endl;
		vhdl<<tab<<declare("lsb")<<"<= shiftedFrac"<<of(4)<<";"<<endl;

		//decide what to add to the guard bit
		vhdl << tab << declare(target->localWireDelay() + target->lutDelay(),"addToRoundBit")
				<<"<= '0' when (lsb='0' and grd='1' and rnd='0' and stk='0')  else '1';"<<endl;


		IntAdder *ra = new IntAdder(target, wE+2+wF+1);

		inPortMap(ra,"X", "expFrac");
		inPortMapCst(ra, "Y", zg(wE+2+wF+1,0) );
		inPortMap( ra, "Cin", "addToRoundBit");
		outPortMap( ra, "R", "RoundedExpFrac");
		vhdl << instance(ra, "roundingAdder");

		addComment("possible update to exception bits");
		vhdl << tab << declare("upExc",2)<<" <= RoundedExpFrac"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("fracR",wF)<<" <= RoundedExpFrac"<<range(wF,1)<<";"<<endl;
		vhdl << tab << declare("expR",wE) <<" <= RoundedExpFrac"<<range(wF+wE,wF+1)<<";"<<endl;

		vhdl << tab << declare("exExpExc",4) << " <= upExc & excRt;"<<endl;
		vhdl << tab << "with exExpExc select "<<endl;
		vhdl << tab << declare(target->localWireDelay() + target->lutDelay(),
													 "excRt2",2)
				 << "<= \"00\" when \"0000\"|\"0100\"|\"1000\"|\"1100\"|\"1001\"|\"1101\","<<endl
				 <<tab<<tab<<"\"01\" when \"0001\","<<endl
				 <<tab<<tab<<"\"10\" when \"0010\"|\"0110\"|\"1010\"|\"1110\"|\"0101\","<<endl
				 <<tab<<tab<<"\"11\" when others;"<<endl;
		vhdl<<tab<<declare(target->localWireDelay() + target->lutDelay(),
											 "excR",2) << " <= \"00\" when (eqdiffsign='1' and EffSub='1') else excRt2;"<<endl;
		// IEEE standard says in 6.3: if exact sum is zero, it should be +zero in RN
		vhdl<<tab<<declare(target->localWireDelay() + target->lutDelay(), "signR2")
				<< " <= '0' when (eqdiffsign='1' and EffSub='1') else signR;"<<endl;


		// assign result
		vhdl<<tab<< declare("computedR",wE+wF+3) << " <= excR & signR2 & expR & fracR;"<<endl;
		vhdl << tab << "R <= computedR;"<<endl;

		/*		manageCriticalPath(target->localWireDelay() +  target->lutDelay());
					vhdl<<tab<<"with sdExnXY select"<<endl;
					vhdl<<tab<<"R <= newX when \"0100\"|\"1000\"|\"1001\", newY when \"0001\"|\"0010\"|\"0110\", computedR when others;"<<endl;*/


	}

	FPAddSinglePath::~FPAddSinglePath() {
	}


	void FPAddSinglePath::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		/* Compute correct value */
		FPNumber fpx(wE, wF, svX);
		FPNumber fpy(wE, wF, svY);
		mpfr_t x, y, r;
		mpfr_init2(x, 1+wF);
		mpfr_init2(y, 1+wF);
		mpfr_init2(r, 1+wF);
		fpx.getMPFR(x);
		fpy.getMPFR(y);
		if(sub)
			mpfr_sub(r, x, y, GMP_RNDN);
		else
			mpfr_add(r, x, y, GMP_RNDN);

		// Set outputs
		FPNumber  fpr(wE, wF, r);
		mpz_class svR = fpr.getSignalValue();
		tc->addExpectedOutput("R", svR);

		// clean up
		mpfr_clears(x, y, r, NULL);
	}





	void FPAddSinglePath::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		// Regression tests
		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", -1.0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::plusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::minusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::plusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::minusInfty);
		tc->addFPInput("Y", FPNumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", -4.375e1);
		tc->addFPInput("Y", 4.375e1);
		emulate(tc);
		tcl->add(tc);

	}



	TestCase* FPAddSinglePath::buildRandomTestCase(int i){

		TestCase *tc;
		mpz_class x,y;
		mpz_class normalExn = mpz_class(1)<<(wE+wF+1);
		mpz_class negative  = mpz_class(1)<<(wE+wF);

		tc = new TestCase(this);
		/* Fill inputs */
		if ((i & 7) == 0) {// cancellation, same exponent
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			y  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
		}
		else if ((i & 7) == 1) {// cancellation, exp diff=1
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			e++; // may rarely lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
		}
		else if ((i & 7) == 2) {// cancellation, exp diff=1
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
			e++; // may rarely lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + normalExn;
		}
		else if ((i & 7) == 3) {// alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + normalExn;
		}
		else if ((i & 7) == 4) {// subtraction, alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow
			y  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
		}
		else if ((i & 7) == 5 || (i & 7) == 6) {// addition, alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow
			y  = getLargeRandom(wF) + (e << wF) + normalExn;
		}
		else{ //fully random
			x = getLargeRandom(wE+wF+3);
			y = getLargeRandom(wE+wF+3);
		}
		// Random swap
		mpz_class swap = getLargeRandom(1);
		if (swap == mpz_class(0)) {
			tc->addInput("X", x);
			tc->addInput("Y", y);
		}
		else {
			tc->addInput("X", y);
			tc->addInput("Y", x);
		}
		/* Get correct outputs */
		emulate(tc);
		return tc;
	}

		OperatorPtr FPAddSinglePath::parseArguments(Target *target, vector<string> &args) {
		int wE;
		UserInterface::parseStrictlyPositiveInt(args, "wE", &wE);
		int wF;
		UserInterface::parseStrictlyPositiveInt(args, "wF", &wF);
		return new FPAddSinglePath(target, wE, wF);
	}

	void FPAddSinglePath::registerFactory(){
		UserInterface::add("FPAddSInglePath", // name
											 "A floating-point adder with a compact single-path architecture.",
											 "BasicFloatingPoint", // categories
											 "",
											 "wE(int): exponent size in bits; \
                        wF(int): mantissa size in bits;",
											 "",
											 FPAddSinglePath::parseArguments
											 ) ;
	}
}
