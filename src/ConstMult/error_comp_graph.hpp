#ifndef ERROR_COMP_GRAPH_HPP
#define ERROR_COMP_GRAPH_HPP

#include <iostream>
#include <pagsuite/adder_graph.h>
#include "IntConstMultShiftAddTypes.hpp"

using namespace std;

namespace IntConstMultShiftAdd_TYPES {
void print_aligned_word_graph(
		PAGSuite::adder_graph_t & adder_graph,	
		string truncations,
		int input_word_size,
		ostream & output_stream
	);

void print_aligned_word_node(
		PAGSuite::adder_graph_base_node_t* node,
		TruncationRegister const & truncationReg,
		int right_shift,
		int total_word_size,
		int input_word_size,
		int  truncation,
		ostream& output_stream
		);
}

#endif
