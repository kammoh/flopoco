#pragma once
#include <memory>

#include "Target.hpp"
#include "Operator.hpp"

namespace flopoco {
	class BaseMultiplierCategory {
		public:
			// Get the range from this category
			int getMaxWordSizeLargeInputUnsigned() const {return maxWordSizeLargeInputUnsigned_;}
			int getMaxWordSizeSmallInputUnsigned() const {return maxWordSizeSmallInputUnsigned_;}

			int getMaxSecondWordSize (int firstW, bool isW1Signed, bool isW2signed) const;

			BaseMultiplierCategory(
					int maxWordSizeLargeInputUnsigned, //maxWordSizeLargeInputUnsigned
					int maxWordSizeSmallInputUnsigned,
					int deltaWidthUnsignedSigned,
					int maxSumOfInputWordSizes = 0,
					string type="undefined!"
				):maxWordSizeLargeInputUnsigned_{maxWordSizeLargeInputUnsigned},
					maxWordSizeSmallInputUnsigned_{maxWordSizeSmallInputUnsigned},
					deltaWidthUnsignedSigned_{deltaWidthUnsignedSigned},
				  	type_{type}
				{
					maxSumOfInputWordSizes_ = (maxSumOfInputWordSizes > 0) ?
								(maxSumOfInputWordSizes) :
								maxWordSizeLargeInputUnsigned + maxWordSizeSmallInputUnsigned;
				}

			int getDeltaWidthSigned() const { return deltaWidthUnsignedSigned_; }

			virtual int getDSPCost(uint32_t wX, uint32_t wY) const = 0;
			virtual double getLUTCost(uint32_t wX, uint32_t wY) const = 0;

			string getType() const {return type_;}

			class Parametrization{
				public:
					Operator *generateOperator(
							Operator *parentOp,
							Target *target
						) const {
						return bmCat_->generateOperator(parentOp, target, *this);
					}

                    unsigned int getMultXWordSize() const {return wX_;}
                    unsigned int getMultYWordSize() const {return wY_;}
                    unsigned int getTileXWordSize() const {return (isFlippedXY_) ? wY_ : wX_;}
                    unsigned int getTileYWordSize() const {return (isFlippedXY_) ? wX_ : wY_;}
					unsigned int getOutWordSize() const;
					bool shapeValid(int x, int y) const;
                    bool isSignedMultX() const {return isSignedX_;}
                    bool isSignedMultY() const {return isSignedY_;}
					bool isFlippedXY() const {return isFlippedXY_;}

				string getMultType() const {return bmCat_->getType();}

				private:
					Parametrization(
							unsigned int  wX, 
							unsigned int  wY, 
							BaseMultiplierCategory const * multCategory,
							bool isSignedX=false, 
							bool isSignedY=false,
							bool isFlippedXY=false
						):wX_{wX},
						wY_{wY},
						isSignedX_{isSignedX},
						isSignedY_{isSignedY},
						isFlippedXY_{isFlippedXY},
						bmCat_{multCategory}{}

					unsigned int wX_;
					unsigned int wY_;
					bool isSignedX_;
					bool isSignedY_;
					bool isFlippedXY_;
					BaseMultiplierCategory const * bmCat_;

				friend BaseMultiplierCategory;
			};

			virtual bool shapeValid(Parametrization const & param, int x, int y) const;


			virtual Operator* generateOperator(
					Operator *parentOp, 
					Target* target, 
					Parametrization const &parameters
				) const = 0;

		Parametrization parametrize(int wX, int wY, bool isSignedX, bool isSignedY) const; 

		private:	
			int maxWordSizeLargeInputUnsigned_;
			int maxWordSizeSmallInputUnsigned_;
			int deltaWidthUnsignedSigned_;
			int maxSumOfInputWordSizes_;

			string type_; /**< Name to identify the corresponding base multiplier in the solution (for debug only) */
	};

	typedef BaseMultiplierCategory::Parametrization BaseMultiplierParametrization;
}
