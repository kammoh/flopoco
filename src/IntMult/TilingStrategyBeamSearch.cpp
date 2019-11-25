#include "TilingStrategyBeamSearch.hpp"

namespace flopoco {
    TilingStrategyBeamSearch::TilingStrategyBeamSearch(
            unsigned int wX_,
            unsigned int wY_,
            unsigned int wOut_,
            bool signedIO_,
            BaseMultiplierCollection* bmc,
            base_multiplier_id_t prefered_multiplier,
            float occupation_threshold,
            size_t maxPrefMult):TilingStrategy(wX_, wY_, wOut_, signedIO_, bmc),
                                prefered_multiplier_{prefered_multiplier},
                                occupation_threshold_{occupation_threshold},
                                max_pref_mult_{maxPrefMult}
    {
        // {width, height, total cost (LUT+Compressor), base multiplier id} of tile
        //TODO: support 2k and k2 tiles
        baseTiles.push_back ({24, 17, 24 * 17,  41.65, 15.3, 0});
        baseTiles.push_back ({17, 24, 24 * 17, 41.65, 15.3, 0});
        baseTiles.push_back ({2, 3, 2 * 3,6.25, 0.96, 1});
        baseTiles.push_back ({3, 2, 2 * 3, 6.25, 0.96, 1});
        baseTiles.push_back ({3, 3, 3 * 3, 9.90, 0.91, 2});
        baseTiles.push_back ({1, 2, 1 * 2, 2.30, 0.87, 1});
        baseTiles.push_back ({2, 1, 2 * 1, 2.30, 0.87, 1});
        baseTiles.push_back ({1, 1, 1 * 1,1.65, 0.625, 1});

        //type g supertile
        superTiles.push_back ({1, 0, 0, 17});
    };

    void TilingStrategyBeamSearch::solve() {
        unsigned int range = 3;
        unsigned int usedDSPBlocks = 0;
        Field baseField(wX, wY);
        float cost = 0.0f;

        queue<unsigned int> path;

        float cmpCost = greedySolution(baseField, path, 0, FLT_MAX);
        unsigned int next = path.front();
        unsigned int lastPath = next;

        cout << "LAST PATH IS " << lastPath << endl;

        path.pop();

        baseField.reset();

        while(baseField.getMissing() > 0) {
            unsigned int minIndex = max(0, next - range);
            unsigned int maxIndex = min((int) baseTiles.size(), next + range);

            unsigned int neededX = baseField.getMissingLine();
            unsigned int neededY = baseField.getMissingHeight();

            unsigned int bestEdge = next;
            lastPath = next;

            for (unsigned int i = minIndex; i < maxIndex; i++) {
                cout << "TESTING WITH TILE " << i << endl;

                //check if we got the already calculated greedy path
                if(i == lastPath) {
                    cout << "Skipping this path" << endl;
                    continue;
                }

                Field tempField(baseField);
                queue<unsigned int> tempPath;
                list<mult_tile_t> tempSolution;
                unsigned int tempUsedDSPBlocks = usedDSPBlocks;
                float currentCost = 0;

                //check if placing the tile even makes sense
                if (placeSingleTile(tempField, tempUsedDSPBlocks, tempSolution, neededX, neededY, i, currentCost)) {
                    cout << "Single tile cost " << currentCost << endl;
                    //get cost for a greedy solution
                    currentCost += greedySolution(tempField, tempPath, tempUsedDSPBlocks, cmpCost);
                    
                    cout << "========================" << currentCost << " vs " << cmpCost << endl;
                    //cout << (cmpCost - currentCost) << endl;

                    if ((currentCost - cmpCost) < 0.005) {
                        cmpCost = currentCost;
                        cout << "Using " << i << " would be cheaper! " << cmpCost << " VS " << currentCost << endl;
                        cout << cmpCost << endl;

                        bestEdge = i;

                        //TODO: use move here?
                        path = tempPath;
                    }
                }
            }

            //place single tile
            float singleCost = 0;
            placeSingleTile(baseField, usedDSPBlocks, solution, neededX, neededY, bestEdge, singleCost);
            cost += singleCost;
            
            if(path.size() > 0) {
                next = path.front();
                path.pop();

                cmpCost -= singleCost;
            }

            cout << "Placing tile " << bestEdge << endl;
            cout << cmpCost << endl;
            cout << "NEXT TILE IS " << next << endl;

            cout << "===================================================================================================================" << endl;
            baseField.printField();
        }

        cout << "Selected cost " << cost << endl;
    }

