#include "IntMult/TilingStrategy.hpp"

namespace flopoco {


TilingStrategy::TilingStrategy(int wX, int wY, int wOut, bool signedIO, BaseMultiplierCollection *baseMultiplierCollection) :
	wX(wX), wY(wY), wOut(wOut), signedIO(signedIO), baseMultiplierCollection(baseMultiplierCollection)
{
}

}   //end namespace flopoco
