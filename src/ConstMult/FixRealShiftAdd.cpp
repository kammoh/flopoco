// TODO: repair FixFIR, FixIIR, FixComplexKCM
/*
 * A faithful multiplier by a real constant, using a variation of the KCM method
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 * 
 * Authors : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr
 * 			 3IF-Dev-Team-2015
 *
 * Initial software.
 * Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 
 * 2008-2011.
 * All rights reserved.
 */
/*

Remaining 1-ulp bug:
flopoco verbose=3 FixRealShiftAdd lsbIn=-8 msbIn=0 lsbOut=-7 constant="0.16" signedInput=true TestBench
It is the limit case of removing one table altogether because it contributes nothng.
I don't really understand

*/



#include "../Operator.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <sollya.h>
#include "../utils.hpp"
#include "FixRealShiftAdd.hpp"
#include "../IntAddSubCmp/IntAdder.hpp"

using namespace std;


namespace flopoco{




	
	//standalone operator
	FixRealShiftAdd::FixRealShiftAdd(OperatorPtr parentOp, Target* target, bool signedInput_, int msbIn_, int lsbIn_, int lsbOut_, string constant_, double targetUlpError_):
		Operator(parentOp, target),
		thisOp(this),
		signedInput(signedInput_),
		msbIn(msbIn_),
		lsbIn(lsbIn_),
		lsbOut(lsbOut_),
		constant(constant_),
		targetUlpError(targetUlpError_),
		addRoundBit(true)
	{
		vhdl << "-- This operator multiplies by " << constant << endl;

		// Convert the input string into a sollya evaluation tree
		sollya_obj_t node;
		node = sollya_lib_parse_string(constant.c_str());
		/* If  parse error throw an exception */
		if (sollya_lib_obj_is_error(node))
		{
			ostringstream error;
			error << srcFileName << ": Unable to parse string "<<
				  constant << " as a numeric constant" <<endl;
			throw error.str();
		}

		mpfr_init2(mpC, 10000);
		mpfr_init2(absC, 10000);

		sollya_lib_get_constant(mpC, node);

		//if negative constant, then set negativeConstant
		negativeConstant = ( mpfr_cmp_si(mpC, 0) < 0 ? true: false );

		signedOutput = negativeConstant || signedInput;
		mpfr_abs(absC, mpC, GMP_RNDN);

		REPORT(DEBUG, "Constant evaluates to " << mpfr_get_d(mpC, GMP_RNDN));


		//compute the logarithm only of the constants
		mpfr_t log2C;
		mpfr_init2(log2C, 100); // should be enough for anybody
		mpfr_log2(log2C, absC, GMP_RNDN);
		msbC = mpfr_get_si(log2C, GMP_RNDU);
		mpfr_clears(log2C, NULL);

		// Now we can check when this is a multiplier by 0: either because the it is zero, or because it is close enough
		constantIsExactlyZero = false;
		constantRoundsToZeroInTheStandaloneCase = false;
		if(mpfr_zero_p(mpC) != 0){
			constantIsExactlyZero = true;
			msbOut=lsbOut; // let us return a result on one bit, why not.
			errorInUlps=0;
			REPORT(INFO, "It seems somebody asked for a multiplication by 0. We can do that.");
			return;
		}

		msbOut = msbIn + msbC;

		REPORT(DEBUG, "Output precisions: msbOut=" << msbOut << ", lsbOut=" << lsbOut);

		mpfr_t mpOp1, mpOp2; //temporary variables for operands
		mpfr_init2(mpOp1, 100);
		mpfr_init2(mpOp2, 100);

		//compute error bounds of the result (epsilon_max)
		mpfr_t mpEpsilonMax;
		mpfr_init2(mpEpsilonMax, 100);
		mpfr_set_si(mpOp1,2,GMP_RNDN);
		mpfr_set_si(mpOp2,lsbOut,GMP_RNDN);

		mpfr_set_si(mpOp1,2,GMP_RNDN);
		mpfr_set_si(mpOp2,lsbOut,GMP_RNDN);
		mpfr_pow(mpEpsilonMax,mpOp1,mpOp2,GMP_RNDN); //2^lsbOut
		ios::fmtflags old_settings = cout.flags();
		REPORT(INFO, "Epsilon max=" << std::scientific << mpfr_get_d(mpEpsilonMax, GMP_RNDN));
		cout.flags(old_settings);

		//compute integer representation of the constant
//		int integerConstantWS=msbC-lsbOut;


		for(int integerConstantWS=1; integerConstantWS < 20; integerConstantWS++)
		{
			mpfr_t mpCInt, mpScale;
			mpfr_init2(mpCInt, integerConstantWS);
			mpfr_init2(mpScale, -lsbOut+1);

			mpfr_set_si(mpOp1,2,GMP_RNDN);
			mpfr_set_si(mpOp2,integerConstantWS-msbC,GMP_RNDN);
			mpfr_pow(mpScale,mpOp1,mpOp2,GMP_RNDN); //2^(-lsbOut)
//			REPORT(DEBUG, "mpScale=" << (int) mpfr_get_d(mpScale, GMP_RNDN));
			mpfr_mul(mpCInt, mpScale, mpC, GMP_RNDN); //C * 2^(-lsbOut)

			//compute rounding error of the selected integer
			mpfr_t mpEpsilonCoeff;
			mpfr_init2(mpEpsilonCoeff, 100);
			mpfr_div(mpEpsilonCoeff,mpCInt,mpScale,GMP_RNDN); //mpR = mpCInt/mpScale
			mpfr_sub(mpEpsilonCoeff,mpC,mpEpsilonCoeff,GMP_RNDN); //mpR = mpC - mpCInt/mpScale

			mpfr_t mpEpsilonCoeffAbs;
			mpfr_init2(mpEpsilonCoeffAbs, 100);
			mpfr_abs(mpEpsilonCoeffAbs,mpEpsilonCoeff,GMP_RNDN);

			if(mpfr_greater_p(mpEpsilonCoeffAbs, mpEpsilonMax)) continue;

			//compute epsilon for multiplier truncation:
			mpfr_t mpEpsilonMult;
			mpfr_init2(mpEpsilonMult, 100);
			mpfr_sub(mpEpsilonMult,mpEpsilonMax,mpEpsilonCoeffAbs,GMP_RNDN); //epsilon_mult = epsilon_max - |epsilon_coeff|

			mpfr_t mpEpsilonMultAbs;
			mpfr_init2(mpEpsilonMultAbs, 100);
			mpfr_abs(mpEpsilonMultAbs,mpEpsilonMult,GMP_RNDN);

			REPORT(INFO, "Integer constant wordsize=" << integerConstantWS << ", coefficient=" << (int) mpfr_get_d(mpCInt, GMP_RNDN) << "/" << (int) mpfr_get_d(mpScale, GMP_RNDN) << "=" << mpfr_get_d(mpCInt, GMP_RNDN)/mpfr_get_d(mpScale, GMP_RNDN) << ", epsilonCoeff=" << std::scientific << mpfr_get_d(mpEpsilonCoeff, GMP_RNDN) << ", epsilonMult=" << mpfr_get_d(mpEpsilonMult, GMP_RNDN));
			old_settings = cout.flags();

			cout.flags(old_settings);

			break; //break on first coefficient found (just for test)
		}


		//cleanup
		mpfr_clears(mpOp1, NULL);
		mpfr_clears(mpOp2, NULL);


		//create IntConstMultShiftAdd instance here



/*
		init();		 // check special cases, computes number of tables and errorInUlps.

		// Now we have everything to compute g
		computeGuardBits();
		
		// To help debug KCM called from other operators, report in FloPoCo CLI syntax
		REPORT(DETAILED, "FixRealShiftAdd  signedInput=" << signedInput << " msbIn=" << msbIn << " lsbIn=" << lsbIn << " lsbOut=" << lsbOut << " constant=\"" << constant << "\"  targetUlpError="<< targetUlpError);
		
		addInput("X",  msbIn-lsbIn+1);
		//		addFixInput("X", signedInput,  msbIn, lsbIn); // The world is not ready yet
		inputSignalName = "X"; // for buildForBitHeap
		addOutput("R", msbOut-lsbOut+1);

		// Special cases
		if(constantRoundsToZeroInTheStandaloneCase || constantIsExactlyZero)	{
			vhdl << tab << "R" << " <= " << zg(msbOut-lsbOut+1) << ";" << endl;
			return;
		}

		if(constantIsPowerOfTwo)	{
			// The code here is different that the one for the bit heap constructor:
			// In the stand alone case we must compute full negation.
			string rTempName = createShiftedPowerOfTwo(inputSignalName); 
			int rTempSize = thisOp->getSignalByName(rTempName)->width();

			if(negativeConstant) { // In this case msbOut was incremented in init()
				vhdl << tab << "R" << " <= " << zg(msbOut-lsbOut+1) << " - ";
				if(signedInput) {
					vhdl << "("
							 <<  rTempName << of(rTempSize-1) << " & " // sign extension 
							 <<  rTempName << range(rTempSize-1, g)
							 << ");" << endl;
				}
				else{ // unsigned input
					vhdl <<  rTempName << range(rTempSize-1, g) << ";" << endl;
				}
			}
			else{		
				vhdl << tab << "R <= "<< rTempName << range(msbOut-lsbOut+g, g) << ";" << endl;
			}
			return;
		}


		// From now we have stuff to do.
		//create the bitheap
		//		int bitheaplsb = lsbOut - g;
		REPORT(DEBUG, "Creating bit heap for msbOut=" << msbOut <<" lsbOut=" << lsbOut <<" g=" << g);
		bitHeap = new BitHeap(this, msbOut-lsbOut+1+g); // hopefully some day we get a fixed-point bit heap

		buildTablesForBitHeap(); // does everything up to bit heap compression

		//compress the bitheap and produce the result
		bitHeap->startCompression();

		// Retrieve the bits we want from the bit heap
		vhdl << tab << declare("OutRes",msbOut-lsbOut+1+g) << " <= " << 
			bitHeap->getSumName() << range(msbOut-lsbOut+g, 0) << ";" << endl; // This range is useful in case there was an overflow?

		vhdl << tab << "R <= OutRes" << range(msbOut-lsbOut+g, g) << ";" << endl;
*/

	}
	



	
	// Just to factor out code.
	/* This builds the input shifted WRT lsb-g
	 but doesn't worry about adding or subtracting it,
	 which depends wether we do a standalone or bit-heap   
	*/
	string FixRealShiftAdd::createShiftedPowerOfTwo(string resultSignalName){
		string rTempName = getName() + "_Rtemp"; // Should be unique in a bit heap if each KCM got a UID.
		// Compute shift that must be applied to x to align it to lsbout-g.
		// This is a shift left: negative means shift right.
		int shift= lsbIn -(lsbOut-g)  + msbC ; 
		int rTempSize = msbC+msbIn -(lsbOut -g) +1; // initial msbOut is msbC+msbIn
		REPORT(DETAILED,"Power of two, msbC=" << msbC << "     Shift left of " << shift << " bits");
		// compute the product by the abs constant
		thisOp->vhdl << tab << thisOp->declare(rTempName, rTempSize) << " <= ";
		// Still there are two cases: 
		if(shift>=0) {  // Shift left; pad   THIS SEEMS TO WORK
			thisOp->vhdl << inputSignalName << " & " << zg(shift);
			// rtempsize= msbIn-lsbin+1   + shift  =   -lsbIn   + msbIn   +1   - (lsbOut-g -lsbIn +msbC)
		}
		else { // shift right; truncate
			thisOp->vhdl << inputSignalName << range(msbIn-lsbIn, -shift);
		}
#if 1 // This used to break the lexer, I keep it as a case study to fix it. TODO
		thisOp->vhdl <<  "; -- constant is a power of two, shift left of " << shift << " bits" << endl;
#else
		ostringstream t;
		t << "; -- constant is a power of two, shift left of " << shift << " bits" << endl;
		thisOp->vhdl <<  t.str();
#endif
		// copy the signedness into rtemp
		thisOp->getSignalByName(rTempName)->setIsSigned(signedInput);
		return rTempName;
	}



