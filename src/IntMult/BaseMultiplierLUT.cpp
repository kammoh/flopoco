#include "BaseMultiplierLUT.hpp"

namespace flopoco {


BaseMultiplierLUT::BaseMultiplierLUT(bool isSignedX, bool isSignedY, int wX, int wY) : BaseMultiplier(isSignedX,isSignedY)
{

    srcFileName = "BaseMultiplierLUT";
    uniqueName_ = string("BaseMultiplierLUT") + to_string(wX) + string("x") + to_string(wY);

    this->wX = wX;
    this->wY = wY;

}

Operator* BaseMultiplierLUT::generateOperator(Operator *parentOp, Target* target)
{

	vector<mpz_class> val;
	for (int yx=0; yx<1<<(wX+wY); yx++) {
		val.push_back(function(yx));
	}
	// ostringstream name;
  //  srcFileName="BaseMultiplierLUTTable";

	//   name <<"BaseMultiplierLUTTable"<< (negate?"M":"P") << dy << "x" << dx << "r" << wO << (signedX?"Xs":"Xu") << (signedY?"Ys":"Yu");

	return new Table(parentOp, target, val);
}



mpz_class BaseMultiplierLUT::function(int yx)
{
    mpz_class r;
    int y = yx>>wX;
    int x = yx -(y<<wX);
    int wF=wX+wY;

    if(isSignedX){
        if ( x >= (1 << (wX-1)))
            x -= (1 << wX);
    }
    if(isSignedY){
        if ( y >= (1 << (wY-1)))
            y -= (1 << wY);
    }
    //if(!negate && isSignedX && isSignedY) cerr << "  y=" << y << "  x=" << x;
    r = x * y;
    //if(!negate && isSignedX && isSignedY) cerr << "  r=" << r;
		// if(negate)       r=-r;
    //if(negate && isSignedX && isSignedY) cerr << "  -r=" << r;
    if ( r < 0)
        r += mpz_class(1) << wF;
    //if(!negate && isSignedX && isSignedY) cerr << "  r2C=" << r;

    if(wX+wY<wF){ // wOut is that of Table
        // round to nearest, but not to nearest even
			int tr=wF-wX-wY; // number of truncated bits
        // adding the round bit at half-ulp position
        r += (mpz_class(1) << (tr-1));
        r = r >> tr;
    }
    //if(!negate && isSignedX && isSignedY) cerr << "  rfinal=" << r << endl;

    return r;

}


}   //end namespace flopoco

