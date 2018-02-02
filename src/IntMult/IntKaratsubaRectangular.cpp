#include <iostream>

#include "IntKaratsubaRectangular.hpp"
#include "IntMult/DSPBlock.hpp"
#include "BitHeap/BitheapNew.hpp"

using namespace std;

namespace flopoco{


IntKaratsubaRectangular:: IntKaratsubaRectangular(Operator *parentOp, Target* target, int wX, int wY) :
    Operator(parentOp,target), wX(wX), wY(wY), wOut(wX+wY)
{

    ostringstream name;
    name << "IntKaratsubaRectangular_" << wX << "x" << wY;
    setCopyrightString("Martin Kumm");
    setName(name.str());

    useNumericStd();

    addInput ("X", wX);
    addInput ("Y", wY);
    addOutput("R", wOut);

    const int TileBaseMultiple=8;
    const int TileWidthMultiple=2;
    const int TileHeightMultiple=3;
    const int TileWidth=TileBaseMultiple*TileWidthMultiple;
    const int TileHeight=TileBaseMultiple*TileHeightMultiple;

    for(int x=0; x < wX/TileWidth; x++)
    {
        vhdl << tab << declare("a" + to_string(TileWidthMultiple*x),TileWidth) << " <= X(" << (x+1)*TileWidth-1 << " downto " << x*TileWidth << ");" << endl;
    }
    for(int y=0; y < wY/TileHeight; y++)
    {
        vhdl << tab << declare("b" + to_string(TileHeightMultiple*y),TileHeight) << " <= Y(" << (y+1)*TileHeight-1 << " downto " << y*TileHeight << ");" << endl;
    }

    BitheapNew *bitHeap = new BitheapNew(this, wOut+1);

	double maxTargetCriticalPath = 1.0 / getTarget()->frequency() - getTarget()->ffDelay();
//	double multDelay = 0.5*maxTargetCriticalPath;
	double multDelay = 0;

	vhdl << tab << declare(multDelay,"c0",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a0) * unsigned(b0));" << endl;
    bitHeap->addSignal("c0");

	vhdl << tab << declare(multDelay,"c2",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a2) * unsigned(b0));" << endl;
    bitHeap->addSignal("c2",2*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c3",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a0) * unsigned(b3));" << endl;
    bitHeap->addSignal("c3",3*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c4",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a4) * unsigned(b0));" << endl;
    bitHeap->addSignal("c4",4*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c5",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a2) * unsigned(b3));" << endl;
    bitHeap->addSignal("c5",5*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c7",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a4) * unsigned(b3));" << endl;
    bitHeap->addSignal("c7",7*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c8",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a2) * unsigned(b6));" << endl;
    bitHeap->addSignal("c8",8*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c9",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a6) * unsigned(b3));" << endl;
    bitHeap->addSignal("c9",9*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c10",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a4) * unsigned(b6));" << endl;
    bitHeap->addSignal("c10",10*TileBaseMultiple);

	vhdl << tab << declare(multDelay,"c12",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a6) * unsigned(b6));" << endl;
    bitHeap->addSignal("c12",12*TileBaseMultiple);


	bool useRectangularKaratsuba=true;
	if(useRectangularKaratsuba)
	{
#define KARATSUBA_SUB
#ifdef KARATSUBA_SUB
		vhdl << tab << declare("d6",TileWidth+1) << " <= std_logic_vector(signed(resize(unsigned(a0)," << TileWidth+1 << ")) - signed(resize(unsigned(a6)," << TileWidth+1 << ")));" << endl;
		getSignalByName(declare("k6",42))->setIsSigned();
		vhdl << tab << declare("b0se",25) << " <= std_logic_vector(resize(unsigned(b0),25));" << endl;
		vhdl << tab << declare("b6se",25) << " <= std_logic_vector(resize(unsigned(b6),25));" << endl;
		newInstance( "DSPBlock", "dsp"+to_string(0), "wX=25 wY=17 usePreAdder=1 preAdderSubtracts=1 isPipelined=1","X1=>b0se, X2=>b6se, Y=>d6", "R=>k6");
		bitHeap->subtractSignal("k6",6*TileBaseMultiple);
		bitHeap->addSignal("c0",6*TileBaseMultiple);
		bitHeap->addSignal("c12",6*TileBaseMultiple);
#else //Add
		vhdl << tab << declare("d6",TileWidth+1) << " <= std_logic_vector(signed(resize(unsigned(a0)," << TileWidth+1 << ")) + signed(resize(unsigned(a6)," << TileWidth+1 << ")));" << endl;
		declare("k6",42);
		vhdl << tab << declare("b0se",25) << " <= std_logic_vector(resize(unsigned(b0),25));" << endl;
		vhdl << tab << declare("b6se",25) << " <= std_logic_vector(resize(unsigned(b6),25));" << endl;
		newInstance( "DSPBlock", "dsp"+to_string(0), "wX=25 wY=17 usePreAdder=1 preAdderSubtracts=0 isPipelined=0","X1=>b0se, X2=>b6se, Y=>d6", "R=>k6");
		bitHeap->addSignal("k6",6*TileBaseMultiple);
		bitHeap->subtractSignal("c0",6*TileBaseMultiple);
		bitHeap->subtractSignal("c12",6*TileBaseMultiple);
#endif
	}
	else
	{
		vhdl << tab << declare(multDelay,"c6a",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a0) * unsigned(b6));" << endl;
		bitHeap->addSignal("c6a",6*TileBaseMultiple);
		vhdl << tab << declare(multDelay,"c6b",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a6) * unsigned(b0));" << endl;
		bitHeap->addSignal("c6b",6*TileBaseMultiple);
	}

    //compress the bitheap
    bitHeap -> startCompression();

    vhdl << tab << "R" << " <= " << bitHeap->getSumName() <<
            range(wOut-1, 0) << ";" << endl;

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
