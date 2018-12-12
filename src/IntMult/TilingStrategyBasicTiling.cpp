#include "TilingStrategyBasicTiling.hpp"

using namespace std;
namespace flopoco {
TilingStrategyBasicTiling::TilingStrategyBasicTiling(
		int wX,
		int wY,
		int wOut,
		bool signedIO,
		BaseMultiplierCollection* bmc,
		base_multiplier_id_t prefered_multiplier):TilingStrategy(
			wX,
			wY,
			wOut,
			signedIO,
			bmc),
		prefered_multiplier_(prefered_multiplier),
		numUsedMults_(0)
	{
	}

	int TilingStrategyBasicTiling::shrinkBox(
			int& xright,
			int& ytop, 
			int& xdim, 
			int& ydim, 
			int offset)
	{
		// Come into the frame
		if (xdim + xright > wX + 1) {
			xdim = wX - xright + 1;
		}
		if (ydim + ytop > wY + 1) {
			ydim = wY - ytop + 1;
		}
		if (xright < 0) {
			xdim += xright;
			xright = 0;
		}
		if (ytop < 0) {
			ydim += ytop;
			ytop = 0;
		}

		int curOffset = xright + ytop - offset;
		if (curOffset > xdim && curOffset > ydim) {
			xright = ytop = -1;
			xdim = ydim = 0;
			return 0;
		}

		// Skip uneeded empty rows and columns
		if (curOffset > xdim) {
			int deltaY = curOffset - xdim;
			ytop += deltaY;
			ydim -= deltaY;
		}

		if (curOffset > ydim) {
			int deltaX = curOffset - ydim;
			xright += deltaX;
			xdim -= deltaX;
		}

		curOffset = max(xright + ytop - offset, 0);
		int area = xdim * ydim - (curOffset * (curOffset + 1))/2;
		return area;
	}

	void TilingStrategyBasicTiling::tileBox(
			int curX,
			int curY,
			int curDeltaX,
			int curDeltaY
		) 
	{
			
	}

	void TilingStrategyBasicTiling::solve()
	{
		BaseMultiplier& preferred_bm = *(bmc->getBaseMultiplier(prefered_multiplier_));	
		int wXmult = bm.getXWordSize();
		int wYmult = bm.getYWordSize();

		bool isTruncated = (wOut_ < wX + wY);
		double multArea = ((double) wXmult * wYmult);

		if (isTruncated) {
			//Perform tiling from left to right to increase the number of full
			//multipliers	
			int offset = wX + wY - wOut;
			int curX = wX - 1;		
			int curY = 0;
			if (curX < offset) {
				curY = offset - curX;
			}

			while (curY < wY) {
				curX = wX - 1;		
				while(curX >= 0) {
					int rightX = curX - wXmult;
					int topY = curY;
					int curDeltaX = wXmult;
					int curDeltaY = wYmult;
					int area = shrinkBox(rightX, topY, curDeltaX, curDeltaY, offset);
					if (area == 0) { // All the box is below the line cut
						break;
					} 

					double occupationRatio = ((double) (area)) /  multArea;
					if (occupationRatio >= occupation_threshold) {
						// Emit a preferred multiplier for this block
						mult_tile_t solutionItem;	
						solutionItem.first = prefered_multiplier_;
						solutionItem.second = make_pair(rightX, topY);
						numUsedMults_ += 1;
					} else {
						//Tile the subBox with smaller multiplier;
						tileBox(rightX, topY, curDeltaX, curDeltaY, offset);
					}
					curX += wXmult;
				}
				curY += wYmult;
			}

		} else {
			//Perform tiling from right to left to avoid small multipliers for
			//low bits
		}
		
	}
};
