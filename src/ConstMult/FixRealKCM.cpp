// TODO: repair FixSOPC, FixIIR, FixComplexKCM
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




	
	//standalone operator
	FixRealKCM::FixRealKCM(Target* target, bool signedInput_, int msbIn_, int lsbIn_, int lsbOut_, string constant_, double targetUlpError_):
							Operator(target),
							signedInput(signedInput_),
							msbIn(msbIn_),
							lsbIn(lsbIn_),
							lsbOut(lsbOut_),
							constant(constant_),
							targetUlpError(targetUlpError_),
							parentOp(this),
							addRoundBit(true)
	{

		init();		 // check special cases, computes number of tables and g.

		// To help debug KCM called from other operators, report in FloPoCo CLI syntax
		REPORT(DETAILED, "FixRealKCM  signedInput=" << signedInput << " msbIn=" << msbIn << " lsbIn=" << lsbIn << " lsbOut=" << lsbOut << " constant=\"" << constant << "\"  targetUlpError="<< targetUlpError);
		
		addInput("X",  msbIn-lsbInOrig+1);
		inputSignalName = "X"; // for buildForBitHeap
		addOutput("R", msbOut-lsbOut+1);

		// Special cases
		if(constantRoundsToZero)	{
			vhdl << tab << "R" << " <= " << zg(msbOut-lsbOut, 0) << ";" << endl;
			return;
		}

		if(constantIsPowerOfTwo)	{
			// Shift it to its place
			THROWERROR ("thisConstantIsAPowerOfTwo: TODO");
			return;
		}


		// From now we have stuff to do.
		//create the bitheap
		int bitheaplsb = lsbOut - g;
		REPORT(DEBUG, "Creating bit heap for msbOut=" << msbOut <<" lsbOut=" << lsbOut <<" g=" << g);
		bitHeap = new BitHeap(this, msbOut-lsbOut+1+g); // hopefully some day we get a fixed-point bit heap

		buildForBitHeap(bitHeap, g); // does everything up to bit heap compression

		manageCriticalPath(target->localWireDelay() + target->lutDelay());

		//compress the bitheap and produce the result
		bitHeap->generateCompressorVHDL();

		//manage the critical path
		syncCycleFromSignal(bitHeap->getSumName());

		// Retrieve the bits we want from the bit heap
		vhdl << declare("OutRes",msbOut-lsbOut+1+g) << " <= " << 
			bitHeap->getSumName() << range(msbOut-lsbOut+g, 0) << ";" << endl; // This range is useful in case there was an overflow?

		vhdl << tab << "R <= OutRes" << range(msbOut-lsbOut+g, g) << ";" << endl;
		
		outDelayMap["R"] = getCriticalPath();
	}
	
	

	//operator incorporated into a global bit heap for use as part of a bigger operator
	// It works in "dry run" mode: it computes g but does not generate any VHDL
	// In this case, the flow is typically (see FixFIR)
	//    1/ call the constructor below. It stops before the VHDL generation 
	//    2/ have it report g using getGuardBits(). 
	//    3/ the bigger operator computes the global g, builds the bit heap.
	//    4/ It calls buildForBitHeap(bitHeap, g)
	FixRealKCM::FixRealKCM(
												 Operator* parentOp_, 
												 Signal* multiplicandX,
												 int lsbOut_, 
												 string constant_,
												 bool addRoundBit_,
												 double targetUlpError_
												 ):
		Operator(parentOp_->getTarget()),
		signedInput(multiplicandX->isFixSigned()),
		msbIn(multiplicandX->MSB()),
		lsbIn(multiplicandX->LSB()),
		lsbOut(lsbOut_),
		constant(constant_),
		targetUlpError(targetUlpError_),
		parentOp(parentOp_),
		bitHeap(NULL), // will be set by buildForBitHeap()
		inputSignalName(multiplicandX->getName()),
		addRoundBit(addRoundBit_)
	{
		// Run-time check that the input is a fixed-point signal -- a bit late after all the initializations.
		if(!multiplicandX->isFix())
			THROWERROR ("FixRealKCM bit heap constructor called for a non-fixpoint input: can't determine its msb and lsb");
		
		init(); // check special cases, computes number of tables, but do not compute g: just take lsbOut as it is. 
		
		// Special cases
		if(constantRoundsToZero)		{
			// do nothing
			return;
		}

		if(constantIsPowerOfTwo)		{
			// Shift it to its place
			THROWERROR ("thisConstantIsAPowerOfTwo: TODO");
			return;
		}
	}
					
	








	/**
	* @brief init : all operator initialization stuff goes here
	*/
	// Init computes the table splitting, because it is independent of g.
	// It is, however, dependent on lsbOut: for "small constants" we consider lsbOut to decide which bits of the input will be used. 
	void FixRealKCM::init()
	{
		useNumericStd();

		srcFileName="FixRealKCM";

		setCopyrightString("Florent de Dinechin (2007-2016), 3IF Dev Team 2015");

		if(lsbIn>msbIn) 
			throw string("FixRealKCM: Error, lsbIn>msbIn");


		g=0; // tentatively. 
    
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

		//if negative constant, then set negativeConstant and remake the constant positive
		negativeConstant = false;
		if(mpfr_cmp_si(mpC, 0) < 0)	{
			//throw string("FixRealKCMBH: only positive constants are supported");
			negativeConstant = true;
		}

		signedOutput = negativeConstant || signedInput;
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
			errorInUlps=0;
			REPORT(INFO, "It seems somebody asked for a multiplication by 0. We can do that.");
			return;
		}
		
		// A few sanity checks related to the magnitude of the constant
		int lsbInUseful = lsbOut -1 -msbC; // input bits with lower weights than that have no inpact on the output

		if(msbIn<lsbInUseful){
			constantRoundsToZero = true;
			msbOut=lsbOut; // let us return a result on one bit, why not.
			errorInUlps=0.5;// TODO this is an overestimation
			g=0; // still no need for gard bits
			REPORT(INFO, "Multiplying the input by such a small constant always returns 0. This simplifies the architecture.");
			return;
		}

		
		// Now we can discuss MSBs
		msbOut = msbC + msbIn;
		if(signedOutput)
			msbOut++; // add the sign bit

		// Now even if the constant doesn't round completely to zero, it could be small enough that some of the inputs bits will have little impact on the output
		// Some day we compute a TMD using continued fractions and we can ignore useless bits.
		// Meanwhile we just print a warning
		lsbInOrig=lsbIn;
		if(lsbIn<lsbInUseful) { // some of the user-provided input bits would have no impact on the output
			// lsbIn=lsbInUseful;   // therefore ignore them.
			REPORT(DETAILED, "WARNING: lsbIn="<<lsbInOrig << " but I observe that bits of weight lower than " << lsbIn << " will have very little impact on the output.")  ;
		}

		// Finally, check if the constant is a power of two -- obvious optimization there
		constantIsPowerOfTwo = (mpfr_cmp_ui_2exp(absC, 1, msbC) == 0);
		if(constantIsPowerOfTwo) {
			// What is the error in this case ? 0 if no truncation, 1 if truncation
			// We decide considering lsbOut only. It is an overestimation (therefore it is OK):
			// maybe due to guard bits there will be no truncation and we assumed there would be one...  
			if(lsbInOrig+msbC<lsbOut) {
				REPORT(DETAILED, "Constant is a power of two. Simple shift will be used instead of tables, but still there will be a truncation");
				// 
				errorInUlps=1;
				g=0;
				return;
			}
			else {
				REPORT(DETAILED, "Constant is a power of two. Simple shift will be used instead of tables, and this KCM will be exact");
				errorInUlps=0;
				return;
			}
		}

		
		REPORT(DEBUG, "msbConstant=" << msbC  << "   (msbIn,lsbIn)=("<< 
					 msbIn << "," << lsbIn << ")    lsbInOrig=" << lsbInOrig << 
				"   (msbOut,lsbOut)=(" << msbOut << "," << lsbOut <<
					 ")  signedOutput=" << signedOutput
		);

		// Compute the splitting of the input bits.
		// Ordering: Table 0 is the one that inputs the MSBs of the input 
		// Let us call l the native LUT size (discounting the MUXes internal to logic clusters that enable to assemble them as larger LUTs).
		// Asymptotically (for large wIn), it is better to split the input in l-bit slices than (l+1)-bit slices, even if we have these muxes for free:
		// costs (in LUTs) are roundUp(wIn/l) versus 2*roundUp(wIn/(l+1))
		// Exception: when one of the tables is a 1-bit one, having it as a separate table will cost one LUT and one addition more,
		// whereas integrating it in the previous table costs one more LUT and one more MUX (the latter being for free) 
		// Fact: the tables which inputs MSBs have larger outputs, hence cost more.
		// Therefore, the table that will have this double LUT cost should be the rightmost one.

		
		int optimalTableInputWidth = getTarget()->lutInputs();
		//		int wIn = msbIn-lsbIn+1;

		// first do something naive, then we'll optimize it.
		numberOfTables=0;
		int tableInMSB= msbIn; 
		int tableInLSB = msbIn-(optimalTableInputWidth-1);
		while (tableInLSB>lsbIn) {
			m.push_back(tableInMSB);
			l.push_back(tableInLSB);
			tableInMSB -= optimalTableInputWidth ;
			tableInLSB -= optimalTableInputWidth ;
			numberOfTables++;
		}
		tableInLSB=lsbIn;
		
		// For the last table we just check if is not of size 1
		if(tableInLSB==tableInMSB && numberOfTables>0) {
			// better enlarge the previous table
			l[numberOfTables-1] --;
		}
		else { // last table is normal
			m.push_back(tableInMSB);
			l.push_back(tableInLSB);
			numberOfTables++;			
		}
		for (int i=0; i<numberOfTables; i++) {
			REPORT(DETAILED, "Table " << i << "   inMSB=" << m[i] << "   inLSB=" << l[i] );
		}

