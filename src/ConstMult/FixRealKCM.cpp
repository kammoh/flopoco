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

#include "../Operator.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <sollya.h>
#include "../utils.hpp"
#include "FixRealKCM.hpp"
#include "../IntAddSubCmp/IntAdder.hpp"

using namespace std;

namespace flopoco{



	/**
	* @brief init : all operator initialization stuff goes here
	*/
	void FixRealKCM::init()
	{
		useNumericStd();

		srcFileName="FixRealKCM";

		setCopyrightString("Florent de Dinechin (2007-2016), 3IF Dev Team 2015");

		if(lsbIn>msbIn) 
			throw string("FixRealKCM: Error, lsbIn>msbIn");
    
		if(targetUlpError > 1.0)
			THROWERROR("FixRealKCM: Error, targetUlpError="<<
					targetUlpError<<">1.0. Should be in ]0.5 ; 1].");
		//Error on final rounding is er <= 2^{lsbout - 1} = 1/2 ulp so 
		//it's impossible to reach 0.5 ulp of precision if cste is real
		if(targetUlpError <= 0.5) 
			THROWERROR("FixRealKCM: Error, targetUlpError="<<
					targetUlpError<<"<0.5. Should be in ]0.5 ; 1]");
		
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

		//if negative constant, then set negativeConstant and remake the
		//constant positive
		negativeConstant = false;
		if(mpfr_cmp_si(mpC, 0) < 0)
		{
			//throw string("FixRealKCMBH: only positive constants are supported");
			negativeConstant = true;
		}

		mpfr_abs(absC, mpC, GMP_RNDN);

		REPORT(DEBUG, "Constant evaluates to " << mpfr_get_d(mpC, GMP_RNDN));

		// build the name
		ostringstream name; 
		name <<"FixRealKCM_"  << vhdlize(msbIn)
				 << "_"  << vhdlize(lsbIn)
				 <<	"_" << vhdlize(lsbOut)
				 << "_" << vhdlize(constant)
				 << (signedInput  ?"_signed" : "_unsigned");
		setNameWithFreqAndUID(name.str());

		//compute the logarithm only of the constants
		mpfr_t log2C;
		mpfr_init2(log2C, 100); // should be enough for anybody
		mpfr_log2(log2C, absC, GMP_RNDN);
		msbC = mpfr_get_si(log2C, GMP_RNDU);
		mpfr_clears(log2C, NULL);

		// Now we can check when this is a multiplier by 0: either because the it is zero, or because it is close enough
		constantRoundsToZero = false;
		if(mpfr_zero_p(mpC) != 0){
			constantRoundsToZero = true;
			msbOut=lsbOut; // let us return a result on one bit, why not.
			wOut = 1;
			REPORT(INFO, "It seems somebody asked for a multiplication by 0. We can do that.");
			return;
		}
		
		// A few sanity checks related to the magnitude of the constant
		int lsbInUseful = lsbOut -1 -msbC; // input bits with lower weights than that have no inpact on the output

		if(msbIn<lsbInUseful){
			constantRoundsToZero = true;
			msbOut=lsbOut; // let us return a result on one bit, why not.
			wOut = 1;
			REPORT(INFO, "Multiplying the input by such a small constant always returns 0. This simplifies the architecture.");
			return;
		}

		// Now we can discuss MSBs
		msbOut = msbC + msbIn;
		if(!signedInput && negativeConstant)
			msbOut++; //Result would be signed

		wOut = msbOut - lsbOut +1;

		// Now even if the constant doesn't round completely to zero, it could be small enough that some of the inputs bits will have no impact on the output
		lsbInOrig=lsbIn;
		if(lsbIn<lsbInUseful) { // some of the user-provided input bits would have no impact on the output
			lsbIn=lsbInUseful;   // therefore ignore them.
			REPORT(DETAILED, "lsbIn="<<lsbInOrig << " but for such a small constant, bits of weight lower than " << lsbIn << " will have no impact on the output. Ignoring them.")  ;
		}

		// Finally, check if the constant is a power of two -- obvious optimization there
		constantIsPowerOfTwo = (mpfr_cmp_ui_2exp(absC, 1, msbC) == 0);
		if(constantIsPowerOfTwo)
			REPORT(DEBUG, "Constant is a power of two. Simple shift will be used instead of tables");
			

		
		REPORT(DEBUG, "msbConstant=" << msbC  << "   (msbIn,lsbIn)=("<< 
					 msbIn << "," << lsbIn << ")    lsbInOrig=" << lsbInOrig << 
				"   (msbOut,lsbOut)=(" << msbOut << "," << lsbOut <<
				")   wOut=" << wOut
		);

		// Compute the splitting of the input bits.
		int optimalTableInputWidth = getTarget()->lutInputs()-1;
		
	}