	// TODO manage correctly rounded cases, at least the powers of two
	// To have MPFR work in fix point, we perform the multiplication in very
	// large precision using RN, and the RU and RD are done only when converting
	// to an int at the end.
	void FixRealShiftAdd::emulate(TestCase* tc)
	{
		// Get I/O values
		mpz_class svX = tc->getInputValue("X");
		bool negativeInput = false;
		int wIn=msbIn-lsbIn+1;
		int wOut=msbOut-lsbOut+1;
		
		// get rid of two's complement
		if(signedInput)	{
			if ( svX > ( (mpz_class(1)<<(wIn-1))-1) )	 {
				svX -= (mpz_class(1)<<wIn);
				negativeInput = true;
			}
		}
		
		// Cast it to mpfr 
		mpfr_t mpX; 
		mpfr_init2(mpX, msbIn-lsbIn+2);	
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN); // should be exact
		
		// scale appropriately: multiply by 2^lsbIn
		mpfr_mul_2si(mpX, mpX, lsbIn, GMP_RNDN); //Exact
		
		// prepare the result
		mpfr_t mpR;
		mpfr_init2(mpR, 10*wOut);
		
		// do the multiplication
		mpfr_mul(mpR, mpX, mpC, GMP_RNDN);
		
		// scale back to an integer
		mpfr_mul_2si(mpR, mpR, -lsbOut, GMP_RNDN); //Exact
		mpz_class svRu, svRd;
		
