#pragma once

#include "TilingStrategy.hpp"

#ifdef HAVE_SCALP
#include <ScaLP/Solver.h>
#include <ScaLP/Exception.h>    // ScaLP::Exception
#include <ScaLP/SolverDynamic.h> // ScaLP::newSolverDynamic
#endif //HAVE_SCALP
#include <iomanip>
#include "BaseMultiplier.hpp"
#include "BaseMultiplierDSPSuperTilesXilinx.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "MultiplierTileCollection.hpp"

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
        BaseMultiplierCollection* bmc,
		base_multiplier_id_t prefered_multiplier,
		float occupation_threshold,
		size_t maxPrefMult,
        MultiplierTileCollection tiles_);

    virtual void solve();

private:
    base_multiplier_id_t small_tile_mult_;
    size_t numUsedMults_;
    size_t max_pref_mult_;
    BaseMultiplierCollection* bmc;
    float occupation_threshold_;
    inline bool shape_contribution(int x, int y, int shape_x, int shape_y, int s);
    inline float shape_cost(int s);
    inline float shape_occupation(int shape_x, int shape_y, int s);
    int dpX, dpY, dpS, wS;
    struct tiledef {                                //Struct to hold information about tiles
        unsigned wX;
        unsigned wY;
        float cost;
        unsigned base_index;
    };
    vector<tiledef> my_tiles;                       //Tiles used for Tiling
    vector<BaseMultiplierDSPSuperTilesXilinx> availSuperTiles;
    vector<BaseMultiplierIrregularLUTXilinx> availNonRectTiles;
    vector<BaseMultiplierCategory*> tiles;
#ifdef HAVE_SCALP
    void constructProblem();

    ScaLP::Solver *solver;
#endif
};

}
