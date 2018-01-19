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
        BaseMultiplierDSP(bool isSignedX, bool isSignedY, int wX, int wY, bool pipelineDSPs, bool flipXY=false);

        virtual Operator *generateOperator(Target *target);

    private:
        bool flipXY;
        bool pipelineDSPs;
	};

    class BaseMultiplierDSPOp : public Operator
    {
    public:
        BaseMultiplierDSPOp(Target* target, bool isSignedX, bool isSignedY, int wX, int wY, bool pipelineDSPs, bool flipXY=false);
    private:
        int wX, wY, wR;
    };

}
#endif
