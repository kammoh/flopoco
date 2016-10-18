/*
  Integer division by a small constant.

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.

*/
#ifndef __INTCONSTDIV_HPP
#define __INTCONSTDIV_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Target.hpp"
#include "Table.hpp"

#define INTCONSTDIV_LINEAR_ARCHITECTURE 0
#define INTCONSTDIV_LOGARITHMIC_ARCHITECTURE 1
namespace flopoco{


	class IntConstDiv : public Operator
	{
	public:

#define INTCONSTDIV_OLDTABLEINTERFACE 0 // 0 for master, 1 for newPipelineFramework

#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method

		/** @brief This table inputs a number X on alpha + gammma bits, and computes its Euclidean division by d: X=dQ+R, which it returns as the bit string Q & R */
		class EuclideanDivTable: public Table {
		public:
			int d;
			int alpha;
			int gamma;
			EuclideanDivTable(Target* target, int d, int alpha, int gamma);
			mpz_class function(int x);
		};


		/** @brief This table is the CBLK table of the Arith23 paper by Ugurdag et al */
		class CBLKTable: public Table {
		public:
			int level; /**< input will be a group of 2^level alpha-bit digits*/
			int d;
			int alpha;
			int gamma;
			int rho;
			CBLKTable(Target* target, int level, int d, int alpha, int gamma, int rho);
			mpz_class function(int x);
		};
#else
		vector<mpz_class>  euclideanDivTable(int d, int alpha, int gamma);
		vector<mpz_class>  firstLevelCBLKTable(int d, int alpha, int gamma);
		vector<mpz_class>  otherLevelCBLKTable(int level, int d, int alpha, int gamma, int rho );

#endif // deprecated overloading of Table method


		/** @brief The constructor
		* @param d The divisor.
		* @param n The size of the input X.
		* @param alpha The size of the chunk, or, use radix 2^alpha
		* @param architecture Architecture used, can be 0 for o(n) area, o(n) time, or 1 for o(n.log(n)) area, o(log(n)) time
		* @param remainderOnly As the name suggests
		*/
		IntConstDiv(Target* target, int wIn, int d, int alpha=-1, int architecture=0, bool remainderOnly=false, map<string, double> inputDelays = emptyDelayMap);

		~IntConstDiv();

		// Overloading the virtual functions of Operator
		// void outputVHDL(std::ostream& o, std::string name);

		void emulate(TestCase * tc);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static TestList unitTest(int index);

		/** Factory register method */ 
		static void registerFactory();

	public:
		int quotientSize();   /**< Size in bits of the quotient output */
		int remainderSize();  /**< Size in bits of a remainder; gamma=ceil(log2(d-1)) */



	private:
		int d; /**<  Divisor*/
		int wIn;  /**<  Size in bits of the input X */
		int alpha; /**< Size of the chunk (should be between 1 and 16)*/
		int architecture; /** 0 for the linear architecture. 1 for the log(n) architecture */
		bool remainderOnly; /**< if true, only the remainder will be computed. If false, quotient will be computed */
		int gamma;  /**< Size in bits of a remainder; gamma=ceil(log2(d-1)) */
		int qSize;   /**< Size in bits of the quotient output */
		int rho;    /**< Size in bits of the quotient of one digit */

	};

}
#endif
