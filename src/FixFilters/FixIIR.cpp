#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"
#if HAVE_WCPG
extern "C"
{
	#include "wcpg.h"
}
#endif

#include "FixIIR.hpp"

#include "sollya.h"

#include "ShiftReg.hpp"
#include "FixSOPC.hpp"



using namespace std;

namespace flopoco {

	FixIIR::FixIIR(Target* target, int lsbIn_, int lsbOut_,  vector<string> coeffb_, vector<string> coeffa_, double H_) :
		Operator(target), lsbIn(lsbIn_), lsbOut(lsbOut_), coeffb(coeffb_), coeffa(coeffa_), H(H_)
	{
		srcFileName="FixIIR";
		setCopyrightString ( "Florent de Dinechin, Louis Beseme (2014)" );
		useNumericStd_Unsigned();


		ostringstream name;
		name << "FixIIR";
		setNameWithFreqAndUID( name.str() );

		m = coeffa.size();
		n = coeffb.size();

		addInput("X", 1-lsbIn, true);

		//Parsing the coefficients, into MPFR (and double for H but it is temporary)

		coeffa_d  = (double*) malloc(m * sizeof(double));
		coeffb_d  = (double*) malloc(n * sizeof(double));
		coeffa_mp = (mpfr_t*) malloc(m * sizeof(mpfr_t));
		coeffb_mp = (mpfr_t*) malloc(n * sizeof(mpfr_t));
		xHistory  = (mpfr_t*) malloc(n * sizeof(mpfr_t));
		yHistory  = (mpfr_t*) malloc((m+1) * sizeof(mpfr_t)); // We need to memorize the m previous y, and the current output


		for (int i=0; i< n; i++)		{
			// parse the coeffs from the string, with Sollya parsing
			sollya_obj_t node;

			node = sollya_lib_parse_string(coeffb[i].c_str());
			if(node == 0)					// If conversion did not succeed (i.e. parse error)
				THROWERROR("Unable to parse string " << coeffb[i] << " as a numeric constant");

			mpfr_init2(coeffb_mp[i], 10000);
			sollya_lib_get_constant(coeffb_mp[i], node);
			coeffb_d[i] = mpfr_get_d(coeffb_mp[i], GMP_RNDN);

		}

		for (int i=0; i< m; i++)		{
			// parse the coeffs from the string, with Sollya parsing
			sollya_obj_t node;

			node = sollya_lib_parse_string(coeffa[i].c_str());
			if(node == 0)					// If conversion did not succeed (i.e. parse error)
				THROWERROR("Unable to parse string " << coeffb[i] << " as a numeric constant");
			
			mpfr_init2(coeffa_mp[i], 10000);
			sollya_lib_get_constant(coeffa_mp[i], node);
			coeffa_d[i] = mpfr_get_d(coeffa_mp[i], GMP_RNDN);
		}


		REPORT(INFO, "H=" << H);
		
		// TODO here compute H if it is not provided
		if(H==0) {
#if HAVE_WCPG

			REPORT(INFO, "computing worst-case peak gain");

#if 1
			if (!WCPG_tf(&H, coeffb_d, coeffa_d, n, m))
				THROWERROR("Could not compute WCPG");
			REPORT(INFO, "Worst-case peak gain is H=" << H);
#endif
			
#else 
			THROWERROR("WCPG was not found (see cmake output), cannot compute worst-case peak gain H. Either provide H, or compile FloPoCo with WCPG");
#endif
		}
		
		// guard bits for a faithful result
		g= intlog2(2*H*(n+m)); // see the paper
		int lsbOutSOPC = lsbOut-g;
		msbOut = ceil(log2(H)); 
		REPORT(INFO, "g=" << g << " so we ask for a SOPC faithful to lsbOutSOPC=" << lsbOutSOPC);
		REPORT(INFO, "msbOut=" << msbOut);


		// Initialisations for the emulate
		hugePrec = 10*(1+msbOut+-lsbOut+g);
		currentIndex=0;

		for (int i = 0; i<m+1; i++)
		{
			mpfr_init2 (yHistory[i], hugePrec);
			mpfr_set_d(yHistory[i], 0.0, GMP_RNDN);
		}
		for(int i=0; i<n; i++) {
			mpfr_init2 (xHistory[i], hugePrec);
			mpfr_set_d(xHistory[i], 0.0, GMP_RNDN);
		}

		// The shift registers
		ShiftReg *inputShiftReg = new ShiftReg(getTarget(), 1-lsbIn, n);
		addSubComponent(inputShiftReg);
		inPortMap(inputShiftReg, "X", "X");
		for(int i = 0; i<n; i++) {
			outPortMap(inputShiftReg, join("Xd", i), join("U", i));
		}
		vhdl << instance(inputShiftReg, "inputShiftReg");
		setCycle(0);

		vhdl << tab << declare("YinternalLoopback", msbOut-lsbOut+1+g) << " <= Yinternal;" << endl; // just so that the inportmap finds it  
		ShiftReg *outputShiftReg = new ShiftReg(getTarget(), 1-lsbOutSOPC, m);
		addSubComponent(outputShiftReg);
		inPortMap(outputShiftReg, "X", "YinternalLoopback");
		for(int i = 0; i<n; i++) {
			outPortMap(outputShiftReg, join("Xd", i), join("Y", i));
		}
		vhdl << instance(outputShiftReg, "outputShiftReg");
		setCycle(0);
		
		// Now building a single SOPC. For this we need the following info:
		//		FixSOPC(Target* target, vector<double> maxX, vector<int> lsbIn, int msbOut, int lsbOut, vector<string> coeff_, int g=-1);
		// We will concatenate coeffb of size n then then coeffa of size m

		// MaxX
		vector<double> maxInSOPC;
		vector<int> lsbInSOPC;
		vector<string> coeffSOPC;
		for(int i=0; i<n; i++) {
			maxInSOPC.push_back(1.0); // max(u) = 1.
			lsbInSOPC.push_back(lsbIn); // for u
			coeffSOPC.push_back(coeffb[i]);
		}
		for (int i = 0; i<m; i++)	{
			maxInSOPC.push_back(H); // max (y) = H. 
			lsbInSOPC.push_back(lsbOutSOPC); // for y
			coeffSOPC.push_back(coeffa[i]);
		}
		
		FixSOPC* fixSOPC = new FixSOPC(getTarget(), maxInSOPC, lsbInSOPC, msbOut, lsbOutSOPC, coeffSOPC, -1); // -1 means: faithful
		addSubComponent(fixSOPC);
		for(int i=0; i<n; i++) {
			inPortMap(fixSOPC, join("X",i), join("U", i));
		}
		for (int i = 0; i<m; i++)	{
			inPortMap(fixSOPC, join("X",i+n), join("Y", i));
		}
		outPortMap(fixSOPC, "R", "Yinternal");
		vhdl << instance(fixSOPC, "fixSOPC");
		syncCycleFromSignal("Yinternal");

		
		addOutput("R", msbOut - lsbOut + 1,   true);
		vhdl << "R <= Yinternal" << range(msbOut-lsbOut+g, g) << ";" << endl;

	};



