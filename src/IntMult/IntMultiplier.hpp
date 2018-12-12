#ifndef IntMultiplierS_HPP
#define IntMultiplierS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BitHeap/BitHeap.hpp"

#include "IntMult/MultiplierBlock.hpp"
#include "IntMult/BaseMultiplier.hpp"

namespace flopoco {
	class IntMultiplier : public Operator {

	public:

        /**
         * The IntMultiplier constructor
         * @param[in] target           the target device
         * @param[in] wX             X multiplier size (including sign bit if any)
         * @param[in] wY             Y multiplier size (including sign bit if any)
         * @param[in] wOut           wOut size for a truncated multiplier (0 means full multiplier)
         * @param[in] signedIO       false=unsigned, true=signed
         **/
        IntMultiplier(Operator *parentOp, Target* target, int wX, int wY, int wOut=0, bool signedIO = false);

		/**
		 * The emulate function.
		 * @param[in] tc               a test-case
		 */
		void emulate ( TestCase* tc );

		void buildStandardTestCases(TestCaseList* tcl);
		
		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);

		/** Factory register method */ 
		static void registerFactory();

		static TestList unitTest(int index);

	protected:

		int wX;                         /**< the width for X after possible swap such that wX>wY */
		int wY;                         /**< the width for Y after possible swap such that wX>wY */
        int wFullP;                     /**< size of the full product: wX+wY  */
        int wOut;                       /**< size of the output, to be used only in the standalone constructor and emulate.  */
        bool signedIO;                   /**< true if the IOs are two's complement */
		bool negate;                    /**< if true this multiplier computes -xy */
	private:
		Operator* parentOp;  			/**< For a virtual multiplier, adding bits to some external BitHeap,
												this is a pointer to the Operator that will provide the actual vhdl stream etc. */
        BitHeap* bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */

        /** Places a single BaseMultiplier at a given position
         * xPos, yPos:                  Position of the lower right corner of the BaseMultiplier
         * xInputLength, yInputLength:  width of the inputs x and y
         * outputLength:                width of the output
         * xInputNonZeros, yInputNonZeros:      how many consecutive bits of the inputs are not constantly zero (might not be needed)
         * totalOffset:                 Multipliers start at position (totalOffset, totalOffset). This has the advantage that BaseMultipliers
         *                              can protude the lower and right border as well. totalOffset is normally zero or twelve
         * id:                          just an unique id to not declare signals twice
         *
         * returns the name of the output (might not be needed)
         * */
        string placeSingleMultiplier(Operator* op, unsigned int xPos, unsigned int yPos, unsigned int xInputLength, unsigned int yInputLength, unsigned int outputLength, unsigned int xInputNonZeros, unsigned int yInputNonZeros, unsigned int totalOffset, unsigned int id);

        /** returns the amount of consecutive bits, which are not constantly zero
         * bm:                          current BaseMultiplier
         * xPos, yPos:                  position of lower right corner of the BaseMultiplier
         * totalOffset:                 see placeSingleMultiplier()
         * */
        unsigned int getOutputLengthNonZeros(BaseMultiplier* bm, unsigned int xPos, unsigned int yPos, unsigned int totalOffset);

        unsigned int getLSBZeros(BaseMultiplier* bm, unsigned int xPos, unsigned int yPos, unsigned int totalOffset, int mode);

        /*!
         * add a unique identifier for the multiplier, and possibly for the block inside the multiplier
         */
        string addUID(string name, int blockUID=-1);

        int multiplierUid;

	};

}
#endif
