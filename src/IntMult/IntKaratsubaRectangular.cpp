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

    vhdl << tab << declare("c0",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a0) * unsigned(b0));" << endl;
    bitHeap->addSignal("c0");

    vhdl << tab << declare("c2",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a2) * unsigned(b0));" << endl;
    bitHeap->addSignal("c2",2*TileBaseMultiple);

    vhdl << tab << declare("c3",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a0) * unsigned(b3));" << endl;
    bitHeap->addSignal("c3",3*TileBaseMultiple);

    vhdl << tab << declare("c4",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a4) * unsigned(b0));" << endl;
    bitHeap->addSignal("c4",4*TileBaseMultiple);

    vhdl << tab << declare("c5",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a2) * unsigned(b3));" << endl;
    bitHeap->addSignal("c5",5*TileBaseMultiple);

    vhdl << tab << declare("c7",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a4) * unsigned(b3));" << endl;
    bitHeap->addSignal("c7",7*TileBaseMultiple);

    vhdl << tab << declare("c8",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a2) * unsigned(b6));" << endl;
    bitHeap->addSignal("c8",8*TileBaseMultiple);

    vhdl << tab << declare("c9",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a6) * unsigned(b3));" << endl;
    bitHeap->addSignal("c9",9*TileBaseMultiple);

    vhdl << tab << declare("c10",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a4) * unsigned(b6));" << endl;
    bitHeap->addSignal("c10",10*TileBaseMultiple);

    vhdl << tab << declare("c12",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a6) * unsigned(b6));" << endl;
    bitHeap->addSignal("c12",12*TileBaseMultiple);

#ifdef KARATSUBA
    vhdl << tab << declare("d6",TileWidth+1) << " <= std_logic_vector(signed(resize(unsigned(a0)," << TileWidth+1 << ") - resize(unsigned(a6)," << TileWidth+1 << ")));" << endl;
    declare("k6",TileWidth+TileHeight+2);
    newInstance( "DSPBlock", "dsp"+to_string(0), "wX=24 wY=17 usePreAdder=1 preAdderSubtracts=1","X1=>b0, X2=>b6, Y=>d6", "R=>k6");
    bitHeap->subtractSignal("k6",6*TileBaseMultiple);
    bitHeap->addSignal("c0",6*TileBaseMultiple);
    bitHeap->addSignal("c12",6*TileBaseMultiple);
#else
    vhdl << tab << declare("c6a",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a0) * unsigned(b6));" << endl;
    bitHeap->addSignal("c6a",6*TileBaseMultiple);
    vhdl << tab << declare("c6b",TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a6) * unsigned(b0));" << endl;
    bitHeap->addSignal("c6b",6*TileBaseMultiple);
#endif

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
