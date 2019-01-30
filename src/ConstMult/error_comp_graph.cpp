#include "error_comp_graph.hpp"

#include<set>

using namespace PAGSuite;

namespace IntConstMultShiftAdd_TYPES {

	void print_aligned_word_node(
			adder_graph_base_node_t* node,
			TruncationRegister const & truncationReg,
			int right_shift,
			int total_word_size,
			int input_word_size,
			int truncation,
			ostream & output_stream
		)
	{
		if (is_a<output_node_t>(*node) || is_a<register_node_t>(*node)) {
			register_node_t& t = *((register_node_t*) node);
			print_aligned_word_node(
					t.input,
					truncationReg,
					right_shift	+ t.input_shift,
					total_word_size,
					input_word_size,
					truncation,
					output_stream
				);
			return;
		}

		int wordsize = computeWordSize(node->output_factor, input_word_size);
		int start = total_word_size - (wordsize + right_shift);
		int64_t output_factor = node->output_factor[0][0];
		output_stream << string(start, ' ');
		output_stream << string(wordsize - truncation, 'x');
		output_stream << string(truncation, '-');
		output_stream << string(1 + total_word_size - (wordsize + start), ' ');
		output_stream << output_factor << "X" << endl;
		if (is_a<adder_subtractor_node_t>(*node)) {
			output_stream << string(start, ' ' ) << "(" << endl;
			adder_subtractor_node_t & t = *(adder_subtractor_node_t*) node;
			size_t nb_inputs = t.inputs.size();
			auto truncVal = truncationReg.getTruncationFor(output_factor, t.stage);
			for (size_t i = 0 ; i < nb_inputs ; ++i) {
				print_aligned_word_node(
						t.inputs[i],
						truncationReg,
						right_shift + t.input_shifts[i],
						total_word_size,
						input_word_size,
						truncVal[i],
						output_stream
					);
			}
			output_stream << string(start, ' ' ) << ")" << endl;
		} 
	}

	void print_aligned_word_graph(
			PAGSuite::adder_graph_t &adder_graph,
			TruncationRegister truncationReg, int input_word_size,
			ostream &output_stream)
	{
		set<input_node_t*> inputs;
		set<output_node_t*> outputs;

		int max_word_size = 0;
		for (auto nodePtr : adder_graph.nodes_list) {
			int word_size = computeWordSize(nodePtr->output_factor, input_word_size);
			if (word_size > max_word_size) {
				max_word_size = word_size;
			}
			if (is_a<output_node_t>(*nodePtr)) {
				outputs.insert((output_node_t*) nodePtr);
			} else if  (is_a<input_node_t>(*nodePtr)) {
				inputs.insert((input_node_t*) nodePtr);
			}
		}

		for (auto outNodePtr : outputs) {
			output_stream << "Decomposition for " <<
						  outNodePtr->output_factor[0][0] << ":" << endl << endl ;
			print_aligned_word_node(
					outNodePtr,
					truncationReg,
					0,
					max_word_size,
					input_word_size,
					0,
					output_stream
			);
			output_stream << endl;
		}
	}

	void print_aligned_word_graph(
			adder_graph_t & adder_graph,
			string truncations,
			int input_word_size,
			ostream & output_stream
		)
	{
		TruncationRegister truncationReg(truncations);
		print_aligned_word_graph(adder_graph,truncationReg,input_word_size,output_stream);
	}

}

