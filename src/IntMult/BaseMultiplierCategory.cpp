#include "BaseMultiplierCategory.hpp"

namespace flopoco {

	unsigned int BaseMultiplierCategory::Parametrization::getOutWordSize() const
	{
		if (wY_ == 1)
			return wX_;
		if (wX_ == 1)
			return wY_;
		return wX_ + wY_;
	}

	bool BaseMultiplierCategory::Parametrization::shapeValid(int x, int y) const
	{
		return bmCat_->shapeValid(*this, x, y);
	}

	BaseMultiplierCategory::Parametrization BaseMultiplierCategory::parametrize(
			int wX,
			int wY,
			bool isSignedX,
			bool isSignedY
		) const
	{
		int effectiveWY = (isSignedY) ? wY - deltaWidthUnsignedSigned_ : wY;
		bool isFlippedXY = (effectiveWY > maxWordSizeSmallInputUnsigned_);
		if (isFlippedXY) {
			std::swap(wX, wY);
			std::swap(isSignedX, isSignedY);
		}

		int maxSizeY = getMaxSecondWordSize(wX, isSignedX, isSignedY);
		if (maxSizeY < wY) {
			throw std::string("BaseMultiplierCategory::parametrize: error, reqauired multiplier area is too big");
		}

		return Parametrization(wX, wY, this, isSignedX, isSignedY, isFlippedXY);
	}

	int BaseMultiplierCategory::getMaxSecondWordSize(
			int firstW, 
			bool isW1Signed, 
			bool isW2signed
		) const
	{
		int effectiveW1Size = firstW;
		if (isW1Signed)
		   effectiveW1Size -= deltaWidthUnsignedSigned_;
		int maxLimit = (effectiveW1Size <= maxWordSizeSmallInputUnsigned_) ? maxWordSizeLargeInputUnsigned_: maxWordSizeSmallInputUnsigned_;
		if (isW2signed)
			maxLimit += deltaWidthUnsignedSigned_;

		int inputWidthSumBounds = maxSumOfInputWordSizes_ - effectiveW1Size;
		if (isW2signed)
			inputWidthSumBounds += deltaWidthUnsignedSigned_;

		int finalLimit = (inputWidthSumBounds <= maxLimit) ? inputWidthSumBounds : maxLimit;
		finalLimit = (finalLimit < 0) ? 0 : finalLimit;
		return finalLimit;
	}
}

bool BaseMultiplierCategory::shapeValid(Parametrization const& param, int x, int y) const
{
    auto xw = param.getTileXWordSize();
    auto yw = param.getTileYWordSize();

	return (x > 0 && x < xw && y > 0 && y < yw);
}
