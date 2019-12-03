#ifndef BaseMultiplierDSP_HPP
#define BaseMultiplierDSP_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BaseMultiplierCategory.hpp"

namespace flopoco {


    class BaseMultiplierDSP : public BaseMultiplierCategory
    {

	public:
		BaseMultiplierDSP(
				int maxWidthBiggestWord,
				int maxWidthSmallestWord,
				int deltaWidthSigned
			) : BaseMultiplierCategory{
				maxWidthBiggestWord,
				maxWidthSmallestWord,
				false,
                false,
				-1,
				"BaseMultiplierDSP"
		}{lut_cost = 0.65*(maxWidthBiggestWord+maxWidthSmallestWord);}

        BaseMultiplierDSP(
                int wX,
                int wY,
                bool isSignedX,
                bool isSignedY
        ) : BaseMultiplierCategory{
                wX,
                wY,
                isSignedX,
                isSignedY,
                -1,
                "BaseMultiplierDSP"
        }{lut_cost = 0.65*(wX+wY);}

		int getDSPCost(uint32_t, uint32_t) const final {return 1;}
		double getLUTCost(uint32_t, uint32_t) const final {return 0.;}
        int getDSPCost() const final {return 1;}
        double getLUTCost() const {return 0.;}

		Operator* generateOperator(
				Operator *parentOp,
				Target *target,
				Parametrization const & parameters
			) const final;

    private:
        bool flipXY;
        bool pipelineDSPs;
	};
}
#endif
