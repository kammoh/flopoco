/*
  Multipartites Tables for FloPoCo

  Authors: Franck Meyer, Florent de Dinechin

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2014.
  All rights reserved.
  */

#include "Multipartite.hpp"
#include "FixFunctionByMultipartiteTable.hpp"
#include "FixFunction.hpp"

#include "../utils.hpp"
#include "../BitHeap/BitHeap.hpp"
//#include "QuineMcCluskey.h"

#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace flopoco;

namespace flopoco
{
	//--------------------------------------------------------------------------------------- Constructors
	Multipartite::Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction *f_, int inputSize_, int outputSize_):
		f(f_), inputSize(inputSize_), outputSize(outputSize_), mpt(mpt_)
	{
		inputRange = 1<<inputSize_;
		epsilonT = 1.0 / (1<<(outputSize+1));
	}

	Multipartite::Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable *mpt_):
		f(f_), m(m_), alpha(alpha_), beta(beta_), gammai(gammai_), betai(betai_), mpt(mpt_)
	{
		inputRange = mpt_->obj->inputRange;
		inputSize = mpt_->obj->inputSize;
		outputSize = mpt_->obj->outputSize;
		pi = vector<int>(m);
		pi[0] = 0;
		for (int i = 1; i < m; i++)
			pi[i] = pi[i-1] + betai[i-1];

		epsilonT = 1 / (1<<(outputSize+1));
		computeMathErrors();
	}

	//------------------------------------------------------------------------------------ Private methods

	void Multipartite::computeGuardBits()
	{
		guardBits =  (int) floor(-outputSize - 1
								 + log2(m /
										(intpow2(-outputSize - 1) - mathError)));
	}


	/** Computes the math errors from the values precalculated in the multipartiteTable */
	void Multipartite::computeMathErrors()
	{
		mathError=0;
		for (int i=0; i<m; i++)
		{
			mathError += mpt->oneTableError[pi[i]][betai[i]][gammai[i]];
		}
	}


	void Multipartite::computeSizes()
	{
		int size = (1<<alpha) * (outputSize + guardBits);
		outputSizeTOi = vector<int>(m);
		sizeTOi = vector<int>(m);
		for (int i=0; i<m; i++)
		{
			computeTOiSize(i);
			size += sizeTOi[i];
		}
		totalSize = size;
	}


	void Multipartite::computeTOiSize(int i)
	{
		double r1, r2, r;
		double delta = deltai(i);
		r1 = abs( delta * si(i,0) );
		r2 = abs( delta * si(i,  (1<<gammai[i]) - 1));
		r = max(r1,r2);
		// This r is a float, the following line scales it to the output
		outputSizeTOi[i]= (int)floor( outputSize + guardBits + log2(r));

		// the size computation takes into account that we exploit the symmetry by using the xor trick
		sizeTOi[i] = (1<< (gammai[i]+betai[i]-1)) * outputSizeTOi[i];
	}


	/** Just as in the article */
	double Multipartite::deltai(int i)
	{
		return mui(i, (1<<betai[i])-1) - mui(i, 0);
	}


	/** Just as in the article  */
	double Multipartite::mui(int i, int Bi)
	{
		int wi = inputSize;
		return  (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) *  intpow2(-wi+pi[i]) * Bi;
	}


	/** Just as in the article */
	double Multipartite::si(int i, int Ai)
	{
		int wi = inputSize;
		double xleft = (f->signedIn ? -1 : 0)
				+ (f->signedIn ? 2 : 1) * intpow2(-gammai[i])  * ((double)Ai);
		double xright= (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * ((intpow2(-gammai[i]) * ((double)Ai+1)) - intpow2(-wi+pi[i]+betai[i]));
		double delta = deltai(i);
		double si =  (f->eval(xleft + delta)
					  - f->eval(xleft)
					  + f->eval(xright+delta)
					  - f->eval(xright) )    / (2*delta);
		return si;
	}

	// counts the size of the input
	// more or less intlog2(abs(val))
	static int countBits(mpz_class val)
	{
		mpz_class calcval = abs(val);
		int count = 0;
		while((calcval >> count) != 0)
			count++;
		if(val >= 0)
			return count;
		else
			return count + 1;
	}



	void Multipartite::compressAndUpdateTIV()
	{
		int maxBitsCounted;
		int size = -1;
		int betterS;
		string srcFileName = mpt->getSrcFileName(); // for REPORT to work

		// First find the optimal value of s
		for(int s = 0; s < alpha; s++)
		{
			int maxBits = 0;
			for(int i = 0; i < (1<<(alpha - s)); i++)
			{
				mpz_class value = tiv[ i*(1<<s) ];
				for(int j = 0; j < (1<<s); j++)
				{
					mpz_class diff = tiv[ i * (1<<s) + j]  -  value;
					maxBits = max(maxBits, countBits(diff));
				}
			}
			int sSize = (outputSize+guardBits)*(1<<(alpha - s))  +  maxBits*(1<<alpha) ;

			if(sSize < size || size < 0)			{
				betterS = s;
				size = sSize;
				maxBitsCounted = maxBits;
				REPORT(DEBUG, "found better s=" << s << ", maxBitsCounted=" << maxBitsCounted);
			}
		}

		// set the class attribute 
		rho = alpha - betterS;

		REPORT(DETAILED, "TIV compression found: s=" << betterS << ", rho=" << rho << ", diffOutputSize=" << maxBitsCounted
					 << ", size=" << size << " (was " << ((outputSize +guardBits)<<alpha) <<")");


		// Now actually fill the table
		for(int i = 0; i < (1<<rho); i++)		{
			mpz_class value = tiv[ i*(1<<betterS) ];
			aTIV.push_back(mpz_class(value));
			for(int j = 0; j < (1<<betterS); j++)
			{
				mpz_class diff = tiv[ i*(1<<betterS) + j] - value;
				// managing two's complement
				diff = (diff + (1 << (maxBitsCounted+1)))    &  ((1<<(maxBitsCounted+1)) - 1);
				diffTIV.push_back(diff);
				REPORT(FULL, "building diffTIV, i=" << i << " j=" << j);
			}
		}
		// set up attributes
		outputSizeATIV = outputSize+guardBits;
		outputSizeDiffTIV = maxBitsCounted;
	}



	//------------------------------------------------------------------------------------- Public methods

	void Multipartite::buildGuardBitsAndSizes()
	{
		computeGuardBits();
		computeSizes();
	}


	void Multipartite::mkTables(Target* target)
	{
		// The TIV
		vector<mpz_class> tivval;
		for (int j=0; j < 1<<alpha; j++) {
				tiv.push_back(TIVFunction(j));
		}
		//		tiv = new Table(mpt, target, tivval, mpt->getName() + "_TIV", alpha, f->wOut + guardBits);
		
		// The TOIs
		//toi = vector<Table*>(m);
		for(int i = 0; i < m; ++i)
		{
			vector<mpz_class> values;
			for (int j=0; j < 1<<(gammai[i]+betai[i]-1); j++) {
				values.push_back(TOiFunction(j, i));
			}
			toi.push_back(values);
			
			//			string name = mpt->getName() + join("_TO",i);
			//toi[i] = new Table(mpt, target, values, name, gammai[i] + betai[i] - 1, outputSizeTOi[i]-1);
		}

		// TIV compression as per Hsiao
		compressAndUpdateTIV();
	}



	//------------------------------------------------------------------------------------- Public classes
	mpz_class Multipartite::TOiFunction(int x, int ti)
	{
		int TOi;
		double dTOi;

		double y, slope;
		int Ai, Bi;

		// to lighten the notation and bring them closer to the paper
		int wI = inputSize;
		int wO = outputSize;
		int g = guardBits;

		Ai = x >> (betai[ti]-1);
		Bi = x - (Ai << (betai[ti]-1));
		slope = si(ti,Ai); // mathematical slope

		y = slope * intpow2(-wI + pi[ti]) * (Bi+0.5);
		dTOi = y * intpow2(wO+g) * intpow2(f->lsbIn - f->lsbOut) * intpow2(inputSize - outputSize);
		TOi = (int)floor(dTOi);
	 

		return mpz_class(TOi);
	}




	mpz_class Multipartite::TIVFunction(int x)
	{
		int TIVval;
		double dTIVval;

		double y, yl, yr;
		double offsetX(0);
		double offsetMatula;

		// to lighten the notation and bring them closer to the paper
		int wO = outputSize;
		int g = guardBits;


		for (unsigned int i = 0; i < pi.size(); i++) {
			offsetX+= (1<<pi[i]) * ((1<<betai[i]) -1);
		}

		offsetX = offsetX / ((double)inputRange);

		if (m % 2 == 1) // odd
					offsetMatula = 0.5*(m-1);
				else //even
					offsetMatula = 0.5 * m;

		offsetMatula += intpow2(g-1); //for the final rounding

		double xVal = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * x * intpow2(-alpha);
		// we compute the function at the left and at the right of
		// the interval
		yl = f->eval(xVal) * intpow2(f->lsbIn - f->lsbOut) * intpow2(inputSize - outputSize);
		yr = f->eval(xVal+offsetX) * intpow2(f->lsbIn - f->lsbOut) * intpow2(inputSize - outputSize);

		// and we take the mean of these values
		y =  0.5 * (yl + yr);
		dTIVval = y * intpow2(g + wO);

		if(m % 2 == 1)
			TIVval = (int) round(dTIVval + offsetMatula);
		else
			TIVval = (int) floor(dTIVval + offsetMatula);

		return mpz_class(TIVval);
	}


	string 	Multipartite::descriptionString(){
		ostringstream s;
		s << "alpha=" << alpha << ", rho=" << rho << "   ";
		for (size_t i =0; i< gammai.size(); i++) {
			s << "   gamma" << i << "=" << gammai[i] << " beta"<<i<<"=" << betai[i]; 
		}
		return s.str();
	}


	
#if 0
	Multipartite::compressedTIV::compressedTIV(Target *target, Table *compressedAlpha, Table *compressedout, int s, int wOC, int wI, int wO)
		: Operator(target), wO_corr(wOC)
	{
		stringstream name("");
		name << "Compressed_TIV_decoder_" << getuid();
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Franck Meyer (2015)");

		addInput("X", wI);
		addOutput("Y1", wO);
		addOutput("Y2", wOC);

		vhdl << tab << declare("XComp", wI-s) << " <= X" << range(wI-1, s) << ";" << endl;
		addSubComponent(compressedAlpha);
		inPortMap(compressedAlpha, "X", "XComp");
		outPortMap(compressedAlpha, "Y", "Y1", false);

		vhdl << tab << instance(compressedAlpha, "TIV_compressed_part");


		addSubComponent(compressedout);
		inPortMap(compressedout, "X", "X");
		outPortMap(compressedout, "Y", "Y2", false);

		vhdl << tab << instance(compressedout, "TIV_correction_part");
	}
#endif


}

