#include "IntMult/TilingStrategyOptimalILP.hpp"

#include "BaseMultiplierLUT.hpp"
#include "MultiplierTileCollection.hpp"

using namespace std;
namespace flopoco {

TilingStrategyOptimalILP::TilingStrategyOptimalILP(
		unsigned int wX_,
		unsigned int wY_,
		unsigned int wOut_,
		bool signedIO_,
		BaseMultiplierCollection* bmc,
		base_multiplier_id_t prefered_multiplier,
		float occupation_threshold,
		size_t maxPrefMult,
        MultiplierTileCollection mtc_):TilingStrategy(
			wX_,
			wY_,
			wOut_,
			signedIO_,
			bmc),
		small_tile_mult_{1}, //Most compact LUT-Based multiplier
		numUsedMults_{0},
		max_pref_mult_ {maxPrefMult},
		occupation_threshold_{occupation_threshold},
		tiles{mtc_.MultTileCollection}
	{

	}

void TilingStrategyOptimalILP::solve()
{

#ifndef HAVE_SCALP
    throw "Error, TilingStrategyOptimalILP::solve() was called but FloPoCo was not built with ScaLP library";
#else
    solver = new ScaLP::Solver(ScaLP::newSolverDynamic({"Gurobi","CPLEX","SCIP","LPSolve"}));
	constructProblem();

    // Try to solve
    cout << "starting solver, this might take a while..." << endl;
    ScaLP::status stat = solver->solve();

    // print results
    cerr << "The result is " << stat << endl;
    //cerr << solver->getResult() << endl;
    ScaLP::Result res = solver->getResult();

    double total_cost = 0;
    for(auto &p:res.values)
    {
        if(p.second > 0.5){     //parametrize all multipliers at a certain position, for which the solver returned 1 as solution, to flopoco solution structure
            std::string var_name = p.first->getName();
            cout << "is true:  " << var_name.substr(2,dpS) << " " << var_name.substr(2+dpS,dpX) << " " << var_name.substr(2+dpS+dpX,dpY) << std::endl;
            unsigned mult_id = stoi(var_name.substr(2,dpS));
            unsigned m_x_pos = stoi(var_name.substr(2+dpS,dpX)) ;
            unsigned m_y_pos = stoi(var_name.substr(2+dpS+dpX,dpY));

            total_cost += (double)tiles[mult_id]->cost();
            auto coord = make_pair(m_x_pos, m_y_pos);
            solution.push_back(make_pair(tiles[mult_id]->getParametrisation().tryDSPExpand(m_x_pos, m_y_pos, wX, wY, signedIO), coord));

        }
    }
    cout << "Total cost:" << total_cost <<std::endl;

#endif
}

#ifdef HAVE_SCALP
void TilingStrategyOptimalILP::constructProblem()
{
    cout << "constructing problem formulation..." << endl;
    wS = tiles.size();

    //Assemble cost function, declare problem variables
    cout << "   assembling cost function, declaring problem variables..." << endl;
    ScaLP::Term obj;
    int nx = wX-1, ny = wY-1, ns = wS-1; dpX = 1; dpY = 1; dpS = 1; //calc number of decimal places, for var names
    while (nx /= 10)
        dpX++;
    while (ny /= 10)
        dpY++;
    while (ns /= 10)
        dpS++;
    vector<vector<vector<ScaLP::Variable>>> solve_Vars(wS, vector<vector<ScaLP::Variable>>(wX, vector<ScaLP::Variable>(wY)));
    for(int s = 0; s < wS; s++){                                    //generate variable for every type of tile at every position
        for(int x = 0; x < wX; x++){
            for(int y = 0; y < wY; y++){
                stringstream nvarName;
                nvarName << " d" << setfill('0') << setw(dpS) << s << setfill('0') << setw(dpX) << x << setfill('0') << setw(dpY) << y;
                ScaLP::Variable tempV = ScaLP::newBinaryVariable(nvarName.str());
                solve_Vars[s][x][y] = tempV;
                obj.add(tempV, (double)tiles[s]->cost());    //append variable to cost function
            }
        }
    }

    // add the Constraints
    cout << "   adding the constraints to problem formulation..." << endl;
    for(int y = 0; y < wY; y++){
        for(int x = 0; x < wX; x++){
            stringstream consName;
            consName << "p" << x << y << ": ";					//one constraint for every position in the area to be tiled
            ScaLP::Term pxyTerm;
            for(int s = 0; s < wS; s++){					//for every available tile...
                for(int ys = 0; ys <= y; ys++){					//...check if the position x,y gets covered by tile s located at position (xs, ys) = (0..x, 0..y)
                    for(int xs = 0; xs <= x; xs++){
                        //if(shape_contribution(x, y, xs, ys, s) == true){
                        if(tiles[s]->shape_contribution(x, y, xs, ys, wX, wY, signedIO) == true){
                            if(tiles[s]->shape_utilisation(xs, ys, wX, wY, signedIO) >=  occupation_threshold_ ){
                                pxyTerm.add(solve_Vars[s][xs][ys], 1);
                            }
                        }
                    }
                }
            }
            ScaLP::Constraint c1Constraint = pxyTerm == (bool)1;
            c1Constraint.name = consName.str();
            solver->addConstraint(c1Constraint);
        }
    }

    //limit use of shape n
    cout << "   adding the constraint to limit the use of DSP-Blocks to " << max_pref_mult_ <<" instances..." << endl;
    int sn = 0;
    stringstream consName;
    consName << "lims" << sn << ": ";
    ScaLP::Term pxyTerm;
    for(int y = 0; y < wY; y++){
        for(int x = 0; x < wX; x++){
            pxyTerm.add(solve_Vars[0][x][y], 1);
            pxyTerm.add(solve_Vars[1][x][y], 1);
        }
    }
    ScaLP::Constraint c1Constraint = pxyTerm <= max_pref_mult_;     //set max usage equ.
    c1Constraint.name = consName.str();
    solver->addConstraint(c1Constraint);

    // Set the Objective
    cout << "   setting objective (minimize cost function)..." << endl;
    solver->setObjective(ScaLP::minimize(obj));

    // Write Linear Program to file for debug purposes
    cout << "   writing LP-file for debuging..." << endl;
    solver->writeLP("tile.lp");
}


#endif

}   //end namespace flopoco