	FixIIR::~FixIIR(){
		free(coeffb_d);
		free(coeffa_d);
		for (int i=0; i<n; i++) {
			mpfr_clear(coeffb_mp[i]);
			mpfr_clear(xHistory[i]);
		}
		for (int i=0; i<m; i++) {
			mpfr_clear(coeffa_mp[i]);
			mpfr_clear(xHistory[i]);
		}
		free(coeffa_mp);
		free(coeffb_mp);
		free(xHistory);
		free(yHistory);
	};



	
	void FixIIR::emulate(TestCase * tc){

		mpz_class sx;
		mpfr_t x, s, t;
		mpfr_init2 (s, hugePrec);
		mpfr_init2 (t, hugePrec);
		mpfr_set_d(s, 0.0, GMP_RNDN); // initialize s to 0

		mpfr_init2 (x, 1-lsbOut);

		sx = tc->getInputValue("X"); 		// get the input bit vector as an integer
		sx = bitVectorToSigned(sx, 1-lsbIn); 						// convert it to a signed mpz_class
		mpfr_set_z (x, sx.get_mpz_t(), GMP_RNDD); 				// convert this integer to an MPFR; this rounding is exact
		mpfr_div_2si (x, x, -lsbIn, GMP_RNDD); 						// multiply this integer by 2^-p to obtain a fixed-point value; this rounding is again exact
		mpfr_set(xHistory[currentIndex % n], x, GMP_RNDN); // exact


		// TODO CHECK HERE
		for (int i=0; i< n; i++)
		{

			mpfr_mul(t, xHistory[(currentIndex+n+i)%n], coeffb_mp[i], GMP_RNDN); 					// Here rounding possible, but precision used is ridiculously high so it won't matter
			mpfr_add(s, s, t, GMP_RNDN); 							// same comment as above
		}

		for (int i=0; i<m; i++)
		{

			mpfr_mul(t, yHistory[(currentIndex+1 +m+1 +i)%(m+1)], coeffa_mp[i], GMP_RNDN); 					// Here rounding possible, but precision used is ridiculously high so it won't matter
			mpfr_add(s, s, t, GMP_RNDN); 							// same comment as above
		}
		mpfr_set(yHistory[currentIndex%(m+1)], t, GMP_RNDN);
		currentIndex++;

		//		coeff		  1 2 3
		//    yh      y 0 0 0 
		// now we should have in s the (exact in most cases) sum
		// round it up and down

		// make s an integer -- no rounding here
		mpfr_mul_2si (s, s, -lsbOut, GMP_RNDN);


		// We are waiting until the first meaningful value comes out of the IIR

		int wO = msbOut-lsbOut+1;
		mpz_class rdz, ruz;
		mpfr_get_z (rdz.get_mpz_t(), s, GMP_RNDD); 					// there can be a real rounding here
		rdz=signedToBitVector(rdz, wO);
		tc->addExpectedOutput ("R", rdz);

		mpfr_get_z (ruz.get_mpz_t(), s, GMP_RNDU); 					// there can be a real rounding here
		ruz=signedToBitVector(ruz, wO);
		tc->addExpectedOutput ("R", ruz);


		mpfr_clears (x, t, s, NULL);

	};

