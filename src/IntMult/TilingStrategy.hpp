#pragma once

#include "BaseMultiplierCollection.hpp"

namespace flopoco {

/*!
 * The TilingStragegy class
 */
class TilingStrategy {

public:
	typedef pair<unsigned int, unsigned int> multiplier_op_size_t;
	typedef pair<base_multiplier_id_t, multiplier_op_size_t> mult_tile_t;
	TilingStrategy(int wX, int wY, int wOut, bool signedIO, BaseMultiplierCollection* baseMultiplierCollection);

	virtual void solve() = 0;

	list<mult_tile_t>& getSolution()
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
	list<mult_tile_t> solution;

	int wX;                         /**< the width for X after possible swap such that wX>wY */
	int wY;                         /**< the width for Y after possible swap such that wX>wY */
	int wOut;                       /**< size of the output, to be used only in the standalone constructor and emulate.  */
	bool signedIO;                   /**< true if the IOs are two's complement */

	BaseMultiplierCollection* baseMultiplierCollection;
};

}
