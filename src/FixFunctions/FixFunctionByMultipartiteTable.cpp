/*
  Multipartites Tables for FloPoCo

  Authors: Franck Meyer, Florent de Dinechin

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2015.
  All rights reserved.
  */

#include <iostream>
#include <math.h>
#include <string.h>
#include <sstream>
#include <vector>

#include <gmp.h>
#include <gmpxx.h>
#include <mpfr.h>

#include <sollya.h>



#include "../utils.hpp"
#include "FixFunctionByMultipartiteTable.hpp"
#include "Multipartite.hpp"
#include "../BitHeap/BitHeap.hpp"
#include "../Table.hpp"



/*
 TODOs:
 One bit could sometimes be saved in the diffTIV table if it never contains 0 
 Handle TIV compression properly in the optimization
 compress TIV only if it saves at least one adder; in general resurrect uncompressed TIV
 attempt final reduction of g
*/

using namespace std;

namespace flopoco
{
	//----------------------------------------------------------------------------- Constructor/Destructor

	/**
	 @brief The FixFunctionByMultipartiteTable constructor
	 @param[string] functionName_		a string representing the function, input range should be [0,1) or [-1,1)
	 @param[int]    lsbIn_		input LSB weight
	 @param[int]    msbOut_		output MSB weight, used to determine wOut
	 @param[int]    lsbOut_		output LSB weight
	 @param[int]	nbTOi_	number of tables which will be created
	 @param[bool]	signedIn_	true if the input range is [-1,1)
	 */
	FixFunctionByMultipartiteTable::FixFunctionByMultipartiteTable(OperatorPtr parentOp, Target *target, string functionName_, int nbTOi_, bool signedIn_, int lsbIn_, int msbOut_, int lsbOut_):
		Operator(parentOp, target), nbTOi(nbTOi_)
{
		f = new FixFunction(functionName_, signedIn_, lsbIn_, msbOut_, lsbOut_);
		epsilonT = 1.0 / (1<<(f->wOut+1));

		srcFileName="FixFunctionByMultipartiteTable";

		ostringstream name;
		name << "FixFunctionByMultipartiteTable_" << getNewUId();
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Franck Meyer, Florent de Dinechin (2015)");
		addHeaderComment("-- Evaluator for " +  f->getDescription() + "\n");
		REPORT(DETAILED, "Entering: FixFunctionByMultipartiteTable \"" << functionName_ << "\" " << nbTOi_ << " " << signedIn_ << " " << lsbIn_ << " " << msbOut_ << " " << lsbOut_ << " ");
		int wX=-lsbIn_;
		addInput("X", wX);
		int outputSize = msbOut_-lsbOut_+1; // TODO finalRounding would impact this line
		addOutput("Y" ,outputSize , 2);
		useNumericStd();


		if(nbTOi==0) {
			// The following is not as clean as it should be because the code was first written with nbTOi a global variable...
			int sizeMax = f->wOut * (1<<f->wIn); // size of a plain table
			Multipartite* 		smallest = new Multipartite(this, f, f->wIn, f->wOut);
			smallest->totalSize = sizeMax;
			nbTOi=1;
			int smallestNbTables=1;
			try {
				while (true) {
					REPORT(INFO, "Exploring nbTOi=" << nbTOi);
					buildOneTableError();
					buildGammaiMin();
					bestMP = enumerateDec();
					if(bestMP->totalSize < smallest->totalSize) {
						delete smallest;
						smallestNbTables=nbTOi;
						smallest = bestMP;
					}
					else {
						delete bestMP;
					}
					REPORT(INFO, "Best size found is nbTOi="<< smallestNbTables << "  size=" << smallest->totalSize);
					nbTOi ++;
				}
			} catch(std::string &s){
				REPORT(INFO, "No decomposition found for nbTOi=" << nbTOi << ", stopping search.");
				if(smallest->totalSize==sizeMax) {
					THROWERROR("\nSorry, the multipartite method doesn't seem to work for this function. \nTry another of the FixFunction* operators");
				}
				else{
					bestMP=smallest;
					nbTOi=smallestNbTables;
				}
			}
		}
		else	{
				// build the required tables of errors
				buildOneTableError();
				buildGammaiMin();
				bestMP = enumerateDec();
		}

		bestMP->mkTables(target);
		
		REPORT(FULL,"Full table dump:" <<endl << bestMP->fullTableDump()); 

		if(bestMP->rho==-1) { // uncompressed TIV
			vhdl << tab << declare("inTIV", bestMP->alpha) << " <= X" << range(f->wIn-1, f->wIn-bestMP->alpha) << ";" << endl;
			Table::newUniqueInstance(this, "inTIV", "outTIV",
															 bestMP->tiv, "TIV", bestMP->alpha, f->wOut );
				vhdl << endl;
		}else
			{ // Hsiao-compressed TIV
				vhdl << tab << declare("inATIV", bestMP->rho) << " <= X" << range(f->wIn-1, f->wIn-bestMP->rho) << ";" << endl;
				Table::newUniqueInstance(this, "inATIV", "outATIV",
																 bestMP->aTIV, "ATIV", bestMP->rho, bestMP->outputSizeATIV );
				vhdl << endl;
				
				vhdl << tab << declare("inDiffTIV", bestMP->alpha) << " <= X" << range(f->wIn-1, f->wIn-bestMP->alpha) << ";" << endl;
				Table::newUniqueInstance(this, "inDiffTIV", "outDiffTIV",
																 bestMP->diffTIV, "DiffTIV", bestMP->alpha, bestMP->outputSizeDiffTIV );
				// TODO need to sign-extend for 1/(1+x), but it makes an error for sin(x)
				//  getSignalByName("outDiffTIV")->setIsSigned(); // so that it is sign-extended in the bit heap
				// No need to sign-extend it, it is already taken care of in the construction of the table.
			}
		
		int p = 0;
		for(unsigned int i = 0; i < bestMP->toi.size(); ++i)		{
			string ai = join("a", i);
			string bi = join("b", i);
			string inTOi = join("inTO", i);
			string outTOi = join("outTO", i);
			string deltai = join("delta", i);
			string nameTOi = join("TO", i);
			string signi = join("sign", i);

			p += bestMP->betai[i];
			vhdl << tab << declare(ai, bestMP->gammai[i]) << " <= X" << range(bestMP->inputSize - 1, bestMP->inputSize - bestMP->gammai[i]) << ";" << endl;
			vhdl << tab << declare(bi, bestMP->betai[i]) << " <= X" << range(p - 1, p - bestMP->betai[i]) << ";" << endl;
			vhdl << tab << declare(signi) << " <= not(" << bi << of( bestMP->betai[i] - 1) << ");" << endl;
			vhdl << tab << declare(inTOi,bestMP->gammai[i]+bestMP->betai[i]-1) << " <= " << ai << " & ((" << bi << range(bestMP->betai[i]-2, 0) << ") xor " << rangeAssign(bestMP->betai[i]-2,0, signi)<< ");" << endl;
			Table::newUniqueInstance(this, inTOi, outTOi,
															 bestMP->toi[i], nameTOi, bestMP->gammai[i]+bestMP->betai[i]-1, bestMP->outputSizeTOi[i]-1);
			string trueSign = (bestMP->negativeTOi[i] ? "(not "+signi+")" : signi);
			vhdl << tab << declare(deltai, bestMP->outputSizeTOi[i]) << " <= " << trueSign << " & (" <<  outTOi  << " xor " << rangeAssign(bestMP->outputSizeTOi[i]-2,0, trueSign)<< ");" << endl;
			getSignalByName(deltai)->setIsSigned(); // so that it is sign-extended in the bit heap
		}
		
		// Throwing everything into a bit heap

		BitHeap *bh = new BitHeap(this, bestMP->outputSize + bestMP->guardBits); // TODO this is using an adder tree

		if(bestMP->rho==-1) { // uncompressed TIV
			bh->addSignal("outTIV");
		}else
			{ // Hsiao-compressed TIV
				bh->addSignal("outATIV", bestMP->nbZeroLSBsInATIV); // shifted because its LSB bits were shaved in the Hsiao compression
				//bh->addSignal("outATIV"); // shifted because its LSB bits were shaved in the Hsiao compression
				bh->addSignal("outDiffTIV");
			}

		for(unsigned int i = 0; i < bestMP->toi.size(); ++i)		{
			bh->addSignal(join("delta", i) );
		}
		bh->startCompression();

		vhdl << tab << "Y <= " << /* "sum" */bh->getSumName() << range(bestMP->outputSize + bestMP->guardBits - 1, bestMP->guardBits) << ";" << endl;
	}