    //TODO: bundle greedy logic into one class
    float TilingStrategyBeamSearch::greedySolution(Field& field, queue<unsigned int>& path, unsigned int usedDSPBlocks, const float cmpcost) {
        auto next (field.getCursor());

        float dspSize = (float) (baseTiles[0].totalsize);
        float extendedSize = (float) ((baseTiles[0].wX + 1) * (baseTiles[0].wY + 1));
        float selectedSize = dspSize;

        float totalcost = 0.0f;

        while(field.getMissing() > 0) {
            //find a tile that would fit (start with smallest tile)
            unsigned int tileidx = baseTiles.size() - 1;
            tiledef basetile = baseTiles[tileidx];
            BaseMultiplierCategory& baseMultiplier = baseMultiplierCollection->getBaseMultiplier(basetile.base_index);
            BaseMultiplierParametrization tile =  baseMultiplier.parametrize( basetile.wX, basetile.wY, false, false);

            float efficiency = -1.0f;
            unsigned int neededX = field.getMissingLine();
            unsigned int neededY = field.getMissingHeight();

            for(unsigned int i = 0; i < baseTiles.size(); i++) {
                tiledef t = baseTiles[i];

                //dsp block
                if(t.base_index == 0) {
                    if(usedDSPBlocks == max_pref_mult_) {
                        continue;
                    }
                }

                if((t.wX <= neededX && t.wY <= neededY) || t.base_index == 0) {
                    unsigned int width = t.wX;
                    unsigned int height = t.wY;
                    bool signedX = false;
                    bool signedY = false;

                    //signedIO for DSP blocks
                    if(t.base_index == 0){
                        selectedSize = dspSize;
                        //limit size of DSP Multiplier, to do not protrude form the tiled area
                        unsigned int newWidth = (unsigned int) wX - next.first;
                        if(newWidth < width) {
                            width = newWidth;
                        }

                        unsigned int newHeight = (unsigned int) wY - next.second;
                        if(newHeight < height) {
                            height = newHeight;
                        }

                        if(signedIO) {
                            if ((unsigned int) wX - (next.first + width) == 1) {
                                width++;
                                cout << "Extended X" << endl;
                                signedX = true;
                                selectedSize = extendedSize;
                            }

                            if ((unsigned int) wY - (next.second + height) == 1) {
                                height++;
                                cout << "Extended Y" << endl;
                                signedY = true;
                                selectedSize = extendedSize;
                            }
                        }
                    }

                    //signedIO for base tiles
                    if(signedIO && !signedX && !signedY) {
                        pair<bool, bool> signs = checkSignedTile(next.first, next.second, width, height);
                        signedX = signs.first;
                        signedY = signs.second;
                    }

                    BaseMultiplierCategory& bm = baseMultiplierCollection->getBaseMultiplier(t.base_index);
                    BaseMultiplierParametrization param =  bm.parametrize( width, height, signedX, signedY);

                    unsigned int tiles = field.checkTilePlacement(next, param);
                    if(tiles == 0) {
                        continue;
                    }

                    cout << t.wX << " " << t.wY << " Covered " << tiles << endl;

                    if(t.base_index == 0) {
                        float usage = tiles / selectedSize;
                        //check threshold
                        if(usage < occupation_threshold_) {
                            cout << "Couldn't place dspblock " << width << " " << height << " because threshold is " << occupation_threshold_ << " and usage is " << usage << endl;
                            cout << tiles << " VS " << dspSize << endl;
                            continue;
                        }

                        //TODO: find a better way for this, think about the effect of > vs >= here
                        // > will prefer horizontal tiles
                        if(tiles > efficiency) {
                            efficiency = tiles;
                        }
                        else {
                            //no need to check anything else ... dspBlock wasn't enough
                            break;
                        }
                    }
                    else {
                        float newefficiency = tiles / t.cost;
                        cout << newefficiency << endl;
                        //TODO: test if this makes a difference
                        // <= will prefer vertical tiles
                        if (!(newefficiency > efficiency)) {
                            if(tiles == t.totalsize) {
                                //this tile wasn't able to compete with the current best tile even if it is used completely ... so checking the rest makes no sense
                                break;
                            }
                            
                            continue;
                        }

                        efficiency = newefficiency;
                    }

                    tile = param;
                    basetile = t;
                    tileidx = i;
                    baseMultiplier = bm;

                    //no need to check the others, because of the sorting they won't be able to beat this tile
                    if(tiles == t.totalsize) {
                        break;
                    }
                }
            }

            if(basetile.base_index == 0) {
                usedDSPBlocks++;
                //TODO: handle supertiles?
            }

            float cost = (basetile.base_index == 0 ? 0 : ceil(baseMultiplier.getLUTCost(tile.getTileXWordSize(), tile.getTileYWordSize()))) + 0.65 * (tile.getTileXWordSize() + tile.getTileYWordSize());
            cout << "SINGLE COST " << cost << endl;
            totalcost += cost;

            if(totalcost > cmpcost) {
                return FLT_MAX;
            }

            cout << "COST " << totalcost << endl;
            path.push(tileidx);

            cout << "Placed tile " << tileidx << " vs " << basetile.base_index << endl;
            auto coord (field.placeTileInField(next, tile));

            next = coord;
        }
        field.printField();

        cout << "total cost " << totalcost << endl;

        return totalcost;
    }

