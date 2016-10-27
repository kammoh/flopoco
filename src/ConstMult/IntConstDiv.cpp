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
#include <algorithm>

#include "IntConstDiv.hpp"


using namespace std;


namespace flopoco{

#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method

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



	IntConstDiv::CBLKTable::CBLKTable(Target* target, int level, int d, int alpha, int gamma, int rho):
		// Sizes below assume alpha>=gamma
  	Table(target, (level==0? alpha: 2*gamma), (level==0? rho+gamma: (1<<(level-1))*alpha+gamma), 0, -1, 1), level(level), d(d), alpha(alpha), gamma(gamma), rho(rho) {
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
		
#else

	vector<mpz_class>  IntConstDiv::euclideanDivTable(int d, int alpha, int gamma) { 		// machine integer arithmetic should be safe here
		vector<mpz_class>  result;
		for (int x=0; x<(d<<alpha); x++){
			int q = x/d;
			int r = x%d;
			result.push_back(mpz_class( (q<<gamma) + r) );
		}
		return result;
	}
		
	vector<mpz_class>  IntConstDiv::firstLevelCBLKTable( int d, int alpha, int gamma ) {
		vector<mpz_class>  result;
		for (int x=0; x<(1<<alpha); x++) {
			mpz_class r = x % d;
			mpz_class q = x / d;
			result.push_back( (q<<gamma) + r ) ;
		}
		return result;
	}
		
	vector<mpz_class>  IntConstDiv::otherLevelCBLKTable(int level, int d, int alpha, int gamma, int rho ) {
		/* input will be a group of 2^level alpha-bit digits*/
		vector<mpz_class>  result;
		for (int x=0; x<(1<<(2*gamma)); x++) {
			// the input consists of two remainders
			int r0 = x & ((1<<gamma)-1);
			int r1 = x >> gamma;
			mpz_class y = mpz_class(r0) + (mpz_class(r1) << alpha*((1<<(level-1))) );
			mpz_class r = y % d;
			mpz_class q = y / d;
			result.push_back( (q<<gamma) + r ) ;
		}
		return result;
	}		
#endif // deprecated overloading of Table method


		
	int IntConstDiv::quotientSize() {return qSize; };

	int IntConstDiv::remainderSize() {return gamma; };


	// This method mostly replicates the computations done in the constructor
	// l is the number of inputs to the LUT of the target FPGA
	int evaluateLUTCostOfLinearArch(int wIn, int d, int l) {
		int gamma = intlog2(d-1);
		int alpha = l-gamma;
		if (alpha<1)	alpha=1;
		int xDigits = wIn/alpha;
		int xPadBits = wIn%alpha;
		if (xPadBits!=0)
			xDigits++;
				
		int tableOutputSize = alpha + gamma;
		int lutsPerOutputBit = 1<<(alpha+gamma-l);
		int cost = tableOutputSize * lutsPerOutputBit * xDigits;
		// cout << "l=" << l << " k=" << alpha << " r=" << gamma << " xDigits=" << xDigits << endl; 
		return cost;
	}



	void IntConstDiv::generateVHDL(){

	}
		

	IntConstDiv::IntConstDiv(Target* target, int wIn_, vector<int> divisors, int alpha_, int architecture_,  bool remainderOnly_, map<string, double> inputDelays)
		: Operator(target),  wIn(wIn_), alpha(alpha_), architecture(architecture_), remainderOnly(remainderOnly_)
	{
		setCopyrightString("F. de Dinechin (2016)");
		srcFileName="IntConstDiv";
		d=1;
		for (auto i: divisors)
			d*=i;
		REPORT(INFO, "Composite division, d=" << d);

		gamma = intlog2(d-1);

		if(alpha==-1){
			if(architecture==0) {
				alpha = target->lutInputs()-gamma;
				if (alpha<1) {
					alpha=1;
				}
			}
			else {
				alpha = target->lutInputs();
			}
		}

		qSize = intlog2(  ((mpz_class(1)<<wIn)-1)/d  );
		std::ostringstream o;
		if(remainderOnly)
			o << "IntConstRem_";
		else
			o << "IntConstDiv_";
		o << d << "_" << wIn << "_"  << alpha ;
		setNameWithFreqAndUID(o.str());

		addInput("X", wIn);
				

		if(!remainderOnly)
			addOutput("Q", qSize);
		addOutput("R", gamma);

			int wInCurrent=wIn;
			int currentDivProd=1;
			int overallCostUp=0;
			for(unsigned int i=0; i<divisors.size(); i++) {
				cost=evaluateLUTCostOfLinearArch(wInCurrent, divisors[i], target->lutInputs());
				REPORT (INFO, "Dividing by " << divisors[i] << " for wIn=" << wInCurrent << " should cost about " << cost << " LUTs");
				currentDivProd *= divisors[i];
				wInCurrent = intlog2( ((mpz_class(1)<<wIn)-1) / currentDivProd );
				overallCostUp+=cost;
			}
			REPORT(INFO, "  Overall cost of composite division: " << overallCostUp << " LUTs");
#if 0
			wInCurrent=wIn;
			currentDivProd=1;
			int overallCostDown=0;
			for(int i=divisors.size()-1; i>=0; i--) {
				cost=evaluateLUTCostOfLinearArch(wInCurrent, divisors[i], target->lutInputs());
				REPORT (INFO, "Dividing by " << divisors[i] << " for wIn=" << wInCurrent << " costs " << cost);
				currentDivProd *= divisors[i];
				wInCurrent = intlog2( ((mpz_class(1)<<wIn)-1) / currentDivProd );
				overallCostDown+=cost;
			}
			REPORT(INFO, "  Overall cost of composite division, counting down: " << overallCostDown );

			// Now we reverse
			if(overallCostDown<overallCostUp) {
				REPORT(INFO, "Reversing the divisor list");
				reverse(divisors.begin(), divisors.end());
			}
#endif


			
			vhdl << tab << declare("Q0",wIn) << " <= X;" << endl;
			wInCurrent=wIn;
			unsigned int i;
			for(i=0; i<divisors.size(); i++) {
				ostringstream params, inportmap,outportmap;
				params << "wIn="<< wInCurrent << " d=" << divisors[i];
				inportmap << "X=>Q"<<i;
				outportmap << "Q=>Q"<<i+1<<",R=>R"<<i+1;
				newInstance("IntConstDiv", join("subDiv",i), params.str(), inportmap.str(), outportmap.str());

				// REPORT(INFO, join("subDiv",i) << " " <<  params.str() << "  " << inportmap.str() << "   " << outportmap.str());
				// Slight sub-optimality here, TODO: we can win one bit from time to time
				// wInCurrent = intlog2( ((mpz_class(1)<<wIn)-1) /divisors[i]);
				wInCurrent = getSignalByName(join("Q",i+1))->width();
			}
			// Now rebuild the remainder
			// for (3,5,7) R = R1 + 3*(R2+3*R3))
			// R = R1 + divisors[0] *(R2 +divisors[1]*R3) )
			
			// recurrence is  RR2=R3   RR_i-1 = R[i] + divisors[i-1]*RR_i    R=RR_0
			vhdl << tab << declare(join("RR",divisors.size()), getSignalByName(join("R",divisors.size()))->width()) << " <= " << join("R",divisors.size()) << ";" << endl;					
			currentDivProd=divisors[divisors.size()-1];
			for(i=divisors.size()-1; i>=1; i--) {
				//				currentDivProd *= divisors[i];
				// wInCurrent = getSignalByName(join("Q",i+1))->width();
				ostringstream multParams;
				multParams << "wIn=" << intlog2(currentDivProd) << " n=" << divisors[i-1];
				newInstance("IntConstMult", join("rMult",i), multParams.str(), "X=>"+join("RR",i+1), "R=>"+join("M",i));
				currentDivProd *= divisors[i-1];
				int sizeRR = intlog2(currentDivProd);
				int sizeM  = getSignalByName(join("M",i))->width();
				// int sizePreviousRR =  getSignalByName(join("RR",i+1))->width();
				vhdl << tab << declare(join("RR",i), sizeRR) << " <= ";
				//				if (sizePreviousRR<sizeRR)
				//	vhdl << zg(sizeRR-sizePreviousRR) << "&";
				vhdl << "R" << i << + " + M" << i
						 << (sizeRR<sizeM? range(intlog2(currentDivProd)-1,0)  : "")
						 <<  ";" << endl;
			}
			vhdl << tab << "Q <= Q" << divisors.size()<<range(qSize-1,0) << ";" << endl;
			vhdl << tab << "R <= RR1;" << endl;
			

			
	}



	
	IntConstDiv::IntConstDiv(Target* target, int wIn_, int d_, int alpha_, int architecture_,  bool remainderOnly_, map<string, double> inputDelays)
		: Operator(target), d(d_), wIn(wIn_), alpha(alpha_), architecture(architecture_), remainderOnly(remainderOnly_)
	{
		setCopyrightString("F. de Dinechin (2011)");
		srcFileName="IntConstDiv";

		if ((d&1)==0) {
			THROWERROR("Divisor is even! Please manage this outside IntConstDiv");
		}

		gamma = intlog2(d-1);

		if(gamma>4) {
			REPORT(LIST, "WARNING: This operator is efficient for small constants. " << d << " is quite large. Attempting anyway.");
		}

						
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
#if 0 // DEBUG IN HEXA
				alpha=4;
#else
				alpha = target->lutInputs();
#endif				
			}
		}
		

