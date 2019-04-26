/*
  An integer multiplier for FloPoCo

  Authors:  Martin Kumm with parts of previous IntMultiplier of Bogdan Pasca, F de Dinechin, Matei Istoan, Kinga Illyes and Bogdan Popa spent

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2017.
  All rights reserved.
*/

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "IntAddSubCmp/IntAdder.hpp"
#include "BaseMultiplierCollection.hpp"
#include "TilingStrategyOptimalILP.hpp"
#include "TilingStrategyBasicTiling.hpp"

using namespace std;

namespace flopoco {

    IntMultiplier::IntMultiplier (Operator *parentOp, Target* target_, int wX_, int wY_, int wOut_, bool signedIO_):
        Operator ( parentOp, target_ ),wX(wX_), wY(wY_), wOut(wOut_),signedIO(signedIO_)
	{
        srcFileName="IntMultiplier";
		setCopyrightString ( "Martin Kumm, Florent de Dinechin, Kinga Illyes, Bogdan Popa, Bogdan Pasca, 2012" );

		ostringstream name;
		name <<"IntMultiplier";
		setNameWithFreqAndUID ( name.str() );

        // the addition operators need the ieee_std_signed/unsigned libraries
        useNumericStd();

        multiplierUid=parentOp->getNewUId();

		if(wOut == 0)
			wOut = wX+wY;

		string xname="X";
		string yname="Y";

        // Set up the IO signals
        addInput(xname, wX, true);
        addInput(yname, wY, true);
        addOutput("R", wOut, 2, true);

		// The larger of the two inputs
		vhdl << tab << declare(addUID("XX"), wX, true) << " <= " << xname << " ;" << endl;
		vhdl << tab << declare(addUID("YY"), wY, true) << " <= " << yname << " ;" << endl;

		if(target_->plainVHDL()) {
			vhdl << tab << declareFixPoint("XX",signedIO,-1, -wX) << " <= " << (signedIO?"signed":"unsigned") << "(" << xname <<");" << endl;
            vhdl << tab << declareFixPoint("YY",signedIO,-1, -wY) << " <= " << (signedIO?"signed":"unsigned") << "(" << yname <<");" << endl;
            vhdl << tab << declareFixPoint("RR",signedIO,-1, -wX-wY) << " <= XX*YY;" << endl;
            vhdl << tab << "R <= std_logic_vector(RR" << range(wX+wY-1, wX+wY-wOut) << ");" << endl;

            return;
        }

//		bitHeap = new BitHeap(this, wOut);
		bitHeap = new BitHeap(this, wOut+10); //!!! just for test

		REPORT(DEBUG, "Creating BaseMultiplierCollection");
//		BaseMultiplierCollection baseMultiplierCollection(parentOp->getTarget(), wX, wY);
		BaseMultiplierCollection baseMultiplierCollection(getTarget(), wX, wY);

		REPORT(DEBUG, "Creating TilingStrategy");
		TilingStrategyBasicTiling tilingStrategy(
				wX, 
				wY, 
				wOut,
				signedIO, 
				&baseMultiplierCollection,
				baseMultiplierCollection.getPreferedMultiplier()
			);

		REPORT(DEBUG, "Solving tiling problem");
		tilingStrategy.solve();

		list<TilingStrategy::mult_tile_t> &solution = tilingStrategy.getSolution();

		unsigned int posInList = 0;    //needed as id to differ between inputvectors
        unsigned int totalOffset = 0; //!!! multiplierSolutionParser->getOffset();

        for(auto it = solution.begin(); it != solution.end(); it++){
            base_multiplier_id_t type = (*it).first;
            unsigned int xPos = (*it).second.first;
            unsigned int yPos = (*it).second.second;

            BaseMultiplier *baseMultiplier = baseMultiplierCollection.getBaseMultiplier(type);

            if(!baseMultiplier) THROWERROR("Multiplier of type " << type << " does not exist");

            cout << "found multiplier " << baseMultiplier->getName() << " (type " <<  type << ") at position (" << (int) (xPos-totalOffset) << "," << (int) (yPos-totalOffset) << ")" << endl;

            //unsigned int outputLength = getOutputLength(baseMultiplier, xPos, yPos);
			//Operator *op = baseMultiplier->getOperator();
            //lets assume, that baseMultiplier ix 3x3, Unsigned ...

            unsigned int xInputLength = baseMultiplier->getXWordSize();
            unsigned int yInputLength = baseMultiplier->getYWordSize();
            unsigned int outputLength = baseMultiplier->getOutWordSize();

            unsigned int lsbZerosInBM = getLSBZeros(baseMultiplier, xPos, yPos, totalOffset, 1);  //normally, starting position of output is computed by xPos + yPos. But if the output starts at an higher weight, lsbZeros counts the offset

            unsigned int completeOffset = getLSBZeros(baseMultiplier, xPos, yPos, totalOffset, 0);
            unsigned int resultVectorOffset = completeOffset - lsbZerosInBM;
            unsigned int xInputNonZeros = xInputLength;
            unsigned int yInputNonZeros = yInputLength;
            unsigned int realBitHeapPosOffset = getLSBZeros(baseMultiplier, xPos, yPos, totalOffset, 2);

            unsigned int outputLengthNonZeros = xInputNonZeros + yInputNonZeros;

            outputLengthNonZeros = getOutputLengthNonZeros(baseMultiplier, xPos, yPos, totalOffset);

//            setCycle(0); //reset to cycle 0

			Operator *op = baseMultiplier->generateOperator(parentOp, getTarget());
//            nextCycle(); //add register after each multiplier

			UserInterface::addToGlobalOpList(op);

			string outputVectorName = placeSingleMultiplier(op, xPos, yPos, xInputLength, yInputLength, outputLength, xInputNonZeros, yInputNonZeros, totalOffset, posInList);

//            syncCycleFromSignal(outputVectorName);

            unsigned int startWeight = 0;
            if(xPos + yPos > (2 * totalOffset)){
                startWeight = xPos + yPos - (2 * totalOffset);
            }
            startWeight += realBitHeapPosOffset;

            unsigned int outputVectorLSBZeros = 0;
            /*
            if(2 * totalOffset > xPos + yPos){
                outputVectorLSBZeros = (2 * totalOffset) - (xPos + yPos);
            }
            */

            //TODO: check cases, where the lsbZerosInBM != 0
            //outputVectorLSBZeros are the amount of lsb Bits of the outputVector, which we have to discard in the vhdl-code.
            outputVectorLSBZeros = getLSBZeros(baseMultiplier, xPos, yPos, totalOffset, 0) - getLSBZeros(baseMultiplier, xPos, yPos, totalOffset, 1);
            cout << "for position " << xPos << "," << yPos << " we got outputVectorLSBZeros = " << outputVectorLSBZeros << endl;
            cout << "outputLengthNonZeros = " << outputLengthNonZeros << endl;
            cout << "startWeight is " << startWeight << endl;
            cout << "complete offset is " << completeOffset << endl;
            cout << "resultVectorOffset is " << resultVectorOffset << endl;
            cout << "realBitHeapPosOffset is " << realBitHeapPosOffset << endl;

            bool isSigned = false;
            //todo: signed case: see line 1110
            if(!isSigned){
//                setCycleFromSignal(outputVectorName);	//we assume that the whole vector has the same cycle
                for(unsigned int i = resultVectorOffset; i < outputLengthNonZeros - lsbZerosInBM; i++){
                    ostringstream s;
                    s << outputVectorName << of(i);
                    bitHeap->addBit(startWeight + (i - resultVectorOffset), s.str());

                    //bitHeap->addBit(startWeight + (i - resultVectorOffset), s.str(), "", 1, getCycleFromSignal(outputVectorName));
                }
            }

            posInList++;
        }

		bitHeap->startCompression();

		vhdl << tab << "R" << " <= " << bitHeap-> getSumName() << range(wOut-1, 0) << ";" << endl;

	}