	void FixIIR::buildStandardTestCases(TestCaseList* tcl){};

	OperatorPtr FixIIR::parseArguments(Target *target, vector<string> &args) {
		int lsbIn;
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		int lsbOut;
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		double h;
		UserInterface::parseFloat(args, "h", &h);
		vector<string> inputa;
		string in;
		UserInterface::parseString(args, "coeffa", &in);
		// tokenize a string, thanks Stack Overflow
		stringstream ss(in);
		while( ss.good() )	{
				string substr;
				getline( ss, substr, ':' );
				inputa.push_back( substr );
			}

		vector<string> inputb;
		UserInterface::parseString(args, "coeffb", &in);
		stringstream ssb(in);
		while( ssb.good() )	{
				string substr;
				getline( ssb, substr, ':' );
				inputb.push_back( substr );
			}
		
		return new FixIIR(target, lsbIn, lsbOut, inputb, inputa, h);
	}


	void FixIIR::registerFactory(){
		UserInterface::add("FixIIR", // name
											 "A fix-point Infinite Impulse Response filter generator.",
											 "FiltersEtc", // categories
											 "",
											 "lsbIn(int): input most significant bit;\
                        lsbOut(int): output least significant bit;\
                        h(real)=0: worst-case peak gain: provide it only if WCPG is not installed;\
                        coeffa(string): colon-separated list of real coefficients using Sollya syntax. Example: coeffa=\"1.234567890123:sin(3*pi/8)\";\
                        coeffb(string): colon-separated list of real coefficients using Sollya syntax. Example: coeffb=\"1.234567890123:sin(3*pi/8)\"",
											 "",
											 FixIIR::parseArguments
											 ) ;
	}

}
