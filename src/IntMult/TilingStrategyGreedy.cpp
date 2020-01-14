#include "TilingStrategyGreedy.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "BaseMultiplierXilinx2xk.hpp"
#include "LineCursor.hpp"
#include "NearestPointCursor.hpp"

#include <cstdlib>
#include <ctime>

namespace flopoco {
    TilingStrategyGreedy::TilingStrategyGreedy(
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
            MultiplierTileCollection& tiles):TilingStrategy(wX, wY, wOut, signedIO, bmc),
                                prefered_multiplier_{prefered_multiplier},
                                occupation_threshold_{occupation_threshold},
                                max_pref_mult_{maxPrefMult},
                                useIrregular_{useIrregular},
                                use2xk_{use2xk},
                                useSuperTiles_{useSuperTiles},
                                tileCollection_{tiles}
    {
        //copy vector
        tiles_ = tiles.BaseTileCollection;

        //sort base tiles
        std::sort(tiles_.begin(), tiles_.end(), [](BaseMultiplierCategory* a, BaseMultiplierCategory* b) -> bool { return a->efficiency() > b->efficiency(); });

        //insert 2k and k2 tiles after dsp tiles and before normal tiles
        if(use2xk) {
            for (unsigned int i = 0; i < tiles_.size(); i++) {
                if (tiles_[i]->getDSPCost() == 0) {
                    tiles_.insert(tiles_.begin() + i, new BaseMultiplierXilinx2xk(2, INT32_MAX));
                    tiles_.insert(tiles_.begin() + i, new BaseMultiplierXilinx2xk(INT32_MAX, 2));
                    break;
                }
            }
        }
    };

    void TilingStrategyGreedy::solve() {
        NearestPointCursor fieldState;
        Field field(wX, wY, signedIO, fieldState);

        double cost = 0.0;
        unsigned int area = 0;
        //only one state, base state is also current state
        greedySolution(fieldState, &solution, nullptr, cost, area);
        cout << "Total cost: " << cost << endl;
        cout << "Total area: " << area << endl;
    }

    bool TilingStrategyGreedy::greedySolution(BaseFieldState& fieldState, list<mult_tile_t>* solution, queue<unsigned int>* path, double& cost, unsigned int& area, double cmpCost, unsigned int usedDSPBlocks, vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>* dspBlocks) {
        Field* field = fieldState.getField();
        auto next (fieldState.getCursor());
        double tempCost = cost;
        unsigned int tempArea = area;

        vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>> tmpBlocks;
        if(dspBlocks == nullptr) {
            dspBlocks = &tmpBlocks;
        }

        while(fieldState.getMissing() > 0) {
            unsigned int neededX = field->getMissingLine(fieldState);
            unsigned int neededY = field->getMissingHeight(fieldState);

            BaseMultiplierCategory* bm = tiles_[tiles_.size() - 1];
            BaseMultiplierParametrization tile = bm->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO);
            double efficiency = -1.0f;
            unsigned int tileIndex = tiles_.size() - 1;

            for(unsigned int i = 0; i < tiles_.size(); i++) {
                BaseMultiplierCategory *t = tiles_[i];
                if (t->getDSPCost() && usedDSPBlocks == max_pref_mult_) {
                    //all available dsp blocks got already used
                    continue;
                }

                if (use2xk_ && t->isVariable()) {
                    //no need to compare it to a dsp block / if there is not enough space anyway
                    if (efficiency < 0 && ((neededX >= 6 && neededY >= 2) || (neededX >= 2 && neededY >= 6))) {
                        if(neededX > neededY) {
                            bm = tileCollection_.VariableXTileCollection[neededX - tileCollection_.variableTileOffset];
                            tileIndex = i + 1;
                        }
                        else {
                            bm = tileCollection_.VariableYTileCollection[neededY - tileCollection_.variableTileOffset];
                            tileIndex = i;
                        }

                        tile = bm->getParametrisation();
                        break;
                    }
                }

                //no need to check normal tiles that won't fit anyway
                if (t->getDSPCost() == 0 && ((neededX * neededY) < t->getArea())) {
                    continue;
                }

                unsigned int tiles = field->checkTilePlacement(next, t, fieldState);
                if (tiles == 0) {
                    continue;
                }

                if (t->getDSPCost()) {
                    double usage = tiles / (double) t->getArea();
                    //check threshold
                    if (usage < occupation_threshold_) {
                        continue;
                    }

                    //TODO: find a better way for this, think about the effect of > vs >= here
                    if (tiles > efficiency) {
                        efficiency = tiles;
                    } else {
                        //no need to check anything else ... dsp block wasn't enough
                        break;
                    }
                } else {
                    //TODO: tile / cost vs t->efficieny()
                    double newEfficiency = t->efficiency() * (tiles / (double) t->getArea());
                    if (newEfficiency < efficiency) {
                        if (tiles == t->getArea()) {
                            //this tile wasn't able to compete with the current best tile even if it is used completely ... so checking the rest makes no sense
                            break;
                        }
                        continue;
                    }

                    efficiency = newEfficiency;
                }

                tile = t->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO);
                bm = t;
                tileIndex = i;
            }

