#ifndef IntKaratsuba_HPP
#define IntKaratsuba_HPP

#include "Operator.hpp"

namespace flopoco{

    class IntKaratsubaRectangular : public Operator
	{
	public:
		/** 
         * The constructor of the IntKaratsubaRectangular class
		 **/
        IntKaratsubaRectangular(Operator* parentOp, Target* target, int wX, int wY);

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

	private:
	};

}
#endif
