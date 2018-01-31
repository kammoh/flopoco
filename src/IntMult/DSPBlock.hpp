#pragma once

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {

/**
* The DSPBlock class represents the implementation of a DSP block commonly found in FPGAs.
*
*/
class DSPBlock : public Operator
{
public:
    /**
     * @brief Constructor of a common DSPBlock that translates to DSP implementations when the sizes are chosen accordingly
     *
     * Depending on the inputs, the following operation is performed.
     * usePostAdder | usePreAdder | Operation
     * -------------------------------------------
     *    false     |    false    |  X * Y
     *    true      |    false    |  X * Y + Z
     *    false     |    true     |  (X1+X2) * Y
     *    true      |    true     |  (X1+X2) * Y + Z
     *
     * @param parentOp A pointer to the parent Operator
     * @param target A pointer to the target
     * @param wX input word size of X or X1 and X2 (when pre-adders are used)
     * @param wY input word size of Y
     * @param wZ input word size of Z (when post-adders are used)
     * @param usePostAdder enables post-adders when set to true (see table above)
     * @param usePreAdder enables pre-adders when set to true (see table above)
     */
    DSPBlock(Operator *parentOp, Target* target, int wX, int wY, int wZ=0, bool usePostAdder=false, bool usePreAdder=false);

    /** Factory method */
    static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
    /** Register the factory */
    static void registerFactory();

private:
};

}

