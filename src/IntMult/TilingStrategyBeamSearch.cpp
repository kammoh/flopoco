#include <utility>

#include "TilingStrategyBeamSearch.hpp"
#include "LineCursor.hpp"
#include "NearestPointCursor.hpp"

namespace flopoco {
    TilingStrategyBeamSearch::TilingStrategyBeamSearch(
            unsigned int wX,
            unsigned int wY,
            unsigned int wOut,
            bool signedIO,
            BaseMultiplierCollection* bmc,
            base_multiplier_id_t prefered_multiplier,
            float occupation_threshold,
            size_t maxPrefMult,
            bool useIrregular,
            bool use2xk,
            bool useSuperTiles,
            MultiplierTileCollection tiles,
            unsigned int beamRange
            ):TilingStrategyGreedy(wX, wY, wOut, signedIO, bmc, prefered_multiplier, occupation_threshold, maxPrefMult, useIrregular, use2xk, useSuperTiles, tiles),
            beamRange_{beamRange}
    {

    };

    void TilingStrategyBeamSearch::solve() {
        NearestPointCursor baseState;
        NearestPointCursor tempState;
        Field field(wX, wY, signedIO, baseState);

        unsigned int usedDSPBlocks = 0U;
        unsigned int range = beamRange_;
        queue<unsigned int> path;

        float bestCost = 0.0f;
        unsigned int bestArea = 0;
        greedySolution(baseState, nullptr, &path, bestCost, bestArea);
        unsigned int next = path.front();
        unsigned int lastPath = next;
        path.pop();

        field.reset();

        float currentTotalCost = 0.0f;
        unsigned int currentArea = 0;
        vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>> dspBlocks;

        while(baseState.getMissing() > 0) {
            unsigned int minIndex = std::max(0, (int)next - (int)range);
            unsigned int maxIndex = std::min((unsigned int)tiles_.size() - 1, next + range);

            unsigned int neededX = field.getMissingLine(baseState);
            unsigned int neededY = field.getMissingHeight(baseState);

            lastPath = next;
            BaseMultiplierCategory* tile = tiles_[next];

            for (unsigned int i = minIndex; i <= maxIndex; i++) {
                //check if we got the already calculated greedy path
                if (i == lastPath) {
                    continue;
                }

                BaseMultiplierCategory* t = tiles_[i];

                tempState.reset(baseState);
                queue<unsigned int> tempPath;
                unsigned int tempUsedDSPBlocks = usedDSPBlocks;
                float tempCost = currentTotalCost;
                unsigned int tempArea = currentArea;

                vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>> localDSPBlocks = dspBlocks;

                if (placeSingleTile(tempState, tempUsedDSPBlocks, nullptr, neededX, neededY, t, tempCost, tempArea, bestCost, localDSPBlocks)) {
                    if(greedySolution(tempState, nullptr, &tempPath, tempCost, tempArea, bestCost, tempUsedDSPBlocks, &localDSPBlocks)) {
                        bestCost = tempCost;
                        bestArea = tempArea;
                        path = move(tempPath);
                        tile = t;
                    }
                }
            }

            //place single tile
            placeSingleTile(baseState, usedDSPBlocks, &solution, neededX, neededY, tile, currentTotalCost, currentArea, FLT_MAX, dspBlocks);

            if(path.size() > 0) {
                next = path.front();
                path.pop();
            }
        }

        if(useSuperTiles_) {
            performSuperTilePass(&dspBlocks, &solution, currentTotalCost);
        }

        for(auto& tile: dspBlocks) {
            unsigned int x = tile.second.first;
            unsigned int y = tile.second.second;

            solution.push_back(make_pair(tile.first->getParametrisation().tryDSPExpand(x, y, wX, wY, signedIO), tile.second));

            currentTotalCost += tile.first->getLUTCost(x, y, wX, wY);
        }

        cout << "Total cost: " << currentTotalCost << endl;
        cout << "Total area: " << currentArea << endl;
        // exit(0);
    }

    bool TilingStrategyBeamSearch::placeSingleTile(BaseFieldState& fieldState, unsigned int& usedDSPBlocks, list<mult_tile_t>* solution, const unsigned int neededX, const unsigned int neededY, BaseMultiplierCategory* tile, float& cost, unsigned int& area, float cmpCost, vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>& dspBlocks) {
        if(tile->getDSPCost() >= 1) {
            if(usedDSPBlocks == max_pref_mult_) {
                return false;
            }
        }
        else if(!tile->isVariable() && (tile->wX() > neededX || tile->wY() > neededY)) {
            return false;
        }

        if(tile->isVariable()) {
            if(!(neededX >= 6 && neededY >= 2) && !(neededX >= 2 && neededY >= 6)) {
                return false;
            }

            unsigned int height = 2;
            unsigned int width = neededX;
            if(neededY > neededX) {
                height = neededY;
                width = 2;
            }
            tile = findVariableTile(width, height);
        }

        Field* field = fieldState.getField();

        auto next (fieldState.getCursor());
        unsigned int tiles = field->checkTilePlacement(next, tile, fieldState);
        if(tiles == 0) {
            return false;
        }

        if(tile->getDSPCost() >= 1) {
            float usage = tiles / (float)tile->getArea();
            //check threshold
            if(usage < occupation_threshold_) {
                return false;
            }
            usedDSPBlocks++;
            dspBlocks.push_back(make_pair(tile, next));
        }

        auto coord (field->placeTileInField(next, tile, fieldState));

        if(tile->getDSPCost() >= 1) {
            return true;
        }

        cost += tile->getLUTCost(next.first, next.second, wX, wY);
        area += tile->getArea();

        if(cost > cmpCost) {
            return false;
        }

        if(solution == nullptr) {
            return true;
        }

        if(tile->isVariable()) {
            solution->push_back(make_pair(tile->getParametrisation(), next));
        }
        else {
            solution->push_back(make_pair(tile->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO), next));
        }

        return true;
    }
}