	//standalone operator
	FixRealKCM::FixRealKCM(Target* target, bool signedInput_, int msbIn_, int lsbIn_, int lsbOut_, string constant_, double targetUlpError_, map<string, double> inputDelays):
							Operator(target, inputDelays),
							signedInput(signedInput_),
							msbIn(msbIn_),
							lsbIn(lsbIn_),
							lsbOut(lsbOut_),
							constant(constant_),
							targetUlpError(targetUlpError_)
	{
		init();		 // check special cases, computes number of tables and g.
		
		addInput("X", msbIn-lsbInOrig+1);
		addOutput("R", wOut);

		// Special cases
		if(constantRoundsToZero)
		{
			vhdl << tab << "R" << " <= " << zg(wOut, 0) << ";" << endl;
			return;
		}

		if(constantIsPowerOfTwo)
		{
			// Shift it to its place
			THROWERROR ("thisConstantIsAPowerOfTwo: TODO");
			vhdl << tab << "R" << " <= " << zg(wOut, 0) << ";" << endl;
			return;
		}


		// From now we have stuff to do.
		//create the bitheap
		int bitheaplsb = lsbOut - g;
		bitHeap = new BitHeap(this, wOut+g);

		//		buildTables("X");
		// addTablesToBitHeap();

		setCriticalPath(getMaxInputDelays(inputDelays));
		manageCriticalPath(target->localWireDelay() + target->lutDelay());


		//compress the bitheap and produce the result
		bitHeap->generateCompressorVHDL();

		//manage the critical path
		syncCycleFromSignal(bitHeap->getSumName());

		//because of final add in bit heap, add one more bit to the result
		vhdl << declare("OutRes", wOut+g) << " <= " << 
			bitHeap->getSumName() << range(wOut+g-1, 0) << ";" << endl;

		vhdl << tab << "R <= OutRes" << range(wOut+g-1, g) << ";" << endl;
		
		outDelayMap["R"] = getCriticalPath();
	}
	
	

	//operator incorporated into a global bit heap for use as part of a bigger operator
	// In this case, the flow is typically (see FixFIR)
	//    1/ call the constructor below. It stops before the VHDL generation 
	//    2/ have it report g using getGuardBits(). 
	//    3/ the bigger operator computes the global g, builds the bit heap.
	//    4/ It calls generateVHDL(bitheap, bitheapLSB)
	FixRealKCM::FixRealKCM(Operator* parentOp, Target* target, Signal* fixPointInput, 
			int lsbOut_, string constant_, double targetUlpError_, map<string, double> inputDelays):
					Operator(target, inputDelays),
					signedInput(fixPointInput->isFixSigned()),
					msbIn(fixPointInput->MSB()),
					lsbIn(fixPointInput->LSB()),
					lsbOut(lsbOut_),
					constant(constant_),
					targetUlpError(targetUlpError_)
	{
		// Run-time check that the input is a fixed-point signal -- a bit late after all the initializations.
		if(!fixPointInput->isFix())
			THROWERROR ("FixRealKCM bit heap constructor called for a non-fixpoint input: can't determine its msb and lsb");
		
		init(); // check special cases, computes number of tables and g. 
		
	}
					
	
	void FixRealKCM::buildForBitHeap() {
			// Special cases
		if(constantRoundsToZero) { // .... do nothing
			return;
		}

		if(constantIsPowerOfTwo)	{
			// Shift it to its place
			THROWERROR ("thisConstantIsAPowerOfTwo: TODO");
			return;
		}

		// From now we have stuff to do.
		//		buildTables(multiplicandX->getName());
		//    addTablesToBitHeap();
	}			



