/*
  Euclidean division by a small constant

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2011.
  All rights reserved.

*/

// TODOs: remove from d its powers of two .

#include <iostream>
#include <fstream>
#include <sstream>

#include "IntConstDiv.hpp"


using namespace std;


namespace flopoco{


	// The classical table for the linear architecture
	
	IntConstDiv::EuclideanDivTable::EuclideanDivTable(Target* target, int d_, int alpha_, int gamma_) :
		Table(target, alpha_+gamma_, alpha_+gamma_, 0, -1, 1), d(d_), alpha(alpha_), gamma(gamma_) {
				ostringstream name;
				srcFileName="IntConstDiv::EuclideanDivTable";
				name <<"EuclideanDivTable_" << d << "_" << alpha ;
				setNameWithFreqAndUID(name.str());
	};

	mpz_class IntConstDiv::EuclideanDivTable::function(int x){
		// machine integer arithmetic should be safe here
		if (x<0 || x>=(1<<(alpha+gamma))){
			ostringstream e;
			e << "ERROR in IntConstDiv::EuclideanDivTable::function, argument out of range" <<endl;
			throw e.str();
		}

		int q = x/d;
		int r = x%d;
		return mpz_class((q<<gamma) + r);
	};


	
	// This table is full of zeroes but we don't care too much
	IntConstDiv::CBLKTable::CBLKTable(Target* target, int d, int alpha, int gamma, int level):
		// Sizes below assume alpha>=gamma
		Table(target, (level==0? alpha: 2*gamma), (level==0? alpha+gamma: (1<<level)*alpha+gamma), 0, -1, 1), d(d), alpha(alpha), gamma(gamma), level(level) {
				ostringstream name;
				srcFileName="IntConstDiv::CLBKTable";
				name <<"CLBKTable_" << d << "_" << alpha << "_l" << level ;
				setNameWithFreqAndUID(name.str());
	}
	
	mpz_class IntConstDiv::CBLKTable::function(int x){
		if(level==0){ // the input is one radix-2^alpha digit 
			mpz_class r = x % d;
			mpz_class q = x / d;
			return (q<<gamma) + r;
		}
		else{ // the input consists of two remainders
			int r0 = x & ((1<<gamma)-1);
			int r1 = x >> gamma;
			mpz_class y = mpz_class(r0) + (mpz_class(r1) << alpha*((1<<(level-1))) );
			mpz_class r = y % d;
			mpz_class q = y / d;
			// cout << getName() << "   x=" << x << "  r0=" << r0 << " r1=" << r1 << "  y=" << y << " r=" << r << "  q=" << q << endl;
			return (q<<gamma) + r;
		}
	}

	
	int IntConstDiv::quotientSize() {return qSize; };

	int IntConstDiv::remainderSize() {return gamma; };




