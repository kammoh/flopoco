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
				deltaWidthSigned,
				0,
				0,
				0
		}{}

		int getDSPCost(uint32_t, uint32_t) const final {return 1;}
		double getLUTCost(uint32_t, uint32_t) const final {return 0.;}

		Operator *generateOperator(
				Operator *parentOp,
				Target *target,
				Parametrization const & params
			) const final;

    private:
        bool flipXY;
        bool pipelineDSPs;
	};
}
#endif
