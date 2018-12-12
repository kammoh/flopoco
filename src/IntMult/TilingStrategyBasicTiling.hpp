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
		int shrinkBox(int xright, int ytop, int xdim, int ydim, int offset);
		void tileBox(int curX, int curY, int curDeltaX, int curDeltaY, offset);
		base_multiplier_id_t prefered_multiplier_;
		size_t numUsedMults_;
		float occupation_threshold=.875;
};

}

#endif
