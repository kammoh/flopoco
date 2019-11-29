#ifndef XilinxXilinxBaseMultiplier2xk_HPP
#define XilinxXilinxBaseMultiplier2xk_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BaseMultiplierCategory.hpp"

namespace flopoco {
    class XilinxBaseMultiplier2xk : public BaseMultiplierCategory
    {
	public:
        XilinxBaseMultiplier2xk(int maxBigOperandWidth):
			BaseMultiplierCategory{
            maxBigOperandWidth,
            2,
            false,
            false,
            -1,
            "XilinxBaseMultiplier2xk"}
		{lut_cost = 2*maxBigOperandWidth/(1.65*maxBigOperandWidth+2.3);}

		int getDSPCost(uint32_t, uint32_t) const final {return 0;}
		double getLUTCost(uint32_t wX, uint32_t wY) const final;

		/** Factory method */
        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
        /** Register the factory */
        static void registerFactory();

        void emulate(TestCase* tc);
        static TestList unitTest(int index);

		Operator *generateOperator(
				Operator *parentOp,
				Target *target,
				Parametrization const & parameters
			) const final;
	};

    class XilinxBaseMultiplier2xkOp : public Operator
    {
    public:
		XilinxBaseMultiplier2xkOp(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int width, bool flipXY=false);
    private:
        int wX, wY;
    };

}
#endif
