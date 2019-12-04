#include "BaseMultiplierLUT.hpp"
#include "IntMultiplierLUT.hpp"

namespace flopoco {

double BaseMultiplierLUT::getLUTCost(uint32_t wX, uint32_t wY) const
{
	uint32_t outputSize;
	if (wY == 1) {
		outputSize = wX;
	} else if (wX == 1) {
		outputSize = wY;
	} else {
		outputSize = wX + wY;
	}

	return lutPerOutputBit_ * outputSize;
}

Operator* BaseMultiplierLUT::generateOperator(
		Operator *parentOp, 
		Target* target,
		Parametrization const & parameters) const
{
	return new IntMultiplierLUT(
			parentOp,
			target,
			parameters.getMultXWordSize(),
			parameters.getMultYWordSize(),
			parameters.isSignedMultX() and parameters.getMultXWordSize() > 1,
			parameters.isSignedMultY() and parameters.getMultYWordSize() > 1,
			parameters.isFlippedXY()
		);
}
}   //end namespace flopoco