		qSize = intlog2(  ((mpz_class(1)<<wIn)-1)/d  );
		std::ostringstream o;
		if(remainderOnly)
			o << "IntConstRem_";
		else
			o << "IntConstDiv_";
		o << d << "_" << wIn << "_"  << alpha ;
		setNameWithFreqAndUID(o.str());

		addInput("X", wIn);
				
		if(!remainderOnly)
			addOutput("Q", qSize);
		addOutput("R", gamma);


		// First evaluate the cost of the atomic divider
		int cost0=evaluateLUTCostOfLinearArch(wIn, d, target->lutInputs());		  
		REPORT(INFO, "  Estimated cost: " << cost0 );
		
		vector<int> divisors;

		int divofd=3;
		int dd=d;
		while (dd>1){
			while(dd % divofd ==0){
				REPORT(INFO, "Found divisor: " << divofd);
				divisors.push_back(divofd);
				dd = dd/divofd;
			}
			divofd +=2;
		}

		
		if(divisors[0]!=d) { // which means that there is more than one
			REPORT(INFO, "This constant can be decomposed in smaller factors, consider doing it. I'm building an atomic divider anyway.");
		}



		rho = intlog2(  ((mpz_class(1)<<alpha)-1)/d  );

		REPORT(INFO, "alpha="<<alpha);
		REPORT(DEBUG, "gamma=" << gamma << " qSize=" << qSize << " rho=" << rho);

