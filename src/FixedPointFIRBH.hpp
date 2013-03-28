#ifndef FixedPointFIRBH_HPP
#define FixedPointFIRBH_HPP

#include "Operator.hpp"
#include "utils.hpp"

#include "BitHeap.hpp"

/*  All flopoco operators and utility functions are declared within
  the flopoco namespace.
    You have to use flopoco:: or using namespace flopoco in order to access these
  functions.
*/

namespace flopoco{

	// new operator class declaration
	class FixedPointFIRBH : public Operator {
	public:
		int p;  						/**< The precision of inputs and outputs */ 
		int n;  						/**< number of taps */
		vector<string> coeff;  			/**< the coefficients as strings */
		mpfr_t mpcoeff[10000];  		/**< the absolute values of the coefficients as MPFR numbers */
		bool coeffsign[10000];  		/**< the signs of the coefficients */

		int wO;  						/**< output size, will be computed out of the constants */
		
		BitHeap* bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */

	public:
		// definition of some function for the operator

		// constructor, defined there with two parameters
		FixedPointFIRBH(Target* target, int p_, vector<string> coeff_);

		// destructor
		~FixedPointFIRBH() {};


		// Below all the functions needed to test the operator
		/* the emulate function is used to simulate in software the operator
		   in order to compare this result with those outputed by the vhdl opertator */
		void emulate(TestCase * tc);

		/* function used to create Standard testCase defined by the developper */
		void buildStandardTestCases(TestCaseList* tcl);


		/* function used to bias the (uniform by default) random test generator
		   See FPExp.cpp for an example */
		// TestCase* buildRandomTestCase(int i);
	};


}

#endif