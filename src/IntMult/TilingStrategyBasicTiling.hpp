#ifndef TILINGSTRATEGYBASICTILING_HPP
#define TILINGSTRATEGYBASICTILING_HPP

#include "TilingStrategy.hpp"

namespace flopoco {

class TilingStrategyBasicTiling : public TilingStrategy {
	public:
		TilingStrategyBasicTiling(
				int wX,
				int wY,
				int wOut,
				bool signedIO,
				BaseMultiplierCollection* bmc,
				base_multiplier_id_t prefered_multiplier);
		void solve();

	private:
		/**
		 * @return the effective area of the Box
		 */
		int shrinkBox(int& xright, int& ytop, int& xdim, int& ydim, int offset);
		void tileBox(int curX, int curY, int curDeltaX, int curDeltaY, int offset, int curArea);
		base_multiplier_id_t prefered_multiplier_;
		base_multiplier_id_t small_tile_mult_;
		size_t numUsedMults_;
		float occupation_threshold=0.; //TODO replace by flopoco command line option value
		bool truncated;
};

}

#endif
