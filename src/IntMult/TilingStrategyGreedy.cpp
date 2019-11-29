#include "TilingStrategyGreedy.hpp"

#include <cstdlib>
#include <ctime>

namespace flopoco {
    TilingStrategyGreedy::TilingStrategyGreedy(
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

    void TilingStrategyGreedy::solve() {
        Field field(wX, wY);
        float cost = createSolution(field, solution, FLT_MAX);
        cout << "Total cost: " << cost << endl;
    }


    float TilingStrategyGreedy::createSolution(Field& field, list<mult_tile_t>& solution, const float cmpcost) {
        auto next (field.getCursor());
        unsigned int usedDSPBlocks = 0;

        float dspSize = (float) (baseTiles[0].totalsize);
        float extendedSize = (float) ((baseTiles[0].wX + 1) * (baseTiles[0].wY + 1));
        float selectedSize = dspSize;

        float totalcost = 0.0f;

        while(field.getMissing() > 0) {
            //find a tile that would fit (start with smallest tile)
            tiledef basetile = baseTiles[baseTiles.size() - 1];
            BaseMultiplierCategory& baseMultiplier = baseMultiplierCollection->getBaseMultiplier(basetile.base_index);
            BaseMultiplierParametrization tile =  baseMultiplier.parametrize( basetile.wX, basetile.wY, false, false);

            float efficiency = -1.0f;
            unsigned int neededX = field.getMissingLine();
            unsigned int neededY = field.getMissingHeight();

            for(tiledef& t: baseTiles) {
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
                            if (neededX == 25 && (unsigned int) wX - (next.first + width) == 1) {
                                width++;
                                cout << "Extended X" << endl;
                                signedX = true;
                                selectedSize = extendedSize;
                            }

                            if (neededY == 18 && (unsigned int) wY - (next.second + height) == 1) {
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
                        if (newefficiency < efficiency) {
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

            totalcost += (basetile.base_index == 0 ? 0 : ceil(baseMultiplier.getLUTCost(tile.getTileXWordSize(), tile.getTileYWordSize()))) + 0.65 * tile.getOutWordSize();

            if(totalcost > cmpcost) {
                return FLT_MAX;
            }

            cout << "COST " << totalcost << endl;

            auto coord (field.placeTileInField(next, tile));
            solution.push_back(make_pair(tile, next));

            next = coord;
        }
        field.printField();

        return totalcost;
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