	FixFunctionByMultipartiteTable::~FixFunctionByMultipartiteTable() {
		delete f;
		delete bestMP;
	}


	//------------------------------------------------------------------------------------- Public methods

	void FixFunctionByMultipartiteTable::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		tc = new TestCase(this);
		tc->addInput("X", 0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", 1);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", (mpz_class(1) << f->wIn) - 1);
		emulate(tc);
		tcl->add(tc);

	}


	void FixFunctionByMultipartiteTable::emulate(TestCase* tc)
	{
		f->emulate(tc);
	}

	//------------------------------------------------------------------------------------ Private classes
	
	// enumerating the alphas is much simpler
	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenum(int alpha, int m)
	{
		vector<vector<int>> res;
		vector<vector<int>> res1;
		int ptr;

		if (m==1)
		{
			res = vector<vector<int>>(alpha, vector<int>(1));
			for(int i = 0; i < alpha; i++)
			{
				res[i][0] = i+1;
			}
		}
		else
		{
			res1 = alphaenum(alpha, m-1);
			res = vector<vector<int>>(alpha*res1.size());
			ptr = 0;

			for(int i = 0; i < alpha; i++)
			{
				for(unsigned int j = 0; j < res1.size(); j++)
				{
					res[ptr] = vector<int>(m);
					res[ptr][0] = i+1;
					for(int k = 1; k < m; k++)
					{
						res[ptr][k] = res1[j][k-1];
					}
					ptr++;
				}
			}
		}
		return res;
	}


	// with a table stating where to start the enumeration
	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenum(int alpha, int m, vector<int> gammaimin)
	{
		//test if it is possible with this alpha
		for (int i=0; i<m; i++)
			if (gammaimin[i]>alpha)
				return vector<vector<int>>(0);// else return an empty enumeration
		//else
		return alphaenumrec(alpha, m, gammaimin);

	}


	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenumrec(int alpha, int m, vector<int> gammaimin)
	{
		vector<vector<int>> res;
		vector<vector<int>> res1;
		int ptr;
		if (m==1)
		{
			res = vector<vector<int>>(alpha-gammaimin[0]+1, vector<int>(1));
			for(int i = gammaimin[0]; i <= alpha; i++)
			{
				res[i-gammaimin[0]][0] = i;
			}
		}
		else
		{
			res1 = alphaenumrec(alpha, m-1, gammaimin);
			res = vector<vector<int>>((alpha-gammaimin[m-1]+1)*res1.size());
			ptr = 0;
			for(int i = gammaimin[m-1]; i <= alpha; i++)
			{
				for(unsigned int j = 0; j < res1.size(); j++)
				{
					res[ptr] = vector<int>(m);
					res[ptr][0] = i;
					for(int k = 1; k < m; k++)
						res[ptr][k] = res1[j][k-1];
					ptr++;
				}
			}
		}
		return res;
	}


	vector<vector<int>> FixFunctionByMultipartiteTable::betaenum (int sum, int size)
	{
		int ptr;
		vector<int> t;
		int totalsize(0);
		vector<vector<int>> res;
		vector<vector<vector<int>>> enums;

		if (size == 1)
		{
			t = vector<int>(size);
			t[0] = sum;
			res = vector<vector<int>>(1);
			res[0] = t;
		}
		else if (2*size>sum)
		{
			res=vector<vector<int>>(0);
		}
		else if (2*size == sum)
		{
			t = vector<int>(size);
			for (int i = 0; i < size; i++)
				t[i] = 2;

			res = vector<vector<int>>(1);
			res[0] = t;
		}
		else
		{ // 2*size < sum
			enums = vector<vector<vector<int>>>(sum-2*size+1);//to flatten at the end

			for (int i = 2; i <= sum-2*(size-1); i++)
			{
				enums[i-2] = betaenum(sum-i, size-1);
				totalsize += enums[i-2].size();
			}
			// now we know the size to alloc
			res = vector<vector<int>>(totalsize);
			// now flatten all these smaller enums
			ptr = 0;
			for (unsigned int i = 0; i < enums.size(); i++)
			{
				for(unsigned int j = 0; j < enums[i].size(); j++)
				{
					res[ptr] = vector<int>(size);
					res[ptr][0] = i+2;
					for(int k = 1; k < size; k++)
					{
						res[ptr][k] = enums[i][j][k-1];
					}
					ptr++;
				}
			}

		}
		return res;
	}


	/** See the article p5, right. */
	void FixFunctionByMultipartiteTable::buildGammaiMin()
	{
		int wi = f->wIn;
		int gammai;
		gammaiMin = vector<vector<int>>(wi, vector<int>(wi));
		for(int pi = 0; pi < wi; pi++)
		{
			for(int betai = 1; betai < (wi-pi-1); betai++)
			{
				gammai=1;
				while ( (gammai < wi) && (oneTableError[pi][betai][gammai] > epsilonT) )
					gammai++;

				gammaiMin[pi][betai] = gammai;
			}
		}
	}


	/**
	 * @brief buildOneTableError : Builds the error for every beta_i and gamma_i, to evaluate precision
	 */
	void FixFunctionByMultipartiteTable::buildOneTableError()
	{
		int wi = f->wIn;
		int gammai, betai, pi;
		oneTableError = vector<vector<vector<double>>>(wi, vector<vector<double>>(wi, vector<double>(wi))); // there will be holes

		for(pi=0; pi<wi; pi++) {
			for(betai=1; betai<wi-pi-1; betai++) {
				for(gammai=1; gammai<= wi-pi-betai; gammai++) {
					oneTableError[pi][betai][gammai] =	errorForOneTable(pi, betai,gammai);
				}
			}
		}
	}


	/**
	 * @brief enumerateDec : This function enumerates all the decompositions and returns the smallest one
	 * @return The smallest Multipartite decomposition.
	 * @throw "It seems we could not find a decomposition" if there isn't any decomposition with an acceptable error
	 */
	Multipartite* FixFunctionByMultipartiteTable::enumerateDec()
	{
		Multipartite *smallest, *mpt;
		int beta, p;
		int n = f->wIn;
		int alphamin = 2;
		int alphamax = (2*n)/3;
		vector<vector<int>> betaEnum;
		vector<vector<int>> alphaEnum;
		vector<int> gammaimin;
		vector<int> gammai;
		vector<int> betai;

		int sizeMax = f->wOut * (1<<f->wIn); // size of a plain table
		smallest = new Multipartite(this, f, f->wIn, f->wOut);
		smallest->totalSize = sizeMax;

		for (int alpha = alphamin; alpha <= alphamax; alpha++)
		{
			beta = n-alpha;
			betaEnum = betaenum(beta, nbTOi);
			for(unsigned int e = 0; e < betaEnum.size(); e++) {
				betai = betaEnum[e];

				gammaimin = vector<int>(nbTOi);
				p = 0;
				for(int i = nbTOi-1; i >= 0; i--) {
					gammaimin[i] = gammaiMin[p][betai[i]];
					p += betai[i];
				}

				alphaEnum = alphaenum(alpha,nbTOi,gammaimin);
				for(unsigned int ae = 0; ae < alphaEnum.size(); ae++)
				{
					gammai = alphaEnum[ae];
					mpt = new Multipartite(f, nbTOi,
																 alpha, beta,
																 gammai, betai, this);
					if(mpt->mathError < epsilonT){
						mpt->buildGuardBitsAndSizes();
					}
					else
						mpt->totalSize = sizeMax;

					if(mpt->totalSize < smallest->totalSize) {
						delete smallest;
						smallest = mpt;
						REPORT(DETAILED, "new best found " << mpt->descriptionString()  << "   Size=" << smallest->totalSize);
					}
					else {
						delete mpt;
					}
				}
			}
		}
		/* If  parse error throw an exception */
		if (smallest->totalSize == sizeMax)
			THROWERROR("It seems we could not find a decomposition");

		REPORT(INFO, "Best decomposition found: "<< endl << smallest->descriptionString());					 
		return smallest;
	}


	/** 5th equation implementation */
	double FixFunctionByMultipartiteTable::epsilon(int ci_, int gammai, int betai, int pi)
	{
		int wi = f->wIn; // for the notations of the paper
		double ci = (double)ci_; // for the notations of the paper

		double xleft = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * intpow2(-gammai) * ci;
		double xright = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * (intpow2(-gammai) * (ci+1) - intpow2(-wi+pi+betai));
		double delta = (f->signedIn ? 2 : 1) * intpow2(pi-wi) * (intpow2(betai) - 1);

		double eps = 0.25 * (f->eval(xleft + delta)
							 - f->eval(xleft)
							 - f->eval(xright+delta)
							 + f->eval(xright)
							 );
		return eps;
	}


	double FixFunctionByMultipartiteTable::epsilon2ndOrder(int Ai, int gammai, int betai, int pi)
	{
		int wi = f->wIn;
		double xleft = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * intpow2(-gammai)  * ((double)Ai);
		double delta = (f->signedIn ? 2 : 1) * ( intpow2(pi-wi) * (intpow2(betai) - 1));
		double epsilon = 0.5 * (f->eval(xleft + (delta-1)/2)
								- (delta-1)/2 * (f->eval(xleft + delta)
												 - f->eval(xleft)));
		return epsilon;
	}


	/** eq 11 */
	double FixFunctionByMultipartiteTable::errorForOneTable(int pi, int betai, int gammai)
	{
		double eps1, eps2, eps;

		// Beware when gammai+betai+pi = inputSize,
		// we then have to compute a second order term for the error
		if(gammai+betai+pi==f->wIn)
		{
			eps1 = abs(epsilon2ndOrder(0, gammai, betai, pi));
			eps2 = abs(epsilon2ndOrder( (int)intpow2(gammai) -1, gammai, betai, pi));
			if (eps1 > eps2)
				eps = eps1;
			else
				eps = eps2;
		}
		else
		{
			eps1 = abs(epsilon(0, gammai, betai, pi));
			eps2 = abs(epsilon( (int)intpow2(gammai) -1, gammai, betai, pi));

			if (eps1 > eps2)
				eps = eps1;
			else
				eps = eps2;
		}
		return eps;
	}




	TestList FixFunctionByMultipartiteTable::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests
			// A few regression tests, first deg2, exhaustive on 15 bits, then deg 3 for 24 bits 
			paramList.push_back(make_pair("f","\"sin(x)\""));
			paramList.push_back(make_pair("lsbIn","-8"));
			paramList.push_back(make_pair("msbOut","0"));
			paramList.push_back(make_pair("lsbOut","-8"));
			paramList.push_back(make_pair("TestBench n=","-2"));
			testStateList.push_back(paramList);
			paramList.clear();

			paramList.push_back(make_pair("f","\"sin(x)\""));
			paramList.push_back(make_pair("lsbIn","-12"));
			paramList.push_back(make_pair("msbOut","0"));
			paramList.push_back(make_pair("lsbOut","-12"));
			paramList.push_back(make_pair("TestBench n=","-2"));
			testStateList.push_back(paramList);
			paramList.clear();
			
			paramList.push_back(make_pair("f","\"1/(x+1)\""));
			paramList.push_back(make_pair("lsbIn","-8"));
			paramList.push_back(make_pair("lsbOut","-8"));
			paramList.push_back(make_pair("msbOut","0"));
			paramList.push_back(make_pair("TestBench n=","-2"));
			testStateList.push_back(paramList);
			paramList.clear();
		 
			paramList.push_back(make_pair("f","\"1/(x+1)\""));
			paramList.push_back(make_pair("lsbIn","-12"));
			paramList.push_back(make_pair("lsbOut","-12"));
			paramList.push_back(make_pair("msbOut","0"));
			paramList.push_back(make_pair("TestBench n=","-2"));
			testStateList.push_back(paramList);
			paramList.clear();

