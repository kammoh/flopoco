#include <utility>

#include "TilingStrategyBeamSearch.hpp"

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
        Field baseField(wX, wY, signedIO);
        unsigned int usedDSPBlocks = 0U;
        unsigned int range = beamRange_;
        queue<unsigned int> path;

        float preCMPCost = FLT_MAX;
        float totalCMPCost = createSolution(baseField, nullptr, &path, preCMPCost, 0);
        unsigned int next = path.front();
        unsigned int lastPath = next;
        path.pop();

        baseField.reset();
        Field tempField(baseField);

        while(baseField.getMissing() > 0) {
            unsigned int minIndex = std::max(0U, next - range);
            unsigned int maxIndex = std::min((unsigned int)tiles_.size() - 1, next + range);

            unsigned int neededX = baseField.getMissingLine();
            unsigned int neededY = baseField.getMissingHeight();

            unsigned int bestEdge = next;
            lastPath = next;

            for (unsigned int i = minIndex; i <= maxIndex; i++) {
                cout << "TESTING WITH TILE " << i << endl;

                //check if we got the already calculated greedy path
                if (i == lastPath) {
                    cout << "Skipping this path" << endl;
                    continue;
                }

                tempField.reset(baseField);
                queue<unsigned int> tempPath;
                unsigned int tempUsedDSPBlocks = usedDSPBlocks;
                float currentTotalCost = 0.0f;
                float currentPreCost = 0.0f;

                if (placeSingleTile(tempField, tempUsedDSPBlocks, nullptr, neededX, neededY, tiles_[i], currentPreCost)) {
                    /*cout << "Single tile cost " << currentCost << endl;
                    //get cost for a greedy solution
                    currentCost += greedySolution(tempField, tempPath, tempUsedDSPBlocks, cmpCost);

                    cout << "========================" << currentCost << " vs " << cmpCost << endl;
                    //cout << (cmpCost - currentCost) << endl;

                    if ((currentCost - cmpCost) < 0.005) {
                        cmpCost = currentCost;
                        cout << "Using " << i << " would be cheaper! " << cmpCost << " VS " << currentCost << endl;
                        cout << cmpCost << endl;

                        bestEdge = i;


                        path = move(tempPath);
                    }*/
                }
            }
        }
    }

    bool TilingStrategyBeamSearch::placeSingleTile(Field& field, unsigned int& usedDSPBlocks, list<mult_tile_t>* solution, const int neededX, const int neededY, BaseMultiplierCategory* tile, float& cost) {
        if(tile->getDSPCost() >= 1) {
            if(usedDSPBlocks == max_pref_mult_) {
                return false;
            }
        }
        else if(!tile->isVariable() && (tile->wX() > neededX || tile->wY() > neededY)) {
            return false;
        }

        if(tile->isVariable()) {
            unsigned int height = 2;
            unsigned int width = neededX;
            if(neededY > neededX) {
                height = neededY;
                width = 2;
            }
            tile = findVariableTile(width, height);
        }

        auto next (field.getCursor());
        unsigned int tiles = field.checkTilePlacement(next, tile);
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
        }

        auto coord (field.placeTileInField(next, tile));

        cost += tile->getLUTCost();

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

    /*
    void TilingStrategyBeamSearch::solve() {
        unsigned int range = beamRange_;
        unsigned int usedDSPBlocks = 0;
        Field baseField(wX, wY, signedIO);
        double cost = 0.0;

        queue<unsigned int> path;

        double cmpCost = greedySolution(baseField, path, 0, FLT_MAX);
        unsigned int next = path.front();
        unsigned int lastPath = next;

        cout << "LAST PATH IS " << lastPath << endl;

        path.pop();

        baseField.reset();

        Field tempField(baseField);

        while(baseField.getMissing() > 0) {
            unsigned int minIndex = max(0, next - range);
            unsigned int maxIndex = min((int) baseTiles.size() - 1, next + range);

            unsigned int neededX = baseField.getMissingLine();
            unsigned int neededY = baseField.getMissingHeight();

            unsigned int bestEdge = next;
            lastPath = next;

            for (unsigned int i = minIndex; i <= maxIndex; i++) {
                cout << "TESTING WITH TILE " << i << endl;

                //check if we got the already calculated greedy path
                if(i == lastPath) {
                    cout << "Skipping this path" << endl;
                    continue;
                }

                tempField.reset(baseField);
                queue<unsigned int> tempPath;
                list<mult_tile_t> tempSolution;
                unsigned int tempUsedDSPBlocks = usedDSPBlocks;
                double currentCost = 0;

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


                        path = move(tempPath);
                    }
                }
            }

            //place single tile
            double singleCost = 0;
            placeSingleTile(baseField, usedDSPBlocks, solution, neededX, neededY, bestEdge, singleCost);
            cout << "COST TEST " << singleCost << " " << bestEdge << endl;

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

        cout << "Total cost: " << cost << endl;
    }*/

    //TODO: bundle greedy logic into one class
   /* double TilingStrategyBeamSearch::greedySolution(Field& field, queue<unsigned int>& path, unsigned int usedDSPBlocks, const double cmpcost) {
        auto next (field.getCursor());

        double dspSize = (double) (baseTiles[0].totalsize);
        double extendedSize = (double) ((baseTiles[0].wX + 1) * (baseTiles[0].wY + 1));
        double selectedSize = dspSize;

        double totalcost = 0.0f;

        while(field.getMissing() > 0) {
            //find a tile that would fit (start with smallest tile)
            unsigned int tileidx = baseTiles.size() - 1;
            tiledef basetile = baseTiles[tileidx];
            BaseMultiplierCategory& baseMultiplier = baseMultiplierCollection->getBaseMultiplier(basetile.base_index);
            BaseMultiplierParametrization tile =  baseMultiplier.parametrize( basetile.wX, basetile.wY, false, false);

            double efficiency = -1.0f;
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

                    unsigned int tiles = 0;

                    if(t.base_index == 0) {
                        pair<unsigned int, unsigned int> size = field.checkDSPPlacement(next, param);
                        tiles = size.first * size.second;

                        // unsigned int tiles = field.checkTilePlacement(next, param);

                        double usage = tiles / selectedSize;
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

                            //check if the tile needs to be "updated"
                            if(size.first != param.getTileXWordSize() || size.second != param.getTileYWordSize()) {
                                param = baseMultiplier.parametrize( size.first, size.second, signedX, signedY);
                            }
                        }
                        else {
                            //no need to check anything else ... dspBlock wasn't enough
                            break;
                        }
                    }
                    else {
                        tiles = field.checkTilePlacement(next, param);
                        if(tiles == 0) {
                            continue;
                        }

                        cout << t.wX << " " << t.wY << " Covered " << tiles << endl;

                        double newefficiency = tiles / t.cost;
                        cout << newefficiency << endl;
                        // <= will prefer vertical tiles, < prefers horizontal tiles (for the current list)
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
                    tileidx = i;
                    baseMultiplier = bm;

                    //no need to check the others, because of the sorting they won't be able to beat this tile
                    //if(tiles == t.totalsize) {
                    //    break;
                    //}
                }
            }

            if(basetile.base_index == 0) {
                usedDSPBlocks++;
                //TODO: handle supertiles?
            }

            double cost = (basetile.base_index == 0 ? 0 : ceil(baseMultiplier.getLUTCost(tile.getTileXWordSize(), tile.getTileYWordSize()))) + 0.65 * tile.getOutWordSize();
            totalcost += cost;

            if(totalcost > cmpcost) {
                return DBL_MAX;
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
        return 0.0;
    }

    bool TilingStrategyBeamSearch::placeSingleTile(Field& field, unsigned int& usedDSPBlocks, list<mult_tile_t>& solution, const int neededX, const int neededY, const int i, double& cost) {
        /*tiledef t = baseTiles[i];

        //max dsp block check
        if(t.base_index == 0) {
            if(usedDSPBlocks == max_pref_mult_) {
                return false;
            }
        }
        else if(t.wX > neededX || t.wY > neededY) {
            return false;
        }

        double dspSize = (double) (baseTiles[0].totalsize);
        double extendedSize = (double) ((baseTiles[0].wX + 1) * (baseTiles[0].wY + 1));
        double selectedSize = dspSize;

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

        BaseMultiplierCategory& baseMultiplier = baseMultiplierCollection->getBaseMultiplier(t.base_index);
        BaseMultiplierParametrization tile =  baseMultiplier.parametrize( width, height, signedX, signedY);

        if(t.base_index == 0) {
            pair<unsigned int, unsigned int> size = field.checkDSPPlacement(next, tile);
            unsigned int tiles = size.first * size.second;
            if(tiles == 0) {
                return false;
            }

            double usage = tiles / selectedSize;
            //check threshold
            if(usage < occupation_threshold_) {
                cout << "Couldn't place dspblock " << width << " " << height << " because threshold is " << occupation_threshold_ << " and usage is " << usage << endl;
                cout << tiles << " VS " << dspSize << endl;
                return false;
            }

            if(size.first != tile.getTileXWordSize() || size.second != tile.getTileYWordSize()) {
                tile = baseMultiplier.parametrize( size.first, size.second, signedX, signedY);
            }
        }
        else {
            unsigned int tiles = field.checkTilePlacement(next, tile);
            if(tiles == 0) {
                return false;
            }
        }

        auto coord (field.placeTileInField(next, tile));

        cost += (t.base_index == 0 ? 0 : ceil(baseMultiplier.getLUTCost(tile.getTileXWordSize(), tile.getTileYWordSize()))) + 0.65 * (tile.getOutWordSize());

        solution.push_back(make_pair(tile, next));

        if(t.base_index == 0) {
            usedDSPBlocks++;
        }

        return true;
    }
    */
}