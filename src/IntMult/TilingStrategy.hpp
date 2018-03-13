#pragma once

#include "BaseMultiplierCollection.hpp"

namespace flopoco {

/*!
 * The TilingStragegy class
 */
class TilingStrategy {

public:
	TilingStrategy(int wX, int wY, int wOut, bool signedIO, BaseMultiplierCollection* baseMultiplierCollection);
	~TilingStrategy();

	virtual void solve() = 0;

	list<pair< unsigned int, pair<unsigned int, unsigned int> > >& getSolution()
	{
		return solution;
	}

protected:
	/*!
	 * The solution data structure represents a tiling solution
	 *
	 * solution.first: type of multiplier, index used in BaseMultiplierCollection
	 * solution.second.first: x-coordinate
	 * solution.second.second: y-coordinate
	 *
	 */
	list<pair< unsigned int, pair<unsigned int, unsigned int> > > solution;

	int wX;                         /**< the width for X after possible swap such that wX>wY */
	int wY;                         /**< the width for Y after possible swap such that wX>wY */
	int wOut;                       /**< size of the output, to be used only in the standalone constructor and emulate.  */
	bool signedIO;                   /**< true if the IOs are two's complement */

	BaseMultiplierCollection* baseMultiplierCollection;
};

}
