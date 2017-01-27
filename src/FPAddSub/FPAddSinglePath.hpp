#ifndef FPADDERSP_HPP
#define FPADDERSP_HPP
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

namespace flopoco{

	/** The FPAddSinglePath class */
	class FPAddSinglePath : public Operator
	{
	public:
		/**
		 * The FPAddSinglePath constructor
		 * @param[in]		target		the target device
		 * @param[in]		wE			the the with of the exponent
		 * @param[in]		wF			the the with of the fraction
		 */
		FPAddSinglePath(Target* target, int wE, int wF, bool sub=false);

		/**
		 * FPAddSinglePath destructor
		 */
		~FPAddSinglePath();


		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		TestCase* buildRandomTestCase(int i);

	private:
		/** The width of the exponent */
		int wE;
		/** The width of the fraction */
		int wF;
		/** Is this an FPAdd or an FPSub? */
		bool sub;

		int sizeRightShift;

	};

}

#endif
