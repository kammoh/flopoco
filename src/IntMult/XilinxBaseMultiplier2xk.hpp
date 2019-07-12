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
			BaseMultiplierCategory{maxBigOperandWidth, 2, 0, 0, "XilinxBaseMultiplier2xk"}
		{}

		int getDSPCost(uint32_t, uint32_t) const final {return 0;}
		double getLUTCost(uint32_t wX, uint32_t wY) const final;

		Operator *generateOperator(
				Operator *parentOp, 
				Target *target,
				Parametrization const & parameters
			) const override;
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