            if(path != nullptr) {
                path->push(tileIndex);
            }

            auto coord (field->placeTileInField(next, bm, fieldState));
            if(bm->getDSPCost()) {
                usedDSPBlocks++;
                if(useSuperTiles_) {
                    dspBlocks->push_back(make_pair(bm, next));
                    next = coord;
                }
            }
            else {
                tempArea += bm->getArea();
                tempCost += bm->getLUTCost(next.first, next.second, wX, wY);

                if(tempCost > cmpCost) {
                    return false;
                }

                if(solution != nullptr) {
                    solution->push_back(make_pair(tile, next));
                }

                next = coord;
            }
        }

        //check each dsp block with another
        if(useSuperTiles_) {
            if(!performSuperTilePass(dspBlocks, solution, tempCost, cmpCost)) {
                return false;
            }

            for(auto& tile: *dspBlocks) {
                unsigned int x = tile.second.first;
                unsigned int y = tile.second.second;

                if(solution != nullptr) {
                    solution->push_back(make_pair(tile.first->getParametrisation().tryDSPExpand(x, y, wX, wY, signedIO), tile.second));
                }

                tempCost += tile.first->getLUTCost(x, y, wX, wY);

                if(tempCost > cmpCost) {
                    return false;
                }
            }
        }

        cost = tempCost;
        area = tempArea;
        return true;
    }

    bool TilingStrategyGreedy::performSuperTilePass(vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>* dspBlocks, list<mult_tile_t>* solution, double& cost, double cmpCost) {
        vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>> tempBlocks;
        tempBlocks = std::move(*dspBlocks);

        //get dspBlocks back into a valid state
        dspBlocks->clear();

        //TODO: use the back swap trick here?
        for(unsigned int i = 0; i < tempBlocks.size(); i++) {
            bool found = false;
            auto &dspBlock1 = tempBlocks[i];
            unsigned int coordX = dspBlock1.second.first;
            unsigned int coordY = dspBlock1.second.second;
            for (unsigned int j = i + 1; j < tempBlocks.size(); j++) {
                auto &dspBlock2 = tempBlocks[j];

                Cursor baseCoord;
                baseCoord.first = std::min(dspBlock1.second.first, dspBlock2.second.first);
                baseCoord.second = std::min(dspBlock1.second.second, dspBlock2.second.second);

                int rx1 = dspBlock1.second.first - baseCoord.first;
                int ry1 = dspBlock1.second.second - baseCoord.second;
                int lx1 = dspBlock1.second.first + dspBlock1.first->wX_DSPexpanded(coordX, coordY, wX, wY, signedIO) - baseCoord.first - 1;
                int ly1 = dspBlock1.second.second + dspBlock1.first->wY_DSPexpanded(coordX, coordY, wX, wY, signedIO) - baseCoord.second - 1;

                int rx2 = dspBlock2.second.first - baseCoord.first;
                int ry2 = dspBlock2.second.second - baseCoord.second;
                int lx2 = dspBlock2.second.first + dspBlock2.first->wX_DSPexpanded(dspBlock2.second.first, dspBlock2.second.second, wX, wY,signedIO) - baseCoord.first - 1;
                int ly2 = dspBlock2.second.second + dspBlock2.first->wY_DSPexpanded(dspBlock2.second.first, dspBlock2.second.second, wX, wY,signedIO) - baseCoord.second - 1;

                BaseMultiplierCategory *tile = MultiplierTileCollection::superTileSubtitution(tileCollection_.SuperTileCollection, rx1, ry1, lx1, ly1, rx2, ry2, lx2, ly2);
                if (tile == nullptr) {
                    continue;
                }

                if (solution != nullptr) {
                    solution->push_back(make_pair(tile->getParametrisation(), baseCoord));
                }

                cost += tile->getLUTCost(baseCoord.first, baseCoord.second, wX, wY);
                if(cost > cmpCost) {
                    return false;
                }

                //TODO: don't use vector here
                tempBlocks.erase(tempBlocks.begin() + j);
                found = true;
                break;
            }

            if(!found) {
                dspBlocks->push_back(dspBlock1);
            }
        }

        return true;
    }
}