    //totalOffset is normally zero or twelve. The whole big multiplier is moved by totalOffset-Bits in x and y direction to support hard multiplier which protude the right and lower borders.
    string IntMultiplier::placeSingleMultiplier(Operator* op, unsigned int xPos, unsigned int yPos, unsigned int xInputLength, unsigned int yInputLength, unsigned int outputLength, unsigned int xInputNonZeros, unsigned int yInputNonZeros, unsigned int totalOffset, unsigned int id){

        //xPos, yPos is the lower right corner of the multiplier
        //xInputLength and yInputLength is the width of the inputs


        cout << "(" << xPos << ";" << yPos << ") "; // << endl;
        int blockUid = 876;
        //declaring x-input:
        unsigned int xStart = xPos;
        unsigned int lowXZeros = 0;
        if(xPos < totalOffset){
            lowXZeros = totalOffset - xPos;
            xStart = totalOffset;
        }
        unsigned int xEnd = (xPos + xInputLength) - 1;
        unsigned int highXZeros = 0;
        if(xPos + xInputLength > (wX + totalOffset)){
            highXZeros = (xPos + xInputLength) - (wX + totalOffset);
            xEnd = (wX + totalOffset) - 1;
        }
        //unsigned int xNonZeros = xInputLength - (lowXZeros + highXZeros);

        vhdl << tab << declare(join(addUID("x",blockUid),"_",id),xInputLength) << " <= ";
        if(highXZeros > 0){
//			cout << "highXzeros: " << highXZeros << endl;
            vhdl << zg((int)highXZeros,0) << " & ";
        }
        vhdl << addUID("XX") << range(xEnd - totalOffset, xStart - totalOffset);
        if(lowXZeros > 0){
//			cout << "lowXzeros: " << lowXZeros << endl;
            vhdl << " & " << zg((int)lowXZeros,0);
        }
        vhdl << ";" << endl;

        cout << "totalOffset=" << totalOffset << ", xStart=" << xStart << ", xEnd=" << xEnd << ", highXZeros=" << highXZeros << ", lowXZeros=" << lowXZeros << endl;


        //declaring y-input:
        unsigned int yStart = yPos;
        unsigned int lowYZeros = 0;
        if(yPos < totalOffset){
            lowYZeros = totalOffset - yPos;
            yStart = totalOffset;
        }
        unsigned int yEnd = (yPos + yInputLength) - 1;
        unsigned int highYZeros = 0;
        if(yPos + yInputLength + lowYZeros > (wY + totalOffset)){
            highYZeros = (yPos + yInputLength) - (wY + totalOffset);
            yEnd = (wY + totalOffset) - 1;
        }
        //unsigned int yNonZeros = yInputLength - (lowYZeros + highYZeros);

        vhdl << tab << declare(join(addUID("y",blockUid),"_",id),yInputLength) << " <= ";
        if(highYZeros > 0){
//			cout << "highYzeros: " << highYZeros << endl;
            vhdl << zg((int)highYZeros,0) << " & ";
        }
        vhdl << addUID("YY") << range(yEnd - totalOffset, yStart - totalOffset);
        if(lowYZeros > 0){
            cout << "lowYzeros: " << lowYZeros << endl;
            vhdl << " & " << zg((int)lowYZeros,0);
        }
        vhdl << ";" << endl;

        //real thing
        inPortMap("X", join(addUID("x",blockUid),"_",id));
        inPortMap("Y", join(addUID("y",blockUid),"_",id));
        outPortMap("R", join(addUID("r",blockUid),"_",id));
        vhdl << instance(op, join(addUID("Mult",blockUid),"_", id));
        useSoftRAM(op);

        cout << join(addUID("r",blockUid),"_",id) << ":" << getSignalByName(join(addUID("r",blockUid),"_",id))->width() << endl;

        return join(addUID("r",blockUid),"_",id);

        /*
        vhdl << tab << declare(join(addUID("input_x_y", blockUid), "_", id), xInputLength + yInputLength) << " <= ";
        vhdl << join(addUID("y",blockUid),"_",id) << " & " << join(addUID("x",blockUid),"_",id) << ";" << endl;

        inPortMap(op, "X",join(addUID("input_x_y", blockUid), "_", id));
        outPortMap(op, "Y", join(addUID("r",blockUid),"_",id));
        vhdl << instance(op, join(addUID("Mult",blockUid),"_", id));
        useSoftRAM(op);

        return join(addUID("r",blockUid),"_",id);
        */
    }