#if 0			
			paramList.push_back(make_pair("f","\"1/(x+1)\""));
			paramList.push_back(make_pair("lsbIn","-16"));
			paramList.push_back(make_pair("lsbOut","-16"));
			paramList.push_back(make_pair("msbOut","0"));
			paramList.push_back(make_pair("TestBench n=","-2"));
			testStateList.push_back(paramList);
			paramList.clear();
#endif

		}
		else     
		{
				// finite number of random test computed out of index
		}	

		return testStateList;
	}

	

	OperatorPtr FixFunctionByMultipartiteTable::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		string f;
		bool signedIn;
		int lsbIn, lsbOut, msbOut, nbTOi;
		UserInterface::parseString(args, "f", &f);
		UserInterface::parsePositiveInt(args, "nbTOi", &nbTOi);
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseInt(args, "msbOut", &msbOut);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		UserInterface::parseBoolean(args, "signedIn", &signedIn);
		return new FixFunctionByMultipartiteTable(parentOp, target, f, nbTOi, signedIn, lsbIn, msbOut, lsbOut);
	}

	void FixFunctionByMultipartiteTable::registerFactory(){
		UserInterface::add("FixFunctionByMultipartiteTable", // name
											 "A function evaluator using the multipartite method.",
											 "FunctionApproximation", // category
											 "",
						   "f(string): function to be evaluated between double-quotes, for instance \"exp(x*x)\";\
lsbIn(int): weight of input LSB, for instance -8 for an 8-bit input;\
msbOut(int): weight of output MSB;\
lsbOut(int): weight of output LSB;\
nbTOi(int)=0: number of Tables of Offsets, between 1 (bipartite) to 4 or 5 for large input sizes -- 0: let the tool choose ;\
signedIn(bool)=false: defines the input range : [0,1) if false, and [-1,1) otherwise\
",

											 "This operator uses the multipartite table method as introduced in <a href=\"http://perso.citi-lab.fr/fdedinec/recherche/publis/2005-TC-Multipartite.pdf\">this article</a>, with the improvement described in <a href=\"http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=6998028&tag=1\">this article</a>. ",
											 FixFunctionByMultipartiteTable::parseArguments,
											 FixFunctionByMultipartiteTable::unitTest
											 ) ;

	}
}

