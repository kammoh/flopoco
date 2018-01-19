#include "BaseMultiplierLUT.hpp"

namespace flopoco {


BaseMultiplierLUT::BaseMultiplierLUT(bool isSignedX, bool isSignedY, int wX, int wY) : BaseMultiplier(isSignedX,isSignedY)
{

    srcFileName = "BaseMultiplierLUT";
    uniqueName_ = string("BaseMultiplierLUT") + to_string(wX) + string("x") + to_string(wY);

    this->wX = wX;
    this->wY = wY;

}

Operator* BaseMultiplierLUT::generateOperator(Target* target)
{
    return new BaseMultiplierLUTOp(target, isSignedX, isSignedY, wX, wY);
}


BaseMultiplierLUTTable::BaseMultiplierLUTTable(Target* target, int dx, int dy, int wO, bool negate, bool signedX, bool signedY) : Table(target, dx+dy, wO, 0, -1, true), // logic table
    dx(dx), dy(dy), wO(wO), negate(negate), signedX(signedX), signedY(signedY)
{
    ostringstream name;
    srcFileName="BaseMultiplierLUTTable";

    name <<"BaseMultiplierLUTTable"<< (negate?"M":"P") << dy << "x" << dx << "r" << wO << (signedX?"Xs":"Xu") << (signedY?"Ys":"Yu");
    setName(name.str());
}

mpz_class BaseMultiplierLUTTable::function(int yx)
{
    mpz_class r;
    int y = yx>>dx;
    int x = yx -(y<<dx);
    int wF=dx+dy;

    if(signedX){
        if ( x >= (1 << (dx-1)))
            x -= (1 << dx);
    }
    if(signedY){
        if ( y >= (1 << (dy-1)))
            y -= (1 << dy);
    }
    //if(!negate && signedX && signedY) cerr << "  y=" << y << "  x=" << x;
    r = x * y;
    //if(!negate && signedX && signedY) cerr << "  r=" << r;
    if(negate)
        r=-r;
    //if(negate && signedX && signedY) cerr << "  -r=" << r;
    if ( r < 0)
        r += mpz_class(1) << wF;
    //if(!negate && signedX && signedY) cerr << "  r2C=" << r;

    if(wOut<wF){ // wOut is that of Table
        // round to nearest, but not to nearest even
        int tr=wF-wOut; // number of truncated bits
        // adding the round bit at half-ulp position
        r += (mpz_class(1) << (tr-1));
        r = r >> tr;
    }

    //if(!negate && signedX && signedY) cerr << "  rfinal=" << r << endl;

    return r;

}

BaseMultiplierLUTOp::BaseMultiplierLUTOp(Target* target, bool isSignedX, bool isSignedY, int wX, int wY) : Operator(target)
{
    int wOut = wX + wY;

    ostringstream name;
    name <<"BaseMultiplierLUTOp"<< (isSignedX?"xS":"xU") << (isSignedY?"yS":"yU") << "_" << wX << "_" << wY;
    setName(name.str());
    srcFileName="BaseMultiplierLUTOp";

    addInput("X", wX, true);
    addInput("Y", wY, true);
    addOutput("R", wOut, 1, true);

    vhdl << tab << declare("YX",wX+wY) << " <= Y" << " & " << "X;" << endl;

    BaseMultiplierLUTTable* bmlt = new BaseMultiplierLUTTable(target, wX, wY, wOut, false, isSignedX, isSignedY);

    addSubComponent(bmlt);
    inPortMap(bmlt, "X", "YX");
    outPortMap(bmlt, "Y", "R", false);

    vhdl << instance(bmlt, "BaseMultiplierLUTTable");

}

}   //end namespace flopoco

