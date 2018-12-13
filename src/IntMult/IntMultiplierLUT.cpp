#include "IntMultiplierLUT.hpp"
#include "Table.hpp"


IntMultiplierLUT::IntMultiplierLUT(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int wX, int wY, bool flipXY) : Operator (parentOp, target), isSignedX(isSignedX), isSignedY(isSignedY), wX(wX), wY(wY)
{
	if(isSignedX || isSignedY)
		THROWERROR("signed input currently not supported by IntMultiplierLUT, sorry");

	if(flipXY)
	{
		//swapp x and y:
		int tmp = wX;
		wX = wY;
		wY = tmp;
	}

	int wR = wX + wY; //!!! check for signed case!

	ostringstream name;
	name << "IntMultiplierLUT_" << wX << (isSignedX==1 ? "_signed" : "") << "x" << wY  << (isSignedY==1 ? "_signed" : "");

	setNameWithFreqAndUID(name.str());

	addInput("X", wX);
	addInput("Y", wY);
	addInput("R", wR);

	vector<mpz_class> val;
	for (int yx=0; yx<1<<(wX+wY); yx++) {
		val.push_back(function(yx));
	}
	Operator *op = new Table(parentOp, target, val);

	UserInterface::addToGlobalOpList(op);

/*
	inPortMap(op, "X", join(addUID("x",blockUid),"_",id));
	inPortMap(op, "Y", join(addUID("y",blockUid),"_",id));
	outPortMap(op, "R", join(addUID("r",blockUid),"_",id));
	vhdl << instance(op, join(addUID("Mult",blockUid),"_", id));
*/
}

mpz_class IntMultiplierLUT::function(int yx)
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
