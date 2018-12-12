#include "IntMult/TilingStrategyOptimalILP.hpp"

namespace flopoco {


void TilingStrategyOptimalILP::solve()
{
#ifndef HAVE_SCALP
    throw "Error, TilingStrategyOptimalILP::solve() was called but FloPoCo was not built with ScaLP library";
#else
     solver = new ScaLP::Solver(ScaLP::newSolverDynamic({"Gurobi","CPLEX","SCIP","LPSolve"}));

     mult_tile_t solutionitem;

     solutionitem.first = 0;
     solutionitem.second = make_pair(0,0);
     solution.push_back(solutionitem);

     solutionitem.first = 0;
     solutionitem.second = make_pair(17,0);
     solution.push_back(solutionitem);
#endif
}

#ifdef HAVE_SCALP
void TilingStrategyOptimalILP::constructProblem()
{
    ScaLP::Variable tempK = ScaLP::newIntegerVariable("x", 0, ScaLP::INF());
}


#endif

}   //end namespace flopoco