    string IntMultiplier::addUID(string name, int blockUID)
    {
        ostringstream s;

        s << name << "_m" << multiplierUid;
        if(blockUID != -1)
            s << "b" << blockUID;
        return s.str() ;
    }

    unsigned int IntMultiplier::getOutputLengthNonZeros(BaseMultiplier* bm, unsigned int xPos, unsigned int yPos, unsigned int totalOffset)
    {
//        unsigned long long int sum = 0;
        mpfr_t sum;
        mpfr_t two;

        mpfr_inits(sum, two, NULL);

        mpfr_set_si(sum, 1, GMP_RNDN); //init sum to 1 as at the end the log(sum+1) has to be computed
        mpfr_set_si(two, 2, GMP_RNDN);

/*
    cout << "sum=";
    mpfr_out_str(stdout, 10, 0, sum, MPFR_RNDD);
    cout << endl;
    flush(cout);
*/

        for(int i = 0; i < bm->getXWordSize(); i++){
            for(int j = 0; j < bm->getYWordSize(); j++){
                if(i + xPos >= totalOffset && j + yPos >= totalOffset){
                    if(i + xPos < (wX + totalOffset) && j + yPos < (wY + totalOffset)){
                        if(bm->shapeValid(i,j)){
                            //sum += one << (i + j);
                            mpfr_t r;
                            mpfr_t e;
                            mpfr_inits(r, e, NULL);

                            mpfr_set_si(e, i+j, GMP_RNDD);
                            mpfr_pow(r, two, e, GMP_RNDD);
                            mpfr_add(sum, sum, r, GMP_RNDD);

//                            cout << " added 2^" << (i+j) << endl;
                        }
                        else{
                            //cout << "not true " << i << " " << j << endl;
                        }
                    }
                    else{
                        //cout << "upper borders " << endl;
                    }
                }
                else{
                    //cout << "lower borders" << endl;
                }
            }
        }

        mpfr_t length;
        mpfr_inits(length, NULL);
        mpfr_log2(length, sum, GMP_RNDU);
        mpfr_ceil(length,length);

        unsigned long length_ul = mpfr_get_ui(length, GMP_RNDD);
//        cout << " outputLength has a length of " << length_ul << endl;
        return (int) length_ul;
    }

