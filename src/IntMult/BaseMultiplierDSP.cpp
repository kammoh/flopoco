#include "BaseMultiplierDSP.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"
#include "IntMult/DSPBlock.hpp"

namespace flopoco {


BaseMultiplierDSP::BaseMultiplierDSP(bool isSignedX, bool isSignedY, int wX, int wY, bool flipXY) : BaseMultiplier(isSignedX,isSignedY)
{

    srcFileName = "BaseMultiplierDSP";
    uniqueName_ = "BaseMultiplierDSP" + std::to_string(wX) + "x" + std::to_string(wY);

    this->flipXY = flipXY;
    this->pipelineDSPs = pipelineDSPs;

    if(!flipXY)
    {
        this->wX = wX;
        this->wY = wY;
    }
    else
    {
        this->wX = wY;
        this->wY = wX;
    }

}

Operator* BaseMultiplierDSP::generateOperator(Operator *parentOp, Target* target)
{
	return new DSPBlock(parentOp, target, wX, wY); //ToDo: no fliplr support anymore, find another solution!
}

}   //end namespace flopoco

