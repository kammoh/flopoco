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
			BaseMultiplierLUT(int maxLutSize, double lutPerOutputBit);
			double getLUTCost(uint32_t wX, uint32_t wY) const override;
			int getDSPCost(uint32_t wX, uint32_t wY) const override { return 0; }

			Operator *generateOperator(
					Operator *parentOp,
					Target *target,
					Parametrization const & parameters
				) const override;
	private:
			double lutPerOutputBit_;
	};
}
#endif
