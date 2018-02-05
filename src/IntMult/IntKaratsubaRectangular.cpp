#include <iostream>

#include "IntKaratsubaRectangular.hpp"
#include "IntMult/DSPBlock.hpp"
#include "assert.h"

using namespace std;

namespace flopoco{


IntKaratsubaRectangular:: IntKaratsubaRectangular(Operator *parentOp, Target* target, int wX, int wY, bool useKaratsuba) :
	Operator(parentOp,target), wX(wX), wY(wY), wOut(wX+wY), useKaratsuba(useKaratsuba)
{

    ostringstream name;
    name << "IntKaratsubaRectangular_" << wX << "x" << wY;
    setCopyrightString("Martin Kumm");
    setName(name.str());

    useNumericStd();

    addInput ("X", wX);
    addInput ("Y", wY);
    addOutput("R", wOut);

    for(int x=0; x < wX/TileWidth; x++)
    {
        vhdl << tab << declare("a" + to_string(TileWidthMultiple*x),TileWidth) << " <= X(" << (x+1)*TileWidth-1 << " downto " << x*TileWidth << ");" << endl;
    }
    for(int y=0; y < wY/TileHeight; y++)
    {
        vhdl << tab << declare("b" + to_string(TileHeightMultiple*y),TileHeight) << " <= Y(" << (y+1)*TileHeight-1 << " downto " << y*TileHeight << ");" << endl;
    }

	bitHeap = new BitheapNew(this, wOut+2);

//	double maxTargetCriticalPath = 1.0 / getTarget()->frequency() - getTarget()->ffDelay();
//	multDelay = 3*maxTargetCriticalPath;
	multDelay = 0;

	if(wX == 64 && wY == 72)
	{
		createMult(0, 0);
		createMult(2, 0);
		createMult(0, 3);
		createMult(4, 0);
		createMult(2, 3);
		createMult(4, 3);
		createMult(2, 6);
		createMult(6, 3);
		createMult(4, 6);
		createMult(6, 6);

		createKaratsubaRect(0,6,6,0);
	}
	else if(wX == 96 && wY == 96)
	{
		createMult(0, 0);
		createMult(2, 0);
		createMult(0, 3);
		createMult(4, 0);
		createMult(2, 3);
		createMult(4, 3);
		createMult(6, 6);
		createMult(8, 6);
		createMult(6, 9);
		createMult(10, 6);
		createMult(8, 9);
		createMult(10, 9);

		createKaratsubaRect(0,6,6,0);
		createKaratsubaRect(2,6,8,0);
		createKaratsubaRect(0,9,6,3);
		createKaratsubaRect(4,6,10,0);
		createKaratsubaRect(2,9,8,3);
		createKaratsubaRect(4,9,10,3);
	}
	else if(wX == 112 && wY == 120)
	{
		//multipliers required for sharing in Karatsuba
		createMult(0, 0);
		createMult(2, 0);
		createMult(4, 0);
		createMult(0, 3);
		createMult(2, 3);
		createMult(4, 3);
		createMult(6, 6);
		createMult(8, 6);
		createMult(10, 6);
		createMult(6, 9);
		createMult(8, 9);
		createMult(10, 9);
		createMult(12, 12);

		//applying the Karatsuba shared multipliers:
		createKaratsubaRect(0,6,6,0);
		createKaratsubaRect(2,6,8,0);
		createKaratsubaRect(4,6,10,0);
		createKaratsubaRect(0,9,6,3);
		createKaratsubaRect(2,9,8,3);
		createKaratsubaRect(4,9,10,3);
		createKaratsubaRect(0,12,12,0);
		createKaratsubaRect(6,12,12,6);

		//multipliers not used for Karatsuba:
		createMult(12, 3);
		createMult(12, 9);
		createMult(2, 12);
		createMult(4, 12);
		createMult(8, 12);
		createMult(10, 12);
	}
	else
	{
		THROWERROR("There is no predifined solution for the given input word sizes, sorry.")
	}

    //compress the bitheap
    bitHeap -> startCompression();

    vhdl << tab << "R" << " <= " << bitHeap->getSumName() <<
            range(wOut-1, 0) << ";" << endl;

}

void IntKaratsubaRectangular::createMult(int i, int j)
{
	/*
	vhdl << tab << declare(multDelay,"c" + to_string(i) + "_" + to_string(j),TileWidth+TileHeight) << " <= std_logic_vector(unsigned(a" <<  + i << " ) * unsigned(b" <<  + j << "));" << endl;
*/
	cout << "implementing a" << i << " * b" << j << " with weight " << (i+j) << endl;
	if(!isSignalDeclared("a" + to_string(i) + "se"))
		vhdl << tab << declare("a" + to_string(i) + "se",17) << " <= std_logic_vector(resize(unsigned(a" << i << "),17));" << endl;
	if(!isSignalDeclared("b" + to_string(j) + "se"))
		vhdl << tab << declare("b" + to_string(j) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << j << "),25));" << endl;
	if(!isSignalDeclared("zero25"))
		vhdl << tab << declare("zero25",25) << " <= (others => '0');" << endl;

	newInstance( "DSPBlock", "dsp" + to_string(i) + "_" + to_string(j), "wX=25 wY=17 usePreAdder=1 preAdderSubtracts=1 isPipelined=0","X1=>b" + to_string(j) + "se, X2=>zero25, Y=>a" + to_string(i) + "se", "R=>c" + to_string(i) + "_" + to_string(j));


	bitHeap->addSignal("c" + to_string(i) + "_" + to_string(j),(i+j)*TileBaseMultiple);
}

void IntKaratsubaRectangular::createKaratsubaRect(int i, int j, int k, int l)
{
	assert(i+j == k+l);

	if(useKaratsuba)
	{
		cout << "implementing a" << i << " * b" << j << " + a" << k << " * b" << l << " with weight " << (i+j) << " as (a" << i << " - a" << k << ") * (b" << j << " - b" << l << ") + a" << i << " * b" << l << " + a" << k << " * b" << j << "" << endl;

		if(!isSignalDeclared("d" + to_string(i) + "_" + to_string(k)))
			vhdl << tab << declare("d" + to_string(i) + "_" + to_string(k),TileWidth+1) << " <= std_logic_vector(signed(resize(unsigned(a" << i << ")," << TileWidth+1 << ")) - signed(resize(unsigned(a" << k << ")," << TileWidth+1 << ")));" << endl;
		getSignalByName(declare("k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),42))->setIsSigned();

		if(!isSignalDeclared("b" + to_string(l) + "se"))
			vhdl << tab << declare("b" + to_string(l) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << l << "),25));" << endl;
		if(!isSignalDeclared("b" + to_string(j) + "se"))
			vhdl << tab << declare("b" + to_string(j) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << j << "),25));" << endl;

		newInstance( "DSPBlock", "dsp" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l), "wX=25 wY=17 usePreAdder=1 preAdderSubtracts=1 isPipelined=0","X1=>b" + to_string(j) + "se, X2=>b" + to_string(l) + "se, Y=>d" + to_string(i) + "_" + to_string(k), "R=>k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l));
		bitHeap->addSignal("k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),(i+j)*TileBaseMultiple);
		bitHeap->addSignal("c" + to_string(i) + "_" + to_string(l),(i+j)*TileBaseMultiple);
		bitHeap->addSignal("c" + to_string(k) + "_" + to_string(j),(i+j)*TileBaseMultiple);
	}
	else
	{
		createMult(i, j);
		createMult(k, l);
	}
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
	bool useKaratsuba;

    UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
	UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
	UserInterface::parseBoolean(args, "useKaratsuba", &useKaratsuba);

	return new IntKaratsubaRectangular(parentOp,target,wX,wY,useKaratsuba);
}

void IntKaratsubaRectangular::registerFactory(){
    UserInterface::add("IntKaratsubaRectangular", // name
                       "Implements a large unsigned Multiplier using rectangular shaped tiles as appears for Xilinx FPGAs. Currently limited to specific, hand-optimized sizes",
                       "BasicInteger", // categories
                       "",
					   "wX(int): size of input X; wY(int): size of input Y;useKaratsuba(bool)=1: Uses Karatsuba when set to 1, instead a standard tiling without sharing is used.",
                       "",
                       IntKaratsubaRectangular::parseArguments
                       ) ;
}

}
