#ifndef FLOPOCO_TILINGSTRATEGYGREEDY_HPP
#define FLOPOCO_TILINGSTRATEGYGREEDY_HPP

#include "TilingStrategy.hpp"
#include "Field.hpp"
#include "MultiplierTileCollection.hpp"

namespace flopoco {
    class TilingStrategyGreedy : public TilingStrategy {
    public:
        TilingStrategyGreedy(
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
                MultiplierTileCollection tiles);
        virtual void solve();

    protected:
        base_multiplier_id_t prefered_multiplier_;
        float occupation_threshold_;
        size_t max_pref_mult_;

        vector<BaseMultiplierCategory*> tiles_;
        vector<BaseMultiplierCategory*> superTiles_;
        vector<BaseMultiplierCategory*> v2xkTiles_;
        vector<BaseMultiplierCategory*> kx2Tiles_;

        bool useIrregular_;
        bool use2xk_;
        bool useSuperTiles_;

        float createSolution(Field& field, list<mult_tile_t>& solution, const float cmpCost);
    };
}
#endif