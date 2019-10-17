#include "IntMult/TilingStrategyOptimalILP.hpp"

#include "BaseMultiplierLUT.hpp"

using namespace std;
namespace flopoco {

TilingStrategyOptimalILP::TilingStrategyOptimalILP(
		unsigned int wX_,
		unsigned int wY_,
		unsigned int wOut_,
		bool signedIO_,
		BaseMultiplierCollection* bmc):TilingStrategy(
			wX_,
			wY_,
			wOut_,
			signedIO_,
			bmc),
		small_tile_mult_{1}, //Most compact LUT-Based multiplier
		numUsedMults_{0},
		max_pref_mult_ {1}
	{

	}

void TilingStrategyOptimalILP::solve()
{
#define HAVE_SCALP
#ifndef HAVE_SCALP
    throw "Error, TilingStrategyOptimalILP::solve() was called but FloPoCo was not built with ScaLP library";
#else

    solver = new ScaLP::Solver(ScaLP::newSolverDynamic({"Gurobi","CPLEX","SCIP","LPSolve"}));
	constructProblem();

    // Try to solve
    ScaLP::status stat = solver->solve();

    // print results
    cerr << "The result is " << stat << endl;
    cerr << solver->getResult() << endl;
    ScaLP::Result res = solver->getResult();

    float total_cost = 0;
    for(auto &p:res.values)
    {
        if(p.second == 1){     //parametrize all multipliers at a certain position, for which the solver returned 1 as solution, to flopoco solution structure
            std::string var_name = p.first->getName();
            cout << "is true:  " << var_name.substr(2,dpS) << " " << var_name.substr(2+dpS,dpX) << " " << var_name.substr(2+dpS+dpX,dpY) << std::endl;
            unsigned mult_id = stoi(var_name.substr(2,dpS));
            unsigned m_x_pos = stoi(var_name.substr(2+dpS,dpX)) ;
            unsigned m_y_pos = stoi(var_name.substr(2+dpS+dpX,dpY));
            unsigned m_width = my_tiles[mult_id].wX;
            unsigned m_height = my_tiles[mult_id].wY;

            if(mult_id == 0){
                if((unsigned)this->wX-m_x_pos < m_width){           //limit size of DSP Multiplier, to do not protrude form the tiled area
                    m_width = this->wX-m_x_pos;
                }
                if((unsigned)this->wY-m_y_pos < m_height){
                    m_height = this->wY-m_y_pos;
                }

                if(signedIO && (unsigned)this->wX-m_x_pos-(int)my_tiles[mult_id].wX== 1){           //enlarge the Xilinx DSP Multiplier by one bit, if the inputs are signed and placed to process the MSBs
                    m_width++;
                }
                if(signedIO && (unsigned)this->wY-m_y_pos-(int)my_tiles[mult_id].wY== 1){
                    m_height++;
                }
            }

            bool x_signed_bit = false, y_signed_bit = false;
            if(signedIO && (unsigned)this->wX-m_x_pos-(int)m_width== 0){           //if the inputs are signed, the MSBs of individual tiles at MSB edge the tiled area |_ have to be signed
                x_signed_bit = true;
            }
            if(signedIO && (unsigned)this->wY-m_y_pos-(int)m_height== 0){
                y_signed_bit = true;
            }

            auto& bm = baseMultiplierCollection->getBaseMultiplier(my_tiles[mult_id].base_index);
            auto param = bm.parametrize( m_width, m_height, x_signed_bit, y_signed_bit);
            auto coord = make_pair(m_x_pos, m_y_pos);
            solution.push_back(make_pair(param, coord));

            float tile_cost = ( ((my_tiles[mult_id].base_index)? ceil(bm.getLUTCost(m_width, m_height)) :0 ) + 0.65 * (m_width + m_height) );
            total_cost += tile_cost;
            cout << "Mult. id:" << mult_id << " wX:" << m_x_pos << " xY:" << m_y_pos << " m_width:" << m_width << " m_height:" << m_height << "tile_cost:" << tile_cost <<std::endl;
        }
    }
    cout << "Total cost:" << total_cost <<std::endl;

#endif
}

#ifdef HAVE_SCALP
void TilingStrategyOptimalILP::constructProblem()
{

    my_tiles.push_back ({24, 17, 0.00, 0});     // {width, height, total cost (LUT+Compressor), base multiplier id} of tile
    my_tiles.push_back ({ 3,  3, 9.90, 2});
    my_tiles.push_back ({ 2,  3, 6.25, 1});
    my_tiles.push_back ({ 3,  2, 6.25, 1});
    my_tiles.push_back ({ 1,  2, 2.30, 1});
    my_tiles.push_back ({ 2,  1, 2.30, 1});
    my_tiles.push_back ({ 1,  1, 1.65, 1});
    wS = my_tiles.size();          //Number of available tiles

    //Assemble cost function, declare problem variables
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
                obj.add(tempV, (double)my_tiles[s].cost);    //append variable to cost function
            }
        }
    }

    // add the Constraints
    for(int y = 0; y < wY; y++){
        for(int x = 0; x < wX; x++){
            stringstream consName;
            consName << "p" << x << y << ": ";					//one constraint for every position in the area to be tiled
            ScaLP::Term pxyTerm;
            for(int s = 0; s < wS; s++){					//for every available tile...
                for(int ys = 0; ys <= y; ys++){					//...check if the position x,y gets covered by tile s located at position (xs, ys) = (0..x, 0..y)
                    for(int xs = 0; xs <= x; xs++){
                        int sign_x = (signedIO && wX-(int)my_tiles[s].wX-1 == xs && s == 0)?1:0;      //The Xilinx DSP-Blocks can process one bit more if signed
                        int sign_y = (signedIO && wY-(int)my_tiles[s].wY-1 == ys && s == 0)?1:0;
                        if(( 0 <= x-xs && x-xs < (int)my_tiles[s].wX+sign_x && 0 <= y-ys && y-ys < (int)my_tiles[s].wY+sign_y ) == true){
                            pxyTerm.add(solve_Vars[s][xs][ys], 1); //even better

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
    int sn = 0;
    stringstream consName;
    consName << "lims" << sn << ": ";
    ScaLP::Term pxyTerm;
    for(int y = 0; y < wY; y++){
        for(int x = 0; x < wX; x++){
            pxyTerm.add(solve_Vars[sn][x][y], 1);
        }
    }
    ScaLP::Constraint c1Constraint = pxyTerm <= max_pref_mult_;     //set max usage equ.
    c1Constraint.name = consName.str();
    solver->addConstraint(c1Constraint);

    // Set the Objective
    solver->setObjective(ScaLP::minimize(obj));

    // Write Linear Program to file for debug purposes
    solver->writeLP("tile.lp");
}


#endif


// determines if a position is coverd by a tile, relative to the tiles origin (0,0)
bool TilingStrategyOptimalILP::shape_contribution(int x, int y, int s){
    auto& bm = baseMultiplierCollection->getBaseMultiplier(s);
    cerr << "shape=" << s << " wX=" << bm.getMaxWordSizeSmallInputUnsigned() << " wY=" << bm.getMaxWordSizeLargeInputUnsigned() << endl;
    return bm.shapeValid(bm.parametrize(bm.getMaxWordSizeSmallInputUnsigned()-1, bm.getMaxWordSizeLargeInputUnsigned()-1, false, false),x,y);
}

//returns cost of a particular tile
float TilingStrategyOptimalILP::shape_cost(int s){
    auto& bm = baseMultiplierCollection->getBaseMultiplier(s);
    //int dspCost = bm.getDSPCost(bm.getMaxWordSizeSmallInputUnsigned(), bm.getMaxWordSizeLargeInputUnsigned());
    return (float)bm.getLUTCost(bm.getMaxWordSizeSmallInputUnsigned(), bm.getMaxWordSizeLargeInputUnsigned());
}


}   //end namespace flopoco
