#include <algorithm>

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
		small_tile_mult_(1), //Most compact LUT-Based multiplier
		numUsedMults_(0)
	{
		truncated = (wOut < (wY + wX));
	}

	int TilingStrategyBasicTiling::shrinkBox(
			int& xright,
			int& ytop, 
			int& xdim, 
			int& ydim, 
			int offset)
	{
		// Come into the frame
		if (xdim + xright > wX) {
			xdim = wX - xright;
		}
		if (ydim + ytop > wY) {
			ydim = wY - ytop;
		}
		if (xright < 0) {
			xdim += xright;
			xright = 0;
		}
		if (ytop < 0) {
			ydim += ytop;
			ytop = 0;
		}

		int curOffset = offset - xright - ytop;
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

		curOffset = max(offset - xright - ytop, 0);
		int area = xdim * ydim - (curOffset * (curOffset + 1))/2;
		return area;
	}

	void TilingStrategyBasicTiling::tileBox(
			int curX,
			int curY,
			int curDeltaX,
			int curDeltaY,
			int offset,
			int curArea
		) 
	{
		auto& bmc = baseMultiplierCollection->getBaseMultiplier(small_tile_mult_);
		bool canOverflowLeft = not truncated;
		bool canOverflowRight = false;
		bool canOverflowTop = (curY == 0);
		bool canOverflowBottom = false;
		int xleft = curX + curDeltaX - 1;
		int ybottom = curY + curDeltaY - 1;
		int nbInputMult = bmc.getMaxUnsignedWordSize();

		int bestXMult = 1;
		int bestYMult = 1;
		int bestArea = -1;
		int bestXAnchor = curX;
		int bestYAnchor = curY;

		for (int i = nbInputMult ; i > 1 ; --i) {
			int xMult, yMult;
			int xanchor, yanchor, xendmultbox, yendmultbox;
			for (xMult = nbInputMult - 1 ; xMult > 0 ; --xMult) {
				yMult = i - xMult;
				if (truncated) {
				xanchor = xleft - xMult + 1;
				yanchor = ybottom - yMult + 1;
				} else {
					xanchor = curX;
					yanchor = curY;	
				}
				xendmultbox = xanchor + xMult - 1;
				yendmultbox = yanchor + yMult - 1;

				// Test if multiplier does not overflow on already handled data
				if (((yanchor < curY) && !canOverflowTop) ||
						((yendmultbox > ybottom) && !canOverflowBottom) ||
						((xanchor < curX) && !canOverflowRight) || 
						((xendmultbox > xleft) && !canOverflowLeft)) {
					continue;
				}

				int copyxanchor, copyyanchor, copyXMult, copyYMult;
				copyxanchor = xanchor;
				copyyanchor = yanchor;
				copyXMult = xMult;
				copyYMult = yMult;
				int effectiveArea = shrinkBox(
						copyxanchor, 
						copyyanchor, 
						copyXMult, 
						copyYMult, 
						offset
						);
				if (effectiveArea == xMult * yMult && effectiveArea > bestArea) {
					bestXMult = xMult;
					bestYMult = yMult;
					bestArea = effectiveArea;
					bestXAnchor = xanchor;
					bestYAnchor = yanchor;
				}
			}
		}

		int xendmultbox = bestXAnchor + bestXMult - 1;
		int yendmultbox = bestYAnchor + bestYMult - 1;
		
		if (bestYMult < curDeltaY) {
			//We need to tile a subbox above or below the current multiplier
			int xStartUpDownbox = curX;
			int deltaXUpDownbox = curDeltaX;
			int yStartUpDownbox, deltaYUpDownbox;
			if (truncated) {
				yStartUpDownbox = curY;
				deltaYUpDownbox = bestYAnchor - curY;
			} else {
				yStartUpDownbox = yendmultbox + 1;	
				deltaYUpDownbox = ybottom - yendmultbox;
			}

			int subboxArea = shrinkBox(
					xStartUpDownbox, 
					yStartUpDownbox, 
					deltaXUpDownbox, 
					deltaYUpDownbox, 
					offset
				);
			if (subboxArea != 0) {
				tileBox(
						xStartUpDownbox, 
						yStartUpDownbox, 
						deltaXUpDownbox, 
						deltaYUpDownbox,
						offset,
						subboxArea
					);
			}
		}
		if (bestXMult < curDeltaX) {
			//we need to tile a subbox left or right from the multiplier
			int yStartUpDownbox = bestYAnchor;
			int deltaYUpDownbox = bestYMult;
			int xStartUpDownbox, deltaXUpDownbox;
			if (truncated) {
				xStartUpDownbox = curX;
				deltaXUpDownbox = bestXAnchor - curX;
			} else {
				xStartUpDownbox = xendmultbox + 1;	
				deltaXUpDownbox = xleft - xendmultbox;
			}

			int subboxArea = shrinkBox(
					xStartUpDownbox, 
					yStartUpDownbox, 
					deltaXUpDownbox, 
					deltaYUpDownbox, 
					offset
				);
			if (subboxArea != 0) {
				tileBox(
						xStartUpDownbox, 
						yStartUpDownbox, 
						deltaXUpDownbox, 
						deltaYUpDownbox,
						offset,
						subboxArea
					);
			}
		}
		//Add the current multiplier
		auto param = bmc.parametrize(bestXMult, bestYMult, false, false);
		auto coord = make_pair(bestXAnchor, bestYAnchor);
		solution.push_back(make_pair(param, coord));
	}

	void TilingStrategyBasicTiling::solve()
	{
		auto& bm = baseMultiplierCollection->getBaseMultiplier(prefered_multiplier_);	
		int wXmultMax = bm.getMaxUnsignedWordSize();
		//TODO Signed int deltaSignedUnsigned = bm.getDeltaWidthSigned();
		int wXmult = 1;
		int wYmult = 1;
		int intMultArea = 1;
		for (int testValX = 1 ; testValX < wXmultMax ; ++testValX) {
			int testValY = bm.getMaxSecondWordSize(testValX, false, false);
			int testMultArea = testValX * testValY;
			if (testMultArea >= intMultArea) {
				wXmult = testValX;
				wYmult = testValY;
				intMultArea = testMultArea;
			}
		}

		double multArea = double(intMultArea);

		if (truncated) {
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
						auto param = bm.parametrize(
								wXmult, 
								wYmult, 
								false,
								false										
							);
						auto coords = make_pair(rightX, topY);
						solution.push_back(make_pair(param, coords));
						numUsedMults_ += 1;
					} else {
						//Tile the subBox with smaller multiplier;
						tileBox(rightX, topY, curDeltaX, curDeltaY, offset, area);
					}
					curX -= wXmult;
				}
				curY += wYmult;
			}

		} else {
			//Perform tiling from right to left to avoid small multipliers for
			//low bits
			int curX = 0;
			int curY = 0;

			while (curY < wY) {
				curX = 0;		
				while(curX < wX) {
					int rightX = curX;
					int topY = curY;
					int curDeltaX = wXmult;
					int curDeltaY = wYmult;
					int area = shrinkBox(rightX, topY, curDeltaX, curDeltaY, 0);
					if (area == 0) { // All the box is below the line cut
						break;
					} 

					double occupationRatio = double(area) /  multArea;
					if (occupationRatio >= occupation_threshold) {
						// Emit a preferred multiplier for this block
						auto param = bm.parametrize(
								wXmult,
								wYmult,
								false,
								false
							);
						auto coord = make_pair(rightX, topY);
						solution.push_back(make_pair(param, coord));
						numUsedMults_ += 1;
					} else {
						//Tile the subBox with smaller multiplier;
						tileBox(rightX, topY, curDeltaX, curDeltaY, 0, area);
					}
					curX += wXmult;
				}
				curY += wYmult;
			}
		}
	}
};
