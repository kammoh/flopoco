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
#include <cmath> //for abs(double)

using namespace std;
using namespace flopoco;

namespace flopoco
{
	//--------------------------------------------------------------------------------------- Constructors
	Multipartite::Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction *f_, int inputSize_, int outputSize_):
		f(f_), inputSize(inputSize_), outputSize(outputSize_), rho(-1), mpt(mpt_)
	{
		inputRange = 1<<inputSize_;
		epsilonT = 1.0 / (1<<(outputSize+1));
	}

	Multipartite::Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable *mpt_):
		f(f_), m(m_), alpha(alpha_), rho(-1), beta(beta_), gammai(gammai_), betai(betai_), mpt(mpt_)
	{
		inputRange = 1 << f->wIn;
		inputSize = f->wIn;
		outputSize = f->wOut;
		pi = vector<int>(m);
		pi[0] = 0;
		for (int i = 1; i < m; i++)
			pi[i] = pi[i-1] + betai[i-1];

		epsilonT = 1 / (1<<(outputSize+1));

		// compute approximation error out of the precomputed tables of MPT
		// poor encapsulation: mpt will test it it is low enough
		mathError=0;
		for (int i=0; i<m; i++)
		{
			mathError += mpt->oneTableError[pi[i]][betai[i]][gammai[i]];
		}
	}

	//------------------------------------------------------------------------------------ Private methods






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



	void Multipartite::computeTIVCompressionParameters() {
		string srcFileName = mpt->getSrcFileName(); // for REPORT to work

		REPORT(FULL, "Entering computeTIVCompressionParameters: alpha=" << alpha << "  uncompressed size=" << sizeTIV); 

		int bestS = 0; 
		for(int s = 1; s < alpha-1; s++) {
			
#if 0 // This works without the convexity hypothesis but is hugely slower
			vector<int64_t> deltas;
			int64_t deltaMax = 0;
			for(int i = 0; i < (1<<(alpha - s)); i++)	{
				int64_t valLeft  = TIVFunction( i<<s );
				int64_t valRight = TIVFunction( ((i+1)<<s)-1 );
				int64_t delta = abs(valLeft-valRight);
				deltas.push_back(delta);
				if (delta>deltaMax)
					deltaMax=delta;
			}
			int deltaBits = intlog2(deltaMax);
			int64_t slack = 0;
			for(int i = 0; i < (1<<(alpha - s)); i++)	{
				slack = max(slack, (1<<deltaBits)-1-deltas[i]);				
			}
#else
			// Under convexity hypothesis, the maximum slack is either left or right of the interval
			// (I think we assume here convexity of the first derivative, too)
			int64_t  valLeft, valRight, deltaLeft, deltaRight, deltaMax, slackMax;
			int i = 0;
			valLeft	 = TIVFunction( i<<s );
			valRight = TIVFunction( ((i+1)<<s)-1 );
			deltaLeft = abs(valLeft-valRight);

			i = (1<<(alpha - s))-1;
			valLeft	 = TIVFunction( i<<s );
			valRight = TIVFunction( ((i+1)<<s)-1 );
			deltaRight = abs(valLeft-valRight);

			deltaMax = deltaLeft>deltaRight? deltaLeft: deltaRight;
			int deltaBits = intlog2(deltaMax);
			slackMax = (1<<deltaBits)-1-deltaMax;			
#endif
			int saved_LSBs_in_ATIV = intlog2(slackMax);
			int tempRho = alpha - s;
			int tempSizeATIV = (outputSize+guardBits-saved_LSBs_in_ATIV)<<tempRho;
			int tempSizeDiffTIV = deltaBits<<alpha;
			int tempCompressedSize = tempSizeATIV + tempSizeDiffTIV; 
			REPORT(FULL, "computeTIVCompressionParameters: alpha=" << alpha << "  s=" << s << " compressedSize=" << tempCompressedSize << "  slack=" << slackMax <<"  saved_LSBs_in_ATIV=" << saved_LSBs_in_ATIV); 
			if (tempCompressedSize < sizeTIV) {
				bestS = s;
				// tentatively set the attributes of the Multipartite class
				rho = alpha - s;
				sizeATIV = tempSizeATIV;
				sizeDiffTIV = tempSizeDiffTIV;
				sizeTIV = tempCompressedSize;
				totalSize = sizeATIV + sizeDiffTIV;
				for (int i=0; i<m; i++)		{
					totalSize += sizeTOi[i];
				}
				
				nbZeroLSBsInATIV = saved_LSBs_in_ATIV; 
				outputSizeATIV = outputSize+guardBits-nbZeroLSBsInATIV;				
				outputSizeDiffTIV = deltaBits;
			}				
		}
		REPORT(FULL, "Exiting computeTIVCompressionParameters: bestS=" << bestS << "  rho=" << rho  << "  nbZeroLSBsInATIV=" << nbZeroLSBsInATIV<< "  sizeTIV=" << sizeTIV  << "  outputSizeATIV" << outputSizeATIV); 
	}




	//------------------------------------------------------------------------------------- Public methods

	void Multipartite::buildGuardBitsAndSizes(bool computeGuardBits)
	{

		if(computeGuardBits) {
			guardBits =  (int) ceil(-outputSize - 1
															 + log2(m /(intpow2(-outputSize - 1) - mathError)));
			// With a slack of 1 it works 97% of the time
			guardBits += mpt->guardBitsSlack; 
		}
		
		sizeTIV = (outputSize + guardBits)<<alpha;
		int size = sizeTIV;
		outputSizeTOi = vector<int>(m);
		sizeTOi = vector<int>(m);
		for (int i=0; i<m; i++)		{
			computeTOiSize(i);
			size += sizeTOi[i];
		}
		totalSize = size;

		if(mpt->compressTIV)
			computeTIVCompressionParameters(); // may change sizeTIV and totalSize
		// else leave rho=-1
	}


	void Multipartite::mkTables(Target* target)
	{
		// The TIV
		tiv.clear();
		for (int j=0; j < 1<<alpha; j++) {
				tiv.push_back(TIVFunction(j));
		}
		
		// The TOIs
		//toi = vector<Table*>(m);
		toi.clear();
		for(int i = 0; i < m; ++i)
		{
			vector<int64_t> values;
			for (int j=0; j < 1<<(gammai[i]+betai[i]-1); j++) {
				values.push_back(TOiFunction(j, i));
			}
			// If the TOi values are negative, we will store their opposite
			bool allnegative=true;
			bool allpositive=true;
			for (size_t j=0; j < values.size(); j++) {
				if(values[j]>0)
					allnegative=false;
				if(values[j]<0)
					allpositive=false;
			}
			if((!allnegative) && (!allpositive)) {
				const string srcFileName="Multipartite";
				THROWERROR("TO"<< i << " doesn't have a constant sign, please report this as a bug");
			}
			if(allnegative) {
				negativeTOi.push_back(true);
				for (size_t j=0; j < values.size(); j++) {
					values[j] = -values[j];
				}
			}
			else {
				negativeTOi.push_back(false);
			}
			toi.push_back(values);
			
			//			string name = mpt->getName() + join("_TO",i);
			//toi[i] = new Table(mpt, target, values, name, gammai[i] + betai[i] - 1, outputSizeTOi[i]-1);
		}

		if(mpt->compressTIV) {
			aTIV.clear();
			diffTIV.clear();
			// TIV compression as per Hsiao with improvements
			int s = alpha-rho;
			for(int i = 0; i < (1<<rho); i++)	{
				int64_t valLeft  = TIVFunction( i<<s );
				int64_t valRight = TIVFunction( ((i+1)<<s)-1 );
				int64_t refVal;
				// life is simpler if the diff table is always positive
				if(valLeft<=valRight) 
					refVal=valLeft;
				else  
					refVal=valRight;
				// the improvement: we may shave a few bits from the LSB
				//			int64_t mask = ((1<<outputSize)-1) - ((1<<nbZeroLSBsInATIV)-1);
				refVal = refVal >> nbZeroLSBsInATIV ;
				aTIV.push_back(refVal);
				for(int j = 0; j < (1<<s); j++)		{
					int64_t diff = tiv[ (i<<s) + j] - (refVal << nbZeroLSBsInATIV);
					diffTIV.push_back(diff);
				}	
			}
		}
	}


	//------------------------------------------------------------------------------------- Public classes
	int64_t Multipartite::TOiFunction(int x, int ti)
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
		if(dTOi>0)
			TOi = (int)floor(dTOi);
		else
			TOi = (int)ceil(dTOi);
		return int64_t(TOi);
	}




	int64_t Multipartite::TIVFunction(int x)
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

		return int64_t(TIVval);
	}


	string 	Multipartite::descriptionString(){
		ostringstream s;
		s << "m=" << m << "    alpha=" << alpha;
		if(rho!=-1)
			s << ", rho=" << rho << ", nbZeroLSBsInATIV=" << nbZeroLSBsInATIV << "   ";
		for (size_t i =0; i< gammai.size(); i++) {
			s << "   gamma" << i << "=" << gammai[i] << " beta"<<i<<"=" << betai[i]; 
		}
		s << "     g=" << guardBits << endl;
		if(rho!=-1){
			s << "    sizeATIV=" << sizeATIV << " sizeDiffTIV=" << sizeDiffTIV ;
		}
		s << "  sizeTIV=" << sizeTIV << " totalSize = " << totalSize ;
		return s.str();
	}

	string 	Multipartite::descriptionStringLaTeX(){
		ostringstream s;
#if 0
		s << "    \\alpha=" << alpha;
		if(rho!=-1)
			s << ", rho=" << rho << ", nbZeroLSBsInATIV=" << nbZeroLSBsInATIV << "   ";
		for (size_t i =0; i< gammai.size(); i++) {
			s << "   \\gamma_" << i << "=" << gammai[i] << " \\beta_"<<i<<"=" << betai[i]; 
		}
		s << endl;
#endif
		s << "         totalSize = " << totalSize << "   =   " ;

		if(rho!=-1){
			s << "    " << outputSizeATIV << ".2^" << rho << " + " << outputSizeDiffTIV << ".2^" << alpha;
		}
		else
			s << "    " << outputSize+guardBits << ".2^" << alpha;

		s << "     ";
		for(int t=m-1; t>=0; t-- ) {
			s << " + " << outputSizeTOi[t] << ".2^" << gammai[t] + betai[t] -1;
		}
		return s.str();
	}



	string 	Multipartite::fullTableDump(){
		ostringstream s;
		if(rho==-1) { //uncompressed TIV
			for(size_t i=0; i<tiv.size(); i++ ) {
				s << "TIV[" << i << "] \t= " << tiv[i] <<endl;
			}
		}
		else {// compressed TIV
			for(size_t i=0; i<aTIV.size(); i++ ) {
				s << "aTIV[" << i << "] \t= " << aTIV[i] <<endl;
			}
			s << endl;
			for(size_t i=0; i<diffTIV.size(); i++ ) {
				s << "diffTIV[" << i << "] \t= " << diffTIV[i] <<endl;
			}
		}
		for(size_t t=0; t<toi.size(); t++ ) {
			s << endl;
			for(size_t i=0; i<toi[t].size(); i++ ) {
				s << "TO" << t << "[" << i << "] \t= " << toi[t][i] <<endl;
			}
		}

		return s.str();
	}

	