		if((d&1)==0)
			REPORT(LIST, "WARNING, d=" << d << " is even, this is suspicious. Might work nevertheless, but surely suboptimal.")


				int xDigits = wIn/alpha;
		int xPadBits = wIn%alpha;
		if (xPadBits!=0)
			xDigits++;
		
		int qDigits = qSize/alpha;
		if ( (qSize%alpha) !=0)
			qDigits++;

		
		REPORT(INFO, "Architecture splits the input in xDigits=" << xDigits  <<  " chunks."   );
		REPORT(DEBUG, "  d=" << d << "  wIn=" << wIn << "  alpha=" << alpha << "  gamma=" << gamma <<  "  xDigits=" << xDigits  <<  "  qSize=" << qSize );

		if(architecture==INTCONSTDIV_LINEAR_ARCHITECTURE) {
			//////////////////////////////////////// Linear architecture //////////////////////////////////:

#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
			EuclideanDivTable* table;
			table = new EuclideanDivTable(target, d, alpha, gamma);
			useSoftRAM(table);
			addSubComponent(table);
			double tableDelay=table->getOutputDelay("Y");
#else // new Table interface
			vector<mpz_class> tableContent = euclideanDivTable(d, alpha, gamma);
			Table* table = new Table(target, tableContent, alpha+gamma, alpha+gamma );
			table->setShared();
			table->setNameWithFreq("EuclideanDivTable_d" + to_string(d) + "_alpha"+ to_string(alpha));
#endif // deprecated overloading of Table method
			string ri, xi, ini, outi, qi;
			ri = join("r", xDigits);
			vhdl << tab << declare(ri, gamma) << " <= " << zg(gamma, 0) << ";" << endl;

			for (int i=xDigits-1; i>=0; i--) {
#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
				manageCriticalPath(tableDelay);
#endif
				//			cerr << i << endl;
				xi = join("x", i);
				if(i==xDigits-1 && xPadBits!=0) // at the MSB, pad with 0es
					vhdl << tab << declare(xi, alpha, true) << " <= " << zg(alpha-xPadBits, 0) <<  " & X" << range(wIn-1, i*alpha) << ";" << endl;
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
				vhdl << tab << declare("tempQ", xDigits*alpha) << " <= " ;
				for (unsigned int i=xDigits-1; i>=1; i--)
					vhdl << "q" << i << " & ";
				vhdl << "q0 ;" << endl;
				vhdl << tab << "Q <= tempQ" << range(qSize-1, 0)  << ";" << endl;
			}

			vhdl << tab << "R <= " << ri << ";" << endl; // This ri is r_0
		}



