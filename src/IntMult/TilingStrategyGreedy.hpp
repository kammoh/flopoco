#ifndef FLOPOCO_TILINGSTRATEGYGREEDY_HPP
#define FLOPOCO_TILINGSTRATEGYGREEDY_HPP

#include <queue>

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

        bool greedySolution(BaseFieldState& fieldState, list<mult_tile_t>* solution, queue<unsigned int>* path, float& cost, unsigned int& area, float cmpCost = FLT_MAX, unsigned int usedDSPBlocks = 0, vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>* dspBlocks = nullptr);
        BaseMultiplierCategory* findVariableTile(unsigned int wX, unsigned int wY);
        bool performSuperTilePass(vector<pair<BaseMultiplierCategory*, multiplier_coordinates_t>>* dspBlocks, list<mult_tile_t>* solution, float& cost, float cmpCost = FLT_MAX);
    };
}
#endif