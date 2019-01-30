#define BOOST_TEST_DYN_LINK   
#define BOOST_TEST_MODULE IntConstMultShiftAddAdderCostTest

#include <boost/test/unit_test.hpp>
#include "pagsuite/adder_graph.h"
#include "ConstMult/error_comp_graph.hpp"

#include <sstream>
#include <iostream>

using namespace std;
using namespace PAGSuite;
using namespace IntConstMultShiftAdd_TYPES;

BOOST_AUTO_TEST_CASE(SignedShiftedAdd) {
	adder_graph_t adder_graph;
	string graph_descr = "{{'R',[1],1,[1],0},{'A',[3],1,[1],0,0,[1],0,1},{'A',[11],2,[1],1,3,[3],1,0},{'O',[11],2,[11],2,0}}";

	adder_graph.parse_to_graph(graph_descr);
	ostringstream output_stream;
	
	print_aligned_word_graph(adder_graph, "", 8, output_stream);

	string TestVal = "Decomposition for 11:\n\nxxxxxxxxxxxx 11X\n(\n xxxxxxxx    1X\n"
		"  xxxxxxxxxx 3X\n  (\n    xxxxxxxx 1X\n   xxxxxxxx  1X\n  )\n)\n\n";

	string errorMsg = "Bad message. [\n" + TestVal + "\n] was expected, got [\n" 
			+ output_stream.str() + "\n]";

	
	BOOST_REQUIRE_MESSAGE(TestVal == output_stream.str(), errorMsg);
}
