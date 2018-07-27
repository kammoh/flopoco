#ifndef MULTIPARTITE_H
#define MULTIPARTITE_H

#include <vector>
#include <map>

#include "FixFunction.hpp"
#include "../Operator.hpp"


using namespace std;

namespace flopoco
{
	class FixFunctionByMultipartiteTable;
	typedef vector<vector<int>> expression;

	class Multipartite
	{
	public:
		const string uniqueName_=""; // for REPORT to work
		//---------------------------------------------------------------------------- Constructors/Destructor

		Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable* mpt_);

		Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction* f_, int inputSize_, int outputSize_);

		mpz_class TIVFunction(int x);
		mpz_class TOiFunction(int x, int ti);
		//------------------------------------------------------------------------------------- Public methods

		/**
		 * @brief buildGuardBitsAndSizes : Builds decomposition's guard bits and tables sizes
		 */
		void buildGuardBitsAndSizes();

		/**
		 * @brief mkTables : fill  the TIV and TOs tables
		 * @param target : The target FPGA
		 */
		void mkTables(Target* target);

		/**
		 * @brief descriptionString(): describe the various parameters in textual form
		 */
		string descriptionString();
		/**
		 * @brief fullTableDump(): as the name suggests, for debug
		 */
		string 	fullTableDump(); 
		//---------------------------------------------------------------------------------- Public attributes
		FixFunction* f;

		int inputSize;
		int inputRange;
		int outputSize;

		double epsilonT;

		/** The number of TOi */
		int m;
		/** Just as in the article  */
		int alpha;
		/** the number of bits that address the first part of a compressed TIV (alpha -s) */
		int rho;
		/** Just as in the article */
		int beta;
		/** Just as in the article */
		vector<int> gammai;
		/** Just as in the article */
		vector<int> betai;
		/** Just as in the article */
		vector<int> pi;

		/** The Table of Initial Values, just as the ARITH 15 article */
		vector<mpz_class> tiv;
		
		/** The first part of the compressed TIV table, just as in the Hsiao article */
		vector<mpz_class> aTIV;

		/** The second part of the compressed TIV table, just as in the Hsiao article */
		vector<mpz_class> diffTIV;

		/** The m Tables of Offset , just as the ARITH 15 article */
		vector<vector<mpz_class>> toi;

		double mathError;

		int guardBits;
		int outputSizeATIV;
		int outputSizeDiffTIV;
		vector<int> outputSizeTOi;
		vector<int> sizeTOi;
		vector<bool> negativeTOi;
		int totalSize;

		// holds precalculated TOi math errors. Valid as long as we don't change m!
		FixFunctionByMultipartiteTable *mpt;

	private:

		//------------------------------------------------------------------------------------ Private methods
		void computeGuardBits();
		void computeMathErrors();
		void computeSizes();
		void computeTOiSize (int i);

		double deltai(int i);
		double mui(int i, int Bi);
		double si(int i, int Ai);

		void compressAndUpdateTIV();

	};

}
#endif // MULTIPARTITE_H
