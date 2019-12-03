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
                MultiplierTileCollection tiles);
        virtual void solve();

    protected:
        base_multiplier_id_t prefered_multiplier_;
        float occupation_threshold_;
        size_t max_pref_mult_;

        vector<BaseMultiplierCategory*> tiles_;
        vector<BaseMultiplierCategory*> superTiles_;

        float createSolution(Field& field, list<mult_tile_t>& solution, const float cmpcost);
        pair<bool, bool> checkSignedTile(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height);
    };
}


#endif
