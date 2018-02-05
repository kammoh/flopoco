#ifndef IntKaratsuba_HPP
#define IntKaratsuba_HPP

#include "Operator.hpp"
#include "BitHeap/BitheapNew.hpp"

namespace flopoco{

    class IntKaratsubaRectangular : public Operator
	{
	public:
		/** 
         * The constructor of the IntKaratsubaRectangular class
		 **/
		IntKaratsubaRectangular(Operator* parentOp, Target* target, int wX, int wY,bool useKaratsuba=true);

        /**
         * Emulates the multiplier
		 */
		void emulate(TestCase* tc);

        /** Factory method */
        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
        /** Register the factory */
        static void registerFactory();

    protected:
	
        int wX; /**< the width (in bits) of the input X  */
        int wY; /**< the width (in bits) of the input X  */
        int wOut; /**< the width (in bits) of the output R  */

		bool useKaratsuba; /**< uses Karatsuba when true, instead uses standard tiling without sharing */
		double multDelay;

		const int TileBaseMultiple=8;
		const int TileWidthMultiple=2;
		const int TileHeightMultiple=3;
		const int TileWidth=TileBaseMultiple*TileWidthMultiple;
		const int TileHeight=TileBaseMultiple*TileHeightMultiple;

		BitheapNew *bitHeap;

		/**
		 * Implements + a_i b_j
		 */
		void createMult(int i, int j);

		/**
		 * Implements a_i b_j + a_k b_l by using (a_i - a_k)(b_j - b_l) + a_i b_l + a_k b_j
		 */
		void createKaratsubaRect(int i, int j, int k, int l);

	private:
	};

}
#endif
