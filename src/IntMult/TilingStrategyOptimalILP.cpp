#include "IntMult/TilingStrategyOptimalILP.hpp"

namespace flopoco {


void TilingStrategyOptimalILP::solve()
{
#ifndef HAVE_SCALP
    throw "Error, TilingStrategyOptimalILP::solve() was called but FloPoCo was not built with ScaLP library";
#else
     solver = new ScaLP::Solver(ScaLP::newSolverDynamic({"Gurobi","CPLEX","SCIP","LPSolve"}));
	constructProblem();

	throw std::string("Not implemented yet !");
#endif
}

#ifdef HAVE_SCALP
void TilingStrategyOptimalILP::constructProblem()
{
    ScaLP::Variable tempK = ScaLP::newIntegerVariable("x", 0, ScaLP::INF());
}


#endif

}   //end namespace flopoco
