#include "BaseMultiplierLUT.hpp"
#include "IntMultiplierLUT.hpp"

namespace flopoco {


BaseMultiplierLUT::BaseMultiplierLUT(bool isSignedX, bool isSignedY, int wX, int wY) : BaseMultiplier(isSignedX,isSignedY)
{

    srcFileName = "BaseMultiplierLUT";
    uniqueName_ = string("BaseMultiplierLUT") + to_string(wX) + string("x") + to_string(wY);

    this->wX = wX;
    this->wY = wY;

}

Operator* BaseMultiplierLUT::generateOperator(Operator *parentOp, Target* target)
{
	return new IntMultiplierLUT(parentOp, target, wX, wY, isSignedX, isSignedY);
}


}   //end namespace flopoco

