#ifndef FPFMA_HPP
#define FPFMA_HPP
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
#include "../TestBenches/IEEENumber.hpp"
#include "../IntAddSubCmp/IntAdder.hpp"

namespace flopoco{

	/** The FPFMA class */
	class FPFMA : public Operator
	{
	public:
		/**
		 * The FPFMA constructor
		 * @param[in]		target		    target device
		 * @param[in]		wE			       the with of the exponent 
		 * @param[in]		wF			       the with of the fraction 
		 * @param[in]		ieee_compliant	 operate on IEEE numbers if true, on FloPoCo numbers if false 
		 */
		FPFMA(OperatorPtr parentOp, Target* target, int wE, int wF, bool ieee_compliant=true);

		/**
		 * FPFMA destructor
		 */
		~FPFMA();


		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		void buildRandomTestCases(TestCaseList* tcl, int n);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);

		//		static TestList unitTest(int index);

		/** Factory register method */ 
		static void registerFactory();



	private:
		/** The width of the exponent */
		int wE; 
		/** The width of the fraction */
		int wF; 
		/** if true, an IEEE-754-2008 operator; if false, use FloPoCo format */
		bool ieee_compliant;
	};

}

#endif
