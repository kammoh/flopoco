#include "BaseMultiplierCategory.hpp"

namespace flopoco {
	BaseMultiplierCategory::Parametrization BaseMultiplierCategory::parametrize(
			int wX,
			int wY,
			bool isSignedX,
			bool isSignedY
		)
	{
		int effectiveWY = (isSignedY) ? wY - deltaWidthUnsignedSigned_ : wY;
		bool isFlippedXY = (effectiveWY > maxSmallWordSizeUnsigned_);
		if (isFlippedXY) {
			std::swap(wX, wY);
			std::swap(isSignedX, isSignedY);
		}

		int maxSizeY = getMaxSecondWordSize(wX, isSignedX, isSignedY);
		if (maxSizeY < wY) {
			throw std::string("BaseMultiplierCategory::parametrize: error, reqauired multiplier area is too big");
		}

		return Parametrization(wX, wY, isSignedX, isSignedY, isFlippedXY, this);
	}

	int BaseMultiplierCategory::getMaxSecondWordSize(
			int firstW, 
			bool isW1Signed, 
			bool isW2signed
		)
	{
		int effectiveW1Size = firstW;
		if (isW1Signed)
		   effectiveW1Size -= deltaWidthUnsignedSigned_;
		int maxLimit = (effectiveW1Size <= maxSmallWordSizeUnsigned_) ? maxWordSizeUnsigned_: maxSmallWordSizeUnsigned_;
		if (isSignedY)
			maxLimit += deltaWidthUnsignedSigned_;

		bool isBiggerWord = (maxLimit == maxWordSizeUnsigned_);
		int curCoeff = (isBiggerWord) ? combinedXCoeff_ : combinedYCoeff_;
		int otherCoeff = (isBiggerWord) ? combinedYCoeff_ : combinedXCoeff_;
		
		if (combinedBound_ == 0 || curCoeff == 0) {
			return maxLimit;
		}

		int res = (combinedBound_ - otherCoeff * effectiveW1Size) / curCoeff;
		if (isSignedY) {
			res += deltaWidthUnsignedSigned_;
		}
		res = (res <= maxLimit) ? res, maxLimit;
		res = (res < 0) ? 0 : res;
		return res;
	}
}
