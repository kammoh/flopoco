#include <iostream>

#include "IntKaratsubaRectangular.hpp"
#include "IntMult/DSPBlock.hpp"

using namespace std;

namespace flopoco{


    IntKaratsubaRectangular:: IntKaratsubaRectangular(Operator *parentOp, Target* target, int wX, int wY) :
        Operator(parentOp,target), wX(wX), wY(wY), wOut(wX+wY)
    {

		ostringstream name;
        name << "IntKaratsubaRectangular_" << wX << "x" << wY;
        setCopyrightString("Martin Kumm");
		setName(name.str());
	
        addInput ("X", wX);
        addInput ("Y", wY);
        addOutput("R", wOut);
	
		//TODO replace 17 with a generic multiplierWidth()

        DSPBlock *dsp = new DSPBlock(parentOp,target,24,17);

        vhdl << tab << " R <= X * Y" << endl;
	}

    void IntKaratsubaRectangular::emulate(TestCase* tc){
        mpz_class svX = tc->getInputValue("X");
        mpz_class svY = tc->getInputValue("Y");
        mpz_class svR = svX * svY;
        tc->addExpectedOutput("R", svR);
    }

    OperatorPtr IntKaratsubaRectangular::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
    {
        int wX,wY;

        UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
        UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);

        return new IntKaratsubaRectangular(parentOp,target,wX,wY);
    }

    void IntKaratsubaRectangular::registerFactory(){
        UserInterface::add("IntKaratsubaRectangular", // name
                           "Implements a large unsigned Multiplier using rectangular shaped tiles as appears for Xilinx FPGAs. Currently limited to specific, hand-optimized sizes",
                           "BasicInteger", // categories
                           "",
                           "wX(int): size of input X; wY(int): size of input Y;",
                           "",
                           IntKaratsubaRectangular::parseArguments
                           ) ;
    }

}
