#include <iostream>
#include <sstream>
#include <iomanip>

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


/* Test with 
./flopoco FixIIR coeffb="1:2" coeffa="1/2:1/4" lsbIn=-12 lsbOut=-12   TestBench n=1000


The example with poles close to 1:
./flopoco generateFigures=1 FixIIR coeffb="0x1.89ff611d6f472p-13:-0x1.2778afe6e1ac0p-11:0x1.89f1af73859fap-12:0x1.89f1af73859fap-12:-0x1.2778afe6e1ac0p-11:0x1.89ff611d6f472p-13" coeffa="-0x1.3f4f52485fe49p+2:0x1.3e9f8e35c8ca8p+3:-0x1.3df0b27610157p+3:0x1.3d42bdb9d2329p+2:-0x1.fa89178710a2bp-1" lsbIn=-12 lsbOut=-12 TestBench n=100
Remarque: H prend du temps à calculer sur cet exemple.

A small Butterworth
 ./flopoco generateFigures=1 FixIIR coeffb="0x1.7bdf4656ab602p-9:0x1.1ce774c100882p-7:0x1.1ce774c100882p-7:0x1.7bdf4656ab602p-9" coeffa="-0x1.2fe25628eb285p+1:0x1.edea40cd1955ep+0:-0x1.106c2ec3d0af8p-1" lsbIn=-12 lsbOut=-12 TestBench n=100

*/

using namespace std;

namespace flopoco {

	FixIIR::FixIIR(Target* target, int lsbIn_, int lsbOut_,  vector<string> coeffb_, vector<string> coeffa_, double H_, double Heps_) :
		Operator(target), lsbIn(lsbIn_), lsbOut(lsbOut_), coeffb(coeffb_), coeffa(coeffa_), H(H_), Heps(Heps_)
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
		yHistory  = (mpfr_t*) malloc((m+2) * sizeof(mpfr_t)); // We need to memorize the m previous y, and the current output. Plus one because it helps debugging


		for (int i=0; i< n; i++)		{
			// parse the coeffs from the string, with Sollya parsing
			sollya_obj_t node;

			node = sollya_lib_parse_string(coeffb[i].c_str());
			if(node == 0)					// If conversion did not succeed (i.e. parse error)
				THROWERROR("Unable to parse string " << coeffb[i] << " as a numeric constant");

			mpfr_init2(coeffb_mp[i], 10000);
			sollya_lib_get_constant(coeffb_mp[i], node);
			coeffb_d[i] = mpfr_get_d(coeffb_mp[i], GMP_RNDN);
			REPORT(DETAILED, "b[" << i << "]=" << setprecision(15) << scientific  << coeffb_d[i]);			
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
			REPORT(DETAILED, "a[" << i << "]=" << scientific << setprecision(15) << coeffa_d[i]);			
		}

		
		// TODO here compute H if it is not provided
		if(H==0 && Heps==0) {
#if HAVE_WCPG

			REPORT(INFO, "H not provided: computing worst-case peak gain");

			if (!WCPG_tf(&H, coeffb_d, coeffa_d, n, m, (int)0))
				THROWERROR("Could not compute WCPG");
			REPORT(INFO, "Worst-case peak gain is H=" << H);

			double one_d[1] = {1.0}; 
			if (!WCPG_tf(&Heps, one_d, coeffa_d, 1, m, (int)0))
				THROWERROR("Could not compute WCPG");
			REPORT(INFO, "Error amplification worst-case peak gain is Heps=" << Heps);
			
#else 
			THROWERROR("WCPG was not found (see cmake output), cannot compute worst-case peak gain H. Either provide H, or compile FloPoCo with WCPG");
#endif
		}
		else {
			REPORT(INFO, "H=" << H);
		}
		
		// guard bits for a faithful result
		g= intlog2(2*Heps*(n+m)); // see the paper
		int lsbExt = lsbOut-g;
		msbOut = ceil(log2(H)); // see the paper
		REPORT(INFO, "g=" << g << " so we ask for a SOPC faithful to lsbExt=" << lsbExt);
		REPORT(INFO, "msbOut=" << msbOut);