#define ETDEBUG 0
	bool Multipartite::exhaustiveTest(){
		double maxError=0;
		double rulp=1;
		int lsbIn=mpt->f->lsbIn;
		int lsbOut=mpt->f->lsbOut;
		if(lsbOut<0)
				rulp = 1.0 / ((double) (1<<(-lsbOut)));
		if(lsbOut>0)
			rulp =  (double) (1<<lsbOut);
		for (int x=0; x<(1<<inputSize); x++) {
			int64_t result;
			if(rho==-1) {
				int a = x>>(inputSize-alpha);
				int64_t yTIV = tiv[a];
				result = yTIV;
#if ETDEBUG 
				cerr 	<< "  x=" << x<< " tiv=" << result;
#endif
			}
			else { //compressed table
				int aa = x>>(inputSize-rho);
				int64_t yATIV = aTIV[aa];

				int adiff = x>>(inputSize-alpha);
				int64_t yDiffTIV = diffTIV[adiff];
				result = (yATIV << nbZeroLSBsInATIV) + yDiffTIV;
#if ETDEBUG 
				cerr 	<< "  x=" << x<< " yaTIV=" << yATIV << " yDiffTIV=" << yDiffTIV ;
#endif
			}
			for(int i=0; i<m; i++) {
				int aTOi = (x>>pi[i]) & ((1<<betai[i])-1);
				int sign = 1-(aTOi >> (betai[i]-1) );
#if ETDEBUG 
				cerr << "  aTO" << i << "I=" << aTOi << "  s=" << sign << "  ";
#endif
				aTOi = (aTOi & ((1<<(betai[i]-1))-1));
				if(sign==1)
					aTOi =  ((1<<(betai[i]-1))-1)  - aTOi; 
				aTOi +=   ((x>>(inputSize-gammai[i])) << (betai[i]-1));
				int64_t yTOi = toi[i][aTOi];
#if ETDEBUG 
				cerr << " aTO" << i << "F=" << aTOi << " yTOi="  << yTOi;
#endif
				if(negativeTOi[i])
					sign=1-sign; 
				if(sign==1)
					yTOi = ~yTOi;
				result += yTOi;
			}
			//final rounding
			result = result >> guardBits;
			double fresult = ((double) result) * rulp;
			double ref = f->eval(   ((double)x) / ((double)(1<<(-lsbIn))) );
			double error = abs(fresult-ref);
			maxError = max(maxError, error);
#if ETDEBUG 
			cerr //<< ((double)x) / ((double)(1<<(-lsbIn)))
			  << "  sum=" << result<< " fresult=" << fresult << "ref=" <<ref << "   e=" << error << " u=" <<rulp << (error > rulp ? " *******  Error here":"") <<  endl;
#endif
		}
		return (maxError < rulp);
	}

}

