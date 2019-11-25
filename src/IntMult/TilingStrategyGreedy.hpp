#ifndef FLOPOCO_TILINGSTRATEGYGREEDY_HPP
#define FLOPOCO_TILINGSTRATEGYGREEDY_HPP

#include "TilingStrategy.hpp"
#include "Field.hpp"

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
                size_t maxPrefMult=0);
        virtual void solve();

    protected:
        struct tiledef {
            unsigned wX;
            unsigned wY;
            unsigned totalsize;
            float cost;
            float efficiency;
            unsigned base_index;
        };

        struct supertiledef {
            unsigned int basetile1;
            unsigned int basetile2;
            unsigned int xOffset;
            unsigned int yOffset;
        };

        base_multiplier_id_t prefered_multiplier_;
        float occupation_threshold_;
        size_t max_pref_mult_;

        vector<tiledef> baseTiles;
        vector<supertiledef> superTiles;

        float createSolution(Field& field, list<mult_tile_t>& solution, const float cmpcost);
        pair<bool, bool> checkSignedTile(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height);
    };
}


#endif