		// Initialisations for the emulate
		hugePrec = 10*(1+msbOut+-lsbOut+g);
		currentIndex=0x0FFFFFFFFFFFFFFFUL; // it will be decremented, let's start from high

		for (int i = 0; i<m+2; i++)
		{
			mpfr_init2 (yHistory[i], hugePrec);
			mpfr_set_d(yHistory[i], 0.0, GMP_RNDN);
		}
		for(int i=0; i<n; i++) {
			mpfr_init2 (xHistory[i], hugePrec);
			mpfr_set_d(xHistory[i], 0.0, GMP_RNDN);
		}

		setSequential();
		getTarget()->setPipelined(true);
		// The shift registers
		ShiftReg *inputShiftReg = new ShiftReg(getTarget(), 1-lsbIn, n);
		addSubComponent(inputShiftReg);
		inPortMap(inputShiftReg, "X", "X");
		for(int i = 0; i<n; i++) {
			outPortMap(inputShiftReg, join("Xd", i), join("U", i));
		}
		vhdl << instance(inputShiftReg, "inputShiftReg");
		setCycle(0);

		vhdl << tab << declare("YinternalLoopback", msbOut-lsbExt+1) << " <= Yinternal;" << endl; // just so that the inportmap finds it  
		ShiftReg *outputShiftReg = new ShiftReg(getTarget(), msbOut-lsbExt+1, m+1);
		addSubComponent(outputShiftReg);
		inPortMap(outputShiftReg, "X", "YinternalLoopback");
		for(int i = 0; i<m; i++) {
			outPortMap(outputShiftReg, join("Xd", i+1), join("Y", i));
		}
		vhdl << instance(outputShiftReg, "outputShiftReg");
		setCycle(0);
		
		// Now building a single SOPC. For this we need the following info:
		//		FixSOPC(Target* target, vector<double> maxX, vector<int> lsbIn, int msbOut, int lsbOut, vector<string> coeff_, int g=-1);
		// We will concatenate coeffb of size n then then coeffa of size m
		
		// MaxX

		setCombinatorial();
		getTarget()->setPipelined(false);
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
			lsbInSOPC.push_back(lsbExt); // for y
			coeffSOPC.push_back("-("+coeffa[i]+")");
		}
		
		FixSOPC* fixSOPC = new FixSOPC(getTarget(), maxInSOPC, lsbInSOPC, msbOut, lsbExt, coeffSOPC, -1); // -1 means: faithful
		addSubComponent(fixSOPC);
		for(int i=0; i<n; i++) {
			inPortMap(fixSOPC, join("X",i), join("U", i));
		}
		for (int i = 0; i<m; i++)	{
			inPortMap(fixSOPC, join("X",i+n), join("Y", i));
		}
		outPortMap(fixSOPC, "R", "Yinternal");

		vhdl << instance(fixSOPC, "fixSOPC");
		//		syncCycleFromSignal("Yinternal");
		
		setSequential();
		getTarget()->setPipelined(true);
		setCycle(0);

		//The final rounding must be computed with an addition, no escaping it

		vhdl << tab << declare("Yrounded", msbOut-lsbOut+2) <<  " <= (Yinternal" << range(msbOut-lsbOut+g, g-1) << ")  +  (" << zg(msbOut-lsbOut+1)  << " & \"1\" );" << endl;

		addOutput("R", msbOut - lsbOut + 1,   true);
		vhdl << "R <= Yrounded" << range(msbOut-lsbOut+1, 1) << ";" << endl;



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
		for (int i=0; i< n; i++)		{
			mpfr_mul(t, xHistory[(currentIndex+i)%n], coeffb_mp[i], GMP_RNDZ); 					// Here rounding possible, but precision used is ridiculously high so it won't matter
			mpfr_add(s, s, t, GMP_RNDN); 							// same comment as above
		}

		for (int i=0; i<m; i++)		{
			mpfr_mul(t, yHistory[(currentIndex +i+1)%(m+2)], coeffa_mp[i], GMP_RNDZ); 					// Here rounding possible, but precision used is ridiculously high so it won't matter
			mpfr_sub(s, s, t, GMP_RNDZ); 							// same comment as above
		}
		mpfr_set(yHistory[(currentIndex  +0)%(m+2)], s, GMP_RNDN);