    /*this function computes the amount of zeros at the lsb position (mode 0). This is done by checking for every output bit position, if there is a) an input of the current BaseMultiplier, and b) an input of the IntMultiplier. if a or b or both are false, then there is a zero at this position and we check the next position. otherwise we break. If mode==1, only condition a) is checked */
    /*if mode is 2, only the offset for the bitHeap is computed. this is done by counting how many diagonals have a position, where is an IntMultiplierInput but not a BaseMultiplier input. */
    unsigned int IntMultiplier::getLSBZeros(BaseMultiplier* bm, unsigned int xPos, unsigned int yPos, unsigned int totalOffset, int mode){
        //cout << "mode is " << mode << endl;
        unsigned int max = min(bm->getXWordSize(), bm->getYWordSize());
        unsigned int zeros = 0;
        unsigned int mode2Zeros = 0;
        bool startCountingMode2 = false;

        for(unsigned int i = 0; i < max; i++){
            unsigned int steps = i + 1; //for the lowest bit make one step (pos 0,0), second lowest 2 steps (pos 0,1 and 1,0) ...
            //the relevant positions for every outputbit lie on a diogonal line.


            for(unsigned int j = 0; j < steps; j++){
                bool bmHasInput = false;
                bool intMultiplierHasBit = false;
                if(bm->shapeValid(j, steps - (j + 1))){
                    bmHasInput = true;
                }
                if(bmHasInput && mode == 1){
                    return zeros;   //only checking condition a)
                }
                //x in range?
                if((xPos + j >= totalOffset && xPos + j < (wX + totalOffset))){
                    //y in range?
                    if((yPos + (steps - (j + 1))) >= totalOffset && (yPos + (steps - (j + 1))) < (wY + totalOffset)){
                        intMultiplierHasBit = true;
                    }
                }
                if(mode == 2 && yPos + xPos + (steps - 1) >= 2 * totalOffset){
                    startCountingMode2 = true;
                }
                //cout << "position " << j << "," << (steps - (j + 1)) << " has " << bmHasInput << " and " << intMultiplierHasBit << endl;
                if(bmHasInput && intMultiplierHasBit){
                    //this output gets some values. So finished computation and return
                    if(mode == 0){
                        return zeros;
                    }
                    else{	//doesnt matter if startCountingMode2 is true or not. mode2Zeros are being incremented at the end of the diagonal
                        return mode2Zeros;
                    }

                }
            }

            zeros++;
            if(startCountingMode2 == true){
                mode2Zeros++;
            }
        }
        if(mode != 2){
            return zeros;       //if reached, that would mean the the bm shares no area with IntMultiplier
        }
        else{
            return mode2Zeros;
        }
    }

