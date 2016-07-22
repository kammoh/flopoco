/*
  An FP logarithm for FloPoCo

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.

*/

// TODO List:
//  * test cases for boundary cases pfinal etc
//  * finetune pipeline
//  * Port back the Arith paper
#include <fstream>
#include <sstream>
#include <math.h>	// for NaN
#include "FPLog.hpp"
#include "ShiftersEtc/LZOC.hpp"
#include "ShiftersEtc/LZOCShifterSticky.hpp"
#include "ShiftersEtc/Shifters.hpp"
#include "Table.hpp"


using namespace std;


namespace flopoco{




	FPLog::FirstInvTable::FirstInvTable(Target* target, int wIn, int wOut) :
		Table(target, wIn, wOut)
	{
		ostringstream name;
		name <<"InvTable_0_"<<wIn<<"_"<<wOut;
		setNameWithFreqAndUID(name.str());

	}

	FPLog::FirstInvTable::~FirstInvTable() {}


	int    FPLog::FirstInvTable::double2input(double x){
		int result;
		cerr << "??? FPLog::FirstInvTable::double2input not yet implemented ";
		exit(1);
		return result;
	}


	double FPLog::FirstInvTable::input2double(int x) {
		double y;
		if(x>>(wIn-1)) //MSB of input
			y= ((double)(x+(1<<wIn))) //      11xxxx (binary)
				/  ((double)(1<<(wIn+1))); // 0.11xxxxx (binary)
		else
			y= ((double)(x+(1<<wIn))) //   10xxxx (binary)
				/  ((double)(1<<(wIn))); // 1.0xxxxx (binary)
		return(y);
	}

	mpz_class FPLog::FirstInvTable::double2output(double x){
		mpz_class result;
		result =  mpz_class(floor(x*((double)(1<<(wOut-1)))));
		return result;
	}

	double FPLog::FirstInvTable::output2double(mpz_class x) {
		double y=((double)x.get_si()) /  ((double)(1<<(wOut-1)));
		return(y);
	}


	mpz_class FPLog::FirstInvTable::function(int x)
	{
		double d;
		mpz_class r;
		d=input2double(x);
		r =  ceil( ((double)(1<<(wOut-1))) / d); // double rounding, but who cares really
		// The following line allows us to prove that the case a0=5 is lucky enough to need one bit less than the general case
		//cout << "FirstInvTable> x=" << x<< "  r=" <<r << "  error=" << ceil( ((double)(1<<(wOut-1))) / d)  - ( ((double)(1<<(wOut-1))) / d) << endl;
		return r;
	}



#if 0
	int FPLog::FirstInvTable::check_accuracy(int wF) {
		int i;
		mpz_class j;
		double x1,x2,y,e1,e2;
		double maxerror=0.0;
		double prod=0.0;

		maxMulOut=0;
		minMulOut=2;

		for (i=minIn; i<=maxIn; i++) {
			// x1 and x2 are respectively the smallest and largest FP possible
			// values leading to input i
			x1=input2double(i);
			if(i>>(wIn-1)) //MSB of input
				x2= - negPowOf2(wF)          // <--wF --->
					+ ((double)(i+1+(1<<wIn))) //   11 11...11 (binary)
					/ ((double)(1<<(wIn+1))); // 0.11 11...11 (binary)
			else
				x2= - negPowOf2(wF-1)
					+ ((double)(i+1+(1<<wIn))) //  10 11...11 (binary)
					/ ((double)(1<<(wIn))); // 1.0 11...11 (binary)
			j=function(i);
			y=output2double(j);
			if(UserInterface::verbose)
				cout << "i="<<i<< " ("<<input2double(i)<<") j="<<j
					  <<" min="<< x1*y <<" max="<< x2*y<< endl;
			prod=x1*y; if (prod<minMulOut) minMulOut=prod;
			prod=x2*y; if (prod>maxMulOut) maxMulOut=prod;
			e1= fabs(x1*y-1); if (e1>maxerror) maxerror=e1;
			e2= fabs(x2*y-1); if (e2>maxerror) maxerror=e2;
		}
		cout << "FirstInvTable: Max error=" <<maxerror << "  log2=" << log2(maxerror) <<endl;
		cout << "               minMulOut=" <<minMulOut << " maxMulOut=" <<maxMulOut  <<endl;

		printf("%1.30e\n", log2(maxerror));

		return (int) (ceil(log2(maxerror)));
	}

#endif







