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

#include "IntConstMultShiftAddRPAG.hpp"

#include "pagsuite/pagexponents.hpp"
#include "pagsuite/compute_successor_set.h"
#include "pagsuite/log2_64.h"
#include "pagsuite/fundamental.h"

#include <algorithm>
#include <cassert>

using namespace std;
using namespace PAGSuite;

namespace flopoco{

    IntConstMultShiftAddRPAG::IntConstMultShiftAddRPAG(Operator* parentOp, Target* target, int wIn, int coeff, bool syncInOut, int epsilon)  : IntConstMultShiftAdd(parentOp, target, wIn, "", false, syncInOut, 1000, false, epsilon)
    {
    	set<int_t> target_set;
    	target_set.insert(coeff);

		PAGSuite::rpag *rpag = new PAGSuite::rpag(); //default is RPAG with 2 input adders

		for(int_t t : target_set)
			rpag->target_set->insert(t);


		PAGSuite::global_verbose = UserInterface::verbose-1; //set rpag to one less than verbose of FloPoCo

		PAGSuite::cost_model_t cost_model = PAGSuite::LL_FPGA;// with default value
		rpag->input_wordsize = wIn;
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

    OperatorPtr flopoco::IntConstMultShiftAddRPAG::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args ) {
        int wIn, constant, epsilon;

        UserInterface::parseInt( args, "wIn", &wIn );
		UserInterface::parseInt( args, "constant", &constant );
		UserInterface::parseInt( args, "epsilon", &epsilon );

        return new IntConstMultShiftAddRPAG(parentOp, target, wIn, constant, false, epsilon);
    }

    void flopoco::IntConstMultShiftAddRPAG::registerFactory() {

        UserInterface::add( "IntConstMultShiftAddRPAG", // name
                            "Integer constant multiplication using shift and add using the RPAG algorithm", // description, string
                            "ConstMultDiv", // category, from the list defined in UserInterface.cpp
                            "", //seeAlso
                            "wIn(int): Input word size; \
                            constant(int): constant; \
                            epsilon(int)=0: Allowable error for truncated constant multipliers;",
                            "Nope.",
                            IntConstMultShiftAddRPAG::parseArguments
                          ) ;
    }

}
#else

#include "IntConstMultShiftAddRPAG.hpp"
namespace flopoco
{
	void IntConstMultShiftAddRPAG::registerFactory() { }
}

#endif //defined(HAVE_PAGLIB) && defined(HAVE_RPAGLIB)

