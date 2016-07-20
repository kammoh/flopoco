#ifndef FIXSIF_HPP
#define FIXSIF_HPP

#include <iostream>
#include <sstream>

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
		/** @brief Constructor ; you must use bitheap in case of negative coefficient*/
		FixSIF(Target* target);

		/** @brief Destructor */
		~FixSIF();

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


	private:


	};

}

#endif
