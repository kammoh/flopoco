#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"
#if HAVE_WCPG
#include "wcpg.h"
#endif

#include "FixIIR.hpp"

#include "sollya.h"

#include "ShiftReg.hpp"
#include "FixSOPC.hpp"



using namespace std;

namespace flopoco {

	FixIIR::FixIIR(Target* target, int lsbIn_, int msbOut_, int lsbOut_,  vector<string> coeffb_, vector<string> coeffa_, double H_) :
		Operator(target), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_), coeffb(coeffb_), coeffa(coeffa_), H(H_)
	{
		srcFileName="FixIIR";
		setCopyrightString ( "Louis Beseme, Florent de Dinechin (2014)" );
		useNumericStd_Unsigned();


		ostringstream name;
		name << "FixIIR";
		setNameWithFreqAndUID( name.str() );

		n = coeffb.size();
		m = coeffa.size();

		addInput("X", 1-lsbIn, true);


		//Parsing the coefficients, into MPFR (and double for H but it is temporary)

		coeffa_d = (double*)malloc(n * sizeof(double*));
		coeffb_d = (double*)malloc(m * sizeof(double*));


		for (int i=0; i< n; i++)		{
			// parse the coeffs from the string, with Sollya parsing
			sollya_obj_t node;

			node = sollya_lib_parse_string(coeffb[i].c_str());
			if(node == 0)					// If conversion did not succeed (i.e. parse error)
				THROWERROR("Unable to parse string " << coeffb[i] << " as a numeric constant");
			
			mpfr_init2(mpcoeffb[i], 10000);
			sollya_lib_get_constant(mpcoeffb[i], node);
			coeffb_d[i] = mpfr_get_d(mpcoeffb[i], GMP_RNDN);
			if(coeffb_d[i] < 0)
				coeffsignb[i] = 1;
			else
				coeffsignb[i] = 0;
			mpfr_abs(mpcoeffb[i], mpcoeffb[i], GMP_RNDN);
		}

		for (int i=0; i< m; i++)		{
			// parse the coeffs from the string, with Sollya parsing
			sollya_obj_t node;

			node = sollya_lib_parse_string(coeffa[i].c_str());
			if(node == 0)					// If conversion did not succeed (i.e. parse error)
				THROWERROR("Unable to parse string " << coeffb[i] << " as a numeric constant");
			
			mpfr_init2(mpcoeffa[i], 10000);
			sollya_lib_get_constant(mpcoeffa[i], node);
			coeffa_d[i] = mpfr_get_d(mpcoeffa[i], GMP_RNDN);
			if(coeffa_d[i] < 0)
				coeffsigna[i] = 1;
			else
				coeffsigna[i] = 0;
			mpfr_abs(mpcoeffa[i], mpcoeffa[i], GMP_RNDN);
		}


		REPORT(INFO, "H=" << H);
		
		// TODO here compute H if it is not provided
		if(H==0) {
#if HAVE_WCPG

			REPORT(INFO, "computing worst-case peak gain");

#if 0
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
		REPORT(INFO, "g=" << g);

		hugePrec = 10*(1+msbOut+-lsbOut+g);
		currentIndexA=0;
		currentIndexB=0;
		for (int i = 0; i<m; i++)
		{
			mpfr_init2 (yHistory[i], hugePrec);
			mpfr_set_d(yHistory[i], 0.0, GMP_RNDN);
		}
		for(int i=0; i<n; i++) {
			xHistory[i]=0;
		}


		wO = (msbOut - lsbOut) + 1; //1 + sign  ;



		int size = wO + g ;
		REPORT(INFO, "Sum size is: "<< size );

	};



	FixIIR::~FixIIR(){
		free(coeffb_d);
		free(coeffa_d);
		for (int i=0; i<n; i++)
				mpfr_clear(mpcoeffb[i]);
		for (int i=0; i<m; i++)
				mpfr_clear(mpcoeffa[i]);
	};



	
	void FixIIR::emulate(TestCase * tc){

		mpz_class sx;

		sx = tc->getInputValue("X"); 		// get the input bit vector as an integer
		xHistory[currentIndexB] = sx;

		mpfr_t x, t, u, s;
		mpfr_init2 (x, 1-lsbOut);
		mpfr_init2 (t, hugePrec);
		mpfr_init2 (u, hugePrec);
		mpfr_init2 (s, hugePrec);

		mpfr_set_d(s, 0.0, GMP_RNDN); // initialize s to 0

		for (int i=0; i< n; i++)
		{
			sx = xHistory[(currentIndexB+n-i)%n];		// get the input bit vector as an integer
			sx = bitVectorToSigned(sx, 1-lsbOut); 						// convert it to a signed mpz_class
			mpfr_set_z (x, sx.get_mpz_t(), GMP_RNDD); 				// convert this integer to an MPFR; this rounding is exact
			mpfr_div_2si (x, x, -lsbOut, GMP_RNDD); 						// multiply this integer by 2^-p to obtain a fixed-point value; this rounding is again exact

			mpfr_mul(t, x, mpcoeffb[i], GMP_RNDN); 					// Here rounding possible, but precision used is ridiculously high so it won't matter

			if(coeffsignb[i]==1)
				mpfr_neg(t, t, GMP_RNDN);

			mpfr_add(s, s, t, GMP_RNDN); 							// same comment as above

		}

		for (int i=0; i<m; i++)
		{

			mpfr_mul(u, yHistory[(currentIndexA+m-i-1)%m], mpcoeffa[i], GMP_RNDN); 					// Here rounding possible, but precision used is ridiculously high so it won't matter

			if(coeffsigna[i]==1)
				mpfr_neg(u, u, GMP_RNDN);

			mpfr_add(s, s, u, GMP_RNDN); 							// same comment as above

		}

		mpfr_set(yHistory[currentIndexA], s, GMP_RNDN);


		// now we should have in s the (exact in most cases) sum
		// round it up and down

		// make s an integer -- no rounding here
		mpfr_mul_2si (s, s, -lsbOut, GMP_RNDN);


		// We are waiting until the first meaningful value comes out of the IIR

		mpz_class rdz, ruz;

		mpfr_get_z (rdz.get_mpz_t(), s, GMP_RNDD); 					// there can be a real rounding here
		rdz=signedToBitVector(rdz, wO);
		tc->addExpectedOutput ("R", rdz);

		mpfr_get_z (ruz.get_mpz_t(), s, GMP_RNDU); 					// there can be a real rounding here
		ruz=signedToBitVector(ruz, wO);
		tc->addExpectedOutput ("R", ruz);


		mpfr_clears (x, t, u, s, NULL);

		currentIndexB = (currentIndexB +1)%n; // We use a circular buffer to store the inputs
		currentIndexA = (currentIndexA +1)%m;


	};

	void FixIIR::buildStandardTestCases(TestCaseList* tcl){};

	OperatorPtr FixIIR::parseArguments(Target *target, vector<string> &args) {
		int msbOut;
		UserInterface::parseInt(args, "msbOut", &msbOut);
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
		
		return new FixIIR(target, lsbIn, msbOut, lsbOut, inputb, inputa, h);
	}


	void FixIIR::registerFactory(){
		UserInterface::add("FixIIR", // name
											 "A fix-point Infinite Impulse Response filter generator.",
											 "FiltersEtc", // categories
											 "",
											 "lsbIn(int): input most significant bit;\
											  msbOut(int): output most significant bit;\
                        lsbOut(int): output least significant bit;\
                        h(real)=0: worst-case peak gain: provide it only if WCPG is not installed;\
                        coeffa(string): colon-separated list of real coefficients using Sollya syntax. Example: coeff=\"1.234567890123:sin(3*pi/8)\";\
                        coeffb(string): colon-separated list of real coefficients using Sollya syntax. Example: coeff=\"1.234567890123:sin(3*pi/8)\"",
											 "",
											 FixIIR::parseArguments
											 ) ;
	}

}
