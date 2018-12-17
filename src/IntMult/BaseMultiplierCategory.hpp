#pragma once
#include <memory>

#include "Target.hpp"
#include "Operator.hpp"

namespace flopoco {
	class BaseMultiplierCategory {
		public:
			// Get the range from this category
			int getMaxUnsignedWordSize() {return maxWordSizeUnsigned_};
			int getSmallWordMaxUnsignedWordSize() {return maxSmallWordSizeUnsigned_;}

			int getMaxSecondWordSize(int firstW, bool isW1Signed, bool isW2signed); 

			void getBoundCoeffs(int& xCoeff, int& yCoeff, uint64_t bound) {
				xCoeff = combinedXCoeff_;
				yCoeff = combinedYCoeff_;
				bound = combinedBound_;
			}

			int getDeltaWidthSigned() { return deltaWidthUnsignedSigned_; }

			virtual int getDSPCost(uint32_t wX, uint32_t wY) = 0;
			virtual double getLUTCost(uint32_t wX, uint32_t wY) = 0;
			virtual Operator *generateOperator(
					Operator *parentOp, 
					Target* target, 
					Parametrization const &parameters
				) = 0;

			class Parametrization{
				public:
					Operator *generateOperator(
							Operator *parentop,
							Target *target
						) {
						return bmCat_->generateOperator(parentOp, target, *this);
					}
				private:
					Parametrization(
							int  wX, 
							int  wY, 
							bool isSignedX=false, 
							bool isSignedY=false,
							bool isFlippedXY=false,
							BaseMultiplierCategory* multCategory;
						):wX_{wX},
						wY{wY},
						isSignedX_{isSignedX},
						isSignedY_{isSignedY},
						isFlippedXY_{isFlippedXY},
						bmCat_{multCategory}{}

					int wX_;
					int wY_;
					bool isSignedX_;
					bool isSignedY_;
					bool isFlippedXY_;
					BaseMultiplierCategory* bmCat_;
			}

		Parametrization parametrize(int wX, int wY, bool isSignedX, bool isSignedY); 

		private:	
			int maxWordSizeUnsigned_;
			int maxSmallWordSizeUnsigned_;
			int deltaWidthUnsignedSigned_;
			int combinedXCoeff_;
			int combinedYCoeff_;
			int combinedBound_;
	};
}
