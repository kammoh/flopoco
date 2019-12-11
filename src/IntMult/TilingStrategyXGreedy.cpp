#include "TilingStrategyXGreedy.hpp"

namespace flopoco {
    TilingStrategyXGreedy::TilingStrategyXGreedy(
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
            MultiplierTileCollection tiles):TilingStrategyGreedy(wX, wY, wOut, signedIO, bmc, prefered_multiplier, occupation_threshold, maxPrefMult, useIrregular, use2xk, useSuperTiles, tiles)
    {
        //find all paired tiles
        for(unsigned int i = 0; i < tiles_.size(); i++) {
            BaseMultiplierCategory* tile = tiles_[i];
            if(tile->getDSPCost() != 1) {
                break;
            }

            for(unsigned int j = 0; j < tiles_.size(); j++) {
                BaseMultiplierCategory* cmp = tiles_[j];
                if(i == j) {
                    continue;
                }

                if(cmp->getDSPCost() != 1) {
                    break;
                }

                if(cmp->wX() == tile->wY() && cmp->wY() == tile->wX()) {
                    pair<unsigned int, unsigned int> p(std::min(i, j), std::max(i, j));
                    bool unique = true;
                    for(auto& u: pairs_) {
                        if(u.first == p.first && u.second == p.second) {
                            unique = false;
                            break;
                        }
                    }

                    if(unique) {
                        pairs_.push_back(p);
                    }
                }
            }
        }
    };

    void TilingStrategyXGreedy::solve() {
        Field field(wX, wY, signedIO);

        for(auto& u: pairs_) {
            cout << u.first << " " << u.second << endl;
        }

        list<mult_tile_t> bestSolution;
        float bestCost = FLT_MAX;
        int bestArea = 0;
        float totalCost = createSolution(field, &bestSolution, nullptr, bestCost, bestArea, 0);
        int runs = std::pow(2, pairs_.size());
        for(int i = 1; i < runs; i++) {
            field.reset();
            int last = i - 1;
            for(unsigned int j = 0; j < pairs_.size(); j++) {
                int cmp = last ^ i;
                int mask =  1 << j;
                if((mask & cmp) != 0) {
                    swapTiles(pairs_[j]);
                }
            }

            list<mult_tile_t> solution;
            float cmpCost = bestCost;
            int area = 0;
            float cost = createSolution(field, &solution, nullptr, cmpCost, area, 0);
            if(cost < bestCost) {
                bestCost = cmpCost;
                totalCost = cost;
                bestArea = area;
                bestSolution = std::move(solution);
            }
        }

        cout << "Total cost: " << totalCost << endl;
        cout << "Total area: " << bestArea << endl;
        solution = std::move(bestSolution);
    }

    void TilingStrategyXGreedy::swapTiles(pair<unsigned int, unsigned int> p) {
        BaseMultiplierCategory* temp = tiles_[p.first];
        tiles_[p.first] = tiles_[p.second];
        tiles_[p.second] = temp;
    }
}