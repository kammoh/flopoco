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
	// ostringstream name;
  //  srcFileName="BaseMultiplierLUTTable";

	//   name <<"BaseMultiplierLUTTable"<< (negate?"M":"P") << dy << "x" << dx << "r" << wO << (signedX?"Xs":"Xu") << (signedY?"Ys":"Yu");

//	return new Table(parentOp, target, val);
	return new IntMultiplierLUT(parentOp, target, isSignedX, isSignedY, wX, wY);
}


}   //end namespace flopoco

