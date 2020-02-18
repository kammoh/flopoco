#ifndef IntConstMultOptTernary_HPP
#define IntConstMultOptTernary_HPP

#if defined(HAVE_PAGLIB)

#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "../Operator.hpp"
#include "IntAddSubCmp/IntAdder.hpp"
#include "IntConstMultShiftAdd.hpp"
//#include "../ConstMultPAG/ConstMultPAG.hpp"

/**
    Integer constant multiplication using minimum number of adders using ternary adders

*/


namespace flopoco{

    class IntConstMultShiftAddOptTernary : public IntConstMultShiftAdd
	{
	public:
		/** The standard constructor, inputs the number to implement */ 
        IntConstMultShiftAddOptTernary(Operator* parentOp, Target* target, int wIn, int coeff, bool syncInOut=true);

//		void emulate(TestCase* tc);
//		void buildStandardTestCases(TestCaseList* tcl);

        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args );
        static void registerFactory();

	private:
		int coeff;  /**< The constant */

	};
}

#endif //HAVE_PAGSUITE
#endif
