#include "IntMult/TilingStrategyOptimalILP.hpp"

#include "BaseMultiplierLUT.hpp"

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
		size_t maxPrefMult):TilingStrategy(
			wX_,
			wY_,
			wOut_,
			signedIO_,
			bmc),
		small_tile_mult_{1}, //Most compact LUT-Based multiplier
		numUsedMults_{0},
		max_pref_mult_ {maxPrefMult},
		occupation_threshold_{occupation_threshold}
	{

	}

void TilingStrategyOptimalILP::solve()
{

#ifndef HAVE_SCALP
    throw "Error, TilingStrategyOptimalILP::solve() was called but FloPoCo was not built with ScaLP library";
#else

    if(baseMultiplierCollection->size() > 6){
        for(int i = 1; i <= 12; i++){
            availSuperTiles.push_back(BaseMultiplierDSPSuperTilesXilinx( (BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE)i) );
        }
        for(int i = 1; i <= 8; i++){
            availNonRectTiles.push_back(BaseMultiplierIrregularLUTXilinx( (BaseMultiplierIrregularLUTXilinx::TILE_SHAPE)i) );
        }
    }

    solver = new ScaLP::Solver(ScaLP::newSolverDynamic({"Gurobi","CPLEX","SCIP","LPSolve"}));
	constructProblem();

    // Try to solve
    cout << "starting solver, this might take a while..." << endl;
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

            if(mult_id <= 1 || mult_id > 7){
                if((unsigned)this->wX-m_x_pos < m_width){           //limit size of DSP Multiplier, to do not protrude from the tiled are
                    m_width = this->wX-m_x_pos;
                }
                if((unsigned)this->wY-m_y_pos < m_height){
                    m_height = this->wY-m_y_pos;
                }
            }
            if(mult_id <= 1){
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

            if(mult_id < 8){
                auto& bm = baseMultiplierCollection->getBaseMultiplier(my_tiles[mult_id].base_index);
                auto param = bm.parametrize( m_width, m_height, x_signed_bit, y_signed_bit);
                auto coord = make_pair(m_x_pos, m_y_pos);
                solution.push_back(make_pair(param, coord));

                float tile_cost = ((1 < mult_id && mult_id <= 7)? my_tiles[mult_id].cost : 0.65 * (m_width + m_height) )  ;
                cout << "cost: " << tile_cost << endl;
                total_cost += tile_cost;
            } else {
                auto& bm = baseMultiplierCollection->getBaseMultiplier(baseMultiplierCollection->size()-((mult_id <= 19)?2:1));
                auto param = bm.parametrize( m_width, m_height, false, false, (mult_id > 19)?mult_id-19:mult_id-7);
                auto coord = make_pair(m_x_pos, m_y_pos);
                solution.push_back(make_pair(param, coord));

                float tile_cost = ( ((mult_id > 19)? my_tiles[mult_id].cost :0.65 * (m_width + m_height) )  );
                cout << "cost: " << tile_cost << endl;
                total_cost += tile_cost;
            }

        }
    }
    cout << "Total cost:" << total_cost <<std::endl;

#endif
}