	FPLog::FirstLogTable::FirstLogTable(Target *target, int wIn, int wOut, FirstInvTable* fit, FPLog* op_) :
		Table(target, wIn, wOut), fit(fit), op(op_)
	{
		ostringstream name;
		name <<"LogTable_0_"<<wIn<<"_"<<wOut;
		setNameWithFreqAndUID(name.str());

		minIn = 0;
		maxIn = (1<<wIn) -1;
		if (wIn!=fit->wIn) {
			cerr<< "FPLog::FirstLogTable::FirstLogTable should use same wIn as FirstInvTable"<<endl;
			exit(1);
		}
	}

	FPLog::FirstLogTable::~FirstLogTable() {}


	int    FPLog::FirstLogTable::double2input(double x){
		int result;
		cerr << "??? FPLog::FirstLogTable::double2input not yet implemented ";
		exit(1);
		//   result = (int) floor(x*((double)(1<<(wIn-1))));
		//   if( result < minIn || result > maxIn) {
		//     cerr << "??? FPLog::FirstLogTable::double2input:  Input "<< result <<" out of range ["<<minIn<<","<<maxIn<<"]";
		//     exit(1);
		//  }
		return result;
	}

	double FPLog::FirstLogTable::input2double(int x) {
		return(fit->input2double(x));
	}

	mpz_class    FPLog::FirstLogTable::double2output(double y){
		//Here y is between -0.5 and 0.5 strictly, whatever wIn.  Therefore
		//we multiply y by 2 before storing it, so the table actually holds
		//2*log(1/m)
		double z = floor(2*y*((double)(1<<(wOut-1))));

		// otherwise, signed arithmetic on wOut bits
		if(z>=0)
			return (mpz_class) z;
		else
			return (z + (double)(1<<wOut));

	}

	double FPLog::FirstLogTable::output2double(mpz_class x) {
		cerr<<" FPLog::FirstLogTable::output2double TODO"; exit(1);
		//  double y=((double)x) /  ((double)(1<<(wOut-1)));
		//return(y);
	}


	mpz_class FPLog::FirstLogTable::function(int x)
	{
		mpz_class result;
		double apprinv;
		mpfr_t i,l;
		mpz_t r;

		mpfr_init(i);
		mpfr_init2(l,wOut);
		mpz_init2(r,400);
		apprinv = fit->output2double(fit->function(x));;
		// result = double2output(log(apprinv));
		mpfr_set_d(i, apprinv, GMP_RNDN);
		mpfr_log(l, i, GMP_RNDN);
		mpfr_neg(l, l, GMP_RNDN);

		// code the log in 2's compliment
		mpfr_mul_2si(l, l, wOut, GMP_RNDN);
		mpfr_get_z(r, l, GMP_RNDN);
		result = mpz_class(r); // signed

		// This is a very inefficient way of converting
		mpz_class t = mpz_class(1) << wOut;
		result = t+result;
		if(result>t) result-=t;

		//  cout << "x="<<x<<" apprinv="<<apprinv<<" logapprinv="<<log(apprinv)<<" result="<<result<<endl;
		mpfr_clear(i);
		mpfr_clear(l);
		mpz_clear(r);
		return  result;
	}



	

	FPLog::FPLog(Target* target,
	             int wE, int wF)
		: Operator(target), wE(wE), wF(wF)
	{

		setCopyrightString("F. de Dinechin");

		ostringstream o;
		srcFileName = "FPLog";

		o << "FPLog_" << wE << "_" << wF;
		setNameWithFreqAndUID(o.str());

		addFPInput("X", wE, wF);
		addFPOutput("R", wE, wF, 2); // 2 because faithfully rounded

		int gLog=3; // TODO
		int alpha=6;

		
		addConstant("g",  "positive",          gLog);
		addConstant("wE", "positive",          wE);
		addConstant("wF", "positive",          wF);
		vhdl << tab << declare("XExnSgn", 3) << " <=  X(wE+wF+2 downto wE+wF);" << endl;
		vhdl << tab << declare("FirstBit") << " <=  X(wF-1);" << endl;
		vhdl << tab << declare("E", wE) << " <= (X(wE+wF-1 downto wF)) - (\"0\" & (wE-2 downto 1 => '1') & (not FirstBit));" << endl;
		vhdl << tab << declare("Y0", wF+2) << " <= \"1\" & X(wF-1 downto 0) & \"0\" when FirstBit = '0' else \"01\" & X(wF-1 downto 0);" << endl;
		vhdl << tab << declare("A", alpha) << " <= Y0" << range(wF+1, wF+1-alpha+1) << " ;" << endl;
		vhdl << tab << declare("Y", wF+2-alpha) << " <= Y0" << range(wF+1-alpha, 0) << " ;" << endl;
		int sizeY=wF+2-alpha;

		//		vhdl << tab << declare("Y0h", wF) << " <= Y0(wF downto 1);" << endl;
		vhdl << tab << "-- Sign of the result;" << endl;
		vhdl << tab << 	declare("sR") << " <= '0'   when  (X(wE+wF-1 downto wF) = ('0' & (wE-2 downto 0 => '1')))  -- binade [1..2)" << endl
		                              << "     else not X(wE+wF-1);                -- MSB of exponent" << endl;

		vhdl << tab << declare("absE", wE) << " <= ((wE-1 downto 0 => '0') - E)   when sR = '1' else E;" << endl;
		vhdl << tab << declare("EeqZero") << " <= '1' when E=(wE-1 downto 0 => '0') else '0';" << endl;


	}




	
	FPLog::~FPLog()
	{
	}

