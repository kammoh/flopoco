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
        //TODO: detect mirrored tiles (only one axis)
    };

    void TilingStrategyXGreedy::solve() {
        //TODO: dynamic for mirrored tiles
        Field field(wX, wY, signedIO);

        // 0 0
        //------------------------------RUN 1 -------------------------------
        list<mult_tile_t> solution1;
        float bestCost = createSolution(field, solution1, FLT_MAX);
        list<mult_tile_t>* best = &solution1;

        // 0 1
        //------------------------------RUN 2 -------------------------------
        field.reset();

        //flip some tiles around
        //swapBaseTiles(2, 3);

        list<mult_tile_t> solution2;
        float costSolution = createSolution(field, solution2, bestCost);
        if(costSolution < bestCost) {
            bestCost = costSolution;
            best = &solution2;
        }

        // 1 0
        //------------------------------RUN 3 -------------------------------
        field.reset();

        //flip some tiles around
        //swapBaseTiles(0, 1);
        //swapBaseTiles(2, 3);

        list<mult_tile_t> solution3;
        costSolution = createSolution(field, solution3, bestCost);
        if(costSolution < bestCost) {
            bestCost = costSolution;
            best = &solution3;
        }

        // 1 1
        //------------------------------RUN 4 -------------------------------
        field.reset();

        //flip some tiles around
        //swapBaseTiles(2, 3);

        list<mult_tile_t> solution4;
        costSolution = createSolution(field, solution4, bestCost);
        if(costSolution < bestCost) {
            bestCost = costSolution;
            best = &solution4;
        }

        cout << "Total cost: " << bestCost << endl;

        solution = *best;
    }
}