		// Bug on  ./flopoco IntConstDiv alpha=6 arch=1 d=3 remainderOnly=false wIn=31 TestBench n=1000
		// because qisize=0 below. The input is split into 6 chunks but Q fits on 30 bits and needs 5 chunks only
		else if (architecture==INTCONSTDIV_LOGARITHMIC_ARCHITECTURE){
			//////////////////////////////////////// Logarithmic architecture //////////////////////////////////:
			// The management of the tree is quite intricate when everything is not a power of two.
			
			// The number of levels is computed out of the number of digits of the _input_
			int levels = intlog2(2*xDigits-1); 
			REPORT(INFO, "levels=" << levels);
			string ri, xi, ini, outi, qi, qs, r;



			/// First level table
			
#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
			CBLKTable* table;

			// level 0
			table = new CBLKTable(target, 0, d, alpha, gamma, rho);
			useSoftRAM(table);
			addSubComponent(table);
#else  // deprecated overloading of Table method
			vector<mpz_class> tableContent = firstLevelCBLKTable(d, alpha, gamma);
			Table* table = new Table(target, tableContent, alpha, rho+gamma);
			table->setShared();
			table->setNameWithFreq("CBLKTable_l0_d"+ to_string(d) + "_alpha"+ to_string(alpha));
#endif  // deprecated overloading of Table method
			//			double tableDelay=table->getOutputDelay("Y");
			for (int i=0; i<xDigits; i++) {
				xi = join("x", i);
				if(i==xDigits-1 && xPadBits!=0) // at the MSB, pad with 0es
					vhdl << tab << declare(xi, alpha, true) << " <= " << zg(alpha-xPadBits, 0) <<  " & X" << range(wIn-1, i*alpha) << ";" << endl;
				else // normal case
					vhdl << tab << declare(xi, alpha, true) << " <= " << "X" << range((i+1)*alpha-1, i*alpha) << ";" << endl;
				outi = join("out", i);

				outPortMap(table, "Y", outi);
				inPortMap(table, "X", xi);
				vhdl << instance(table, join("table",i));
				ri = join("r_l0_", i);
				qi = join("qs_l0_", i);
				// The qi out of the table are on rho bits, and we want to pad them to alpha bits
				int qiSize;
				if(i<qDigits-1) {
					qiSize = alpha;
					vhdl << tab << declare(qi, qiSize, true) << " <= " << zg(qiSize -rho) << " & (" <<outi << range(rho+gamma-1, gamma) << ");" << endl;
				}
				else if(i==qDigits-1)  {
					qiSize = qSize - (qDigits-1)*alpha;
					REPORT(INFO, "-- qsize=" << qSize << " qisize=" << qiSize << "   rho=" << rho);
					if(qiSize>=rho)
						vhdl << tab << declare(qi, qiSize, true) << " <= " << zg(qiSize -rho) << " & (" <<outi << range(rho+gamma-1, gamma) << ");" << endl;
					else
						vhdl << tab << declare(qi, qiSize, true) << " <= " << outi << range(qiSize+gamma-1, gamma) << ";" << endl;
				}
				vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl;
			}

			bool previousLevelOdd = ((xDigits&1)==1);
			// The following levels
			for (int level=1; level<levels; level++){
				int rLevelSize = xDigits/(1<<level); // how many parts with remainder bits we have in this level
				if (xDigits%((1<<level)) !=0 )
					rLevelSize++;
				int qLevelSize = qDigits/(1<<level); // how many sub-quotients we have in this level
				if (qDigits%((1<<level)) !=0 )
					qLevelSize++;
				REPORT(INFO, "level=" << level << "  rLevelSize=" << rLevelSize << "  qLevelSize=" << qLevelSize);


#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
				table = new CBLKTable(target, level, d, alpha, gamma, rho);
				useSoftRAM(table);
				addSubComponent(table);

#else  // deprecated overloading of Table method
				vector<mpz_class> tableContent = otherLevelCBLKTable(level, d, alpha, gamma, rho);
				Table* table = new Table(target, tableContent,
																 2*gamma, /* wIn*/
																 (1<<(level-1))*alpha+gamma /*wOut*/  );
				table->setShared();
				table->setNameWithFreq("CBLKTable_l" + to_string(level) + "_d"+ to_string(d) + "_alpha"+ to_string(alpha));
#endif
				for(int i=0; i<rLevelSize; i++) {
					string tableNumber = "l" + to_string(level) + "_" + to_string(i);
					string tableName = "table_" + tableNumber;
					string in = "in_" + tableNumber;
					string out = "out_" + tableNumber;
					r = "r_"+ tableNumber;
					string q = "q_"+ tableNumber;
					string qsl =  "qs_l" + to_string(level-1) + "_" + to_string(2*i+1);
					string qsr =  "qs_l" + to_string(level-1) + "_" + to_string(2*i);
					qs = "qs_"+ tableNumber; // not declared here because we need it to exit the loop

					if(i==rLevelSize-1 && previousLevelOdd) 
						vhdl << tab << declare(in, 2*gamma) << " <= " << zg(gamma) << " & r_l" << level-1 << "_" << 2*i  << ";"  << endl;
					else
						vhdl << tab << declare(in, 2*gamma) << " <= " << "r_l" << level-1 << "_" << 2*i+1 << " & r_l" << level-1 << "_" << 2*i  << ";"  << endl;

					outPortMap(table, "Y", out);
					inPortMap(table, "X", in);
					vhdl << instance(table, "table_"+ tableNumber);

					/////////// The remainder
					vhdl << tab << declare(r, gamma) << " <= " << out << range (gamma-1, 0) << ";"  << endl;

					/////////// The quotient bits
					int subQSize; // The size, in bits, of the part of Q we are building
					if (i<qLevelSize-1) { // The simple chunks where we just assemble a full binary tree
						subQSize = (1<<level)*alpha;
						REPORT(INFO, "level=" << level << "  i=" << i << "  subQSize=" << subQSize << "  tableOut=" << table->wOut << " gamma=" << gamma );
						vhdl << tab << declare(q, subQSize) << " <= " << zg(subQSize - (table->wOut-gamma)) << " & " << out << range (table->wOut-1, gamma) << ";"  << endl;
						// TODO simplify the content of the zg above
#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
						vhdl << tab << declare(qs, subQSize) << " <= " << q << " + (" <<  qsl << " & " << qsr << ");  -- partial quotient so far"  << endl;
#else
						vhdl << tab << declare(getTarget()->adderDelay(subQSize), qs, subQSize) << " <= " << q << " + (" <<  qsl << " & " << qsr << ");  -- partial quotient so far"  << endl;
#endif
					}
					else if (i==qLevelSize-1){ // because i can reach qLevelSize when rlevelSize=qLevelSize+1, but then we have nothing to do
						// Lefttmost chunk
						subQSize = qSize-(qLevelSize-1)*(1<<level)*alpha;
						REPORT(INFO, "level=" << level << "  i=" << i << "  subQSize=" << subQSize << "  tableOut=" << table->wOut << " gamma=" << gamma  << "  (leftmost)");
						
						vhdl << tab << declare(q, subQSize) << " <= " ;
						if(subQSize >= (table->wOut-gamma))
							vhdl << zg(subQSize - (table->wOut-gamma)) << " & " << out << range (table->wOut-1, gamma) << ";"  << endl;
						else
							vhdl << out << range (subQSize+gamma-1, gamma) << ";"  << endl;
						if( (subQSize <= (1<<(level-1))*alpha) ) {
#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
							vhdl << tab << declare(qs, subQSize) << " <= " << q << " + " << qsr << ";  -- partial quotient so far"  << endl;
#else
							vhdl << tab << declare(getTarget()->adderDelay(subQSize), qs, subQSize) << " <= " << q << " + " << qsr << ";  -- partial quotient so far"  << endl;
#endif
						}
						else {
#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method
							vhdl << tab << declare(qs, subQSize) << " <= " << q << " + (" <<  qsl << " & " << qsr << ");  -- partial quotient so far"  << endl;
#else
							vhdl << tab << declare(getTarget()->adderDelay(subQSize), qs, subQSize) << " <= " << q << " + (" <<  qsl << " & " << qsr << ");  -- partial quotient so far"  << endl;
#endif
						}

						
					}
				} // for i
				previousLevelOdd = ((rLevelSize&1)==1);
				
			} // for level
			
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
		mpz_class R = X%d;
		if(!remainderOnly)
			tc->addExpectedOutput("Q", Q);
		tc->addExpectedOutput("R", R);
	}



