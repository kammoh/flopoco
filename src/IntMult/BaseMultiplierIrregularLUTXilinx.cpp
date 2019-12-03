#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "Table.hpp"
#include "IntMultiplier.hpp"

namespace flopoco
{

Operator* BaseMultiplierIrregularLUTXilinx::generateOperator(
        Operator *parentOp,
        Target* target,
        Parametrization const & parameters) const
{
    return new BaseMultiplierIrregularLUTXilinxOp(
            parentOp,
            target,
            parameters.isSignedMultX(),
            parameters.isSignedMultY(),
            (TILE_SHAPE)parameters.getShapePara()
    );
}

const int BaseMultiplierIrregularLUTXilinx::shape_size[8][6] = {{3, 3, 5, 5, 1, 8},  // A (x,y,r,MSB,LSB)
                                                                {3, 3, 3, 4, 2, 5},
                                                                {3, 2, 3, 3, 1, 4},
                                                                {2, 3, 3, 3, 1, 4},
                                                                {3, 2, 4, 3, 0, 5},
                                                                {3, 2, 4, 4, 1, 5},
                                                                {2, 3, 4, 3, 0, 5},
                                                                {2, 3, 4, 4, 1, 5}}; // H

int BaseMultiplierIrregularLUTXilinx::getRelativeResultLSBWeight(Parametrization const& param) const
{
    if(!param.getShapePara() || param.getShapePara() > 8)
        throw string("Error in ") + string("srcFileName") + string(": shape unknown");
    return getRelativeResultLSBWeight((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE)param.getShapePara());
}

int BaseMultiplierIrregularLUTXilinx::getRelativeResultMSBWeight(Parametrization const& param) const
{
    if(!param.getShapePara() || param.getShapePara() > 8)
        throw string("Error in ") + string("srcFileName") + string(": shape unknown");
    return getRelativeResultMSBWeight((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE)param.getShapePara())+1;
}


    bool BaseMultiplierIrregularLUTXilinx::shapeValid(Parametrization const& param, unsigned x, unsigned y) const
    {
        int pattern = BaseMultiplierIrregularLUTXilinxOp::get_pattern((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE)param.getShapePara());
        if(0 < param.getTileXWordSize() && x < param.getTileXWordSize() && 0 < param.getTileYWordSize() && y < param.getTileYWordSize())
            return (pattern & (1 << (y * param.getTileXWordSize() + x)));
        return false;
    }

