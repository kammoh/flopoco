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

  		setCopyrightString("Bogdan Pasca, Florent de Dinechin (2010)");

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
  		vhdl<<"-- Exponent difference and swap  --"<<endl;

  		vhdl << tab << declare("expFracX",wE+wF) << " <= X"<<range(wE+wF-1, 0) <<";"<<endl;
  		vhdl << tab << declare("expFracY",wE+wF) << " <= Y"<<range(wE+wF-1, 0) <<";"<<endl;

		/*		setCriticalPath(getMaxInputDelays(inputDelays));
					manageCriticalPath(target->localWireDelay() + target->eqComparatorDelay(wE+wF+2));
					vhdl<< tab << declare("eqdiffsign") << " <= '1' when excExpFracX = excExpFracY else '0';"<<endl; */

					setCriticalPath(getMaxInputDelays(inputDelays));
					manageCriticalPath(target->localWireDelay() + target->adderDelay(wE+1));
					vhdl<< tab << declare("eXmeY",wE+1) << " <= (\"0\" & X"<<range(wE+wF-1,wF)<<") - (\"0\" & Y"<<range(wE+wF-1,wF)<<");"<<endl;
					vhdl<< tab << declare("eYmeX",wE+1) << " <= (\"0\" & Y"<<range(wE+wF-1,wF)<<") - (\"0\" & X"<<range(wE+wF-1,wF)<<");"<<endl;
					double cpeXmeY = getCriticalPath();


					setCriticalPath(getMaxInputDelays(inputDelays));

					if (wF < 30){
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wE)); //comparator delay implemented for now as adder
			vhdl<< tab << declare("swap")       << " <= '0' when expFracX >= expFracY else '1';"<<endl;
		}else{
			IntAdder *cmpAdder = new IntAdder(target, wE+wF+1);
			addSubComponent(cmpAdder);

			vhdl << tab << declare("addCmpOp1",wE+wF+1) << "<= " << zg(1,0) << " & expFracX;"<<endl;
			vhdl << tab << declare("addCmpOp2",wE+wF+1) << "<= " << og(1,0) << " & not(expFracY);"<<endl;

			inPortMap(cmpAdder, "X", "addCmpOp1");
			inPortMap(cmpAdder, "Y", "addCmpOp2");
			inPortMapCst(cmpAdder, "Cin", "'1'");
			outPortMap (cmpAdder, "R", "cmpRes");

			vhdl << instance(cmpAdder, "cmpAdder") << endl;
			syncCycleFromSignal("cmpRes");
			setCriticalPath( cmpAdder->getOutputDelay("R") );
			vhdl<< tab << declare("swap")       << " <= cmpRes"<<of(wE+wF)<<";"<<endl;
		}

		double cpswap = getCriticalPath();

		manageCriticalPath(target->localWireDelay() + target->lutDelay());

		string pmY="Y";
		if ( sub ) {
			vhdl << tab << declare("mY",wE+wF+1)   << " <= not(Y"<<of(wE+wF)<<") & Y" << range(wE+wF-1,0) << ";"<<endl;
			pmY = "mY";
		}

		// depending on the value of swap, assign the corresponding values to the newX and newY signals

		vhdl<<tab<<declare("newX",wE+wF+1) << " <= X when swap = '0' else "<< pmY << ";"<<endl;
		vhdl<<tab<<declare("newY",wE+wF+1) << " <= " << pmY <<" when swap = '0' else X;"<<endl;
		//break down the signals
		vhdl << tab << declare("expX",wE) << "<= newX"<<range(wE+wF-1,wF)<<";"<<endl;
		vhdl << tab << declare("expY",wE) << "<= newY"<<range(wE+wF-1,wF)<<";"<<endl;
		vhdl << tab << declare("signX")   << "<= newX"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("signY")   << "<= newY"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("EffSub") << " <= signX xor signY;"<<endl;
		vhdl << tab << declare("ExpXY",2*wE)<<"<= expX & expY;"<<endl;
		manageCriticalPath(target->localWireDelay()+ target->lutDelay());
	 	vhdl <<tab<<declare("subNormX") << "<= '1' when expX="<<zg(wE)<<" and newX"<<range(wF-1,0)<<"/="<<zg(wF)<<" else '0';"<<endl;
	 	vhdl <<tab<<declare("subNormY") << "<= '1' when expY="<<zg(wE)<<" and newY"<<range(wF-1,0)<<"/="<<zg(wF)<<" else '0';"<<endl;

		vhdl << tab << declare("fracY",wF+1) << " <= "<< zg(wF+1)<<" when expY="<<zg(wE)<<" and subNormY='0' else (not(subNormY) & newY("<<wF-1<<" downto 0));"<<endl; 	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		double cpfracY = getCriticalPath();



		//exception bits: need to be updated but for not FIXME
		manageCriticalPath(target->localWireDelay()+2*target->lutDelay());
		vhdl <<tab<<"with ExpXY select "<<endl;
		vhdl <<tab<<declare("excRtmp",2) << " <= \"00\" when "<<zg(2*wE)<<","<<endl
		<<tab<<tab<<"\"10\" when "<<zg(wE, -1)<<og(wE, 1)<<"|"<<og(wE, -1)<<zg(wE, 1)<<"|"<<og(2*wE)<<","<<endl
		<<tab<<tab<<"\"01\" when others;"<<endl;
		vhdl <<tab<<declare("excRtmp2",2) << "<= \"10\" when excRtmp=\"01\" and (expX="<<og(wE)<<" or expY="<<og(wE)<<") else excRtmp;"<<endl;
	 	vhdl <<tab<<declare("excRt",2) << "<= \"11\" when excRtmp2=\"10\" and ((newX"<<range(wF-1, 0)<<"="<<zg(wF)<<" and newY"
																									 	<<range(wF-1,0)<<"="<<zg(wF)<<" and signX/=signY)"
																									 	<< " or (newX"<<range(wF-1, 0)<<"/="<<zg(wF)<<" and expX="<<og(wE)<<")"
																									 	<< " or (newY"<<range(wF-1, 0)<<"/="<<zg(wF)<<" and expY="<<og(wE)<<")) else excRtmp2;"<<endl;
	 	manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl <<tab<<declare("signR") << "<= '0' when ExpXY="<<zg(2*wE)<<" and signX/=signY else signX;"<<endl;

	 	setCycleFromSignal("swap");;
	 	if ( getCycleFromSignal("eYmeX") == getCycleFromSignal("swap") )
	 		setCriticalPath(max(cpeXmeY, cpswap));
	 	else{
	 		if (syncCycleFromSignal("eYmeX"))
	 			setCriticalPath(cpeXmeY);
	 	}
		manageCriticalPath(target->localWireDelay() + target->lutDelay());//multiplexer
		vhdl<<tab<<declare("expDiff",wE+1) << " <= eXmeY when swap = '0' else eYmeX;"<<endl;
		manageCriticalPath(target->localWireDelay() + target->eqConstComparatorDelay(wE+1));
		vhdl<<tab<<declare("shiftedOut") << " <= '1' when (expDiff >= "<<wF+2<<") else '0';"<<endl;
		//shiftVal=the number of positions that fracY must be shifted to the right

		//		cout << "********" << wE << " " <<  sizeRightShift  <<endl;

		if (wE>sizeRightShift) {
			manageCriticalPath(target->localWireDelay() + target->lutDelay());
			vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= expDiff("<< sizeRightShift-1<<" downto 0)"
			<< " when shiftedOut='0' else CONV_STD_LOGIC_VECTOR("<<wF+3<<","<<sizeRightShift<<") ;" << endl;
		}
		else if (wE==sizeRightShift) {
			vhdl<<tab<<declare("shiftVal", sizeRightShift) << " <= expDiff" << range(sizeRightShift-1,0) << ";" << endl ;
		}
		else 	{ //  wE< sizeRightShift
			vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= CONV_STD_LOGIC_VECTOR(0,"<<sizeRightShift-wE <<") & expDiff;" <<	endl;
		}

		if ( getCycleFromSignal("fracY") == getCycleFromSignal("shiftVal") )
			setCriticalPath( max(cpfracY, getCriticalPath()) );
		else{
			if (syncCycleFromSignal("fracY"))
				setCriticalPath(cpfracY);
		}

		// shift right the significand of new Y with as many positions as the exponent difference suggests (alignment)
		REPORT(DETAILED, "Building far path right shifter");
		rightShifter = new Shifter(target,wF+1,wF+3, Shifter::Right, inDelayMap("X",getCriticalPath()));
		rightShifter->changeName(getName()+"_RightShifter");
		addSubComponent(rightShifter);
		inPortMap  (rightShifter, "X", "fracY");
		inPortMap  (rightShifter, "S", "shiftVal");
		outPortMap (rightShifter, "R","shiftedFracY");
		vhdl << instance(rightShifter, "RightShifterComponent");
		syncCycleFromSignal("shiftedFracY");
		setCriticalPath(rightShifter->getOutputDelay("R"));
		nextCycle();         ////
		setCriticalPath(0.0);////
		double cpshiftedFracY = getCriticalPath();
		//sticky compuation in parallel with addition, no need for manageCriticalPath
		//FIXME: compute inside shifter;
		//compute sticky bit as the or of the shifted out bits during the alignment //
		manageCriticalPath(target->localWireDelay() + target->eqConstComparatorDelay(wF+1));
		vhdl<<tab<< declare("sticky") << " <= '0' when (shiftedFracY("<<wF<<" downto 0)=CONV_STD_LOGIC_VECTOR(0,"<<wF+1<<")) else '1';"<<endl;
		double cpsticky = getCriticalPath();

		setCycleFromSignal("shiftedFracY");
		nextCycle();         ////
		setCriticalPath(0.0);////
		setCriticalPath(cpshiftedFracY);
		//pad fraction of Y [overflow][shifted frac having inplicit 1][guard][round]
		vhdl<<tab<< declare("fracYfar", wF+4)      << " <= \"0\" & shiftedFracY("<<2*wF+3<<" downto "<<wF+1<<");"<<endl;
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl<<tab<< declare("EffSubVector", wF+4) << " <= ("<<wF+3<<" downto 0 => EffSub);"<<endl;
		vhdl<<tab<< declare("fracYfarXorOp", wF+4) << " <= fracYfar xor EffSubVector;"<<endl;
		//pad fraction of X [overflow][inplicit 1][fracX][guard bits]

		vhdl<<tab<< declare("fracXfar", wF+4)      << " <= '0' & not(subNormX) & (newX("<<wF-1<<" downto 0)) & \"00\";"<<endl;

		if (getCycleFromSignal("sticky")==getCycleFromSignal("fracXfar"))
			setCriticalPath( max (cpsticky, getCriticalPath()) );
		else
			if (syncCycleFromSignal("sticky"))
				setCriticalPath(cpsticky);
			manageCriticalPath(target->localWireDelay()+ target->lutDelay());
		vhdl<<tab<< declare("cInAddFar")           << " <= EffSub and not sticky;"<< endl;//TODO understand why

		//result is always positive.
		fracAddFar = new IntAdder(target,wF+4, inDelayMap("X", getCriticalPath()));
		addSubComponent(fracAddFar);
		inPortMap  (fracAddFar, "X", "fracXfar");
		inPortMap  (fracAddFar, "Y", "fracYfarXorOp");
		inPortMap  (fracAddFar, "Cin", "cInAddFar");
		outPortMap (fracAddFar, "R","fracAddResult");
		vhdl << instance(fracAddFar, "fracAdder");
		syncCycleFromSignal("fracAddResult");
		setCriticalPath(fracAddFar->getOutputDelay("R"));

		if (getCycleFromSignal("sticky")==getCycleFromSignal("fracAddResult"))
			setCriticalPath(max(cpsticky, getCriticalPath()));
		else{
			if (syncCycleFromSignal("sticky"))
				setCriticalPath(cpsticky);
		}

		//shift in place
		vhdl << tab << declare("fracGRS",wF+5) << "<= fracAddResult & sticky; "<<endl;

		//incremented exponent.
		vhdl << tab << declare("extendedExpInc",wE+2) << "<= (\"00\" & expX) + '1';"<<endl;


		lzocs = new LZOCShifterSticky(target, wF+5, wF+5, intlog2(wF+5), false, 0, inDelayMap("I",getCriticalPath()));
		addSubComponent(lzocs);
		inPortMap  (lzocs, "I", "fracGRS");
		outPortMap (lzocs, "Count","nZerosNew");
		outPortMap (lzocs, "O","shiftedFrac");
		vhdl << instance(lzocs, "LZC_component");
		syncCycleFromSignal("shiftedFrac");
		setCriticalPath(lzocs->getOutputDelay("O"));
		// 		double cpnZerosNew = getCriticalPath();
		double cpshiftedFrac = getCriticalPath();




		//need to decide how much to add to the exponent
		/*		manageCriticalPath(target->localWireDelay() + target->adderDelay(wE+2));*/
		// 	vhdl << tab << declare("expPart",wE+2) << " <= (" << zg(wE+2-lzocs->getCountWidth(),0) <<" & nZerosNew) - 1;"<<endl;
		//update exponent

		manageCriticalPath(target->localWireDelay() + target->adderDelay(wE+2));
		vhdl << tab << declare("updatedExp",wE+2) << " <= extendedExpInc - (" << zg(wE+2-lzocs->getCountWidth(),0) <<" & nZerosNew);"<<endl;
		vhdl << tab << declare("eqdiffsign")<< " <= '1' when nZerosNew="<<og(lzocs->getCountWidth(),0)<<" else '0';"<<endl;


		//concatenate exponent with fraction to absorb the possible carry out
		vhdl<<tab<<declare("expFrac",wE+2+wF+1)<<"<= updatedExp & shiftedFrac"<<range(wF+3,3)<<";"<<endl;
		double cpexpFrac = getCriticalPath();


		// 		//at least in parallel with previous 2 statements
		setCycleFromSignal("shiftedFrac");
		setCriticalPath(cpshiftedFrac);
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl<<tab<<declare("stk")<<"<= shiftedFrac"<<of(1)<<" or shiftedFrac"<<of(0)<<";"<<endl;
		vhdl<<tab<<declare("rnd")<<"<= shiftedFrac"<<of(2)<<";"<<endl;
		vhdl<<tab<<declare("grd")<<"<= shiftedFrac"<<of(3)<<";"<<endl;
		vhdl<<tab<<declare("lsb")<<"<= shiftedFrac"<<of(4)<<";"<<endl;

		//decide what to add to the guard bit
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl<<tab<<declare("addToRoundBit")<<"<= '0' when (lsb='0' and grd='1' and rnd='0' and stk='0')  else '1';"<<endl;
		//round

		if (getCycleFromSignal("expFrac") == getCycleFromSignal("addToRoundBit"))
			setCriticalPath(max(cpexpFrac, getCriticalPath()));
		else
			if (syncCycleFromSignal("expFrac"))
				setCriticalPath(cpexpFrac);

			IntAdder *ra = new IntAdder(target, wE+2+wF+1, inDelayMap("X", getCriticalPath() ) );
			addSubComponent(ra);

			inPortMap(ra,"X", "expFrac");
			inPortMapCst(ra, "Y", zg(wE+2+wF+1,0) );
			inPortMap( ra, "Cin", "addToRoundBit");
			outPortMap( ra, "R", "RoundedExpFrac");
			vhdl << instance(ra, "roundingAdder");
			setCycleFromSignal("RoundedExpFrac");
			setCriticalPath(ra->getOutputDelay("R"));

		// 		vhdl<<tab<<declare("RoundedExpFrac",wE+2+wF+1)<<"<= expFrac + addToRoundBit;"<<endl;

		//possible update to exception bits
			vhdl << tab << declare("upExc",2)<<" <= RoundedExpFrac"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;

			vhdl << tab << declare("fracR",wF)<<" <= RoundedExpFrac"<<range(wF,1)<<";"<<endl;
			vhdl << tab << declare("expR",wE) <<" <= RoundedExpFrac"<<range(wF+wE,wF+1)<<";"<<endl;

			manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl << tab << declare("exExpExc",4) << " <= upExc & excRt;"<<endl;
		vhdl << tab << "with (exExpExc) select "<<endl;
		vhdl << tab << declare("expRt",wE) << "<= "<<zg(wE)<<" when \"0000\"|\"0100\"|\"1000\"|\"1100\"|\"1001\"|\"1101\","<<endl
		<<tab<<tab<<"expR when \"0001\","<<endl
		<<tab<<tab<<og(wE)<<" when \"0010\"|\"0110\"|\"1010\"|\"1110\"|\"0101\","<<endl
		<<tab<<tab<<og(wE)<<" when others;"<<endl;
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl<<tab<<declare("expRt2",wE) << " <= "<<zg(wE)<<" when (eqdiffsign='1' and EffSub='1') and expRt/="<<og(wE)<<" else expRt;"<<endl; 
		vhdl<<tab<<declare("fracRt", wF) << " <= "<<zg(wF)<<" when expRt2="<<og(wE)<<" and (exExpExc=\"0010\" or exExpExc=\"0110\" or exExpExc=\"1010\" "  
		<< "or exExpExc=\"1110\" or exExpExc=\"0101\" or exExpExc=\"0001\") else fracR;"<<endl;
		vhdl<<tab<<declare("fracRt2", wF) << " <= "<<og(wF)<<" when expRt2="<<og(wE)<<" and exExpExc/=\"0010\" and exExpExc/=\"0110\" and exExpExc/=\"1010\" "  
		<< "and exExpExc/=\"1110\" and exExpExc/=\"0101\" and exExpExc/=\"0001\" else fracRt;"<<endl;
		// IEEE standard says in 6.3: if exact sum is zero, it should be +zero in RN
		vhdl<<tab<<declare("signR2") << " <= '0' when (eqdiffsign='1' and EffSub='1') or (fracRt2="<<og(wF)<<" and expRt2="<<og(wE)<<") else signR;"<<endl;



		// assign result
		vhdl<<tab<< declare("computedR",wE+wF+1) << " <= signR2 & expRt2 & fracRt2;"<<endl;	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		vhdl << tab << "R <= computedR;"<<endl;

		/*		manageCriticalPath(target->localWireDelay() +  target->lutDelay());
					vhdl<<tab<<"with sdExnXY select"<<endl;
					vhdl<<tab<<"R <= newX when \"0100\"|\"1000\"|\"1001\", newY when \"0001\"|\"0010\"|\"0110\", computedR when others;"<<endl;*/


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
					tc->addIEEEInput("X", IEEENumber::plusZero);
					tc->addIEEEInput("Y", 2.8e-45);
					emulate(tc);
					tcl->add(tc);

					
					tc = new TestCase(this);
					tc->addIEEEInput("X", 2.4E-44);
					tc->addIEEEInput("Y", 2.8e-45);
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

	void FPAddSinglePathIEEE::nextTestState(TestState * previousTestState)
	{
		static vector<vector<pair<string,string>>> testStateList;
		vector<pair<string,string>> paramList;

		if(previousTestState->getIndex() == 0)
		{

			previousTestState->setTestBenchSize(250);
			// for(int j = 0; j<2; j++)
			// {
				paramList.push_back(make_pair("wE","5"));
				paramList.push_back(make_pair("wF","10"));
				// if(j==1)
				// {
				// 	paramList.push_back(make_pair("sub","true"));
				// }			
				testStateList.push_back(paramList);

				paramList.clear();
				paramList.push_back(make_pair("wE","8"));
				paramList.push_back(make_pair("wF","23"));
				// if(j==1)
				// {
				// 	paramList.push_back(make_pair("sub","true"));
				// }				
				testStateList.push_back(paramList);

				paramList.clear();
				paramList.push_back(make_pair("wE","11"));
				paramList.push_back(make_pair("wF","52"));
				// if(j==1)
				// {
				// 	paramList.push_back(make_pair("sub","true"));
				// }				
				testStateList.push_back(paramList);

				paramList.clear();
				paramList.push_back(make_pair("wE","15"));
				paramList.push_back(make_pair("wF","112"));
				// if(j==1)
				// {
				// 	paramList.push_back(make_pair("sub","true"));
				// }				
				testStateList.push_back(paramList);

				paramList.clear();
				paramList.push_back(make_pair("wE","19"));
				paramList.push_back(make_pair("wF","236"));
				// if(j==1)
				// {
				// 	paramList.push_back(make_pair("sub","true"));
				// }				
				testStateList.push_back(paramList);


				/*for(int i = 5; i<53; i++)
				{
					int nbByteWE = 6+(i/10);
					while(nbByteWE>i)
					{
						nbByteWE -= 2;
					}
					paramList.clear();
					paramList.push_back(make_pair("wF",to_string(i)));
					paramList.push_back(make_pair("wE",to_string(nbByteWE)));
					if(j==1)
					{
						paramList.push_back(make_pair("sub","true"));
					}
					testStateList.push_back(paramList);
				}*/
			}
			previousTestState->setTestsNumber(testStateList.size());
		// }

		vector<pair<string,string>>::iterator itVector;
		int index = previousTestState->getIndex();

		for(itVector = testStateList[index].begin(); itVector != testStateList[index].end(); ++itVector)
		{
			previousTestState->changeValue((*itVector).first,(*itVector).second);
		}
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
											 FPAddSinglePathIEEE::nextTestState
											 ) ;
	}
}
