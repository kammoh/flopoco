#ifndef FLOPOCO_TILINGSTRATEGYXGREEDY_HPP
#define FLOPOCO_TILINGSTRATEGYXGREEDY_HPP

#include "TilingStrategyGreedy.hpp"

namespace flopoco {
    class TilingStrategyXGreedy : public TilingStrategyGreedy {
    public:
        TilingStrategyXGreedy(
                unsigned int wX,
                unsigned int wY,
                unsigned int wOut,
                bool signedIO,
                BaseMultiplierCollection* bmc,
                base_multiplier_id_t prefered_multiplier,
                float occupation_threshold,
                size_t maxPrefMult=0);
        virtual void solve();

    private:
        void swapBaseTiles(const unsigned int i, const unsigned int j);
        float compareSolution(list<mult_tile_t>** oldSolution, float oldCost, list<mult_tile_t>* newSolution, float newCost);

    };
}


#endif
