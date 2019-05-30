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
#include "IntMult/BaseMultiplierCategory.hpp"
#include "TilingStrategy.hpp"

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
         * @param[in] texOutput      true=generate a tek file with the found tiling solution
         **/
        IntMultiplier(Operator *parentOp, Target* target, int wX, int wY, int wOut=0, bool signedIO = false, bool texOutput = false);

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
//		Operator* parentOp;  			/**< For a virtual multiplier, adding bits to some external BitHeap,
		/**
		 * Realise a tile by instantiating a multiplier, selecting the inputs and connecting the output to the bitheap
		 *
		 * @param tile: the tile to instentiate
		 * @param idx: the tile identifier for unique name
		 * @param output_name: the name to which the output of this tile should be mapped
		 */
		Operator* realiseTile(
				TilingStrategy::mult_tile_t const & tile,
				size_t idx,
				string output_name
			);

        /** returns the amount of consecutive bits, which are not constantly zero
		 * @param bm:                          current BaseMultiplier
		 * @param xPos, yPos:                  position of lower right corner of the BaseMultiplier
		 * @param totalOffset:                 see placeSingleMultiplier()
         * */
        unsigned int getOutputLengthNonZeros(
				BaseMultiplierParametrization const & parameter,
				unsigned int xPos,
				unsigned int yPos,
				unsigned int totalOffset
			);

        unsigned int getLSBZeros(
				BaseMultiplierParametrization const & parameter,
				unsigned int xPos,
				unsigned int yPos,
				unsigned int totalOffset,
				int mode
			);

		/**
		 * @brief Compute the number of bits below the output msb that we need to keep in the summation
		 * @param wX first input width
		 * @param wY second input width
		 * @param wOut number of bits kept in the output
		 * @return the the number of bits below the output msb that we need to keep in the summation to ensure faithful rounding
		 */
		unsigned int computeGuardBits(unsigned int wX, unsigned int wY, unsigned int wOut);

        /*!
         * add a unique identifier for the multiplier, and possibly for the block inside the multiplier
         */
        string addUID(string name, int blockUID=-1);

        int multiplierUid;

		void branchToBitheap(BitHeap* bh, list<TilingStrategy::mult_tile_t> const &solution , unsigned int bitheapLSBWeight);

	};

}
#endif
