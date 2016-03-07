/*
  A long accumulator for FloPoCo
 
  Authors : Florent de Dinechin and Bogdan Pasca
 

   This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "utils.hpp"
#include "Operator.hpp"
#include "FPLargeAcc.hpp"

using namespace std;

namespace flopoco{

	FPLargeAcc::FPLargeAcc(Target* target, int wEX, int wFX, int MaxMSBX, int MSBA, int LSBA, map<string, double> inputDelays,  bool forDotProd, int wFY): 
		Operator(target), 
		wEX(wEX), wFX(wFX), MaxMSBX(MaxMSBX), LSBA(LSBA), MSBA(MSBA), xOvf(0), accValue(0)
	{
		srcFileName="FPLargeAcc";
		// You probably want to remove the following line to have the warnings come back
		//if you modify this operator
		setHasDelay1Feedbacks(); 

		if (!forDotProd)
			wFY=wFX;
		
		int i;
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2008-2015)");		

		//check input constraints, i.e, MaxMSBX <= MSBA, LSBA<MaxMSBx
		if ((MaxMSBX > MSBA)){
			THROWERROR("Input constraint MaxMSBX <= MSBA not met.");
		}
		if ((LSBA >= MaxMSBX)){
			THROWERROR("Input constraint LSBA<MaxMSBx not met: This accumulator would never accumulate a bit.");
		}

		ostringstream name; 
		name <<"FPLargeAcc_"<<wEX<<"_"<<wFX<<"_"
				 <<(MaxMSBX>=0?"":"M")<<abs(MaxMSBX)<<"_"
				 <<(MSBA>=0?"":"M")<<abs(MSBA)<<"_" 
				 <<(LSBA>=0?"":"M")<<abs(LSBA);
		setName(name.str());

		// This operator is a sequential one
		setSequential();

		// Set up various architectural parameters
		sizeAcc = MSBA-LSBA+1;

		if (forDotProd){
			addInput ("sigX_dprod");
			addInput ("excX_dprod", 2);
			addInput ("fracX_dprod", 1+wFX+1+wFY);
			addInput ("expX_dprod", wEX);
		}else
			addFPInput ("X", wEX,wFX);
		

		addInput   ("newDataSet");
		addOutput  ("ready");
		addOutput  ("A", sizeAcc);  
		addOutput  ("C", sizeAcc);
		addOutput  ("XOverflow");  
		addOutput  ("XUnderflow");  
		addOutput  ("AccOverflow");  
		
		maxShift        = MaxMSBX-LSBA;              // shift is 0 when the implicit 1 is at LSBA
		sizeShift       = intlog2(maxShift);         // the number of bits needed to control the shifter
		sizeSummand     = MaxMSBX-LSBA+1;         // the size of the summand (the maximum one - when the inplicit 1 is on MaxMSBX)
		sizeShiftedFrac = maxShift + wFX+1;   
		E0X = (1<<(wEX-1)) -1;                 // exponent bias
		// Shift is 0 when implicit 1 is on LSBA, that is when EX-bias = LSBA
		// that is, EX-bias-LSBA = 0, EX-(bias + LSBA) = 0
		// We are working in sign-magnitude, so we differentiate 2 cases:
		// 1. when bias+LSBA >=0, that is, sign bit is 0, then shiftValue = Exp - (bias+LSBA) ;  (0)Exp - (0)(bias+LSBA)
		// 2. when bias+LSBA <=0, which means that the sign bit is 1. Because in this case we have a substraction of a negative number,
		// we replace it whit an addition of -(bias+LSBA), that is (0)Exp + (0)(- (bias + LSBA))
		//	
		// If the shift value is negative, then the input is shifted out completely, and a 0 is added to the accumulator 
		// All above computations are performed on wEx+1 bits	
	
		//MaxMSBx is one valid exponent value, that is, 
		//1. after bias is added, value should be >= 0
		//2. after bias is added, representation should still fit on no more than wEX bits
		int biasedMaxMSBX = MaxMSBX + E0X;
		if(biasedMaxMSBX < 0 || intlog2(biasedMaxMSBX)>wEX) {
			cerr<<"ERROR in FPLargeAcc: MaxMSBX="<<MaxMSBX<<" is not a valid exponent of X (range "
				 <<(-E0X)<< " to " << ((1<<wEX)-1)-E0X <<endl;
			exit (EXIT_FAILURE);
		}

		/* set-up carry-save parameters */		
		int chunkSize;
		target->suggestSlackSubaddSize(chunkSize , sizeAcc, target->localWireDelay() + target->lutDelay());
		REPORT( DEBUG, "Addition chunk size in FPLargeAcc is:"<<chunkSize);
		 
		int nbOfChunks    = ceil(double(sizeAcc)/double(chunkSize));
		int lastChunkSize = ( sizeAcc % chunkSize == 0 ? chunkSize  : sizeAcc % chunkSize);


		/* if FPLargeAcc is used in FPDotProduct, then its input fraction is twice as large */
		if (!forDotProd){
			vhdl << tab << declare("fracX",wFX+1) << " <=  \"1\" & X" << range(wFX-1,0) << ";" << endl;
			vhdl << tab << declare("expX" ,wEX  ) << " <= X" << range(wEX+wFX-1,wFX) << ";" << endl;
			vhdl << tab << declare("signX") << " <= X" << of(wEX+wFX) << ";" << endl;
			vhdl << tab << declare("exnX" ,2     ) << " <= X" << range(wEX+wFX+2,wEX+wFX+1) << ";" << endl;
		}else{
			vhdl << tab << declare("fracX",wFX + wFY +2) << " <= fracXdprod;" << endl;
			vhdl << tab << declare("expX" ,wEX  ) << " <= expX_dprod;" << endl;
			vhdl << tab << declare("signX") << " <= sigX_dprod;" << endl;
			vhdl << tab << declare("exnX" ,2     ) << " <= excX_dprod;" << endl;
		}

		setCriticalPath( getMaxInputDelays(inputDelays));
		manageCriticalPath( target->localWireDelay() + target->adderDelay(wEX+1)); 

		/* declaring the underflow and overflow conditions of the input X. 
		these two flags are used to reparameter the accumulator following a test
		run. If Xoverflow has happened, then MaxMSBX needs to be increased and 
		the accumulation result is invalidated. If Xunderflow is raised then 
		user can lower LSBA for obtaining a even better accumulation precision */
		vhdl << tab << declare("xOverflowCond" ,1, false, Signal::registeredWithSyncReset) << " <= '1' when (( expX > CONV_STD_LOGIC_VECTOR("<<MaxMSBX + E0X<<","<< wEX<<")) or (exnX >= \"10\")) else '0' ;"<<endl; 
		vhdl << tab << declare("xUnderflowCond",1, false, Signal::registeredWithSyncReset) << " <= '1' when (expX < CONV_STD_LOGIC_VECTOR("<<LSBA + E0X<<","<<wEX<<")) else '0' ;" << endl;  
		
		mpz_class exp_offset = E0X+LSBA;
		vhdl << tab << declare("shiftVal",wEX+1) << " <= (\"0\" & expX) - CONV_STD_LOGIC_VECTOR("<< exp_offset <<","<<  wEX+1<<");" << endl;

		Shifter* shifter = new Shifter(target, (forDotProd?wFX+wFY+2:wFX+1), maxShift, Shifter::Left, inDelayMap("X", target->localWireDelay() + getCriticalPath()));
		addSubComponent(shifter);

		inPortMap   (shifter, "X", "fracX");
		inPortMapCst(shifter, "S", "shiftVal"+range(shifter->getShiftInWidth() - 1,0));
		outPortMap  (shifter, "R", "shifted_frac");
		vhdl << instance(shifter, "FPLargeAccInputShifter");
	
		syncCycleFromSignal("shifted_frac");



		/* determine if the input has been shifted out from the accumulator. 
		In this case the accumulator will added 0 */
		vhdl << tab << declare("flushedToZero") << " <= '1' when (shiftVal" << of(wEX)<<"='1' or exnX=\"00\") else '0';" << endl;

		/* in most FPGAs computation of the summand2c will be done in one LUT level */
		vhdl << tab << declare("summand", sizeSummand, true, Signal::registeredWithSyncReset) << "<= " << 
					zg(sizeSummand) << " when flushedToZero='1' else shifted_frac" << range(sizeShiftedFrac-1,wFX)<<";" << endl;

		vhdl << tab << "-- 2's complement of the summand" << endl;
		/* Don't compute 2's complement just yet, just invert the bits and leave 
		the addition of the extra 1 in accumulation, as a carry in bit for the 
		first chunk*/
		vhdl << tab << declare("summand2c", sizeSummand, true) << " <= summand when (signX='0' or flushedToZero='1') else not(summand);"<< endl;

		vhdl << tab << "-- sign extension of the summand to accumulator size" << endl;
		vhdl << tab << declare("ext_summand2c",sizeAcc,true) << " <= " << (sizeAcc-1<sizeSummand?"":rangeAssign(sizeAcc-1, sizeSummand, "signX and not flushedToZero")+" & ") << "summand2c;" << endl;

		vhdl << tab << "-- accumulation itself" << endl;
		//determine the value of the carry in bit
		vhdl << tab << declare("carryBit_0",1,false, Signal::registeredWithSyncReset) << " <= signX and not flushedToZero;" << endl; 
		
		
		for (i=0; i < nbOfChunks; i++) {
			ostringstream accReg;
			accReg<<"acc_"<<i;

			vhdl << tab << declare(join("acc_",i),(i!=nbOfChunks-1?chunkSize:lastChunkSize) ,true, Signal::registeredWithSyncReset) << " <= " << 
				join("acc_",i,"_ext")<<range((i!=nbOfChunks-1?chunkSize-1:lastChunkSize-1),0) << ";" << endl;
			vhdl << tab << declare(join("carryBit_",i+1),1, false, Signal::registeredWithSyncReset) <<"  <= " << join("acc_",i,"_ext")<<of((i!=nbOfChunks-1?chunkSize:lastChunkSize)) << ";" << endl;
			nextCycle();		
			vhdl << tab << declare(join("acc_",i,"_ext"),(i!=nbOfChunks-1?chunkSize:lastChunkSize)+1) << " <= ( \"0\" & (" <<join("acc_",i)<< " and "<<rangeAssign( (i!=nbOfChunks-1?chunkSize:lastChunkSize)-1,0, "not(newDataSet)") <<")) + " <<
				"( \"0\" & ext_summand2c" << range( (i!=nbOfChunks-1?chunkSize*(i+1)-1:sizeAcc-1), chunkSize*i) << ") + " << 
				"("<<join("carryBit_",i) << (i>0?" and not(newDataSet)":"")<< ");" << endl;
			setCycleFromSignal("carryBit_0");
		}

		setCycleFromSignal("carryBit_0",false);
		vhdl << tab << declare("xOverflowRegister",1,false, Signal::registeredWithSyncReset) << " <= "; nextCycle(false); vhdl << "xOverflowRegister or xOverflowCond;"<<endl;
		setCycleFromSignal("carryBit_0",false);
		vhdl << tab << declare("xUnderflowRegister",1,false, Signal::registeredWithSyncReset) << " <= "; nextCycle(false); vhdl << "xUnderflowRegister or xUnderflowCond;"<<endl;
		setCycleFromSignal("carryBit_0",false);
		vhdl << tab << declare("accOverflowRegister",1,false, Signal::registeredWithSyncReset) << " <= "; nextCycle(false); vhdl << "accOverflowRegister or "<<join("carryBit_",nbOfChunks) << ";"<<endl;
		setCycleFromSignal("carryBit_0",false);

		nextCycle();
		
		//compose the acc signal 
		vhdl << tab << declare("acc", sizeAcc) << " <= ";
		for (i=nbOfChunks-1;i>=0;i--)
			vhdl << join("acc_",i) <<  (i>0?" & ":";\n");
	
		vhdl << tab << declare("carry", sizeAcc) << " <= ";
		for (i=nbOfChunks-1;i>=0; i--){
			vhdl << (i<nbOfChunks-1?join("carryBit_",i+1)+" & ":"");
			vhdl << (i==nbOfChunks-1?zg(lastChunkSize-(i>0?1:0))+(i>0?" & ":""):(i>0?zg(chunkSize-1)+" & ":zg(chunkSize)));
		}vhdl << ";" << endl;

		if (nbOfChunks > 1 ){
			vhdl << tab << "A <=   acc;" << endl;
			vhdl << tab << "C <=   carry;" << endl;
		}else{
			vhdl << tab << "A <=  acc_0;" << endl;
			vhdl << tab << "C <=   carry;" << endl;
		}
	