	// if index==-1, run the unit tests, otherwise just compute one single test state  out of index, and return it
	TestList IntConstDiv::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) // The unit tests
			{ 

				for(int wIn=8; wIn<34; wIn+=1) 
					{ // test various input widths
						for(int d=3; d<=17; d+=2) 
							{ // test various divisors
								for(int arch=0; arch <2; arch++) 
									{ // test various architectures // TODO FIXME TO TEST THE LINEAR ARCH, TOO
										paramList.push_back(make_pair("wIn", to_string(wIn) ));	
										paramList.push_back(make_pair("d", to_string(d) ));	
										paramList.push_back(make_pair("arch", to_string(arch) ));
										testStateList.push_back(paramList);
										paramList.clear();
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



	OperatorPtr IntConstDiv::parseArguments(Target *target, vector<string> &args) {
		int wIn, arch, alpha;
		vector<int> divisors;
		bool remainderOnly;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn); 
		UserInterface::parseIntList(args, "d", &divisors);
		UserInterface::parseInt(args, "alpha", &alpha);
		UserInterface::parsePositiveInt(args, "arch", &arch);
		UserInterface::parseBoolean(args, "remainderOnly", &remainderOnly);
		if(divisors.size()==1) {
			return new IntConstDiv(target, wIn, divisors[0], alpha, arch, remainderOnly);
		}
		else { // composite divisor
			return new IntConstDiv(target, wIn, divisors, alpha, arch, remainderOnly);
		}
	}

	void IntConstDiv::registerFactory(){
		UserInterface::add("IntConstDiv", // name
											 "Integer divider by a small constant.",
											 "ConstMultDiv",
											 "", // seeAlso
											 "wIn(int): input size in bits; \
											 d(intlist): integer to divide by. Either a small integer, or a comma-separated list of small integers, in which case a composite divider by the product is built;  \
											 arch(int)=0: architecture used -- 0 for linear-time, 1 for log-time; \
											 remainderOnly(bool)=false: if true, the architecture doesn't output the quotient; \
											 alpha(int)=-1: Algorithm uses radix 2^alpha. -1 choses a sensible default.",
											 "This operator is described, for arch=0, in <a href=\"bib/flopoco.html#dedinechin:2012:ensl-00642145:1\">this article</a>, and for arch=1, in <a href=\"bib/flopoco.html#UgurdagEtAl2016\">this article</a>.",
											 IntConstDiv::parseArguments,
											 IntConstDiv::unitTest
											 ) ;
	}

	
}