#if 1 // debugging the emulate
		cout << "x=" << 	mpfr_get_d(xHistory[currentIndex % n], GMP_RNDN);
		cout << " //// y=" << 	mpfr_get_d(s,GMP_RNDN) << "  ////// ";
		for (int i=0; i< n; i++)		{
			cout << "  x" << i<< "c" << i<<  "=" <<
				mpfr_get_d(xHistory[(currentIndex+i)%n],GMP_RNDN) << "*" << mpfr_get_d(coeffb_mp[i],GMP_RNDN);
		}
		cout << "  // ";
		for (int i=0; i<m; i++) {
			cout <<"  ya" << i+1 << "=" <<
				mpfr_get_d(yHistory[(currentIndex +i+1)%(m+2)],GMP_RNDN) << "*" << mpfr_get_d(coeffa_mp[i],GMP_RNDN);
		}
		cout << endl;
		  
#endif

		currentIndex--;

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
#if 0 // to unplug the conversion that fails to see if it diverges further
		rdz=signedToBitVector(rdz, wO);
		tc->addExpectedOutput ("R", rdz);

		mpfr_get_z (ruz.get_mpz_t(), s, GMP_RNDU); 					// there can be a real rounding here
		ruz=signedToBitVector(ruz, wO);
		tc->addExpectedOutput ("R", ruz);
#endif
#if 0 // debug: with this we observe if the simulation diverges
		double d =  mpfr_get_d(s, GMP_RNDD);
		cout << "log2(|y|)=" << (ceil(log2(abs(d)))) << endl;
#endif
		
		mpfr_clears (x, t, s, NULL);

	};



	
	void FixIIR::buildStandardTestCases(TestCaseList* tcl){
		// First fill with a few ones, then a few zeroes
		TestCase *tc;

		
		for (int i=0; i<3; i++) {
			tc = new TestCase(this);
			tc->addInput("X", mpz_class(1)<<(-lsbIn-1)); // 0.5
			emulate(tc);
			tcl->add(tc);
		}
		
		for (int i=0; i<3; i++) {
			tc = new TestCase(this);
			tc->addInput("X", mpz_class(0)); // 0
			emulate(tc);
			tcl->add(tc);
		}		

	};


	
	OperatorPtr FixIIR::parseArguments(Target *target, vector<string> &args) {
		int lsbIn;
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		int lsbOut;
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		double h;
		UserInterface::parseFloat(args, "H", &h);
		double heps;
		UserInterface::parseFloat(args, "Heps", &heps);
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
		
		return new FixIIR(target, lsbIn, lsbOut, inputb, inputa, h, heps);
	}


	void FixIIR::registerFactory(){
		UserInterface::add("FixIIR", // name
											 "A fix-point Infinite Impulse Response filter generator.",
											 "FiltersEtc", // categories
											 "",
											 "lsbIn(int): input most significant bit;\
                        lsbOut(int): output least significant bit;\
                        H(real)=0: worst-case peak gain. if 0, it will be computed by the WCPG library;\
                        Heps(real)=0: worst-case peak gain of the feedback loop. if 0, it will be computed by the WCPG library;\
                        coeffa(string): colon-separated list of real coefficients using Sollya syntax. Example: coeffa=\"1.234567890123:sin(3*pi/8)\";\
                        coeffb(string): colon-separated list of real coefficients using Sollya syntax. Example: coeffb=\"1.234567890123:sin(3*pi/8)\"",
											 "",
											 FixIIR::parseArguments
											 ) ;
	}

}
