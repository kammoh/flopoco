#pragma once
#include <memory>

#include "Target.hpp"
#include "Operator.hpp"

namespace flopoco {
	class BaseMultiplierCategory {
		public:
			// Get the range from this category
			int getMaxUnsignedWordSize() const {return maxWordSizeUnsigned_;}
			int getSmallWordMaxUnsignedWordSize() const {return maxSmallWordSizeUnsigned_;}

			int getMaxSecondWordSize (int firstW, bool isW1Signed, bool isW2signed) const; 

			void getBoundCoeffs (int& xCoeff, int& yCoeff, uint64_t bound) const {
				xCoeff = combinedXCoeff_;
				yCoeff = combinedYCoeff_;
				bound = combinedBound_;
			}

			BaseMultiplierCategory(
					int maxWordSizeUnsigned,
					int maxSmallWordSizeUnsigned,
					int deltaWidthUnsignedSigned,
					int combinedXCoeff,
					int combinedYCoeff,
					int combinedBound,
					string type="undefined!"
				):maxWordSizeUnsigned_{maxWordSizeUnsigned},
					maxSmallWordSizeUnsigned_{maxSmallWordSizeUnsigned},
					deltaWidthUnsignedSigned_{deltaWidthUnsignedSigned},
					combinedXCoeff_{combinedXCoeff},
					combinedYCoeff_{combinedYCoeff},
					combinedBound_{combinedBound},
				  	type_{type}
				{}

			int getDeltaWidthSigned() const { return deltaWidthUnsignedSigned_; }

			virtual int getDSPCost(uint32_t wX, uint32_t wY) const = 0;
			virtual double getLUTCost(uint32_t wX, uint32_t wY) const = 0;

			string getType() const {return type_;}

			class Parametrization{
				public:
					Operator *generateOperator(
							Operator *parentOp,
							Target *target
						) {
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
			int maxWordSizeUnsigned_;
			int maxSmallWordSizeUnsigned_;
			int deltaWidthUnsignedSigned_;
			int combinedXCoeff_;
			int combinedYCoeff_;
			int combinedBound_;

			string type_; /**< Name to identify the corresponding base multiplier in the solution (for debug only) */
	};

	typedef BaseMultiplierCategory::Parametrization BaseMultiplierParametrization;
}
