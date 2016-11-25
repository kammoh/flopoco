/*
  Floating Point Adder for FloPoCo

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Authors:   BogdFPAddSinglePathIEEEan Pasca, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 2008-2010.
  All right reserved.

  */

#include "FPAddSinglePathIEEE.hpp"

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


	FPAddSinglePathIEEE::FPAddSinglePathIEEE(Target* target,
		int wE, int wF,
		bool sub, map<string, double> inputDelays) :
	Operator(target), wE(wE), wF(wF), sub(sub) {

		srcFileName="FPAddSinglePathIEEE";

		ostringstream name;
		if(sub)
			name<<"FPSub_";
		else
			name<<"FPAdd_";

		name <<wE<<"_"<<wF;
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Florent de Dinechin, Valentin Huguet (2016)");

		sizeRightShift = intlog2(wF+3);

	/* Set up the IO signals */
	/* Inputs: 2b(Exception) + 1b(Sign) + wE bits (Exponent) + wF bits(Fraction) */
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		addIEEEInput ("X", wE, wF);
		addIEEEInput ("Y", wE, wF);
		addIEEEOutput("R", wE, wF);

	//=========================================================================|
	//                          Swap/Difference                                |
	// ========================================================================|
		vhdl << endl;
		addComment("Swap/Difference", tab);
  	
  	vhdl << tab << declare("expFracX", wE+wF) << " <= X" << range(wE+wF-1, 0) << ";" << endl;
  	vhdl << tab << declare("expFracY", wE+wF) << " <= Y" << range(wE+wF-1, 0) << ";" << endl;
  	vhdl << tab << declare("expXmExpY",wE+1) << " <= ('0' & X" << range(wE+wF-1, wF) << ") - ('0'  & Y" << range(wE+wF-1, wF) << ") " << ";" << endl;
  	vhdl << tab << declare("expYmExpX",wE+1) << " <= ('0' & Y" << range(wE+wF-1, wF) << ") - ('0'  & X" << range(wE+wF-1, wF) << ") " << ";" << endl;

  	vhdl << tab << declare("swap") << " <= '0' when expFracX >= expFracY else '1';" << endl;

  	string pmY="Y";
		if ( sub ) {
			vhdl << tab << declare("mY",wE+wF+1) << " <= not(Y"<<of(wE+wF)<<") & expFracY"<<endl;
			pmY = "mY";
		}

		vhdl << tab << declare("newX", wE+wF+1) << " <= X when swap = '0' else " << pmY << ";" << endl;
		vhdl << tab << declare("newY", wE+wF+1) << " <= " << pmY << " when swap = '0' else X;" << endl;
		vhdl << tab << declare("expNewX", wE) << " <= newX" << range(wE+wF-1, wF) << ";" << endl;
		vhdl << tab << declare("expNewY", wE) << " <= newY" << range(wE+wF-1, wF) << ";" << endl;
		vhdl << tab << declare("signNewX")   << " <= newX"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("signNewY")   << " <= newY"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("EffSub") << " <= signNewX xor signNewY;"<<endl;		

		addComment("Special case dectection", tab);		
		vhdl << tab << declare("subNormalNewXOr0") << " <= '1' when expNewX=" << zg(wE) << " else '0';" << endl;
		vhdl << tab << declare("subNormalNewYOr0") << " <= '1' when expNewY=" << zg(wE) << " else '0';" << endl;
		vhdl << tab << declare("bothSubNormalOr0") << " <= '1' when subNormalNewXOr0='1' and subNormalNewYOr0='1' else '0';" << endl;
		vhdl << tab << declare("xIsNaN")   << " <= '1' when expNewX=" << og(wE) << " and newX" << range(wF-1, 0) << "/=" << zg(wF) << " else '0';"<<endl;
		vhdl << tab << declare("yIsNaN")   << " <= '1' when expNewY=" << og(wE) << " and newY" << range(wF-1, 0) << "/=" << zg(wF) << " else '0';"<<endl;
		vhdl << tab << declare("xIsInfinity")   << " <= '1' when expNewY=" << og(wE) << " and newX" << range(wF-1, 0) << "=" << zg(wF) << " else '0';"<<endl;
		vhdl << tab << declare("yIsInfinity")   << " <= '1' when expNewY=" << og(wE) << " and newY" << range(wF-1, 0) << "=" << zg(wF) << " else '0';"<<endl;
		vhdl << tab << declare("resultIsNaN") << " <= '1' when (xIsNaN='1' or yIsNaN='1' or (xIsInfinity='1' and yIsInfinity='1' and EffSub='1')) else '0';" << endl;
		
		vhdl << tab << declare("signR") << " <= '0' when (expNewX=" << zg(wE) << " and expNewY=" << zg(wE) << " and signNewX/=signNewY and bothSubNormalOr0='0') else signNewX;"<<endl;
		vhdl << tab << declare("fracNewY", wF+1) << " <= not(subNormalNewYOr0) & newY" << range(wF-1, 0) << ";" << endl;


		//=========================================================================|
		//                             Alignment                                   |
		// ========================================================================|
		vhdl << endl;
		addComment("Alignment", tab);

		vhdl << tab << declare("expDiff",wE+1) << " <= expXmExpY when swap = '0' else expYmExpX;"<<endl;
		vhdl << tab << declare("shiftedOut") << " <= '1' when (expDiff >= "<<wF+2<<") else '0';"<<endl;
		vhdl << tab << declare("rightShiftValue",sizeRightShift) << " <= expDiff("<< sizeRightShift-1<<" downto 0) when shiftedOut='0' else CONV_STD_LOGIC_VECTOR("<<wF+3<<","<<sizeRightShift<<") ;" << endl;
		vhdl << tab << declare("finalRightShiftValue", sizeRightShift) << " <= rightShiftValue-'1' when (subNormalNewYOr0='1' and subNormalNewXOr0='0') else rightShiftValue;" << endl;
		
		Shifter* rightShifter = (Shifter*) newInstance("Shifter", "RightShifterComponent", "wIn=" + to_string(wF+1) + " maxShift=" + to_string(wF+3) + " dir=1", "X=>fracNewY;S=>finalRightShiftValue;R=>shiftedFracNewY");


		//=========================================================================|
		//                              Addition                                   |
		// ========================================================================|
		vhdl << endl;
		addComment("Addition", tab);

		//expSigShiftedNewY size = exponent size + RightShifter's wOut_ size
  	vhdl << tab << declare("expSigShiftedNewY", wE+(2*wF)+4) << " <= " << zg(wE) << " & shiftedFracNewY;" << endl;
		vhdl << tab << declare("EffSubVector", wE+(2*wF)+4) << " <= (" << wE+(2*wF)+3 << " downto 0 => EffSub);"<<endl;
		vhdl << tab << declare("fracNewYfarXorOp", wE+(2*wF)+4) << " <= expSigShiftedNewY xor EffSubVector;"<<endl;
		vhdl << tab << declare("expSigNewX", wE+(2*wF)+4) << " <= newX" << range(wE+wF-1, wF) << " & not(subNormalNewXOr0) & newX" << range(wF-1,0) << " & "<< zg(wF+3) <<";" << endl;
		/*This carry is necessary if it's a substraction, for two's complement*/
		vhdl << tab << declare("cInAddFar") << " <= EffSub;"<< endl;

		IntAdder* fracAddFar = (IntAdder*) newInstance("IntAdder", "fracAdder", "wIn=" + to_string(wE+(2*wF)+4), "X=>expSigNewX;Y=>fracNewYfarXorOp;Cin=>cInAddFar;R=>expSigAddResult");


		//=========================================================================|
		//                             Renormalize                                 |
		// ========================================================================|
		vhdl << endl;
		addComment("Renormalize", tab);

		vhdl << tab << declare("sigAddResult", (2*wF)+4) << " <= expSigAddResult" << range((2*wF)+3, 0) << ";" << endl;
		vhdl << tab << declare("expAddResult", wE) << " <= expSigAddResult" << range(wE+(2*wF)+3, (2*wF)+4) << ";" << endl;
		vhdl << tab << declare("ozbLZOC") << " <= '0';" << endl;
		
		/*LZOC if it's a substraction*/
		LZOC* lzoc = (LZOC*) newInstance("LZOC", "LZOCComponent", "wIn=" + to_string((2*wF)+4), "I=>sigAddResult;OZB=>ozbLZOC;O=>lzc");

		if(intlog2((2*wF)+4) < wE+1){
			sizeLeftShift = wE+1;
		}else{
			sizeLeftShift = intlog2((2*wF)+4)+1;
		}


		/*compute left shifter value if it's an addition*/
			//subnormal numbers sum with carry out
		vhdl << tab << declare("subNormalSumCarry") << " <= '1' when EffSub='0' and bothSubNormalOr0='1' and sigAddResult" << of((2*wF)+3) << "='1' else '0';" << endl;
		vhdl << tab << declare("leftShiftAddValue", sizeLeftShift+1) << " <= '1' - ((" << zg((sizeLeftShift-wE)+1) << " & expAddResult) - (" << zg((sizeLeftShift-wE)+1) << " & expNewX));" << endl;
		vhdl << tab << declare("leftShiftAddValue2", sizeLeftShift) << " <= " << zg(sizeLeftShift) << " when leftShiftAddValue" << of(sizeLeftShift) << "='1' else leftShiftAddValue" << range(sizeLeftShift-1, 0) << ";" << endl;
		
		/*compute left shifter value if it's a substraction*/
		vhdl << tab << declare("leftShiftSubValue", sizeLeftShift+1) << " <= (" << zg(sizeLeftShift+1-intlog2((2*wF)+4)) << " & lzc) + '1';" << endl;
		vhdl << tab << declare("cancellation") << " <= '1' when leftShiftSubValue" << range(sizeLeftShift-1, 0) << ">expAddResult else '0';" << endl;		
		vhdl << tab << declare("leftShiftedOut") << " <= '1' when (leftShiftSubValue >= "<<(2*wF)+5<<") else '0';"<<endl;
		vhdl << tab << declare("leftShiftSubValue2", sizeLeftShift) << " <= (" << zg(sizeLeftShift-wE) << " & expAddResult )+ bothSubNormalOr0 when cancellation='1' else leftShiftSubValue" << range(sizeLeftShift-1, 0) << ";" << endl;
		vhdl << tab << declare("expSubResult", wE) << "<= " << zg(wE) << " when (cancellation = '1' or leftShiftedOut='1') else expAddResult - (leftShiftSubValue2" << range(wE-1, 0) << "-'1');" << endl;
		
		vhdl << tab << declare("finalLeftShiftValue", sizeLeftShift) << " <= leftShiftAddValue2 when EffSub='0' else leftShiftSubValue2;" << endl;

		/*LeftShifter to renormalize number*/
		vhdl << tab << declare("finalLeftShift",intlog2((2*wF)+4))  << " <= finalLeftShiftValue" + range(intlog2((2*wF)+4)-1,0) << ";" << endl; 
		Shifter* leftShifter = (Shifter*) newInstance("Shifter", "LeftShifterComponent", "wIn=" + to_string((2*wF)+4) + " maxShift=" + to_string((2*wF)+6) + " dir=0", "X=>sigAddResult;S=>finalLeftShift;R=>leftShiftedFrac");
		vhdl << tab << declare("finalExpResult", wE) << " <= expSubResult when EffSub='1' else expAddResult+subNormalSumCarry;" << endl;


		//=========================================================================|
		//                              Rounding                                   |
		// ========================================================================|
		vhdl << endl;
		addComment("Rounding", tab);

		vhdl << tab << declare("expSigResult", wE+wF+1) << " <=  '0' & finalExpResult & leftShiftedFrac" << range((2*wF)+3, wF+4) << ";" << endl;
		
		vhdl << tab << declare("stk") << " <= '0' when leftShiftedFrac"<< range(wF+1, 0) << "=" << zg(wF+2) << " else '1';" << endl;
		vhdl << tab << declare("rnd") << " <= leftShiftedFrac" << of(wF+2) << ";" << endl;
		vhdl << tab << declare("grd") << " <= leftShiftedFrac" << of(wF+3) << ";" << endl;
		vhdl << tab << declare("lsb") << " <= leftShiftedFrac" << of(wF+4) << ";" << endl;

		vhdl << tab << declare("addToRoundBit")<<" <= '1' when (grd='1' and ((rnd='1' or stk='1') or (rnd='0' and stk='0' and lsb='1'))) else '0';"<<endl;

		IntAdder* roundingAdder = (IntAdder*) newInstance("IntAdder", "roundingAdder", "wIn=" + to_string(wE+wF+1), "X=>expSigResult;Cin=>addToRoundBit;R=>RoundedExpSigResult", "Y=>" + zg(wE+wF+1,0));
		
		//=========================================================================|
		//                               Result                                    |
		// ========================================================================|
		vhdl << endl;
		addComment("Result", tab);

		/*Special case infinity*/
		vhdl << tab << declare("expSigResult2", wE+wF) << " <= " << og(wE, -1) << zg(wF, 1) << " when ((xIsInfinity='1' or yIsInfinity='1' or expSigResult" << range(wE+wF-1, wF) << "=" << og(wE) << ") and resultIsNaN='0') else RoundedExpSigResult" << range(wE+wF-1, 0) << ";" << endl;
		
		/*Special case NaN*/
		vhdl << tab << declare("expSigResult3", wE+wF) << " <= " << og(wE+wF) << " when (xIsNaN='1' or yIsNaN='1' or (xIsInfinity='1' and yIsInfinity='1' and EffSub='1')) else expSigResult2;" << endl;		
		
		/*Update sign if necessary*/
		vhdl << tab << declare("signR2") << " <= '0' when ((resultIsNaN='1' or (leftShiftedOut='1' and xIsInfinity='0' and yIsInfinity='0')) and bothSubNormalOr0='0') else signR;" << endl;

		/*Result*/
		vhdl<<tab<< declare("computedR",wE+wF+1) << " <= signR2 & expSigResult3;"<<endl;
		vhdl << tab << "R <= computedR;"<<endl;
	}


	FPAddSinglePathIEEE::~FPAddSinglePathIEEE() {
	}


	void FPAddSinglePathIEEE::emulate(TestCase * tc)
	{
		/* Get I/O values */
			mpz_class svX = tc->getInputValue("X");
			mpz_class svY = tc->getInputValue("Y");

		/* Compute correct value */
			IEEENumber ieeex(wE, wF, svX);
			IEEENumber ieeey(wE, wF, svY);
			mpfr_t x, y, r;
			mpfr_init2(x, 1+wF);
			mpfr_init2(y, 1+wF);
			mpfr_init2(r, 1+wF);
			ieeex.getMPFR(x);
			ieeey.getMPFR(y);
			if(sub)
				mpfr_sub(r, x, y, GMP_RNDN);
			else
				mpfr_add(r, x, y, GMP_RNDN);

			//double d=mpfr_get_d(r, GMP_RNDN);
			//cout << "R=" << d << endl;
		// Set outputs
			IEEENumber  ieeer(wE, wF, r);
			mpz_class svR = ieeer.getSignalValue();
			tc->addExpectedOutput("R", svR);

		// clean up
			mpfr_clears(x, y, r, NULL);
	}


	void FPAddSinglePathIEEE::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;


		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::minusZero);
		tc->addIEEEInput("Y", IEEENumber::minusZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::plusZero);
		tc->addIEEEInput("Y", 2.8e-45);
		emulate(tc);
		tcl->add(tc);

		
		tc = new TestCase(this);
		tc->addIEEEInput("X", 2.4e-44);
		tc->addIEEEInput("Y", 2.8e-45);
		emulate(tc);
		tcl->add(tc);
		
		tc = new TestCase(this);
		tc->addIEEEInput("X", 1.469368e-39);
		tc->addIEEEInput("Y", 4.5e-44);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::greatestSubNormal);
		tc->addIEEEInput("Y", IEEENumber::greatestSubNormal);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::greatestNormal);
		tc->addIEEEInput("Y", IEEENumber::greatestNormal);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", 1.1754945e-38);
		tc->addIEEEInput("Y", 1.1754945e-38);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::NaN);
		tc->addIEEEInput("Y", IEEENumber::NaN);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::plusZero);
		tc->addIEEEInput("Y", IEEENumber::minusZero);
		emulate(tc);
		tcl->add(tc);

		// Regression tests
		tc = new TestCase(this);
		tc->addIEEEInput("X", 1.0);
		tc->addIEEEInput("Y", -1.0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", 1.0);
		tc->addIEEEInput("Y", IEEENumber::plusZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", 1.0);
		tc->addIEEEInput("Y", IEEENumber::minusZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::plusInfty);
		tc->addIEEEInput("Y", IEEENumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::plusInfty);
		tc->addIEEEInput("Y", IEEENumber::plusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", IEEENumber::minusInfty);
		tc->addIEEEInput("Y", IEEENumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addIEEEInput("X", -4.375e1);
		tc->addIEEEInput("Y", 4.375e1);
		emulate(tc);
		tcl->add(tc);

	}


	TestCase* FPAddSinglePathIEEE::buildRandomTestCase(int i){

		TestCase *tc;
		mpz_class x,y;
		mpz_class negative  = mpz_class(1)<<(wE+wF);

		tc = new TestCase(this);
		/* Fill inputs */
		if ((i & 7) == 0) {// cancellation, same exponent
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF);
			y  = getLargeRandom(wF) + (e << wF) + negative;
		}
		else if ((i & 7) == 1) {// cancellation, exp diff=1
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF);
			if(e < (mpz_class(1)<<(wE)-1))
				e++; // may rarely lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + negative;
		}
		else if ((i & 7) == 2) {// cancellation, exp diff=1
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + negative;
			e++; // may rarely lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF);
		}
		else if ((i & 7) == 3) {// alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + negative;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF);
		}
		else if ((i & 7) == 4) {// subtraction, alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF);
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow
			if(e > (mpz_class(1)<<(wE)-1))
				e = (mpz_class(1)<<(wE)-1);
			y  = getLargeRandom(wF) + (e << wF) + negative;
		}
		else if ((i & 7) == 5 || (i & 7) == 6) {// addition, alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF);
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow
			y  = getLargeRandom(wF) + (e << wF);
		}
		else{ //fully random
			x = getLargeRandom(wE+wF+1);
			y = getLargeRandom(wE+wF+1);
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


	TestList FPAddSinglePathIEEE::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			paramList.push_back(make_pair("wE","5"));
			paramList.push_back(make_pair("wF","10"));		
			testStateList.push_back(paramList);

			paramList.clear();
			paramList.push_back(make_pair("wE","8"));
			paramList.push_back(make_pair("wF","23"));			
			testStateList.push_back(paramList);

			paramList.clear();
			paramList.push_back(make_pair("wE","11"));
			paramList.push_back(make_pair("wF","52"));	
			testStateList.push_back(paramList);

			paramList.clear();
			paramList.push_back(make_pair("wE","15"));
			paramList.push_back(make_pair("wF","112"));			
			testStateList.push_back(paramList);

			paramList.clear();
			paramList.push_back(make_pair("wE","19"));
			paramList.push_back(make_pair("wF","236"));			
			testStateList.push_back(paramList);
		}
		else     
		{
				// finite number of random test computed out of index
		}	

		return testStateList;
	}

	OperatorPtr FPAddSinglePathIEEE::parseArguments(Target *target, vector<string> &args) {
		int wE;
		UserInterface::parseStrictlyPositiveInt(args, "wE", &wE);
		int wF;
		UserInterface::parseStrictlyPositiveInt(args, "wF", &wF);
		return new FPAddSinglePathIEEE(target, wE, wF);
	}

	void FPAddSinglePathIEEE::registerFactory(){
		UserInterface::add("FPAddSinglePathIEEE", // name
											 "A floating-point adder with a new, more compact single-path architecture.",
											 "BasicFloatingPoint", // categories
											 "",
											 "wE(int): exponent size in bits; \
											 wF(int): mantissa size in bits;",
											 "",
											 FPAddSinglePathIEEE::parseArguments,
											 FPAddSinglePathIEEE::unitTest
											 ) ;
	}
}