	IntConstDiv::IntConstDiv(Target* target, int wIn_, int d_, int alpha_, int architecture_,  bool remainderOnly_, map<string, double> inputDelays)
		: Operator(target), d(d_), wIn(wIn_), alpha(alpha_), architecture(architecture_), remainderOnly(remainderOnly_)
	{
		setCopyrightString("F. de Dinechin (2011)");
		srcFileName="IntConstDiv";

		gamma = intlog2(d-1);
		if(alpha==-1){
			if(architecture==0) {
				alpha = target->lutInputs()-gamma;
				if (alpha<1) {
					REPORT(LIST, "WARNING: This value of d is too large for the LUTs of this FPGA (alpha="<<alpha<<").");
					REPORT(LIST, " Building an architecture nevertheless, but it may be very large and inefficient.");
					alpha=1;
				}
			}
			else {
				alpha=4; // TODO
			}
		}
		REPORT(INFO, "alpha="<<alpha);

		if((d&1)==0)
			REPORT(LIST, "WARNING, d=" << d << " is even, this is suspiscious. Might work nevertheless, but surely suboptimal.")

		/* Generate unique name */

		std::ostringstream o;
		if(remainderOnly)
			o << "IntConstRem_";
		else
			o << "IntConstDiv_";
		o << d << "_" << wIn << "_"  << alpha ;

		setNameWithFreqAndUID(o.str());

		qSize = wIn - intlog2(d) +1;


		addInput("X", wIn);

		if(!remainderOnly)
			addOutput("Q", qSize);
		addOutput("R", gamma);

		int k = wIn/alpha;
		int rem = wIn-k*alpha;
		if (rem!=0) k++;

		REPORT(INFO, "Architecture splits the input in k=" << k  <<  " chunks."   );
		REPORT(DEBUG, "  d=" << d << "  wIn=" << wIn << "  alpha=" << alpha << "  gamma=" << gamma <<  "  k=" << k  <<  "  qSize=" << qSize );

		if(architecture==0) {
			//////////////////////////////////////// Linear architecture //////////////////////////////////:
			EuclideanDivTable* table;
			table = new EuclideanDivTable(target, d, alpha, gamma);
			useSoftRAM(table);
			addSubComponent(table);
			double tableDelay=table->getOutputDelay("Y");
			
			string ri, xi, ini, outi, qi;
			ri = join("r", k);
			vhdl << tab << declare(ri, gamma) << " <= " << zg(gamma, 0) << ";" << endl;

			setCriticalPath( getMaxInputDelays(inputDelays) );


			for (int i=k-1; i>=0; i--) {

				manageCriticalPath(tableDelay);

				//			cerr << i << endl;
				xi = join("x", i);
				if(i==k-1 && rem!=0) // at the MSB, pad with 0es
					vhdl << tab << declare(xi, alpha, true) << " <= " << zg(alpha-rem, 0) <<  " & X" << range(wIn-1, i*alpha) << ";" << endl;
				else // normal case
					vhdl << tab << declare(xi, alpha, true) << " <= " << "X" << range((i+1)*alpha-1, i*alpha) << ";" << endl;
				ini = join("in", i);
				vhdl << tab << declare(ini, alpha+gamma) << " <= " << ri << " & " << xi << ";" << endl; // This ri is r_{i+1}
				outi = join("out", i);
				outPortMap(table, "Y", outi);
				inPortMap(table, "X", ini);


				vhdl << instance(table, join("table",i));
				ri = join("r", i);
				qi = join("q", i);
				vhdl << tab << declare(qi, alpha, true) << " <= " << outi << range(alpha+gamma-1, gamma) << ";" << endl;
				vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl;
			}


			if(!remainderOnly) { // build the quotient output
				vhdl << tab << declare("tempQ", k*alpha) << " <= " ;
				for (unsigned int i=k-1; i>=1; i--)
					vhdl << "q" << i << " & ";
				vhdl << "q0 ;" << endl;
				vhdl << tab << "Q <= tempQ" << range(qSize-1, 0)  << ";" << endl;
			}

			vhdl << tab << "R <= " << ri << ";" << endl; // This ri is r_0
		}




		else if (architecture==1){
			//////////////////////////////////////// Logarithmic architecture //////////////////////////////////:

			int levels = intlog2(k);
			REPORT(INFO, "levels=" << levels);
			CBLKTable* table;
			string ri, xi, ini, outi, qi, qs, r;

			// level 0
			table = new CBLKTable(target, d, alpha, gamma,0);
			useSoftRAM(table);
			addSubComponent(table);
			double tableDelay=table->getOutputDelay("Y");
			
			for (int i=0; i<k; i++) {
				//			cerr << i << endl;
				xi = join("x", i);
				if(i==k-1 && rem!=0) // at the MSB, pad with 0es
					vhdl << tab << declare(xi, alpha, true) << " <= " << zg(alpha-rem, 0) <<  " & X" << range(wIn-1, i*alpha) << ";" << endl;
				else // normal case
					vhdl << tab << declare(xi, alpha, true) << " <= " << "X" << range((i+1)*alpha-1, i*alpha) << ";" << endl;
				outi = join("out", i);

				outPortMap(table, "Y", outi);
				inPortMap(table, "X", xi);
				vhdl << instance(table, join("table",i));
				ri = join("r_l0_", i);
				qi = join("qs_l0_", i);
				vhdl << tab << declare(qi, alpha, true) << " <= " << outi << range(alpha+gamma-1, gamma) << ";" << endl;
				vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl;
			}


			// The following levels
			for (int level=1; level<levels; level++){
				REPORT(INFO, "level=" << level);
				table = new CBLKTable(target, d, alpha, gamma, level);
				useSoftRAM(table);
				addSubComponent(table);
				int levelSize = k/(1<<level);
				int padSize=0;
				if(k%(1<<level) !=0) {
					levelSize++;
					padSize = levelSize*((1<<level)) -k;
				}
				for(int i=0; i<levelSize; i++) {
					REPORT(INFO, "level=" << level << "  i=" << i)
					string tableNumber = "l" + to_string(level) + "_" + to_string(i);
					string tableName = "table_" + tableNumber;
					string in = "in_" + tableNumber;
					string out = "out_" + tableNumber;
				  r = "r_"+ tableNumber;
					string q = "q_"+ tableNumber;

					vhdl << tab << declare(in, 2*gamma) << " <= " << "r_l" << level-1 << "_" << 2*i+1 << " & r_l" << level-1 << "_" << 2*i  << ";"  << endl;
					
					outPortMap(table, "Y", out);
					inPortMap(table, "X", in);
					vhdl << instance(table, "table_"+ tableNumber);
					
					vhdl << tab << declare(r, gamma) << " <= " << out << range (gamma-1, 0) << ";"  << endl;
					vhdl << tab << declare(q, (1<<level)*alpha) << " <= " << out << range ((1<<level)*alpha+gamma-1, gamma) << ";"  << endl;
					
					qs = "qs_"+ tableNumber;
					string qsl =  "qs_l" + to_string(level-1) + "_" + to_string(2*i+1);
					string qsr =  "qs_l" + to_string(level-1) + "_" + to_string(2*i);
					vhdl << tab << declare(qs, (1<<level)*alpha) << " <= " << q + " + (" <<  qsl << " & " << qsr << ");  -- partial quotient so far"  << endl;
					}

				
			}

			if(!remainderOnly) { // build the quotient output
				vhdl << tab << "Q <= " << qs  << range(qSize-1, 0) << ";" << endl;
			}

			vhdl << tab << "R <= " << r << ";" << endl;


		}

		
		else{
			THROWERROR("arch=" << architecture << " not supported");
		}
		
	}