#if 1
		// Now we have everything to compute g
		if(numberOfTables==2 && targetUlpError==1.0)
			g=0; // specific case: two CR tables make up a faithful sum
		else
			g = ceil(log2(numberOfTables/((targetUlpError-0.5)*exp2(-lsbOut)))) -1 -lsbOut;
		REPORT(DEBUG, " g=" << g);
#endif
		// Finally computing the error due to this setup. We express it as ulps at position lsbOut-g, whatever g will be
		errorInUlps=0.5*numberOfTables;
	}







	void FixRealKCM::buildForBitHeap(BitHeap* bitHeap_, int g_) {
		bitHeap=bitHeap_;
		g=g_;

		for(int i=0; i<numberOfTables; i++) {
			string sliceInName = join(parentOp->getName() + "_A", i); // Should be unique in a bit heap if each KCM got a UID.
			string sliceOutName = join(parentOp->getName() + "_T", i); // Should be unique in a bit heap if each KCM got a UID.
			string instanceName = join(parentOp->getName() + "_Table", i); // Should be unique in a bit heap if each KCM got a UID.
			
			parentOp->vhdl << tab << declare(sliceInName, m[i]- l[i] +1 ) << " <= "
										 << inputSignalName << range(m[i]-lsbInOrig, l[i]-lsbInOrig) << ";" << endl;
			FixRealKCMTable* t = new FixRealKCMTable(parentOp->getTarget(), this, i);
			addSubComponent(t);
			parentOp->inPortMap (t , "X", sliceInName);
			parentOp->outPortMap(t , "Y", sliceOutName);
			parentOp->vhdl << parentOp->instance(t , instanceName);

			// Add these bits to the bit heap
			if(i==0 && signedOutput) {
				bitHeap -> addSignedBitVector(0, // weight
																			sliceOutName, // name
																			getSignalByName(sliceOutName)->width() // size
																			);
			}
			else {
				bitHeap -> addUnsignedBitVector(0, // weight
																				sliceOutName, // name
																				getSignalByName(sliceOutName)->width() // size
																				);
			}
		}
	}			



	
	// TODO manage correctly rounded cases, at least the powers of two
	// To have MPFR work in fix point, we perform the multiplication in very
	// large precision using RN, and the RU and RD are done only when converting
	// to an int at the end.
	void FixRealKCM::emulate(TestCase* tc)
	{
		// Get I/O values
		mpz_class svX = tc->getInputValue("X");
		bool negativeInput = false;
		int wIn=msbIn-lsbInOrig+1;
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
		mpfr_init2(mpX, msbIn-lsbInOrig+2);	
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN); // should be exact
		
		// scale appropriately: multiply by 2^lsbIn
		mpfr_mul_2si(mpX, mpX, lsbInOrig, GMP_RNDN); //Exact
		
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


	FixRealKCMTable::FixRealKCMTable(Target* target, FixRealKCM* mother, int i):
			Table(target,
						mother->m[i] - mother->l[i]+1, // wIn
						mother->m[i] + mother->msbC  - mother->lsbOut + mother->g +1, //wOut
						0, // minIn
						-1, // maxIn
						1), // logicTable 
			mother(mother), 
			index(i),
			lsbInWeight(mother->l[i])
	{
		ostringstream name; 
		srcFileName="FixRealKCM";
		name << mother->getName() << "_Table_" << index;
		setName(name.str()); // This one not a setNameWithFreqAndUID
		setCopyrightString("Florent de Dinechin (2007-2011-?), 3IF Dev Team"); 
	}
  



	mpz_class FixRealKCMTable::function(int x0)
	{
		int x;
		bool negativeInput = false;
		
		// get rid of two's complement
		x = x0;
		//Only the MSB "digit" has a negative weight
		if(mother->signedInput && (index==0))
		{
			if ( x0 > ((1<<(wIn-1))-1) )
			{
				x -= (1<<wIn);
				negativeInput = true;
			}
		}
		//		cout << x0 << "  sx=" << x <<"  wIn="<<wIn<< "   ";

		mpz_class result;
		mpfr_t mpR, mpX;

		mpfr_init2(mpR, 10*wOut);
		mpfr_init2(mpX, 2*wIn); //To avoid mpfr bug if wIn = 1

		if(x == 0){
			result = mpz_class(0);
		}
		else	{
			mpfr_set_si(mpX, x, GMP_RNDN); // should be exact
			// Scaling so that the input has its actual weight
			mpfr_mul_2si(mpX, mpX, lsbInWeight, GMP_RNDN); //Exact

			double dx = mpfr_get_d(mpX, GMP_RNDN);
			//			cout << "input as double=" <<dx << "  lsbInWeight="  << lsbInWeight << "    ";
			
			// do the mult in large precision
			mpfr_mul(mpR, mpX, mother->absC, GMP_RNDN);
			
			// Result is integer*C, which is more or less what we need: just scale it to an integer.
			mpfr_mul_2si(
					mpR, 
					mpR,
					-mother->lsbOut + mother->g,	
					GMP_RNDN
				); //Exact

			//      double dr=mpfr_get_d(mpR, GMP_RNDN);
			//			cout << "  dr=" << dr << "  ";
			
			// Here is when we do the rounding
			mpfr_get_z(result.get_mpz_t(), mpR, GMP_RNDN); // Should be exact

			//Get the real sign
			if(negativeInput != mother->negativeConstant && result != 0) {
				if(result<0)
					result +=(mpz_class(1) << wOut);
			}
		}
		
		//			cout << "  sdr=" << result << "  ";

		// // TODO F2D:  understand the following
		// //Result is result -1 in order to avoid to waste a sign bit
		// if(negativeSubproduct)
		// {
		// 	if(result == 0)
		// 		result = (mpz_class(1) << wOut) - 1;
		// 	else
		// 		result -= 1;
		// }

		// //In case of global bitHeap, we need to invert msb for signedInput
		// //last bit for fast sign extension.
		// if((index==0) && mother->signedInput)	{
		// 	mpz_class shiftedOne = mpz_class(1) << (wOut - 1);
		// 	if( result < shiftedOne ) // MSB=0
		// 		result += shiftedOne;  // set it to 1
		// 	else
		// 		result -= shiftedOne;  // set it to 0
		// }

		
		//		cout << result << endl;
			
		if(mother->addRoundBit && (index==mother->numberOfTables-1) && (mother->g>0))
			result += 1<<(mother->g-1);
		return result;
	}
}





