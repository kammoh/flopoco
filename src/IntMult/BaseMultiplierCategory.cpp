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

    int BaseMultiplierCategory::Parametrization::getRelativeResultLSBWeight() const
    {
        return bmCat_->getRelativeResultLSBWeight(*this);
    }

    int BaseMultiplierCategory::Parametrization::getRelativeResultMSBWeight() const
    {
        return bmCat_->getRelativeResultMSBWeight(*this);
    }

	BaseMultiplierCategory::Parametrization BaseMultiplierCategory::parametrize(
			int wX,
			int wY,
			bool isSignedX,
			bool isSignedY,
			int shape_para
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
            //cerr << "maxSizeY=" << maxWordSizeLargeInputUnsigned_ << " wY=" << wY << "maxSizeX=" << maxWordSizeSmallInputUnsigned_ << " wX=" << wX << endl;
			throw std::string("BaseMultiplierCategory::parametrize: error, required multiplier area is too big");
		}

		return Parametrization(wX, wY, this, isSignedX, isSignedY, isFlippedXY, shape_para);
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
		if (effectiveW1Size > maxWordSizeLargeInputUnsigned_)
			return 0;
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

bool BaseMultiplierCategory::shapeValid(Parametrization const& param, unsigned x, unsigned y) const
{
    auto xw = param.getTileXWordSize();
    auto yw = param.getTileYWordSize();

	return (x >= 0 && x < xw && y >= 0 && y < yw);
}

int BaseMultiplierCategory::getRelativeResultLSBWeight(Parametrization const & param) const
{
    return 0;
}

int BaseMultiplierCategory::getRelativeResultMSBWeight(Parametrization const & param) const
{
    if(param.getTileXWordSize() == 0 || param.getTileYWordSize() == 0)
        return 0;
    if(param.getTileXWordSize() == 1)
        return param.getTileYWordSize();
    if(param.getTileYWordSize() == 1)
        return param.getTileXWordSize();
    return param.getTileXWordSize() + param.getTileYWordSize();
}