#ifdef HAVE_SCALP
void TilingStrategyOptimalILP::constructProblem()
{
    cout << "constructing problem formulation..." << endl;
    my_tiles.push_back ({24, 17, 0.00, 0});     // {width, height, total cost (LUT+Compressor), base multiplier id} of tile
    my_tiles.push_back ({17, 24, 0.00, 0});
    my_tiles.push_back ({ 3,  3, 9.90, 2});
    my_tiles.push_back ({ 2,  3, 6.25, 1});
    my_tiles.push_back ({ 3,  2, 6.25, 1});
    my_tiles.push_back ({ 1,  2, 2.30, 1});
    my_tiles.push_back ({ 2,  1, 2.30, 1});
    my_tiles.push_back ({ 1,  1, 1.65, 1});

    for(int i = 0; i < (int)availSuperTiles.size(); i++){
        my_tiles.push_back ({(unsigned)availSuperTiles[i].getMaxWordSizeLargeInputUnsigned(), (unsigned)availSuperTiles[i].getMaxWordSizeSmallInputUnsigned(), 0.00, 8 + (unsigned)i});
    }
    for(int i = 0; i < (int)availNonRectTiles.size(); i++){
        my_tiles.push_back ({(unsigned)availNonRectTiles[i].getMaxWordSizeLargeInputUnsigned(), (unsigned)availNonRectTiles[i].getMaxWordSizeSmallInputUnsigned(), (float)(availNonRectTiles[i].getLUTCost(0,0)), 18 + (unsigned)i});
    }

    wS = my_tiles.size();          //Number of available tiles

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
                obj.add(tempV, (double)my_tiles[s].cost);    //append variable to cost function
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
                        if(shape_contribution(x, y, xs, ys, s) == true){
                            if((1 < s && s < 8) || (shape_occupation(xs, ys, s) >=  occupation_threshold_) ){
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


// determines if a position (x,y) is coverd by a tile (s), relative to the tiles origin position(shape_x,shape_y)
inline bool TilingStrategyOptimalILP::shape_contribution(int x, int y, int shape_x, int shape_y, int s){
    //cerr << "tile=" << s << " xs=" << shape_x << " ys=" << shape_y << " x=" << x << " y=" << y << endl;
    if(s <=7){
        int sign_x = (signedIO && wX-(int)my_tiles[s].wX-1 == shape_x && s <= 1)?1:0;      //The Xilinx DSP-Blocks can process one bit more if signed
        int sign_y = (signedIO && wY-(int)my_tiles[s].wY-1 == shape_y && s <= 1)?1:0;
        return ( 0 <= x-shape_x && x-shape_x < (int)my_tiles[s].wX+sign_x && 0 <= y-shape_y && y-shape_y < (int)my_tiles[s].wY+sign_y );
    } else if (7 < s && s <= 19){                                                            //Handle Xilinx Super Tiles
        //cout << x << "," << y << " " << shape_x << "," << shape_y << ":" << availSuperTiles[s-7].shapeValid(x-shape_x, y-shape_y) << endl;
        return availSuperTiles[s-8].shapeValid(x-shape_x, y-shape_y);
    } else if (19 < s && s <= 27){                                                            //Handle Xilinx Iregular Tiles
        //cout << x << "," << y << " " << shape_x << "," << shape_y << ":" << availSuperTiles[s-7].shapeValid(x-shape_x, y-shape_y) << endl;
        return availNonRectTiles[s-20].shapeValid(x-shape_x, y-shape_y);
    } else {
        return false;
    }

}

//returns cost of a particular tile
inline float TilingStrategyOptimalILP::shape_cost(int s){
    auto& bm = baseMultiplierCollection->getBaseMultiplier(s);
    //int dspCost = bm.getDSPCost(bm.getMaxWordSizeSmallInputUnsigned(), bm.getMaxWordSizeLargeInputUnsigned());
    return (float)bm.getLUTCost(bm.getMaxWordSizeSmallInputUnsigned(), bm.getMaxWordSizeLargeInputUnsigned());
}

//determine the occupation ratio of a given multiplier tile range [0..1],
inline float TilingStrategyOptimalILP::shape_occupation(int shape_x, int shape_y, int s){
    unsigned covered_positions = 0, utilized_positions = 0;
    for(int y = shape_y; y < shape_y + (int)my_tiles[s].wY; y++){
        for(int x = shape_x; x < shape_x + (int)my_tiles[s].wX; x++){
            if(shape_contribution(x, y, shape_x, shape_y, s)){
                covered_positions++;
                if(x < wX && y < wY){
                    utilized_positions++;
                }
            }
        }
    }
    return (float)utilized_positions/(float)covered_positions;
}


}   //end namespace flopoco