    bool BaseMultiplierIrregularLUTXilinx::shapeValid(int x, int y)
    {
        int pattern = BaseMultiplierIrregularLUTXilinxOp::get_pattern(this->shape);
        //cout << "pattern " << pattern << " valid " << (pattern & (1 << (y * wX + x))) << " mask " << ( (1 << (y * wX + x)))  << " wX " << wX   << " wY " << wY << endl;
        if(0 < wX && x < wX && 0 < wY && y < wY)
            return (pattern & (1 << (y * wX + x)));
        return false;
    }


BaseMultiplierIrregularLUTXilinxOp::BaseMultiplierIrregularLUTXilinxOp(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, BaseMultiplierIrregularLUTXilinx::TILE_SHAPE shape) : Operator(parentOp,target), shape(shape), wX(BaseMultiplierIrregularLUTXilinx::get_wX(shape)), wY(BaseMultiplierIrregularLUTXilinx::get_wY(shape)), isSignedX(isSignedX), isSignedY(isSignedY)
{
	int wR = static_cast<int>(BaseMultiplierIrregularLUTXilinx::get_wR(shape)); //!!! check for signed case!

	ostringstream name;
	name << "BaseMultiplierIrregularLUTXilinx_" << wX << (isSignedX==1 ? "_signed" : "") << "x" << wY  << (isSignedY==1 ? "_signed" : "");

	setNameWithFreqAndUID(name.str());

	addInput("X", wX);
	addInput("Y", wY);
	addOutput("O", wR);

	if (wX == 1 and not isSignedX) {
		vhdl << tab << declare(0.0, "replicated", wY) << " <= (" << (wY - 1) << " downto 0 => X(0));" << endl;
		vhdl << tab << declare(target->logicDelay(2), "prod", wY) << " <= Y and replicated;" << endl;
		vhdl << tab << "O <= prod;" << endl;
		return;
	} else if (wY == 1 and not isSignedY) {
		vhdl << tab << declare(0.0, "replicated", wX) << " <= (" << (wX - 1) << " downto 0 => Y(0));" << endl;
		vhdl << tab << declare(target->logicDelay(2), "prod", wX) << " <= X and replicated;" << endl;
		vhdl << tab << "O <= prod;" << endl;
		return;
	}

	vector<mpz_class> val;
	REPORT(DEBUG, "Filling table for a non-rectangular LUT multiplier of max size " << wX << "x" << wY << " (out put size is " << wR << ")")
	for (int yx=0; yx < 1<<(wX+wY); yx++)
	{
		val.push_back(function(yx));
	}
	Operator *op = new Table(this, target, val, "MultTable", wX+wY, wR);
	op->setShared();
	UserInterface::addToGlobalOpList(op);

	vhdl << declare(0.0,"Xtable",wX+wY) << " <= Y & X;" << endl;

    inPortMap("X", "Xtable");
    outPortMap("Y", "Y1");

    vhdl << tab << "O <= Y1;" << endl;
	vhdl << instance(op, "TableMult");
}

const unsigned short BaseMultiplierIrregularLUTXilinxOp::bit_pattern[8] = {0x1fe, 0xf4, 0x1e, 0x1e, 0x1f, 0x3e, 0x1f, 0x3e};

mpz_class BaseMultiplierIrregularLUTXilinxOp::function(int yx)
{
    int temp = 0;
	int y = yx>>wX;
	int x = yx -(y<<wX);

    for( int yp = 0; yp < wY; yp++)
        for( int xp = 0; xp < wX; xp++)
            temp += (bit_pattern[(this->shape)-1]&(1<<(yp*wX+xp)))?(x&(1<<xp))*(y&(1<<yp)):0;

    mpz_class r  = (temp >> BaseMultiplierIrregularLUTXilinx::getRelativeResultLSBWeight(this->shape));

	REPORT(DEBUG, "Value for x=" << x << ", y=" << y << " : " << r.get_str(2))

	return r;

}

OperatorPtr BaseMultiplierIrregularLUTXilinx::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
	int wS;
	UserInterface::parseStrictlyPositiveInt(args, "wS", &wS);
	return new BaseMultiplierIrregularLUTXilinxOp(parentOp,target, false, false, (BaseMultiplierIrregularLUTXilinx::TILE_SHAPE)wS);
}

void BaseMultiplierIrregularLUTXilinx::registerFactory(){
	UserInterface::add("BaseMultiplierIrregularLUTXilinx", // name
					   "Implements a non rectangular LUT multiplier from a set that yields a relatively high efficiency compared to recangular LUT multipliers \n"
        "  _ _     _        _ _       _     _ _ _    _ _      _ _     _\n"
        " |_|_|_  |_|_     |_|_|_    |_|_  |_|_|_|  |_|_|_   |_|_|   |_|_\n"
        " |_|_|_| |_|_|_     |_|_|   |_|_|   |_|_|  |_|_|_|  |_|_|   |_|_|\n"
        " |_|_|_|   |_|_|              |_|                     |_|   |_|_|\n"
        " shape 1  shape 2 shape 3 shape 4 shape 5  shape 6 shape 7  shape 8\n"

					   ,
					   "BasicInteger", // categories
					   "",
					   "wS(int): shape ID",
					   "",
					   BaseMultiplierIrregularLUTXilinx::parseArguments,
					   BaseMultiplierIrregularLUTXilinxOp::unitTest
	) ;
}

void BaseMultiplierIrregularLUTXilinxOp::emulate(TestCase* tc)
{
	mpz_class svX = tc->getInputValue("X");
	mpz_class svY = tc->getInputValue("Y");
    mpz_class svR = 0;
    for( int yp = 0; yp < wY; yp++) {
        for (int xp = 0; xp < wX; xp++)
            if (bit_pattern[(this->shape) - 1] & (1 << (yp * wX + xp)))
                svR += (svX & (1 << xp)) * (svY & (1 << yp));
    }
    svR >>= BaseMultiplierIrregularLUTXilinx::getRelativeResultLSBWeight(this->shape);
	tc->addExpectedOutput("O", svR);
}

TestList BaseMultiplierIrregularLUTXilinxOp::unitTest(int index)
{
	// the static list of mandatory tests
	TestList testStateList;
	vector<pair<string,string>> paramList;

	//test square multiplications:
	for(int w=1; w <= 6; w++)
	{
		paramList.push_back(make_pair("wS", to_string(w)));
		testStateList.push_back(paramList);
		paramList.clear();
	}

	return testStateList;
}


}
