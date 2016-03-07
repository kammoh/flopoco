#ifndef FPLARGEACCUMULATOR_HPP
#define FPLARGEACCUMULATOR_HPP
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "Operator.hpp"
#include "ShiftersEtc/Shifters.hpp"
#include "TestBenches/FPNumber.hpp"
#include "utils.hpp"

namespace flopoco{

	/** Implements a long, fixed point accumulator for accumulating floating point numbers
	 */
	class FPLargeAcc : public Operator
	{
	public:
		/** Constructor
		 * @param target the target device
		 * @param wEX the width of the exponent 
		 * @param eFX the width of the fractional part
		 * @param MaxMSBX the weight of the MSB of the expected exponent of X
		 * @param LSBA the weight of the least significand bit of the accumulator
		 * @param MSBA the weight of the most significand bit of the accumulator
		 */ 
		FPLargeAcc(Target* target, int wEX, int wFX, int MaxMSBX, int MSBA, int LSBA, map<string, double> inputDelays = emptyDelayMap, bool forDotProd = false, int wFY = -1);
	
		/** Destructor */
		~FPLargeAcc();
	
		void test_precision(int n); /**< Undocumented */
		void test_precision2(); /**< Undocumented */
	

		void emulate(TestCase* tc);

		TestCase* buildRandomTestCase(int i);


		mpz_class mapFP2Acc(FPNumber X);
	
		mpz_class sInt2C2(mpz_class X, int width);
		
		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		/** Factory register method */ 
		static void registerFactory();

	protected:
		int wEX;     /**< the width of the exponent  */
		int wFX;     /**< the width of the fractional part */
		int MaxMSBX; /**< the weight of the MSB of the expected exponent of X */
		int LSBA;    /**< the weight of the least significand bit of the accumulator */
		int MSBA;    /**< the weight of the most significand bit of the accumulator */

		int currentIteration;
		int xOvf;	

	private:
		int      sizeAcc;          /**<The size of the accumulator  = MSBA-LSBA+1; */
		int      sizeSummand;      /**< the maximum size of the summand  = MaxMSBX-LSBA+1; */
		int      sizeShiftedFrac;  /**< size of ths shifted frac  = sizeSummand + wFX;  to accomodate very small summands */
		int      maxShift;         /**< maximum shift ammount */
		int      E0X;              /**< the bias value */
		int      sizeShift;        /**< the shift size */
		int      c2ChunkSize;      /**< for c2 addition */

		// For emulate()
		mpz_class accValue;
	};

}

#endif
