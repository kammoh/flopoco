#ifndef FPADDERSPIEEE_HPP
#define FPADDERSPIEEE_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"
#include "../ShiftersEtc/LZOC.hpp"
#include "../ShiftersEtc/Shifters.hpp"
#include "../ShiftersEtc/LZOCShifterSticky.hpp"
#include "../TestBenches/FPNumber.hpp"
#include "../IntAddSubCmp/IntAdder.hpp"
#include "../IntAddSubCmp/IntDualSub.hpp"

namespace flopoco{

	/** The FPAddSinglePathIEEE class */
	class FPAddSinglePathIEEE : public Operator
	{
	public:
		/**
		 * The FPAddSinglePathIEEE constructor
		 * @param[in]		target		the target device
		 * @param[in]		wE			the the with of the exponent
		 * @param[in]		wF			the the with of the fraction
		 */
		FPAddSinglePathIEEE(Target* target, int wE, int wF, bool sub=false, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FPAddSinglePathIEEE destructor
		 */
		~FPAddSinglePathIEEE();


		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		TestCase* buildRandomTestCase(int i);

		/** Create the next TestState based on the previous TestState */
		static void nextTestState(TestState * previousTestState);

		// User-interface stuff
		/** Factory method */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static void registerFactory();


	private:
		/** The width of the exponent */
		int wE;
		/** The width of the fraction */
		int wF;
		/** Is this an FPAdd or an FPSub? */
		bool sub;

		/** The combined leading zero counter and shifter for the close path */
		LZOCShifterSticky* lzocs;
		/** The integer adder object for subtraction in the close path */
		IntAdder *fracSubClose;
		/** The dual subtractor for the close path */
		IntDualSub *dualSubClose;
		/** The fraction adder for the far path */
		IntAdder *fracAddFar;
		/** The adder that does the final rounding */
		IntAdder *finalRoundAdd;
		/** The right shifter for the far path */
		Shifter* rightShifter;

		int sizeRightShift;

	};

}

#endif
