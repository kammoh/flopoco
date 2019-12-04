#include "TilingStrategyGreedy.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "BaseMultiplierXilinx2xk.hpp"

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
            MultiplierTileCollection tiles):TilingStrategy(wX, wY, wOut, signedIO, bmc),
                                prefered_multiplier_{prefered_multiplier},
                                occupation_threshold_{occupation_threshold},
                                max_pref_mult_{maxPrefMult}
    {
        //copy vector
        tiles_ = tiles.MultTileCollection;

        cout << tiles_.size() << endl;

        //build supertile collection
        for(unsigned int i = 0; i < tiles_.size(); i++) {
            BaseMultiplierCategory* t = tiles_[i];
            if(t->getDSPCost() == 2) {
                superTiles_.push_back(t);
            }
        }

        //remove supertiles from tiles vector
        tiles_.erase(remove_if(tiles_.begin(), tiles_.end(), []( BaseMultiplierCategory* t) { return t->getDSPCost() == 2; }),tiles_.end());

        //sort remaining tiles
        sort(tiles_.begin(), tiles_.end(), [](BaseMultiplierCategory* a, BaseMultiplierCategory* b) -> bool { return a->efficiency() > b->efficiency(); });

        //inject 2k and k2 tiles after dspblocks and before normal tiles
        for(unsigned int i = 0; i < tiles_.size(); i++) {
            if(tiles_[i]->getDSPCost() == 0) {
                tiles_.insert(tiles_.begin() + i, new BaseMultiplierXilinx2xk(INT32_MAX));
                break;
            }
        }

        for(BaseMultiplierCategory* b: tiles_) {
            cout << b->getType() << endl;
        }
    };

    void TilingStrategyGreedy::solve() {
        Field field(wX, wY, signedIO);
        float cost = createSolution(field, solution, FLT_MAX);
        cout << "Total cost: " << cost << endl;
    }


    float TilingStrategyGreedy::createSolution(Field& field, list<mult_tile_t>& solution, const float cmpCost) {
        auto next (field.getCursor());
        unsigned int usedDSPBlocks = 0;
        float totalCost = 0.0f;
        //TODO: change this
        vector<mult_tile_t> dspBlocks;
        float dspCost = 0.0f;

        while(field.getMissing() > 0) {
            unsigned int neededX = field.getMissingLine();
            unsigned int neededY = field.getMissingHeight();

            BaseMultiplierCategory* bm = tiles_[tiles_.size()-1];
            BaseMultiplierParametrization tile = bm->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO);
            float efficiency = -1.0f;

            for(BaseMultiplierCategory* t: tiles_) {
                cout << "Checking " << t->getType() << endl;
                //dsp block
                if(t->getDSPCost()) {
                    if(usedDSPBlocks == max_pref_mult_) {
                        continue;
                    }
                }

                BaseMultiplierParametrization param = t->getParametrisation();
                if(dynamic_cast<BaseMultiplierXilinx2xk*>(t) != nullptr) {
                    //no need to compare it to a dspblock
                    if(efficiency < 0 && neededX >= 6 && neededY >= 2) {
                        //TODO: everything in this scope
                        BaseMultiplierXilinx2xk* tile2k = new BaseMultiplierXilinx2xk(neededX);
                        tile = tile2k->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO);;
                        bm = tile2k;
                        break;
                    }
                }

                //no need to check normal tiles that won't fit anyway
                if(t->getDSPCost() == 0 && (neededX * neededY < t->getArea())) {
                    cout << "Not enough space anyway" << endl;
                    cout << neededX << " " << neededY << endl;
                    cout << param.getTileXWordSize() << " " << param.getTileYWordSize() << endl;
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
                    cout << newEfficiency << endl;
                    cout << t->efficiency()<< endl;
                    cout << t->getArea() << endl;
                    if (newEfficiency < efficiency) {
                        if(tiles == t->getArea()) {
                            //this tile wasn't able to compete with the current best tile even if it is used completely ... so checking the rest makes no sense
                            break;
                        }

                        continue;
                    }

                    efficiency = newEfficiency;

                    //TODO: check if a 2k Multiplier would be better here
                }

                tile = t->getParametrisation().tryDSPExpand(next.first, next.second, wX, wY, signedIO);;
                bm = t;
            }

            totalCost += bm->cost();
            cout << "COST " << totalCost << endl;
            cout << bm->cost() << endl;

            //TODO: cmpCost needs to contain pre SuperTile costs in some way
            if(totalCost > cmpCost) {
                return FLT_MAX;
            }

            auto coord (field.placeTileInField(next, bm));
            if(bm->getDSPCost()) {
                usedDSPBlocks++;
                if(superTiles_.size() > 0) {
                    dspBlocks.push_back(make_pair(tile, next));
                    dspCost = bm->cost();
                    next = coord;
                    continue;
                }
            }

            solution.push_back(make_pair(tile, next));
            next = coord;
        }

        field.printField();
        cout << superTiles_.size() << endl;

        //check each dspblock with another
        if(dspBlocks.size() > 0) {
            for(unsigned int i = 0; i < dspBlocks.size(); i++) {
                cout << "Testing tile " << i << endl;
                bool found = false;
                auto& dspBlock1 = dspBlocks[i];
                for(unsigned int j = i + 1; j < dspBlocks.size(); j++) {
                    auto& dspBlock2 = dspBlocks[j];
                    cout << dspBlock1.second.first << " " << dspBlock1.second.second << endl;
                    cout << dspBlock2.second.first << " " << dspBlock2.second.second << endl;

                    pair<unsigned int, unsigned int> baseCoord;
                    baseCoord.first = min(dspBlock1.second.first, dspBlock2.second.first);
                    baseCoord.second = min(dspBlock1.second.second, dspBlock2.second.second);

                    int rx1 = dspBlock1.second.first - baseCoord.first;
                    int ry1 = dspBlock1.second.second - baseCoord.second;
                    int lx1 = dspBlock1.second.first + dspBlock1.first.getTileXWordSize() - baseCoord.first - 1;
                    int ly1 = dspBlock1.second.second + dspBlock1.first.getTileYWordSize() - baseCoord.second - 1;

                    int rx2 = dspBlock2.second.first - baseCoord.first;
                    int ry2 = dspBlock2.second.second - baseCoord.second;
                    int lx2 = dspBlock2.second.first + dspBlock2.first.getTileXWordSize() - baseCoord.first - 1;
                    int ly2 = dspBlock2.second.second + dspBlock2.first.getTileYWordSize() - baseCoord.second - 1;

                    cout << baseCoord.first << " " << baseCoord.second << endl;
                    cout << rx1 << " " << ry1 << " " << lx1 << " " << ly1 << endl;
                    cout << rx2 << " " << ry2 << " " << lx2 << " " << ly2 << endl;

                    BaseMultiplierCategory* tile = MultiplierTileCollection::superTileSubtitution(superTiles_, rx1, ry1, lx1, ly1, rx2, ry2, lx2, ly2);
                    if(tile == nullptr) {
                        cout << "No Supertile found" << endl;
                        continue;
                    }

                    cout << "Found Supertile of type " << tile->getType() << endl;
                    solution.push_back(make_pair(tile->getParametrisation(), baseCoord));
                    totalCost -= dspCost;
                    totalCost -= dspCost;
                    totalCost += tile->cost();

                    dspBlocks.erase(dspBlocks.begin() + j);
                    found = true;
                    break;
                }

                if (!found) {
                    solution.push_back(dspBlock1);

                }
            }
        }

        cout << dspBlocks.size() << endl;

        return totalCost;
    }

    pair<bool, bool> TilingStrategyGreedy::checkSignedTile(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height) {
        pair<bool, bool> result;

        if((unsigned int)wX - (x + width) == 0) {
            result.first = true;
        }

        if((unsigned int)wY - (y + height) == 0) {
            result.second = true;
        }

        return result;
    }
}