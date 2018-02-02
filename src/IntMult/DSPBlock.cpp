#include "DSPBlock.hpp"

namespace flopoco {

DSPBlock::DSPBlock(Operator *parentOp, Target* target, int wX, int wY, int pipelineDepth, int wZ, bool usePostAdder, bool usePreAdder, bool preAdderSubtracts) : Operator(parentOp,target)
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

		vhdl << tab << declare(target->DSPAdderDelay(),"X1_resized",18) << " <= std_logic_vector(resize(signed(X1),18));" << endl;
		vhdl << tab << declare(target->DSPAdderDelay(),"X2_resized",18) << " <= std_logic_vector(resize(signed(X2),18));" << endl;

        //implement pre-adder:
		vhdl << tab << declare("X",wX) << " <= std_logic_vector(signed(X1_resized) ";
        if(preAdderSubtracts)
        {
            vhdl << "-";
        }
        else
        {
            vhdl << "+";
        }
		vhdl << " signed(X2_resized)); -- pre-adder" << endl;
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

		//target->DSPMultiplierDelay()+target->DSPAdderDelay()
		vhdl << tab << declare("M",wM) << " <= std_logic_vector(signed(X) * signed(Y)); -- multiplier" << endl;
        vhdl << tab << "R <= std_logic_vector(resize(signed(M)," << wR << ") + resize(signed(Z)," << wR << ")); -- post-adder" << endl;

    }
    else
    {
		double maxTargetCriticalPath = 1.0 / getTarget()->frequency() - getTarget()->ffDelay();
		double totalDelay = pipelineDepth*maxTargetCriticalPath + getTarget()->ffDelay();

		cout << "maxTargetCriticalPath=" << maxTargetCriticalPath << endl;
		cout << "totalDelay=" << totalDelay << endl;

		addOutput("R", wM);

		if(pipelineDepth > 0)
		{
			vhdl << tab << declare("M",wM) << " <= std_logic_vector(signed(X) * signed(Y)); -- multiplier" << endl;
			vhdl << tab << declare(totalDelay,"Rtmp",wM) << " <= M;" << endl;
			vhdl << tab << "R <= Rtmp;" << endl;
		}
		else
		{
			vhdl << tab << declare(target->DSPMultiplierDelay(),"M",wM) << " <= std_logic_vector(signed(X) * signed(Y)); -- multiplier" << endl;
			vhdl << tab << "R <= M;" << endl;
		}
    }
}

OperatorPtr DSPBlock::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
    int wX,wY,wZ;
    bool usePostAdder, usePreAdder, preAdderSubtracts;
	int pipelineDepth;

    UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
    UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
	UserInterface::parsePositiveInt(args, "wZ", &wZ);
	UserInterface::parsePositiveInt(args, "depth", &pipelineDepth);
	UserInterface::parseBoolean(args,"usePostAdder",&usePostAdder);
    UserInterface::parseBoolean(args,"usePreAdder",&usePreAdder);
    UserInterface::parseBoolean(args,"preAdderSubtracts",&preAdderSubtracts);

	return new DSPBlock(parentOp,target,wX,wY,pipelineDepth,wZ,usePostAdder,usePreAdder,preAdderSubtracts);
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
						depth(int)=0: pipeline depth;\
                        usePostAdder(bool)=0: use post-adders;\
                        usePreAdder(bool)=0: use pre-adders;\
                        preAdderSubtracts(bool)=0: if true, the pre-adder performs a pre-subtraction;",
                       "",
                       DSPBlock::parseArguments
                       ) ;
}


}   //end namespace flopoco