		mpfr_get_z(svRd.get_mpz_t(), mpR, GMP_RNDD);
		mpfr_get_z(svRu.get_mpz_t(), mpR, GMP_RNDU);

		//		cout << " emulate x="<< svX <<"  before=" << svRd;
 		if(negativeInput != negativeConstant)		{
			svRd += (mpz_class(1) << wOut);
			svRu += (mpz_class(1) << wOut);
		}
		//		cout << " emulate after=" << svRd << endl;

		//Border cases
		if(svRd > (mpz_class(1) << wOut) - 1 )		{
			svRd = 0;
		}

		if(svRu > (mpz_class(1) << wOut) - 1 )		{
			svRu = 0;
		}

		tc->addExpectedOutput("R", svRd);
		tc->addExpectedOutput("R", svRu);

		// clean up
		mpfr_clears(mpX, mpR, NULL);
	}





	
	TestList FixRealShiftAdd::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			vector<string> constantList; // The list of constants we want to test
			constantList.push_back("\"0\"");
			constantList.push_back("\"0.125\"");
			constantList.push_back("\"-0.125\"");
			constantList.push_back("\"4\"");
			constantList.push_back("\"-4\"");
			constantList.push_back("\"log(2)\"");
			constantList.push_back("-\"log(2)\"");
			constantList.push_back("\"0.00001\"");
			constantList.push_back("\"-0.00001\"");
			constantList.push_back("\"0.0000001\"");
			constantList.push_back("\"-0.0000001\"");
			constantList.push_back("\"123456\"");
			constantList.push_back("\"-123456\"");

			for(int wIn=3; wIn<16; wIn+=4) { // test various input widths
				for(int lsbIn=-1; lsbIn<2; lsbIn++) { // test various lsbIns
					string lsbInStr = to_string(lsbIn);
					string msbInStr = to_string(lsbIn+wIn);
					for(int lsbOut=-1; lsbOut<2; lsbOut++) { // test various lsbIns
						string lsbOutStr = to_string(lsbOut);
						for(int signedInput=0; signedInput<2; signedInput++) {
							string signedInputStr = to_string(signedInput);
							for(auto c:constantList) { // test various constants
								paramList.push_back(make_pair("lsbIn",  lsbInStr));
								paramList.push_back(make_pair("lsbOut", lsbOutStr));
								paramList.push_back(make_pair("msbIn",  msbInStr));
								paramList.push_back(make_pair("signedInput", signedInputStr));
								paramList.push_back(make_pair("constant", c));
								testStateList.push_back(paramList);
								paramList.clear();
							}
						}
					}
				}
			}
		}
		else     
		{
				// finite number of random test computed out of index
		}	

		return testStateList;
	}

	OperatorPtr FixRealShiftAdd::parseArguments(OperatorPtr parentOp, Target* target, std::vector<std::string> &args)
	{
		int lsbIn, lsbOut, msbIn;
		bool signedInput;
		double targetUlpError;
		string constant;
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseString(args, "constant", &constant);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		UserInterface::parseInt(args, "msbIn", &msbIn);
		UserInterface::parseBoolean(args, "signedInput", &signedInput);
		UserInterface::parseFloat(args, "targetUlpError", &targetUlpError);	
		return new FixRealShiftAdd(
													parentOp,
													target, 
													signedInput,
													msbIn,
													lsbIn,
													lsbOut,
													constant, 
													targetUlpError
													);
	}

	void FixRealShiftAdd::registerFactory()
	{
		UserInterface::add(
				"FixRealShiftAdd",
				"Table based real multiplier. Output size is computed",
				"ConstMultDiv",
				"",
				"signedInput(bool): 0=unsigned, 1=signed; \
				msbIn(int): weight associated to most significant bit (including sign bit);\
				lsbIn(int): weight associated to least significant bit;\
				lsbOut(int): weight associated to output least significant bit; \
				constant(string): constant given in arbitrary-precision decimal, or as a Sollya expression, e.g \"log(2)\"; \
				targetUlpError(real)=1.0: required precision on last bit. Should be strictly greater than 0.5 and lesser than 1;",
				"This variant of Ken Chapman's Multiplier is briefly described in <a href=\"bib/flopoco.html#DinIstoMas2014-SOPCJR\">this article</a>.<br> Special constants, such as 0 or powers of two, are handled efficiently.",
				FixRealShiftAdd::parseArguments,
				FixRealShiftAdd::unitTest
		);
	}
}





