//
// Created by user on 11/20/19.
//

#ifndef FLOPOCO_TILINGSTRATEGYBEAMSEARCH_HPP
#define FLOPOCO_TILINGSTRATEGYBEAMSEARCH_HPP

#include "TilingStrategy.hpp"
#include "Field.hpp"
#include <queue>

namespace flopoco {
    class TilingStrategyBeamSearch : public TilingStrategy {
    public:
        TilingStrategyBeamSearch(
                unsigned int wX,
                unsigned int wY,
                unsigned int wOut,
                bool signedIO,
                BaseMultiplierCollection* bmc,
                base_multiplier_id_t prefered_multiplier,
                float occupation_threshold,
                size_t maxPrefMult=0,
                unsigned int beamRange=0);
        void solve();

    private:
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
        unsigned int beamRange_;

        vector<tiledef> baseTiles;
        vector<supertiledef> superTiles;

        double greedySolution(Field& field, queue<unsigned int>& path, unsigned int usedDSPBlocks, const double cmpcost);
        bool placeSingleTile(Field& field, unsigned int& usedDSPBlocks, list<mult_tile_t>& solution, const int neededX, const int neededY, const int tile, double& cost);

        pair<bool, bool> checkSignedTile(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
    };
}


#endif //FLOPOCO_TILINGSTRATEGYBEAMSEARCH_HPP
