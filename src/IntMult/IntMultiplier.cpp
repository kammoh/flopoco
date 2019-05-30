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

#include <cassert>
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

	/**
	 * @brief Compute the size required to store the untruncated product of inputs of a given width
	 * @param wX size of the first input
	 * @param wY size of the second input
	 * @return the number of bits needed to store a product of I<wX> * I<WY>
	 */
	inline unsigned int prodsize(unsigned int wX, unsigned int wY)
	{
		if(wX == 0 || wY == 0)
			return 0;
		
		if(wX == 1)
			return wY;

		if(wY == 1)
			return wX;

		return wX + wY;
	}

    IntMultiplier::IntMultiplier (Operator *parentOp, Target* target_, int wX_, int wY_, int wOut_, bool signedIO_, bool texOutput):
		Operator ( parentOp, target_ ),wX(wX_), wY(wY_), wOut(wOut_),signedIO(signedIO_)
	{
        srcFileName="IntMultiplier";
		setCopyrightString ( "Martin Kumm, Florent de Dinechin, Kinga Illyes, Bogdan Popa, Bogdan Pasca, 2012" );

		ostringstream name;
		name <<"IntMultiplier";
		setNameWithFreqAndUID (name.str());

        // the addition operators need the ieee_std_signed/unsigned libraries
        useNumericStd();

        multiplierUid=parentOp->getNewUId();

		if(wOut == 0)
			wOut = wX+wY;

		unsigned int guardBits = computeGuardBits(
					static_cast<unsigned int>(wX),
					static_cast<unsigned int>(wY),
					static_cast<unsigned int>(wOut)
				);

		REPORT(INFO, "IntMultiplier(): Constructing a multiplier of size " <<
					wX << "x" << wY << " faithfully rounded to bit " << wOut <<
					". Will use " << guardBits << " guard bits.")

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

		BitHeap bitHeap(this, wOut + guardBits);
//		bitHeap = new BitHeap(this, wOut+10); //!!! just for test

		REPORT(INFO, "Creating BaseMultiplierCollection")
//		BaseMultiplierCollection baseMultiplierCollection(parentOp->getTarget(), wX, wY);
		BaseMultiplierCollection baseMultiplierCollection(getTarget());

		baseMultiplierCollection.print();

		REPORT(INFO, "Creating TilingStrategy")
		TilingStrategyBasicTiling tilingStrategy(
				wX, 
				wY, 
				wOut,
				signedIO, 
				&baseMultiplierCollection,
				baseMultiplierCollection.getPreferedMultiplier()
			);

		REPORT(DEBUG, "Solving tiling problem")
		tilingStrategy.solve();

		tilingStrategy.printSolution();

		list<TilingStrategy::mult_tile_t> &solution = tilingStrategy.getSolution();
		if (texOutput) {
            ofstream texfile;
            texfile.open("multiplier.tex");
            if((texfile.rdstate() & ofstream::failbit) != 0) {
                cerr << "Error when opening multiplier.tex file for output. Will not print tiling configuration." << endl;
            } else {
				tilingStrategy.printSolutionTeX(texfile);
                texfile.close();
            }
        }

		schedule();

		branchToBitheap(&bitHeap, solution, prodsize(wX, wY) - (wOut + guardBits));

		bitHeap.startCompression();

		vhdl << tab << "R" << " <= " << bitHeap.getSumName() << range(wOut-1 + guardBits, guardBits) << ";" << endl;
	}

	unsigned int IntMultiplier::computeGuardBits(unsigned int wX, unsigned int wY, unsigned int wOut)
	{
		unsigned int minW = (wX < wY) ? wX : wY;
		unsigned int maxW = (wX < wY) ? wY : wX;

		auto ps = prodsize(wX, wY);
		auto nbDontCare = ps - wOut;

		mpz_class errorBudget{1};
		errorBudget <<= nbDontCare;
		errorBudget -= 1;

		unsigned int nbUnneeded;
		for (nbUnneeded = 1 ; nbUnneeded < nbDontCare ; nbUnneeded += 1) {
			unsigned int bitAtCurCol;
			if (nbUnneeded < minW) {
				bitAtCurCol = nbUnneeded;
			} else if (nbUnneeded <= maxW) {
				bitAtCurCol = minW;
			} else {
				bitAtCurCol = ps -nbUnneeded;
			}
			REPORT(DETAILED, "IntMultiplier::computeGuardBits: Nb bit in column " << nbUnneeded << " : " << bitAtCurCol)
			mpz_class currbitErrorAmount{bitAtCurCol};
			currbitErrorAmount <<= (nbUnneeded - 1);
			errorBudget -= currbitErrorAmount;
			if(errorBudget < 0)
				break;
		}
		nbUnneeded -= 1;

		return nbDontCare - nbUnneeded;
	}

	/**
	 * @brief getZeroMSB if the tile goes out of the multiplier,
	 *		  compute the number of extra zeros that should be inputed to the submultiplier as MSB
	 * @param anchor lsb of the tile
	 * @param tileWidth width of the tile
	 * @param multiplierLimit msb of the overall multiplier
	 * @return
	 */
	inline unsigned int getZeroMSB(int anchor, unsigned int tileWidth, int multiplierLimit)
	{
		unsigned int result = 0;
		if (static_cast<int>(tileWidth) + anchor > multiplierLimit) {
			result = static_cast<unsigned int>(
						static_cast<int>(tileWidth) + anchor - multiplierLimit
						);
		}
		return result;
	}

	/**
	 * @brief getInputSignal Perform the selection of the useful bit and pad if necessary the input signal to fit a certain format
	 * @param msbZeros number of zeros to add left of the selection
	 * @param selectSize number of bits to get from the inputSignal
	 * @param offset where to start taking bits from the input signal
	 * @param lsbZeros number of zeros to append to the selection for right padding
	 * @param signalName input signal name from which to select the bits
	 * @return the string corresponding to the signal selection and padding
	 */
	inline string getInputSignal(unsigned int msbZeros, unsigned int selectSize, unsigned int offset, unsigned int lsbZeros, string const & signalName)
	{
		stringstream s;
		if (msbZeros > 0)
			s << zg(static_cast<int>(msbZeros)) << " & ";

		s << signalName << range(
				 static_cast<int>(selectSize - 1 + offset),
				 static_cast<int>(offset)
			);

		if (lsbZeros > 0)
			s << " & " << zg(static_cast<int>(lsbZeros));

		return s.str();
	}

	void IntMultiplier::branchToBitheap(BitHeap* bitheap, list<TilingStrategy::mult_tile_t> const &solution, unsigned int bitheapLSBWeight)
	{
		size_t i = 0;
		stringstream oname, ofname;
		for(auto & tile : solution) {
			auto& parameters = tile.first;
			auto& anchor = tile.second;

			int xPos = anchor.first;
			int yPos = anchor.second;

			unsigned int xInputLength = parameters.getTileXWordSize();
			unsigned int yInputLength = parameters.getTileYWordSize();
			//unsigned int outputLength = parameters.getOutWordSize();

			unsigned int lsbZerosXIn = (xPos < 0) ? static_cast<unsigned int>(-xPos) : 0;
			unsigned int lsbZerosYIn = (yPos < 0) ? static_cast<unsigned int>(-yPos) : 0;

			unsigned int msbZerosXIn = getZeroMSB(xPos, xInputLength, wX);
			unsigned int msbZerosYIn = getZeroMSB(yPos, yInputLength, wY);

			unsigned int selectSizeX = xInputLength - (msbZerosXIn + lsbZerosXIn);
			unsigned int selectSizeY = yInputLength - (msbZerosYIn + lsbZerosYIn);

			unsigned int effectiveAnchorX = (xPos < 0) ? 0 : static_cast<unsigned int>(xPos);
			unsigned int effectiveAnchorY = (yPos < 0) ? 0 : static_cast<unsigned int>(yPos);

			unsigned int outLSBWeight = effectiveAnchorX + effectiveAnchorY;
			unsigned int truncated = (outLSBWeight < bitheapLSBWeight) ? bitheapLSBWeight - outLSBWeight : 0;
			unsigned int bitHeapOffset = (outLSBWeight < bitheapLSBWeight) ? 0 : outLSBWeight - bitheapLSBWeight;

			unsigned int toSkip = lsbZerosXIn + lsbZerosYIn + truncated;
			unsigned int tokeep = prodsize(selectSizeX, selectSizeY) - toSkip;
			assert(tokeep > 0); //A tiling should not give a useless tile

			oname.str("");
			oname << "tile_" << i << "_output";
			realiseTile(tile, i, oname.str());

			ofname.str("");
			ofname << "tile_" << i << "filtered_output";

			vhdl << declare(.0, ofname.str(), tokeep) << " <= " << oname.str() <<
					range(toSkip + tokeep - 1, toSkip) << ";" << endl;

			bitheap->addSignal(ofname.str(), bitHeapOffset);
			i += 1;
		}
	}



	Operator* IntMultiplier::realiseTile(TilingStrategy::mult_tile_t const & tile, size_t idx, string output_name)
	{
		auto& parameters = tile.first;
		auto& anchor = tile.second;
		int xPos = anchor.first;
		int yPos = anchor.second;

		unsigned int xInputLength = parameters.getTileXWordSize();
		unsigned int yInputLength = parameters.getTileYWordSize();
		//unsigned int outputLength = parameters.getOutWordSize();

		unsigned int lsbZerosXIn = (xPos < 0) ? static_cast<unsigned int>(-xPos) : 0;
		unsigned int lsbZerosYIn = (yPos < 0) ? static_cast<unsigned int>(-yPos) : 0;

		unsigned int msbZerosXIN = getZeroMSB(xPos, xInputLength, wX);
		unsigned int msbZerosYIn = getZeroMSB(yPos, yInputLength, wY);

		unsigned int selectSizeX = xInputLength - (msbZerosXIN + lsbZerosXIn);
		unsigned int selectSizeY = yInputLength - (msbZerosYIn + lsbZerosYIn);

		unsigned int effectiveAnchorX = (xPos < 0) ? 0 : static_cast<unsigned int>(xPos);
		unsigned int effectiveAnchorY = (yPos < 0) ? 0 : static_cast<unsigned int>(yPos);

		auto tileXSig = getInputSignal(msbZerosXIN, selectSizeX, effectiveAnchorX, lsbZerosXIn, "X");
		auto tileYSig = getInputSignal(msbZerosYIn, selectSizeY, effectiveAnchorY, lsbZerosYIn, "Y");

		stringstream nameX, nameY, nameOutput;
		nameX << "tile_" << idx << "_X";
		nameY << "tile_" << idx << "_Y";

		nameOutput << "tile_" << idx << "_Out";

		vhdl << tab << declare(0., nameX.str(), xInputLength) << " <= " << tileXSig << ";" << endl;
		vhdl << tab << declare(0., nameY.str(), yInputLength) << " <= " << tileYSig << ";" << endl;

		string multIn1SigName = nameX.str();
		string multIn2SigName = nameY.str();

		if (parameters.isFlippedXY())
			std::swap(multIn1SigName, multIn2SigName);

		nameOutput.str("");
		nameOutput << "tile_" << idx << "_mult";

		inPortMap(nullptr, "X", multIn1SigName);
		inPortMap(nullptr, "Y", multIn2SigName);
		outPortMap(nullptr, "O", output_name);
		auto mult = parameters.generateOperator(this, getTarget());

		vhdl << instance(mult, nameOutput.str(), false) <<endl;
		return mult;
	}

    string IntMultiplier::addUID(string name, int blockUID)
    {
        ostringstream s;

        s << name << "_m" << multiplierUid;
        if(blockUID != -1)
            s << "b" << blockUID;
        return s.str() ;
    }

    unsigned int IntMultiplier::getOutputLengthNonZeros(
			BaseMultiplierParametrization const & parameter, 
			unsigned int xPos,
			unsigned int yPos, 
			unsigned int totalOffset
		)
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

        for(unsigned int i = 0; i < parameter.getTileXWordSize(); i++){
            for(unsigned int j = 0; j < parameter.getTileYWordSize(); j++){
                if(i + xPos >= totalOffset && j + yPos >= totalOffset){
                    if(i + xPos < (wX + totalOffset) && j + yPos < (wY + totalOffset)){
                        if(parameter.shapeValid(i,j)){
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
    unsigned int IntMultiplier::getLSBZeros(
			BaseMultiplierParametrization const & param,
		   	unsigned int xPos,
			unsigned int yPos,
			unsigned int totalOffset,
		    int mode
		){
        //cout << "mode is " << mode << endl;
        unsigned int max = min(int(param.getMultXWordSize()), int(param.getMultYWordSize()));
        unsigned int zeros = 0;
        unsigned int mode2Zeros = 0;
        bool startCountingMode2 = false;

        for(unsigned int i = 0; i < max; i++){
            unsigned int steps = i + 1; //for the lowest bit make one step (pos 0,0), second lowest 2 steps (pos 0,1 and 1,0) ...
            //the relevant positions for every outputbit lie on a diogonal line.


            for(unsigned int j = 0; j < steps; j++){
                bool bmHasInput = false;
                bool intMultiplierHasBit = false;
                if(param.shapeValid(j, steps - (j + 1))){
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
        bool signedIO,superTile,texOuput;
		UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
		UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
		UserInterface::parsePositiveInt(args, "wOut", &wOut);
		UserInterface::parseBoolean(args, "signedIO", &signedIO);
		UserInterface::parseBoolean(args, "superTile", &superTile);
        UserInterface::parseBoolean(args, "texOutput", &texOuput);
        return new IntMultiplier(parentOp, target, wX, wY, wOut, signedIO, texOuput);
	}


	
	void IntMultiplier::registerFactory(){
		UserInterface::add("IntMultiplier", // name
											 "A pipelined integer multiplier.",
											 "BasicInteger", // category
											 "", // see also
											 "wX(int): size of input X; wY(int): size of input Y;\
                        wOut(int)=0: size of the output if you want a truncated multiplier. 0 for full multiplier;\
                        signedIO(bool)=false: inputs and outputs can be signed or unsigned;\
                        superTile(bool)=false: if true, attempts to use the DSP adders to chain sub-multipliers. This may entail lower logic consumption, but higher latency.;\
                        texOutput(bool)=false: if true, generate a tex document with the tiling represented in it", // This string will be parsed
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