    bool TilingStrategyBeamSearch::placeSingleTile(Field& field, unsigned int& usedDSPBlocks, list<mult_tile_t>& solution, const int neededX, const int neededY, const int i, float& cost) {
        tiledef t = baseTiles[i];

        //max dsp block check
        if(t.base_index == 0) {
            if(usedDSPBlocks == max_pref_mult_) {
                return false;
            }
        }
        else if(t.wX > neededX || t.wY > neededY) {
            return false;
        }

        float dspSize = (float) (baseTiles[0].totalsize);
        float extendedSize = (float) ((baseTiles[0].wX + 1) * (baseTiles[0].wY + 1));
        float selectedSize = dspSize;

        auto next (field.getCursor());
        unsigned int width = t.wX;
        unsigned int height = t.wY;
        bool signedX = false;
        bool signedY = false;

        //signedIO for DSP blocks
        if(t.base_index == 0){
            selectedSize = dspSize;
            //limit size of DSP Multiplier, to do not protrude the multiplier
            unsigned int newWidth = (unsigned int) wX - next.first;
            if(newWidth < width) {
                width = newWidth;
            }

            unsigned int newHeight = (unsigned int) wY - next.second;
            if(newHeight < height) {
                height = newHeight;
            }

            if(signedIO) {
                if ((unsigned int) wX - (next.first + width) == 1) {
                    width++;
                    cout << "Extended X" << endl;
                    signedX = true;
                    selectedSize = extendedSize;
                }

                if ((unsigned int) wY - (next.second + height) == 1) {
                    height++;
                    cout << "Extended Y" << endl;
                    signedY = true;
                    selectedSize = extendedSize;
                }
            }
        }

        //signedIO for base tiles
        if(signedIO && !signedX && !signedY) {
            pair<bool, bool> signs = checkSignedTile(next.first, next.second, width, height);
            signedX = signs.first;
            signedY = signs.second;
        }

        BaseMultiplierCategory& baseMultiplier = baseMultiplierCollection->getBaseMultiplier(t.base_index);
        BaseMultiplierParametrization tile =  baseMultiplier.parametrize( width, height, signedX, signedY);

        unsigned int tiles = field.checkTilePlacement(next, tile);
        if(tiles == 0) {
            return false;
        }

        if(t.base_index == 0) {
            float usage = tiles / selectedSize;
            //check threshold
            if(usage < occupation_threshold_) {
                cout << "Couldn't place dspblock " << width << " " << height << " because threshold is " << occupation_threshold_ << " and usage is " << usage << endl;
                cout << tiles << " VS " << dspSize << endl;
                return false;
            }
        }

        auto coord (field.placeTileInField(next, tile));

        cost += (t.base_index == 0 ? 0 : ceil(baseMultiplier.getLUTCost(tile.getTileXWordSize(), tile.getTileYWordSize()))) + 0.65 * (tile.getTileXWordSize() + tile.getTileYWordSize());

        solution.push_back(make_pair(tile, next));

        if(t.base_index == 0) {
            usedDSPBlocks++;
        }

        return true;
    }

    pair<bool, bool> TilingStrategyBeamSearch::checkSignedTile(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
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