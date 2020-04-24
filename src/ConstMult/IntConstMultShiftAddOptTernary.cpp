/*
  Integer constant multiplication using minimum number of ternary adders due to

  Kumm, M., Gustafsson, O., Garrido, M., & Zipf, P. (2018). Optimal Single Constant Multiplication using Ternary Adders.
  IEEE Transactions on Circuits and Systems II: Express Briefs, 65(7), 928–932. http://doi.org/10.1109/TCSII.2016.2631630

  Author : Martin Kumm kumm@uni-kassel.de, (emulate adapted from Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr)

  All rights reserved.

*/
#if defined(HAVE_PAGLIB)

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"

#include "IntConstMultShiftAddOptTernary.hpp"
#include "IntConstMultShiftAdd.hpp"

#include "tscm_solutions.hpp"
#include "pagsuite/rpagt.h"
#include "pagsuite/rpag_functions.h"

#include "pagsuite/compute_successor_set.h"
#include "pagsuite/log2_64.h"
#include "pagsuite/fundamental.h"

#include <algorithm>

using namespace std;
using namespace PAGSuite;

int rpag_pointer::input_wordsize;

namespace flopoco{

    IntConstMultShiftAddOptTernary::IntConstMultShiftAddOptTernary(Operator* parentOp, Target* target, int wIn, int coeff, bool syncInOut) : IntConstMultShiftAdd(parentOp, target, wIn, "", false, syncInOut, 1000, false, epsilon)
    {
		int maxCoefficient = 4194303; //=2^22-1

        int shift;
        int coeffOdd = fundamental(coeff, &shift);

		if(coeffOdd <= maxCoefficient)
        {
            cout << "nofs[" << coeff << "]=" << nofs[(coeff-1)>>1][0] << " " << nofs[(coeff-1)>>1][1] << endl;

            int nof1 = nofs[(coeffOdd-1)>>1][0];
            int nof2 = nofs[(coeffOdd-1)>>1][1];

            set<int_t> coefficient_set;
            coefficient_set.insert(coeff);

            // set<int> nof_set;
            // if(nof1 != 0) nof_set.insert(nof1);
            // if(nof2 != 0) nof_set.insert(nof2);

            stringstream ss;
            ss << coeff;

            // string adderGraphString = "{'A',[12289],1,[1],0,12,[1],0,13,[1],0,"
            //                           "0},{'O',[12289],1,[12289],1,0}";

            rpag_pointer *rpagp = new rpagt();
            rpagp->target_vec.push_back((char*) ss.str().c_str());
            rpagp->ternary_adders = true;
            cost_model_t cost_model = LL_FPGA;
            ((rpag_base<int_t> *)rpagp)->set_cost_model(cost_model);
            rpagp->cost_model = cost_model;

            rpagp->input_wordsize = wIn;


            // computeAdderGraphTernary(nof_set, coefficient_set, adderGraphString);
            rpagp->optimize();

            auto pipeline_set_best =
                ((rpag_base<int_t> *)rpagp)->get_best_pipeline_set();

            list<realization_row<int_t>> pipelined_adder_graph;

            std::cout << "pipeline_set_to_adder_graph: [begin]" << std::endl;
            pipeline_set_to_adder_graph(
                pipeline_set_best, pipelined_adder_graph,
                ((rpag_base<int_t> *)rpagp)->is_this_a_two_input_system(),
                ((rpag_base<int_t> *)rpagp)->get_c_max(),
                ((rpag_base<int_t> *)rpagp)->ternary_sign_filter);
            std::cout << "pipeline_set_to_adder_graph: [DONE]" << std::endl;
            append_targets_to_adder_graph(pipeline_set_best, pipelined_adder_graph, coefficient_set);
            remove_redundant_inputs_from_adder_graph(pipelined_adder_graph, pipeline_set_best);

            string adderGraphString = output_adder_graph(pipelined_adder_graph,
                                                      true);

            std::cout << "adderGraphString = " << adderGraphString << std::endl;
            delete rpagp;
            
            // stringstream adderGraphStringStream;
            // adderGraphStringStream << "{" << adderGraphString;

            // if(coeff != coeffOdd)
            // {
            //     if(adderGraphString.length() > 1) adderGraphStringStream << ",";
			// 	adderGraphStringStream << "{'O',[" << coeff << "],2,[" << coeffOdd << "],1," << shift << "}";
            // }
            // adderGraphStringStream << "}";

            cout << "adder_graph=" << adderGraphString << endl;

            ProcessIntConstMultShiftAdd(target, adderGraphString);

            ostringstream name;
            name << "IntConstMultShiftAddOptTernary_" << coeffOdd << "_" << wIn;
            setName(name.str());

        }
        else
        {
			cerr << "Error: Coefficient too large, max. odd coefficient is " << maxCoefficient << endl;
            exit(-1);
        }

	}

    OperatorPtr flopoco::IntConstMultShiftAddOptTernary::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args ) {
        int wIn, constant;

        UserInterface::parseInt( args, "wIn", &wIn );
        UserInterface::parseInt( args, "constant", &constant );

        return new IntConstMultShiftAddOptTernary(parentOp, target, wIn, constant, false);
    }
}
#endif

namespace flopoco{
    void flopoco::IntConstMultShiftAddOptTernary::registerFactory() {

#if defined(HAVE_PAGLIB)
        UserInterface::add( "IntConstMultShiftAddOptTernary", // name
                            "Integer constant multiplication using shift and ternary additions in an optimal way (i.e., with minimum number of ternary adders). Works for coefficients up to 4194303 (22 bit)", // description, string
                            "ConstMultDiv", // category, from the list defined in UserInterface.cpp
                            "", //seeAlso
                            "wIn(int): Input word size; \
                            constant(int): constant;",
                            "Nope.",
                            IntConstMultShiftAddOptTernary::parseArguments
                          ) ;
#endif
    }
}

