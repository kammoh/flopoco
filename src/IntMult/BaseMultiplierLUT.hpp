#ifndef BaseMultiplierLUT_HPP
#define BaseMultiplierLUT_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "BaseMultiplierCategory.hpp"

namespace flopoco {
    class BaseMultiplierLUT : public BaseMultiplierCategory
    {

	public:
        BaseMultiplierLUT(int maxSize, double lutPerOutputBit):
                BaseMultiplierCategory{
                        maxSize,
                        1,
                        false,
                        false,
                        -1,
                        "BaseMultiplierLUT"
                }, lutPerOutputBit_{lutPerOutputBit}
        {}

        BaseMultiplierLUT(
                int wX,
                int wY
        ) : BaseMultiplierCategory{
                    wX,
                    wY,
                    false,
                    false,
                    -1,
                    "BaseMultiplierLUT_" + string(1,((char) wX) + '0') + "x" + string(1,((char) wY) + '0')
        }{
            int ws = ((wX+wY) <= 3)?wX+wY-1:wX+wY;
            lut_cost = ((ws <= 5)?ws/2+ws%2:ws) + 0.65*ws;
        }

			double getLUTCost(uint32_t wX, uint32_t wY) const override;
			int getDSPCost(uint32_t wX, uint32_t wY) const override { return 0; }
            double getLUTCost() const { return this->lut_cost;};
            int getDSPCost() const override { return 0; }
            double getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY);

			Operator *generateOperator(
					Operator *parentOp,
					Target *target,
					Parametrization const & parameters
				) const final;
	private:
			double lutPerOutputBit_;
	};
}
#endif
