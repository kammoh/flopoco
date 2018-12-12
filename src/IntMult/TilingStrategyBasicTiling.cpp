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
		numUsedMults_(0)
	{
		orderedBmc = bmc->getMultipliersIDByArea(true);
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
			int curDeltaY,
			int offset,
			int curArea
		) 
	{
		int thresholdArea = curDeltaX + curDeltaY;
		auto cur = std::lower_bound(
				orderedBmc.begin(),
				orderedBmc.end(),
				thresholdArea,
				[this, thresholdArea](const base_multiplier_id_t & id, const int val)->bool{
					auto const & multiplier = *(baseMultiplierCollection->getBaseMultiplier(id));
					int multArea = multiplier.getXWordSize() + multiplier.getYWordSize();
					return multArea > thresholdArea;
				}
			);			
		auto last = orderedBmc.end();
		bool canOverflowLeft = not truncated;
		bool canOverflowRight = false;
		bool canOverflowTop = (curY == 0);
		bool canOverflowBottom = (curY == wY - 1);
		int xleft = curX + curDeltaX - 1;
		int ybottom = curY + curDeltaY - 1;
		int xanchor, yanchor, xendmultbox, yendmultbox;
		int xMult;
		int yMult;

		for (; cur != last ; ++cur) {
			auto& bm = *(baseMultiplierCollection->getBaseMultiplier(*cur));
			xMult = bm.getXWordSize();
			yMult = bm.getYWordSize();

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
			if (effectiveArea != xMult + yMult ) {
				// We have found our multiplier
				break;	
			}
		}
		if (yMult < curDeltaY) {
			//We need to tile a subbox above or below the current multiplier
			int xStartUpDownbox = curX;
			int deltaXUpDownbox = curDeltaX;
			int yStartUpDownbox, deltaYUpDownbox;
			if (truncated) {
				yStartUpDownbox = curY;
				deltaYUpDownbox = yanchor - curY;
			} else {
				yStartUpDownbox = yendmultbox + 1;	
				deltaYUpDownbox = ybottom - yendmultbox;
			}

			int subboxArea = shrinkBox(
					xStartUpDownbox, 
					yStartUpDownbox, 
					deltaXUpDownbox, 
					deltaXUpDownbox, 
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
		if (xMult < curDeltaX) {
			//we need to tile a subbox left or right from the multiplier
			int yStartUpDownbox = yanchor;
			int deltaYUpDownbox = yMult;
			int xStartUpDownbox, deltaXUpDownbox;
			if (truncated) {
				xStartUpDownbox = curX;
				deltaXUpDownbox = xanchor - curX;
			} else {
				xStartUpDownbox = xendmultbox + 1;	
				deltaXUpDownbox = xleft - xendmultbox;
			}

			int subboxArea = shrinkBox(
					xStartUpDownbox, 
					yStartUpDownbox, 
					deltaXUpDownbox, 
					deltaXUpDownbox, 
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
		mult_tile_t solutionItem;
		solutionItem.first = *cur;
		solutionItem.second = make_pair(xanchor, yanchor);
		solution.push_back(solutionItem);
	}

	void TilingStrategyBasicTiling::solve()
	{
		BaseMultiplier& bm = *(baseMultiplierCollection->
				getBaseMultiplier(prefered_multiplier_));	
		int wXmult = bm.getXWordSize();
		int wYmult = bm.getYWordSize();

		double multArea = ((double) wXmult * wYmult);

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
						mult_tile_t solutionItem;	
						solutionItem.first = prefered_multiplier_;
						solutionItem.second = make_pair(rightX, topY);
						solution.push_back(solutionItem);
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
						mult_tile_t solutionItem;	
						solutionItem.first = prefered_multiplier_;
						solutionItem.second = make_pair(rightX, topY);
						solution.push_back(solutionItem);
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