	IntConstDiv::~IntConstDiv()
	{
	}





	
	void IntConstDiv::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class X = tc->getInputValue("X");
		/* Compute correct value */
		mpz_class Q = X/d;
		mpz_class R = X-Q*d;
		if(!remainderOnly)
			tc->addExpectedOutput("Q", Q);
		tc->addExpectedOutput("R", R);
	}




	
	OperatorPtr IntConstDiv::parseArgumentsDiv(Target *target, vector<string> &args) {
		int wIn, d, arch, alpha;
		bool remainderOnly;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn); 
		UserInterface::parseStrictlyPositiveInt(args, "d", &d);
		UserInterface::parseInt(args, "alpha", &alpha);
		UserInterface::parsePositiveInt(args, "arch", &arch);
		UserInterface::parseBoolean(args, "remainderOnly", &remainderOnly);
		return new IntConstDiv(target, wIn, d, alpha, arch, remainderOnly);
	}

	void IntConstDiv::registerFactory(){
		UserInterface::add("IntConstDiv", // name
											 "Integer divider by a small constant.",
											 "ConstMultDiv",
											 "", // seeAlso
											 "wIn(int): input size in bits; \
                        d(int): small integer to divide by;  \
                        arch(int)=0: architecture used -- 0 for linear-time, 1 for log-time; \
                        remainderOnly(bool)=false: if true, the architecture doesn't output the quotient; \
                        alpha(int)=-1: Algorithm uses radix 2^alpha. -1 choses a sensible default.",
											 "This operator is described in <a href=\"bib/flopoco.html#dedinechin:2012:ensl-00642145:1\">this article</a>.",
											 IntConstDiv::parseArgumentsDiv
											 ) ;
	}

	
}
