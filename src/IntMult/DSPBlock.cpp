#include "DSPBlock.hpp"

namespace flopoco {

DSPBlock::DSPBlock(Operator *parentOp, Target* target, int wX, int wY, bool xIsSigned, bool yIsSigned, bool isPipelined, int wZ, bool usePostAdder, bool usePreAdder, bool preAdderSubtracts) : Operator(parentOp,target)
{
    useNumericStd();

    ostringstream name;
//	name << "DSPBlock_" << wX << "x" << wY << (usePreAdder==1 ? "_PreAdd" : "") << (usePostAdder==1 ? "_PostAdd" : "") << (isPipelined==1 ? "_pip" : "") << "_uid" << getNewUId();
	name << "DSPBlock_" << wX << "x" << wY << (usePreAdder==1 && !preAdderSubtracts ? "_PreAdd" : usePreAdder==1 && preAdderSubtracts ? "_PreSub" : "") << (usePostAdder==1 ? "_PostAdd" : "") << (isPipelined==1 ? "_pip" : "");
	setShared(); //set this operator to be a shared operator, does not work for pipelined ones!!
	setSequential(); //Dirty hack!!

	setNameWithFreqAndUID(name.str());

	if(wZ == 0 && usePostAdder) THROWERROR("usePostAdder was set to true but no word size for input Z was given.");

	double maxTargetCriticalPath = 1.0 / getTarget()->frequency() - getTarget()->ffDelay();
	double stageDelay;
	if(isPipelined) stageDelay = 0.9 * maxTargetCriticalPath;


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

	int wM; //the width of the multiplier result
	if(usePreAdder)
    {

        //implement pre-adder:
		if(!isPipelined) stageDelay = getTarget()->DSPAdderDelay();
		vhdl << tab << declare(stageDelay,"X",wX) << " <= std_logic_vector(" << (xIsSigned ? "signed" : "unsigned") << "(X1) ";
        if(preAdderSubtracts)
        {
            vhdl << "-";
        }
        else
        {
            vhdl << "+";
        }
		vhdl << " " << (xIsSigned ? "signed" : "unsigned") << "(X2)); -- pre-adder" << endl;
    }
	wM = wX + wY;

//	cout << "maxTargetCriticalPath=" << maxTargetCriticalPath << endl;

	if(!isPipelined) stageDelay = getTarget()->DSPMultiplierDelay();
	vhdl << tab << declare(stageDelay,"M",wM) << " <= std_logic_vector(" << (xIsSigned ? "signed" : "unsigned") << "(X) * " << (yIsSigned ? "signed" : "unsigned") << "(Y)); -- multiplier" << endl;

	if(usePostAdder)
    {
		if(wZ > wM) THROWERROR("word size for input Z (which is " << wZ << " ) must be less or equal to word size of multiplier result (which is " << wM << " ).");
		addInput("Z", wZ);
		if(!isPipelined) stageDelay = getTarget()->DSPAdderDelay();
		vhdl << tab << declare(stageDelay,"A",wM) << " <= std_logic_vector(" << (xIsSigned ? "signed" : "unsigned") << "(M) + " << (xIsSigned ? "signed" : "unsigned") << "(Z)); -- post-adder" << endl;
		if(!isPipelined) stageDelay = 0;
		vhdl << tab << declare(stageDelay,"Rtmp",wM) << " <= A;" << endl;
	}
    else
    {
		vhdl << tab << declare(stageDelay,"Rtmp",wM) << " <= M;" << endl;
    }
	addOutput("R", wM);
	vhdl << tab << "R <= Rtmp;" << endl;
}

OperatorPtr DSPBlock::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
    int wX,wY,wZ;
    bool usePostAdder, usePreAdder, preAdderSubtracts;
	bool isPipelined;
	bool xIsSigned,yIsSigned;
    UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
    UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
	UserInterface::parsePositiveInt(args, "wZ", &wZ);
	UserInterface::parseBoolean(args, "isPipelined", &isPipelined);
	UserInterface::parseBoolean(args,"usePostAdder",&usePostAdder);
	UserInterface::parseBoolean(args,"usePreAdder",&usePreAdder);
	UserInterface::parseBoolean(args,"xIsSigned",&xIsSigned);
	UserInterface::parseBoolean(args,"yIsSigned",&yIsSigned);
	UserInterface::parseBoolean(args,"preAdderSubtracts",&preAdderSubtracts);

	return new DSPBlock(parentOp,target,wX,wY,xIsSigned,yIsSigned,isPipelined,wZ,usePostAdder,usePreAdder,preAdderSubtracts);
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
						xIsSigned(bool)=0: input X is signed;\
						yIsSigned(bool)=0: input Y is signed;\
						isPipelined(bool)=1: every stage is pipelined when set to 1;\
                        usePostAdder(bool)=0: use post-adders;\
                        usePreAdder(bool)=0: use pre-adders;\
                        preAdderSubtracts(bool)=0: if true, the pre-adder performs a pre-subtraction;",
                       "",
                       DSPBlock::parseArguments
                       ) ;
}


}   //end namespace flopoco
