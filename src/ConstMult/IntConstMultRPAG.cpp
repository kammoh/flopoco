/*
  Integer constant multiplication using minimum number of adders due to

  Gustafsson, O., Dempster, A., Johansson, K., Macleod, M., & Wanhammar, L. (2006).
  Simplified Design of Constant Coefficient Multipliers. Circuits, Systems, and Signal Processing, 25(2), 225–251.

  All constants up to 19 bit will be realized optimal using precomputed tables provided by the SPIRAL project (http://spiral.ece.cmu.edu/mcm/).
  
  Author : Martin Kumm kumm@uni-kassel.de, (emulate adapted from Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr)

  All rights reserved.

*/
#if defined(HAVE_PAGLIB) && defined(HAVE_RPAGLIB)

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"

#include "IntConstMultRPAG.hpp"

#include "pagsuite/scm_solutions.hpp"
#include "pagsuite/pagexponents.hpp"
#include "pagsuite/compute_successor_set.h"
#include "pagsuite/log2_64.h"
#include "pagsuite/fundamental.h"

#include <algorithm>
#include <cassert>

using namespace std;
using namespace PAGSuite;

namespace flopoco{

    IntConstMultRPAG::IntConstMultRPAG(Operator* parentOp, Target* target, int wIn, int coeff, bool syncInOut)  : IntConstMultShiftAdd(parentOp, target, wIn, "", false, syncInOut, 1000, false)
    {
    	set<int_t> target_set;
    	target_set.insert(coeff);

		PAGSuite::rpag *rpag = new PAGSuite::rpag(); //default is RPAG with 2 input adders

		for(int_t t : target_set)
			rpag->target_set->insert(t);
		
		PAGSuite::cost_model_t cost_model = PAGSuite::HL_FPGA;// with default value
		PAGSuite::global_verbose = 0;
		rpag->set_cost_model(cost_model);
		rpag->optimize();

		vector<set<int_t>> pipeline_set = rpag->get_best_pipeline_set();

		list<realization_row<int_t> > pipelined_adder_graph;
		pipeline_set_to_adder_graph(pipeline_set, pipelined_adder_graph, true, rpag->get_c_max());
		append_targets_to_adder_graph(pipeline_set, pipelined_adder_graph, target_set);

		string adderGraph = output_adder_graph(pipelined_adder_graph,true);

		cout << "adderGraph=" << adderGraph << endl;

		ProcessIntConstMultShiftAdd(target,adderGraph);

        ostringstream name;
        name << "IntConstMultRPAG_" << coeff << "_" << wIn;
        setName(name.str());
    }

    OperatorPtr flopoco::IntConstMultRPAG::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args ) {
        int wIn, constant;

        UserInterface::parseInt( args, "wIn", &wIn );
        UserInterface::parseInt( args, "constant", &constant );

        return new IntConstMultRPAG(parentOp, target, wIn, constant, false);
    }

    void flopoco::IntConstMultRPAG::registerFactory() {

        UserInterface::add( "IntConstMultRPAG", // name
                            "Integer constant multiplication using shift and add in an optimal way (i.e., with minimum number of adders). Works for coefficients up to " + std::to_string(MAX_SCM_CONST) + " (19 bit)", // description, string
                            "ConstMultDiv", // category, from the list defined in UserInterface.cpp
                            "", //seeAlso
                            "wIn(int): Input word size; \
                            constant(int): constant;",
                            "Nope.",
                            IntConstMultRPAG::parseArguments
                          ) ;
    }

}
#else

#include "IntConstMultRPAG.hpp"
namespace flopoco
{
	void IntConstMultRPAG::registerFactory() { }
}

#endif //defined(HAVE_PAGLIB) && defined(HAVE_RPAGLIB)

