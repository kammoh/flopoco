#include "DSPBlock.hpp"

namespace flopoco {

DSPBlock::DSPBlock(Operator *parentOp, Target* target, int wX, int wY, int wZ, bool usePostAdder, bool usePreAdder) : Operator(parentOp,target)
{
    useNumericStd();

    ostringstream name;
    name << "DSPBlock_" << wX << "x" << wY;
    setName(name.str());

    if(usePreAdder)
    {
        addInput("X1", wX);
        addInput("X2", wX);
    }
    else
    {
        addInput("X", wX);
    }
    addInput("Y", wY);
    if(usePostAdder)
    {
        addInput("Z", wZ);
    }

    wR = wX + wY; //ToDo: addjust this for signed
    addOutput("R", wR);

	vhdl << tab << "R <= std_logic_vector(unsigned(X) * unsigned(Y));" << endl;

}

}   //end namespace flopoco

