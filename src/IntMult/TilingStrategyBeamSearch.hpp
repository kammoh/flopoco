//
// Created by user on 11/20/19.
//

#ifndef FLOPOCO_TILINGSTRATEGYBEAMSEARCH_HPP
#define FLOPOCO_TILINGSTRATEGYBEAMSEARCH_HPP

#include "Field.hpp"
#include "TilingStrategyGreedy.hpp"
#include <queue>

namespace flopoco {
    class TilingStrategyBeamSearch : public TilingStrategyGreedy {
    public:
        TilingStrategyBeamSearch(
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
                unsigned int beamRange);
        void solve();

    private:
        unsigned int beamRange_;
        bool placeSingleTile(BaseFieldState& fieldState, Field& field, unsigned int& usedDSPBlocks, list<mult_tile_t>* solution, const unsigned int neededX, const unsigned int neededY, BaseMultiplierCategory* tile, float& preCost, float& totalCost, int& area, vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>& dspBlocks);
    };
}


#endif //FLOPOCO_TILINGSTRATEGYBEAMSEARCH_HPP
