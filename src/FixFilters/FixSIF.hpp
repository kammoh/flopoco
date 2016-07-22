#ifndef FIXSIF_HPP
#define FIXSIF_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "gmp.h"
#include "mpfr.h"
#include "sollya.h"
#if HAVE_WCPG
extern "C"
{
	#include "wcpg.h"
}
#endif

#include "Operator.hpp"
#include "utils.hpp"
#include "BitHeap/BitHeap.hpp"
#include "FixSOPC.hpp"
#include "ShiftReg.hpp"

namespace flopoco{

	class FixSIF : public Operator {

	public:
		/**
		 * Constructor of the FixSIF
		 */
		FixSIF(Target* target, string paramFileName, string testsFileName);

		/**
		 * Destructor of the FixSIF
		 */
		~FixSIF();

		/**
		 * Parse the file containing the parameters for the SIF
		 */
		void parseCoeffFile();

		// Below all the functions needed to test the operator
		/**
		 * @brief the emulate function is used to simulate in software the operator
		 * in order to compare this result with those outputed by the vhdl opertator
		 */
		void emulate(TestCase * tc);

		/** @brief function used to create Standard testCase defined by the developper */
		void buildStandardTestCases(TestCaseList* tcl);

		// User-interface stuff
		/** Factory method */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);
		static void registerFactory();

	private:
		string paramFileName;        /**< the name of the file containing the parameters for the SIF */
		string testsFileName;        /**< the name of the file containing the predefined tests for the SIF */

		int n_u;                     /**< the size of the u vector (the number of inputs) */
		int n_t;                     /**< the size of the t vector (the number of temporary variables) */
		int n_x;                     /**< the size of the x vector (the number of states) */
		int n_y;                     /**< the size of the y vector (the number of outputs) */

		vector<int> m_u;             /**< the vector of MSBs of each u */
		vector<int> l_u;             /**< the vector of LSBs of each u */
		vector<int> m_t;             /**< the vector of MSBs of each t */
		vector<int> l_t;             /**< the vector of LSBs of each t */
		vector<int> m_x;             /**< the vector of MSBs of each x */
		vector<int> l_x;             /**< the vector of LSBs of each x */
		vector<int> m_y;             /**< the vector of MSBs of each y */
		vector<int> l_y;             /**< the vector of LSBs of each y */

		vector<vector<string>> Z;    /**< the matrix of coefficients Z */

	};

}

#endif
