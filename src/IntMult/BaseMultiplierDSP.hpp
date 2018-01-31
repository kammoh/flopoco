#ifndef BaseMultiplierDSP_HPP
#define BaseMultiplierDSP_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {


    class BaseMultiplierDSP : public BaseMultiplier
    {

	public:
		BaseMultiplierDSP(bool isSignedX, bool isSignedY, int wX, int wY, bool flipXY=false);

		virtual Operator *generateOperator(Operator *parentOp, Target *target);

    private:
        bool flipXY;
        bool pipelineDSPs;
	};
}
#endif