    void IntMultiplier::emulate (TestCase* tc)
    {
        mpz_class svX = tc->getInputValue("X");
        mpz_class svY = tc->getInputValue("Y");
        mpz_class svR;

        if(!signedIO)
        {
            svR = svX * svY;
        }
        else
        {
            // Manage signed digits
            mpz_class big1 = (mpz_class(1) << (wX));
            mpz_class big1P = (mpz_class(1) << (wX-1));
            mpz_class big2 = (mpz_class(1) << (wY));
            mpz_class big2P = (mpz_class(1) << (wY-1));

            if(svX >= big1P)
                svX -= big1;

            if(svY >= big2P)
                svY -= big2;

            svR = svX * svY;
        }

		if(negate)
			svR = -svR;

        // manage two's complement at output
        if(svR < 0)
            svR += (mpz_class(1) << wFullP);

        if(wOut>=wFullP)
            tc->addExpectedOutput("R", svR);
        else	{
            // there is truncation, so this mult should be faithful
            svR = svR >> (wFullP-wOut);
            tc->addExpectedOutput("R", svR);
            svR++;
            svR &= (mpz_class(1) << (wOut)) -1;
            tc->addExpectedOutput("R", svR);
        }
    }


	void IntMultiplier::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		mpz_class x, y;

		// 1*1
		x = mpz_class(1);
		y = mpz_class(1);
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// -1 * -1
        x = (mpz_class(1) << wX) -1;
        y = (mpz_class(1) << wY) -1;
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// The product of the two max negative values overflows the signed multiplier
        x = mpz_class(1) << (wX -1);
        y = mpz_class(1) << (wY -1);
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);
	}




	
	OperatorPtr IntMultiplier::parseArguments(OperatorPtr parentOp, Target *target, std::vector<std::string> &args) {
		int wX,wY, wOut ;
		bool signedIO,superTile;
		UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
		UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
		UserInterface::parsePositiveInt(args, "wOut", &wOut);
		UserInterface::parseBoolean(args, "signedIO", &signedIO);
		UserInterface::parseBoolean(args, "superTile", &superTile);
        return new IntMultiplier(parentOp, target, wX, wY, wOut, signedIO);
	}


	
	void IntMultiplier::registerFactory(){
		UserInterface::add("IntMultiplier", // name
											 "A pipelined integer multiplier.",
											 "BasicInteger", // category
											 "", // see also
											 "wX(int): size of input X; wY(int): size of input Y;\
                        wOut(int)=0: size of the output if you want a truncated multiplier. 0 for full multiplier;\
                        signedIO(bool)=false: inputs and outputs can be signed or unsigned;\
                        superTile(bool)=false: if true, attempts to use the DSP adders to chain sub-multipliers. This may entail lower logic consumption, but higher latency.", // This string will be parsed
											 "", // no particular extra doc needed
											IntMultiplier::parseArguments,
											IntMultiplier::unitTest
											 ) ;
	}


	TestList IntMultiplier::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		//ToDo: Add further tests:
		int wX=24;
		int wY=24;
		{
			paramList.push_back(make_pair("wX", to_string(wX)));
			paramList.push_back(make_pair("wY", to_string(wY)));
			testStateList.push_back(paramList);
			paramList.clear();
		}

		return testStateList;
	}

}
