#include "DSPBlock.hpp"

namespace flopoco {

DSPBlock::DSPBlock(Operator *parentOp, Target* target, int wX, int wY, int wZ, bool usePostAdder, bool usePreAdder) : Operator(parentOp,target)
{
    useNumericStd();

    ostringstream name;
    name << "DSPBlock_" << wX << "x" << wY;
    setName(name.str());

    if(wZ == 0 && usePostAdder) THROWERROR("usePostAdder was set to true but no word size for input Z was given.");

    if(usePreAdder)
    {
        addInput("X1", wX);
        addInput("X2", wX);
        wX += 1; //the input X is now one bit wider
        //implement pre-adder:
        vhdl << tab << declare("X",wX) << " <= std_logic_vector(resize(signed(X1)," << wX << ") + resize(signed(X2)," << wX << ")); -- pre-adder" << endl;
    }
    else
    {
        addInput("X", wX);
    }
    addInput("Y", wY);

    int wM = wX + wY;

    if(usePostAdder)
    {
        addInput("Z", wZ);
        int wR = max(wM,wZ)+1;
        addOutput("R", wR);

        vhdl << tab << declare(target->DSPMultiplierDelay()+target->DSPAdderDelay(),"M",wM) << " <= std_logic_vector(signed(X) * signed(Y)); -- multiplier" << endl;
        vhdl << tab << "R <= std_logic_vector(resize(signed(M)," << wR << ") + resize(signed(Z)," << wR << ")); -- post-adder" << endl;

    }
    else
    {
        vhdl << tab << declare(target->DSPMultiplierDelay(),"M",wM) << " <= std_logic_vector(signed(X) * signed(Y)); -- multiplier" << endl;
        addOutput("R", wM);
        vhdl << tab << "R <= M;" << endl;
    }
}

OperatorPtr DSPBlock::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
    int wX,wY,wZ;
    bool usePostAdder, usePreAdder;

    UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
    UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
    UserInterface::parsePositiveInt(args, "wZ", &wZ);
    UserInterface::parseBoolean(args,"usePostAdder",&usePostAdder);
    UserInterface::parseBoolean(args,"usePreAdder",&usePreAdder);

    return new DSPBlock(parentOp,target,wX,wY,wZ,usePostAdder,usePreAdder);
}

void DSPBlock::registerFactory()
{
    UserInterface::add( "DSPBlock", // name
                        "Implements a DSP block commonly found in FPGAs incl. pre-adders and post-adders computing R = (X1+X2) * Y + Z",
                        "BasicInteger", // categories
                        "",
                        "wX(int): size of input X (or X1 and X2 if pre-adders are used);\
                        wY(int): size of input Y;\
                        wZ(int)=0: size of input Z (if post-adder is used);\
                        usePostAdder(bool)=0: use post-adders;\
                        usePreAdder(bool)=0: use pre-adders;",
                       "",
                       DSPBlock::parseArguments
                       ) ;
}


}   //end namespace flopoco