	void FPLog::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");

		/* Compute correct value */
		FPNumber fpx(wE, wF);
		fpx = svX;

		mpfr_t x, ru,rd;
		mpfr_init2(x,  1+wF);
		mpfr_init2(ru, 1+wF);
		mpfr_init2(rd, 1+wF);
		fpx.getMPFR(x);
		mpfr_log(rd, x, GMP_RNDD);
		mpfr_log(ru, x, GMP_RNDU);
#if 0
		mpfr_out_str (stderr, 10, 30, x, GMP_RNDN); cerr << " ";
		mpfr_out_str (stderr, 10, 30, rd, GMP_RNDN); cerr << " ";
		mpfr_out_str (stderr, 10, 30, ru, GMP_RNDN); cerr << " ";
		cerr << endl;
#endif
		FPNumber  fprd(wE, wF, rd);
		FPNumber  fpru(wE, wF, ru);
		mpz_class svRD = fprd.getSignalValue();
		mpz_class svRU = fpru.getSignalValue();
		tc->addExpectedOutput("R", svRD);
		tc->addExpectedOutput("R", svRU);
		mpfr_clears(x, ru, rd, NULL);
	}


	// TEST FUNCTIONS


	void FPLog::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;
		mpz_class x;


		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addComment("1.0");
		emulate(tc);
		tcl->add(tc);



	}



	// One test out of 8 fully random (tests NaNs etc)
	// All the remaining ones test positive numbers.
	// with special treatment for exponents 0 and -1,
	// and for the range reduction worst case.

	TestCase* FPLog::buildRandomTestCase(int i){

		TestCase *tc;
		mpz_class a;
		mpz_class normalExn = mpz_class(1)<<(wE+wF+1);

		tc = new TestCase(this);
		/* Fill inputs */
		if ((i & 7) == 0)
			a = getLargeRandom(wE+wF+3);
		else if ((i & 7) == 1) // exponent of 1
			a  = getLargeRandom(wF) + ((((mpz_class(1)<<(wE-1))-1)) << wF) + normalExn;
		else if ((i & 7) == 2) // exponent of 0.5
			a  = getLargeRandom(wF) + ((((mpz_class(1)<<(wE-1))-2)) << wF) + normalExn;
		else
			a  = getLargeRandom(wE+wF)  + normalExn; // 010xxxxxx

		tc->addInput("X", a);
		/* Get correct outputs */
		emulate(tc);
		// add to the test case list
		return tc;
	}

		OperatorPtr FPLog::parseArguments(Target *target, vector<string> &args) {
		int wE;
		UserInterface::parseStrictlyPositiveInt(args, "wE", &wE);
		int wF;
		UserInterface::parseStrictlyPositiveInt(args, "wF", &wF);
		return new FPLog(target, wE, wF);
	}

	void FPLog::registerFactory(){
		UserInterface::add("FPLog", // name
											 "Floating-point logarithm using an iterative method.",
											 "ElementaryFunctions", // categories
											 "",
											 "wE(int): exponent size in bits; \
                        wF(int): mantissa size in bits;",
											 "For details on the technique used, ... no publication yet.",
											 FPLog::parseArguments,
											 FPLog::unitTest
											 ) ;

	}

	TestList FPLog::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int wF=5; wF<53; wF+=1) // test various input widths
			{ 
				int nbByteWE = 6+(wF/10);
				while(nbByteWE>wF)
				{
					nbByteWE -= 2;
				}

				paramList.push_back(make_pair("wF",to_string(wF)));
				paramList.push_back(make_pair("wE",to_string(nbByteWE)));
				testStateList.push_back(paramList);
				paramList.clear();
			}
		}
		else     
		{
				// finite number of random test computed out of index
		}	

		return testStateList;
	}

}