	// To have MPFR work in fix point, we perform the multiplication in very
	// large precision using RN, and the RU and RD are done only when converting
	// to an int at the end.
	void FixRealKCM::emulate(TestCase* tc)
	{
		// Get I/O values
		mpz_class svX = tc->getInputValue("X");
		bool negativeInput = false;
		int wIn=msbIn-lsbInOrig;
		
		// get rid of two's complement
		if(signedInput)
		{
			if ( svX > ( (mpz_class(1)<<(wIn-1))-1) )
			{
				svX = (mpz_class(1)<<wIn) - svX;
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
		mpfr_mul(mpR, mpX, absC, GMP_RNDN);
		
		// scale back to an integer
		mpfr_mul_2si(mpR, mpR, -lsbOut, GMP_RNDN); //Exact
		mpz_class svRu, svRd;
		
		mpfr_get_z(svRd.get_mpz_t(), mpR, GMP_RNDD);
		mpfr_get_z(svRu.get_mpz_t(), mpR, GMP_RNDU);

		if(negativeInput != negativeConstant)
		{
			svRd = (mpz_class(1) << wOut) - svRd;
			svRu = (mpz_class(1) << wOut) - svRu;
		}

		//Border cases
		if(svRd > (mpz_class(1) << wOut) - 1 )
		{
			svRd = 0;
		}

		if(svRu > (mpz_class(1) << wOut) - 1 )
		{
			svRu = 0;
		}

		tc->addExpectedOutput("R", svRd);
		tc->addExpectedOutput("R", svRu);

		// clean up
		mpfr_clears(mpX, mpR, NULL);
	}




	OperatorPtr FixRealKCM::parseArguments(Target* target, std::vector<std::string> &args)
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
		return new FixRealKCM(
				target, 
				signedInput,
				msbIn,
				lsbIn,
				lsbOut,
				constant, 
				targetUlpError
			);
	}

	void FixRealKCM::registerFactory()
	{
		UserInterface::add(
				"FixRealKCM",
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
				FixRealKCM::parseArguments		
		);
	}

	/************************** The FixRealKCMTable class ********************/


	FixRealKCMTable::FixRealKCMTable(Target* target, FixRealKCM* mother, int i, int weight, int wIn, int wOut, bool negativeSubproduct, bool last, int pipeline):
			Table(target, wIn, wOut, 0, -1, pipeline), 
			mother(mother), 
			index(i), 
			weight(weight), 
			negativeSubproduct(negativeSubproduct), 
			last(last)
	{
		ostringstream name; 
		srcFileName="FixRealKCM";
		name << mother->getName() << "_Table_" << index;
		setNameWithFreqAndUID(name.str());
		setCopyrightString("Florent de Dinechin (2007-2011-?), 3IF Dev Team"); 
	}
  
	mpz_class FixRealKCMTable::function(int x0)
	{
		int x;
		bool negativeInput = false;
		
		// get rid of two's complement
		x = x0;
		//Only the last "digit" has a negative weight
		if(mother->signedInput && last)
		{
			if ( x0 > ((1<<(wIn-1))-1) )
			{
				x = (1<<wIn) - x;
				negativeInput = true;
			}
		}

		mpz_class result;
		mpfr_t mpR, mpX;

		mpfr_init2(mpR, 10*wOut);
		mpfr_init2(mpX, 2*wIn); //To avoid mpfr bug if wIn = 1

		if(x == 0)
		{
			result = mpz_class(0);
		}
		else
		{
			mpfr_set_si(mpX, x, GMP_RNDN); // should be exact
			//Getting real weight
			mpfr_mul_2si(mpX, mpX, weight, GMP_RNDN); //Exact


			// do the mult in large precision
			mpfr_mul(mpR, mpX, mother->absC, GMP_RNDN);
			
			// Result is integer*C, which is more or less what we need: just
			// scale to add g bits.
			mpfr_mul_2si(
					mpR, 
					mpR,
					-mother->lsbOut + mother->g,	
					GMP_RNDN
				); //Exact

			// Here is when we do the rounding
			mpfr_get_z(result.get_mpz_t(), mpR, GMP_RNDN); // Should be exact

			//Get the real sign
			if(negativeInput != mother->negativeConstant && result != 0)
			{
				result = (mpz_class(1) << wOut) - result;
			}
		}

		//Result is result -1 in order to avoid to waste a sign bit
		if(negativeSubproduct)
		{
			if(result == 0)
				result = (mpz_class(1) << wOut) - 1;
			else
				result -= 1;
		}

		//In case of global bitHeap, we need to invert msb for signedInput
		//last bit for fast sign extension.
		if(last && mother->signedInput)
		{
			mpz_class shiffter = mpz_class(1) << (wOut - 1);
			if(result < shiffter )
			{
				result += shiffter;
			}
			else
			{
				result -= shiffter;
			}
		}


		return result;
	}
}