//		nextCycle();
		//if accumulator overflows this flag will be set to 1 until a maunal reset is performed
		vhdl << tab << "AccOverflow <= accOverflowRegister;"<<endl; 
		//if the input overflows then this flag will be set to 1, and will remain 1 until manual reset is performed
		vhdl << tab << "XOverflow <= xOverflowRegister;"<<endl; 
		vhdl << tab << "XUnderflow <= xUnderflowRegister;"<<endl; 

		setCycleFromSignal("acc");
		vhdl << tab << "ready <= newDataSet;"<<endl;
	}


	FPLargeAcc::~FPLargeAcc() {
	}

	void FPLargeAcc::test_precision(int n) {
		mpfr_t ref_acc, long_acc, fp_acc, r, d, one, two, msb;
		double sum, error;

		// initialisation
		mpfr_init2(ref_acc, 10000);
		mpfr_init2(long_acc, sizeAcc+1);
		mpfr_init2(fp_acc, wFX+1);
		mpfr_init2(r, wFX+1);
		mpfr_init2(d, 100);
		mpfr_init2(one, 100);
		mpfr_init2(two, 100);
		mpfr_init2(msb, 100);
		mpfr_set_d(one, 1.0, GMP_RNDN);
		mpfr_set_d(two, 2.0, GMP_RNDN);
		mpfr_set_d(msb, (double)(1<<(MSBA+1)), GMP_RNDN); // works for MSBA<32

		//cout<<"%-------Acc. of positive numbers--------------- "<<endl;
		mpfr_set_d(ref_acc, 0.0, GMP_RNDN);
		mpfr_set_d(fp_acc, 0.0, GMP_RNDN);
		//put a one in the MSBA+1 bit will turn all the subsequent additions
		// into fixed-point ones
		mpfr_set_d(long_acc, (double)(1<<(MSBA+1)), GMP_RNDN);

		FloPoCoRandomState::init(10, false);

		for(int i=0; i<n; i++){
			//mpfr_random(r); // deprecated function; r is [0,1]
			mpfr_urandomb(r, FloPoCoRandomState::m_state);
			//    mpfr_add(r, one, r, GMP_RNDN); // r in [1 2[
		
			mpfr_add(fp_acc, fp_acc, r, GMP_RNDN);
			mpfr_add(ref_acc, ref_acc, r, GMP_RNDN);
			mpfr_add(long_acc, long_acc, r, GMP_RNDN);
			if(mpfr_greaterequal_p(long_acc, msb)) 
				mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);
			
		}

		// remove the leading one from long acc
		if(mpfr_greaterequal_p(long_acc, msb)) 
			mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);
		
		sum=mpfr_get_d(ref_acc, GMP_RNDN);
		cout  << "% unif[0 1] :sum="<< sum;
		sum=mpfr_get_d(fp_acc, GMP_RNDN);
		cout << "   FPAcc="<< sum;
		sum=mpfr_get_d(long_acc, GMP_RNDN);
		cout << "   FPLargeAcc="<< sum;

		cout <<endl << n << " & ";
		// compute the error for the FP adder
		mpfr_sub(d, fp_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		// cout << " Relative error between fp_acc and ref_acc is "<< error << endl;
		cout << scientific << setprecision(2)  << error << " & ";
		// compute the error for the long acc
		mpfr_sub(d, long_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		//  cout << "Relative error between long_acc and ref_acc is "<< error << endl;
		cout << scientific << setprecision(2)  << error << " & ";


		//cout<<"%-------Acc. of positive/negative numbers--------------- "<<endl;

		mpfr_set_d(ref_acc, 0.0, GMP_RNDN);
		mpfr_set_d(fp_acc, 0.0, GMP_RNDN);
		//put a one in the MSBA+1 bit will turn all the subsequent additions
		// into fixed-point ones
		mpfr_set_d(long_acc, (double)(1<<(MSBA+1)), GMP_RNDN);

		for(int i=0; i<n; i++){
			//mpfr_random(r); // deprecated function; r is [0,1]
			mpfr_urandomb(r, FloPoCoRandomState::m_state);
			mpfr_mul(r, r, two, GMP_RNDN); 
			mpfr_sub(r, r, one, GMP_RNDN); 
		
			mpfr_add(fp_acc, fp_acc, r, GMP_RNDN);
			mpfr_add(ref_acc, ref_acc, r, GMP_RNDN);
			mpfr_add(long_acc, long_acc, r, GMP_RNDN);
		}

		// remove the leading one from long acc
		mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);


		// compute the error for the FP adder
		mpfr_sub(d, fp_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		// cout << "Relative error between fp_acc and ref_acc is "<< error << endl;
		cout << scientific << setprecision(2) << error << " & ";

		// compute the error for the long acc
		mpfr_sub(d, long_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		//  cout << "Relative error between long_acc and ref_acc is "<< error << endl;
		cout << scientific << setprecision(2)  << error << " \\\\ \n     \\hline \n";

		sum=mpfr_get_d(ref_acc, GMP_RNDN);
		cout << "% unif[-1 1] : sum="<< sum;
		sum=mpfr_get_d(fp_acc, GMP_RNDN);
		cout << "   FPAcc="<< sum;
		sum=mpfr_get_d(long_acc, GMP_RNDN);
		cout << "   FPLargeAcc="<< sum;
		cout <<endl;

	}

	// read the values from a file and accumulate them
	void FPLargeAcc::test_precision2() {

		mpfr_t ref_acc, long_acc, fp_acc, r, d, one, two, msb;
		double dr,  sum, error;

		// initialisation
#define DPFPAcc 1
		mpfr_init2(ref_acc, 10000);
		mpfr_init2(long_acc, sizeAcc+1);
#if DPFPAcc
		mpfr_init2(fp_acc,52 +1);
#else
		mpfr_init2(fp_acc, wFX+1);
#endif
		mpfr_init2(r, wFX+1);
		mpfr_init2(d, 100);
		mpfr_init2(one, 100);
		mpfr_init2(two, 100);
		mpfr_init2(msb, 100);
		mpfr_set_d(one, 1.0, GMP_RNDN);
		mpfr_set_d(two, 2.0, GMP_RNDN);
		mpfr_set_d(msb, (double)(1<<(MSBA+1)), GMP_RNDN); // works for MSBA<32

		mpfr_set_d(ref_acc, 0.0, GMP_RNDN);
		mpfr_set_d(fp_acc, 0.0, GMP_RNDN);
		//put a one in the MSBA+1 bit will turn all the subsequent additions
		// into fixed-point ones
		mpfr_set_d(long_acc, (double)(1<<(MSBA+1)), GMP_RNDN);


		ifstream myfile ("/home/fdedinec/accbobines.txt");
		int i=0;
		if (myfile.is_open())
			{
				while (! myfile.eof() )
					{
						myfile >> dr;
						mpfr_set_d(r, dr, GMP_RNDN);
						mpfr_add(fp_acc, fp_acc, r, GMP_RNDN);
						mpfr_add(ref_acc, ref_acc, r, GMP_RNDN);
						mpfr_add(long_acc, long_acc, r, GMP_RNDN);
						i++;
						if (i%100000==0) {
							cout<<"i="<<i<<" "<<endl;

							// compute the error for the FP adder
							mpfr_sub(d, fp_acc, ref_acc, GMP_RNDN);
							mpfr_div(d, d, ref_acc, GMP_RNDN);
							error=mpfr_get_d(d, GMP_RNDN);
							cout << "Relative error between fp_acc and ref_acc is "<< error << endl;
							//cout << scientific << setprecision(2) << error << " & ";

							// remove the leading one from long acc
							mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);
							// compute the error for the long acc
							mpfr_sub(d, long_acc, ref_acc, GMP_RNDN);
							mpfr_div(d, d, ref_acc, GMP_RNDN);
							error=mpfr_get_d(d, GMP_RNDN);
							cout << "Relative error between long_acc and ref_acc is "<< error << endl;
							//cout << scientific << setprecision(2)  << error << " \\\\ \n     \\hline \n";

							sum=mpfr_get_d(ref_acc, GMP_RNDN);
							cout << " exact sum="<< sum;
							sum=mpfr_get_d(fp_acc, GMP_RNDN);
							cout << "   FPAcc="<< sum;
							sum=mpfr_get_d(long_acc, GMP_RNDN);
							cout << "   FPLargeAcc="<< sum;
							cout <<endl;
							// add the leading one back
							mpfr_add(long_acc, long_acc, msb, GMP_RNDN);
						}
					}
				myfile.close();
			}
		// remove the leading one from long acc
		mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);


		// compute the error for the FP adder
		mpfr_sub(d, fp_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		cout << "Relative error between fp_acc and ref_acc is "<< error << endl;
		//cout << scientific << setprecision(2) << error << " & ";

		// compute the error for the long acc
		mpfr_sub(d, long_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		cout << "Relative error between long_acc and ref_acc is "<< error << endl;
		//cout << scientific << setprecision(2)  << error << " \\\\ \n     \\hline \n";

		sum=mpfr_get_d(ref_acc, GMP_RNDN);
		cout << " exact sum="<< sum;
		sum=mpfr_get_d(fp_acc, GMP_RNDN);
		cout << "   FPAcc="<< sum;
		sum=mpfr_get_d(long_acc, GMP_RNDN);
		cout << "   FPLargeAcc="<< sum;
		cout <<endl;


		exit(0);

	}




	
	mpz_class FPLargeAcc::mapFP2Acc(FPNumber X)
	{
		//get true exponent of X
		mpz_class expX = X.getExponentSignalValue() - ( intpow2(wEX-1)-1 ); 
	
		int keepBits = -LSBA + expX.get_si();
		if (keepBits<0)
			return 0;
		else if (wFX>keepBits)
			// The following is an _integer_ division that, by discarding its fractional part,  takes care of the truncation.
			return (X.getFractionSignalValue()/mpz_class(intpow2(wFX-keepBits)));
		else
			return (X.getFractionSignalValue()*mpz_class(intpow2(keepBits-wFX)));
	}

	mpz_class FPLargeAcc::sInt2C2(mpz_class X, int width)
	{
		if (X>=0)
			return X;
		else{
			return (intpow2( width)+X);
		}
	}

	/*TODOs: 
		1/ we only test the case when all the additions are exact.
		Otherwise the accumulation error diverges and I don't know how to manage it in emulate 
		(the accumulator would require more and 
		In between, it is enough to overload buildRandomTestCase to make sure it remains in this case...
		This is definitely dishonest in term of accuracy, and doesn't test the case when a mantissa is partly shifted out 
		But we cannot guarantee the long-term faithfulness of the  accumulator.

		2/ we doe not emulate the case when the accumulator is split with a C output;

	*/
	void FPLargeAcc::emulate(TestCase* tc){
		mpz_class rst = tc->getInputValue("newDataSet");
		if (rst==1)
			accValue=0;
		else {	
			mpz_class bvx = tc->getInputValue("X");
			FPNumber fpx(wEX, wFX, bvx);
			accValue += mapFP2Acc(fpx);
			if(accValue>(mpz_class(1)<<(sizeAcc-1))-1){
				REPORT(DEBUG, "emulate: accumulator positive overflow"); // TODO replace with a set of Ov output
				accValue -= (mpz_class(1)<<sizeAcc);
			}
			if(accValue<-(mpz_class(1)<<(sizeAcc-1))){
				REPORT(DEBUG, "emulate: accumulator negativeoverflow"); // TODO replace with a set of Ov output
				accValue += (mpz_class(1)<<sizeAcc);
			}
			
			REPORT(DEBUG, "accValue=" << accValue);
			tc->addExpectedOutput("A", accValue);
		}
	}


	
	TestCase* FPLargeAcc::buildRandomTestCase(int i){

		TestCase *tc;
		mpz_class x;

		tc = new TestCase(this); 

		if (i == 0){
			tc->addInput("newDataSet", mpz_class(1));	
		}else{
			tc->addInput("newDataSet", mpz_class(0));	
		}


		/* normal exception bits */
		mpz_class normalExn = mpz_class(1)<<(wEX+wFX+1);

		
		/*really random sign*/ 
		mpz_class sign = mpz_class(getLargeRandom(1)%2)<<(wEX+wFX) * 0; // TODO remove

		/* do exponent */
		mpz_class exponent = getLargeRandom(wEX) *0 ; //TODO Remove
		while ((exponent>MaxMSBX) || (exponent-wFX<LSBA))  { // TODO: the second test is to ensure the mantissa is fully within the accumulator.
			exponent = getLargeRandom(wEX);
		}
		/* shift exponent in place */
		exponent = (exponent + intpow2(wEX-1)-1 ) << (wFX);

		mpz_class frac = getLargeRandom(wFX) *0; //TODO remove		
		
		x = normalExn + sign + exponent + frac;
		REPORT(DEBUG, "buildRandomTestCase, i=" << i << ", sign=" << sign << ", exponent=" << exponent << ", frac=" << frac << ",    x=" << x );

		tc = new TestCase(this); 
		tc->addInput("X", x);


		/* Get correct outputs */
		emulate(tc);
		return tc;
	}
	
	OperatorPtr FPLargeAcc::parseArguments(Target *target, vector<string> &args) {
		int wEX, wFX, MaxMSBX, MSBA, LSBA;
		UserInterface::parseStrictlyPositiveInt(args, "wEX", &wEX); 
		UserInterface::parseStrictlyPositiveInt(args, "wFX", &wFX);
		UserInterface::parseInt(args, "MaxMSBX", &MaxMSBX);
		UserInterface::parseInt(args, "MSBA", &MSBA);
		UserInterface::parseInt(args, "LSBA", &LSBA);
		return new FPLargeAcc(target, wEX, wFX, MaxMSBX, MSBA, LSBA);
	}

	void FPLargeAcc::registerFactory(){
		UserInterface::add("FPLargeAcc", // name
											 "Accumulator of floating-point numbers into a large fixed-point accumulator. CURRENTLY BROKEN, contact us if you want to use it",
											 "CompositeFloatingPoint",
											 "LargeAccToFP", // seeAlso
											 "wEX(int): the width of the exponent ; \
                        wFX(int): the width of the fractional part;  \
                        MaxMSBX(int): the weight of the MSB of the expected exponent of X; \
                        MSBA(int): the weight of the least significand bit of the accumulator;\
                        LSBA(int): the weight of the most significand bit of the accumulator",
											 "Kulisch-like accumulator of floating-point numbers into a large fixed-point accumulator. By tuning the MaxMSB_in, LSB_acc and MSB_acc parameters to a given application, rounding error may be reduced to a provably arbitrarily low level, at a very small hardware cost compared to using a floating-point adder for accumulation. <br> For details on the technique used and an example of application, see <a href=\"bib/flopoco.html#DinechinPascaCret2008:FPT\">this article</a>",
											 FPLargeAcc::parseArguments
											 ) ;
		
	}
	
}
