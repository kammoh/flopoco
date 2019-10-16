#pragma once

#include "TilingStrategy.hpp"

#ifdef HAVE_SCALP
#include <ScaLP/Solver.h>
#include <ScaLP/Exception.h>    // ScaLP::Exception
#include <ScaLP/SolverDynamic.h> // ScaLP::newSolverDynamic
#endif //HAVE_SCALP
#include <iomanip>

namespace flopoco {

/*!
 * The TilingStrategyOptimalILP class
 */
class TilingStrategyOptimalILP : public TilingStrategy
{

public:
    using TilingStrategy::TilingStrategy;
    TilingStrategyOptimalILP(
        unsigned int wX,
        unsigned int wY,
        unsigned int wOut,
        bool signedIO,
        BaseMultiplierCollection* bmc);

    virtual void solve();

private:
    base_multiplier_id_t small_tile_mult_;
    size_t numUsedMults_;
    size_t max_pref_mult_;
    BaseMultiplierCollection* bmc;
    bool shape_contribution(int x, int y, int s);
    float shape_cost(int s);
    int dpX, dpY, dpS, wS;
    struct tiledef {                                //Struct to hold information about tiles
        unsigned wX;
        unsigned wY;
        float cost;
        unsigned base_index;
    };
    vector<tiledef> my_tiles;                       //Tiles used for Tiling
#ifdef HAVE_SCALP
    void constructProblem();

    ScaLP::Solver *solver;
#endif
};

}
