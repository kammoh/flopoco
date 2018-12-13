#ifndef BaseMultiplierLUT_HPP
#define BaseMultiplierLUT_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {


    class BaseMultiplierLUT : public BaseMultiplier
    {

	public:
			BaseMultiplierLUT(bool isSignedX, bool isSignedY, int wX, int wY);

			virtual Operator *generateOperator(Operator *parentOp, Target *target);
	};
}
#endif
