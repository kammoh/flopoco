#include "TilingStrategyGreedy.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "BaseMultiplierXilinx2xk.hpp"
#include "LineCursorField.hpp"
#include "NearestPointCursorField.hpp"

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
            MultiplierTileCollection tiles):TilingStrategy(wX, wY, wOut, signedIO, bmc),
                                prefered_multiplier_{prefered_multiplier},
                                occupation_threshold_{occupation_threshold},
                                max_pref_mult_{maxPrefMult},
                                useIrregular_{useIrregular},
                                use2xk_{use2xk},
                                useSuperTiles_{useSuperTiles}
    {
        //copy vector
        tiles_ = tiles.BaseTileCollection;
        superTiles_ = tiles.SuperTileCollection;
        v2xkTiles_ = tiles.VariableYTileCollection;
        kx2Tiles_ = tiles.VariableXTileCollection;

        //remove variable length tiles
        if(use2xk) {
            v2xkTiles_.push_back(new BaseMultiplierXilinx2xk(2, INT32_MAX));
            kx2Tiles_.push_back(new BaseMultiplierXilinx2xk(INT32_MAX, 2));

            //sort 2xk tiles
            std::sort(v2xkTiles_.begin(), v2xkTiles_.end(), [](BaseMultiplierCategory* a, BaseMultiplierCategory* b) -> bool { return a->wY() < b->wY(); });
            //sort kx2 tiles
            std::sort(kx2Tiles_.begin(), kx2Tiles_.end(), [](BaseMultiplierCategory* a, BaseMultiplierCategory* b) -> bool { return a->wX() < b->wX(); });
        }

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

        /*cout << "tiles" << endl;
        for(BaseMultiplierCategory* b: tiles_) {
            cout << b->getType() << " "  << b->wX() << " " << b->wY() << endl;
        }

        cout << "2xk" << endl;
        for(BaseMultiplierCategory* b: v2xkTiles_) {
            cout << b->getType() << " "  << b->wX() << " " << b->wY() << endl;
        }

        cout << "kx2" << endl;
        for(BaseMultiplierCategory* b: kx2Tiles_) {
            cout << b->getType() << " "  << b->wX() << " " << b->wY() << endl;
        }*/

        /* unsigned int dspArea = 0;
        for(BaseMultiplierCategory* b: tiles_) {
            if(b->getDSPCost()) {
                dspArea = b->getArea();
                break;
            }
        }

        if(dspArea != 0) {
            totalDSPArea_ = maxPrefMult * dspArea;
            unsigned int total = wX * wY;
            unsigned int numDSPs = total / dspArea;
            //TODO: perhaps use float + rounding here
            if(totalDSPArea_ >= total) {
                useExtraDSPMetric_ = false;
            }
            else {
                float percent = 1.0 - ((total - totalDSPArea_) / (float) total);
                if(percent > 0.90) {
                    cout << "Percent too high " << percent << endl;
                    useExtraDSPMetric_ = false;
                }
                else {
                    useExtraDSPMetric_ = true;
                    coveredDSPArea_ = 0;
                }
            }

            cout << "DSP METRICS "<< useExtraDSPMetric_ << endl;
            cout << total << " VS " << totalDSPArea_ << endl;
        }*/

        useExtraDSPMetric_ = false;
    };

    void TilingStrategyGreedy::solve() {
        NearestPointCursorField field(wX, wY, signedIO);
        float cmp = FLT_MAX;
        int area = 0;
        float cost = createSolution(field, &solution, nullptr, cmp, area, 0);
        cout << "Total cost: " << cost << endl;
        cout << "Total area: " << area << endl;

        //exit(0);
    }

    BaseMultiplierCategory* TilingStrategyGreedy::findVariableTile(unsigned int wX, unsigned int wY) {
        if(wY == 2) {
            auto iter = lower_bound(kx2Tiles_.begin(), kx2Tiles_.end(), nullptr,[wX](BaseMultiplierCategory* a, BaseMultiplierCategory* b) -> bool {
                if(a != nullptr) {
                    return a->wX() < wX;
                }
                return b->wX() < wX;
            });

            if(iter == kx2Tiles_.end()) {
                cout << "Seems like no kx2 tile with width " << wX << " exists !";
                return nullptr;
            }

            return *iter;
        }
        else if(wX == 2) {
            auto iter = lower_bound(v2xkTiles_.begin(), v2xkTiles_.end(), nullptr,[wY](BaseMultiplierCategory* a, BaseMultiplierCategory* b) -> bool {
                if(a != nullptr) {
                    return a->wY() < wY;
                }
                return b->wY() < wY;
            });

            if(iter == v2xkTiles_.end()) {
                cout << "Seems like no 2xk tile with width " << wY << " exists !";
                return nullptr;
            }

            return *iter;
        }
        else {
            return nullptr;
        }
    }

    float TilingStrategyGreedy::createSolution(Field& field, list<mult_tile_t>* solution, queue<unsigned int>* path, float& cmpCost, int& area, unsigned int usedDSPBlocks, vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>* dspBlocks) {
        auto next (field.getCursor());
        float totalCost = 0.0f;
        float preCost = 0.0f;

        vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>> tmpBlocks;
        if(dspBlocks == nullptr) {
            dspBlocks = &tmpBlocks;
        }

        while(field.getMissing() > 0) {
            unsigned int neededX = field.getMissingLine();
            unsigned int neededY = field.getMissingHeight();

            BaseMultiplierCategory* bm = tiles_[tiles_.size() - 1];
            BaseMultiplierParametrization tile = bm->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO);
            float efficiency = -1.0f;
            unsigned int tileIndex = tiles_.size() - 1;

            float missingPercent = 1.0 - (((wX * wY) - field.getMissing()) / (float) (wX * wY));

            for(unsigned int i = 0; i < tiles_.size(); i++) {
                BaseMultiplierCategory* t = tiles_[i];
                // cout << "Checking " << t->getType() << endl;
                //dsp block
                if(t->getDSPCost()) {
                    if(usedDSPBlocks == max_pref_mult_) {
                        continue;
                    }
                }

                if(use2xk_ && t->isVariable()) {
                    //no need to compare it to a dspblock / if there is not enough space anyway
                    if(efficiency < 0 && ((neededX >= 6 && neededY >= 2) || (neededX >= 2 && neededY >= 6))) {
                        //TODO: rethink about tileIndex handling
                        int width = 0;
                        int height = 0;
                        if(neededX > neededY) {
                            width = neededX;
                            height = 2;

                            bm = findVariableTile(width, height);
                            if(bm == nullptr) {
                                continue;
                            }

                            tileIndex = i;
                        }
                        else {
                            width = 2;
                            height = neededY;

                            bm = findVariableTile(width, height);
                            if(bm == nullptr) {
                                continue;
                            }

                            tileIndex = i + 1;
                        }

                        /*cout << width << " " << height << endl;
                        cout << bm->wX() << " " << bm->wY() << endl;*/

                        tile = bm->getParametrisation();
                        break;
                    }
                }

                //no need to check normal tiles that won't fit anyway
                if(t->getDSPCost() == 0 && ((neededX * neededY) < t->getArea())) {
                    /*cout << "Not enough space anyway" << endl;
                    cout << neededX << " " << neededY << endl;
                    cout << param.getTileXWordSize() << " " << param.getTileYWordSize() << endl; */
                    continue;
                }

                unsigned int tiles = field.checkTilePlacement(next, t);
                if(tiles == 0) {
                    continue;
                }

                if(t->getDSPCost()) {
                    float usage = tiles / (float)t->getArea();
                    //check threshold
                    if(usage < occupation_threshold_) {
                        continue;
                    }

                    //TODO: check against
                    if(useExtraDSPMetric_ && tiles != t->getArea()) {
                        float percent = 1.0 - ((t->getArea() - tiles) / (float) t->getArea());
                        //don't check usage for the last few percent

                        if(percent < 0.85 && missingPercent > 0.3) {
                            // cout << "PLACEMENT PROBLEM " << tiles << " vs " << t->getArea() << " " << percent << endl;
                            continue;
                        }
                    }

                    //TODO: find a better way for this, think about the effect of > vs >= here
                    if(tiles > efficiency) {
                        efficiency = tiles;
                    }
                    else {
                        //no need to check anything else ... dspBlock wasn't enough
                        break;
                    }
                }
                else {
                    //TODO: tile / cost vs t->efficieny()
                    float newEfficiency = t->efficiency() * (tiles / (float)t->getArea());
                    /*cout << newEfficiency << endl;
                    cout << t->efficiency()<< endl;
                    cout << t->getArea() << endl;*/
                    if (newEfficiency < efficiency) {
                        if(tiles == t->getArea()) {
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

            preCost += bm->getLUTCost(next.first, next.second, wX, wY);

            //TODO: cmpCost needs to contain pre SuperTile costs in some way
            if(preCost > cmpCost) {
                return FLT_MAX;
            }

            if(path != nullptr) {
                path->push(tileIndex);
            }

            auto coord (field.placeTileInField(next, bm));
            if(bm->getDSPCost()) {
                usedDSPBlocks++;
                if(useSuperTiles_ && superTiles_.size() > 0) {
                    dspBlocks->push_back(make_pair(bm, next));
                    next = coord;
                    continue;
                }
            }
            else {
                area += bm->getArea();
            }

            if(solution != nullptr) {
                solution->push_back(make_pair(tile, next));
            }

            totalCost += bm->getLUTCost(next.first, next.second, wX, wY);
            next = coord;
        }

        // field.printField();
        // cout << superTiles_.size() << endl;

        //check each dspblock with another
        if(useSuperTiles_) {
            totalCost += performSuperTilePass(dspBlocks, solution);

            for(auto& tile: *dspBlocks) {
                unsigned int x = tile.second.first;
                unsigned int y = tile.second.second;

                if(solution != nullptr) {
                    solution->push_back(make_pair(tile.first->getParametrisation().tryDSPExpand(x, y, wX, wY, signedIO), tile.second));
                }

                totalCost += tile.first->getLUTCost(x, y, wX, wY);
            }
        }

        // cout << dspBlocks->size() << endl;

        cmpCost = preCost;
        return totalCost;
    }

    float TilingStrategyGreedy::performSuperTilePass(vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>* dspBlocks, list<mult_tile_t>* solution) {
        vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>> tempBlocks;
        tempBlocks = std::move(*dspBlocks);

        //get dspBlocks back into a valid state
        dspBlocks->clear();

        float totalSubCost = 0.0f;
        for(unsigned int i = 0; i < tempBlocks.size(); i++) {
            // cout << "Testing tile " << i << endl;
            bool found = false;
            auto &dspBlock1 = tempBlocks[i];
            unsigned int coordX = dspBlock1.second.first;
            unsigned int coordY = dspBlock1.second.second;
            for (unsigned int j = i + 1; j < tempBlocks.size(); j++) {
                auto &dspBlock2 = tempBlocks[j];
                // cout << dspBlock1.second.first << " " << dspBlock1.second.second << endl;
                // cout << dspBlock2.second.first << " " << dspBlock2.second.second << endl;

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

                /*cout << baseCoord.first << " " << baseCoord.second << endl;
                cout << rx1 << " " << ry1 << " " << lx1 << " " << ly1 << endl;
                cout << rx2 << " " << ry2 << " " << lx2 << " " << ly2 << endl;*/

                BaseMultiplierCategory *tile = MultiplierTileCollection::superTileSubtitution(superTiles_, rx1, ry1, lx1, ly1, rx2, ry2, lx2, ly2);
                if (tile == nullptr) {
                    // cout << "No Supertile found" << endl;
                    continue;
                }

                // cout << "Found Supertile of type " << tile->getType() << endl;

                if (solution != nullptr) {
                    solution->push_back(make_pair(tile->getParametrisation(), baseCoord));
                }

                totalSubCost += tile->getLUTCost(baseCoord.first, baseCoord.second, wX, wY);

                tempBlocks.erase(tempBlocks.begin() + j);
                found = true;
                break;
            }

            if(!found) {
                dspBlocks->push_back(dspBlock1);
            }
        }

        return totalSubCost;
    }
}