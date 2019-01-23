/*  IntConstMultShiftAdd_TYPES.cpp
 *  Version:
 *  Datum: 20.11.2014
 */
#ifdef HAVE_PAGLIB
#include "IntConstMultShiftAddTypes.hpp"

#include <algorithm>
#include <sstream>

//#include "FloPoCo.hpp"
#include "PrimitiveComponents/GenericLut.hpp"
#include "PrimitiveComponents/GenericMux.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_TernaryAdd_2State.hpp"
//#include "PrimitiveComponents/Altera/Altera_TernaryAdd.hpp"

using namespace PAGSuite;

namespace IntConstMultShiftAdd_TYPES {
string IntConstMultShiftAdd_BASE::target_ID = "NONE";
int IntConstMultShiftAdd_BASE::noOfConfigurations = 0;
int IntConstMultShiftAdd_BASE::configurationSignalWordsize = 0;
int IntConstMultShiftAdd_BASE::noOfInputs = 0;


string IntConstMultShiftAdd_INPUT::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    return "";
}

string IntConstMultShiftAdd_REG::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    register_node_t* t = (register_node_t*)(base_node);
    
    bool negate=false;
    for(unsigned int i=0;i<t->output_factor.size();++i){
        for(unsigned int j=0;j<t->output_factor[i].size();++j){
            if ( t->output_factor[i][j] != DONT_CARE ){
                if ( (t->output_factor[i][j] > 0 && t->input->output_factor[i][j] < 0)
                    || (t->output_factor[i][j] < 0 && t->input->output_factor[i][j] > 0) ){
						negate = true;
                        i = t->output_factor.size();
						break;
				}
			}
		}
	}
    
    base_op->vhdl << "\t" << base_op->declare(outputSignalName,wordsize) << " <= ";
    if(negate) base_op->vhdl << "std_logic_vector(-signed(";
    base_op->vhdl << getShiftAndResizeString(InfoMap[t->input],wordsize,t->input_shift,false);
    if(negate) base_op->vhdl << "))";
    base_op->vhdl << ";" << endl;
    return "";
}

void IntConstMultShiftAdd_BASE::binary_adder(
			IntConstMultShiftAdd_BASE* left,
			IntConstMultShiftAdd_BASE* right,
			int left_shift,
			int right_shift,
			int left_trunc,
			int right_trunc,
			string result_name
		)
{
	int left_word_size_init = left->wordsize;
	int right_word_size_init = right->wordsize;

	int left_word_size = left_word_size_init + left_shift;
	int right_word_size = right_word_size_init + right_shift;

	int nb_zeros_left = left_trunc + left_shift;
	int nb_zeros_right = right_trunc + right_shift;
	
	//The two input superposition is cut in three segments:
	//The rightmost one (lsb) is a certain number of known zeros
	int nb_trail_zeros = min(nb_zeros_right, nb_zeros_left);
	string trailing_zeros = (nb_trail_zeros > 0) ? "& " + zg(nb_trail_zeros)  : "";
	
	//then if the number of known zeros of the two words are unequal, there is
	//some bits which are copied without further processing;
	int max_known_zeros = max(nb_zeros_right, nb_zeros_left);
	int min_word_size = min(left_word_size, right_word_size);
	int copy_as_is_boundary = min(max_known_zeros, min_word_size);
	int nb_copy_as_is = copy_as_is_boundary - nb_trail_zeros;

	IntConstMultShiftAdd_BASE* single_trail_input = 
		(nb_zeros_left < nb_zeros_right) ? left : right;

	string copy_as_is = (nb_copy_as_is > 0) ? 
		"& " + single_trail_input->outputSignalName + 
		range(copy_as_is_boundary - 1, nb_trail_zeros) : "";

	//then it depends whether the two outputs actually overlaps or not
	bool needs_adder = (min_word_size > max_known_zeros);
	string msb_part;

	if (true or needs_adder) { //TODO Check signed case for overlap to see if
							   //we can use the direct extension
		int adder_word_size = wordsize - copy_as_is_boundary;
		
		int left_left_bound = max(copy_as_is_boundary, left_word_size);
		int right_left_bound = max(copy_as_is_boundary, right_word_size);
		int left_right_bound = max(copy_as_is_boundary, nb_zeros_left);
		int right_right_bound = max(copy_as_is_boundary, nb_zeros_right);

		int padding_head_left = wordsize - left_left_bound; 		
		int padding_head_right = wordsize - right_left_bound;
		int useful_bits_left = left_left_bound - left_right_bound;
		int useful_bits_right = right_left_bound - right_right_bound;

		string left_add_operand = left->getTemporaryName();
		string right_add_operand = right->getTemporaryName();
		
		ostringstream left_sign_ext, right_sign_ext;
		bool concat_left, concat_right;
		concat_left = concat_right = true;
		if (padding_head_left > 0) {
			left_sign_ext << "(" << padding_head_left - 1 << " downto 0 => " <<
				left->outputSignalName << of(left->wordsize - 1) <<")";
		} else {
			concat_left = false;
		}

		if (padding_head_right > 0) {
			right_sign_ext << "(" << padding_head_right - 1 << " downto 0 => " <<
				right->outputSignalName << of(right->wordsize - 1) <<")";
		} else {
			concat_right = false;
		}

		string select_range_left, select_range_right;
		select_range_left = select_range_right = "";

		if (useful_bits_left > 0) {
			int startidx_left = left_right_bound - left_shift;
			select_range_left = left->outputSignalName + 
				range(startidx_left + useful_bits_left - 1, startidx_left);
		} else {
			concat_left = false;
		}

		if (useful_bits_right > 0) {
			int startidx_right = right_right_bound - right_shift;
			select_range_right = right->outputSignalName + 
				range(startidx_right + useful_bits_right - 1, startidx_right);
		} else {
			concat_right = false;
		}

		string concat_left_str = (concat_left) ? " & " : "";
		string concat_right_str = (concat_right) ? " & " : "";

		string left_declare = left_sign_ext.str() + concat_left_str + select_range_left; 
		string right_declare = right_sign_ext.str() + concat_right_str + select_range_right; 

		base_op->vhdl << "\t" << base_op->declare(
				.0, 
				left_add_operand, 
				adder_word_size
			) << " <= " << left_declare << ";" << endl;

		base_op->vhdl << "\t" << base_op->declare(
				.0, 
				right_add_operand, 
				adder_word_size
			) << " <= " << right_declare << ";" << endl;
			
		GenericAddSub* addsub = new GenericAddSub(base_op, target, adder_word_size);
		cerr << "-->" << addsub->getName() << endl;
		msb_part = "sum_o_" + outputSignalName;
		base_op->inPortMap(addsub, addsub->getInputName(0), left_add_operand);
		base_op->inPortMap(addsub, addsub->getInputName(1), right_add_operand);
		base_op->outPortMap(addsub, "sum_o", msb_part);
		base_op->vhdl << base_op->instance(addsub, "adder_"+outputSignalName);
		base_op->addSubComponent(addsub);
	} else {
		IntConstMultShiftAdd_BASE* single_head_input = 
			(single_trail_input == left) ? right : left;	
		string right_padd = (copy_as_is_boundary < max_known_zeros) ?
			" & " + zg(max_known_zeros - copy_as_is_boundary) : "";

		int leftmost_word_size_init = 
			(single_head_input == left) ? left_word_size_init : right_word_size_init;
		int leftmost_truncate = 
			(single_head_input == left) ? left_trunc : right_trunc;
		msb_part = "\"0\" & " + single_head_input->outputSignalName + 
			range(leftmost_word_size_init - leftmost_truncate, left_word_size_init); 
	}

	base_op->vhdl << "\t" <<  result_name << " <= " << 
		msb_part + copy_as_is + trailing_zeros << ";" << endl ;
}

string IntConstMultShiftAdd_ADD2::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);

	IntConstMultShiftAdd_BASE* left = InfoMap[t->inputs[0]];
	IntConstMultShiftAdd_BASE* right = InfoMap[t->inputs[1]];
	int left_trunc = truncations[0];
	int right_trunc = truncations[1];
	int left_word_size_init = left->wordsize;
	int right_word_size_init = right->wordsize;
	if (
			right_trunc >= right_word_size_init || 
			left_trunc >= left_word_size_init 
	) {
		throw string{"IntConstMultShiftAdd_ADD2::get_realisation:"
		" one input truncation is greater than its wordsize"};
	}

	declare(outputSignalName, wordsize);

	binary_adder(
			left, 
			right, 
			t->input_shifts[0], 
			t->input_shifts[1], 
			left_trunc, 
			right_trunc,
			outputSignalName
		);

	return "";//base_op->vhdl.str();
}

string IntConstMultShiftAdd_ADD3::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);
	vector<int> opsize_word(3);
	vector<int> opsize_shifted_word(3);
	vector<int> known_zeros(3);
	vector<IntConstMultShiftAdd_BASE*> inputs_info(3);

	for (size_t i = 0 ; i < 3 ; ++i) {
		inputs_info[i] = InfoMap[t->inputs[i]];
		IntConstMultShiftAdd_BASE& currentInput = *(inputs_info[i]);
		opsize_word[i] = currentInput.wordsize;
		opsize_shifted_word[i] = currentInput.wordsize + t->input_shifts[i];
		known_zeros[i] = truncations[i] + t->input_shifts[i];
	}

	//Find if we have trailing zeros to remove them from the addition
	auto min_known_zeros_it = min_element(known_zeros.begin(), known_zeros.end());
	size_t trail_op_idx = min_known_zeros_it - known_zeros.begin();

	//How many bits do we have with exactly two inputs at zero 
	auto second_min_ptr = max_element(known_zeros.begin(), known_zeros.end());
	int  second_min = *second_min_ptr;
	for (size_t i = 0 ; i < 3 ; ++i) {
		if (i == trail_op_idx) {
			continue;
		}
		if (known_zeros[i] < second_min) {
			second_min = known_zeros[i];
		}
	}

	//If the "trailing" operand fits in he leading zeros, we don't need a
	//ternary adder
	string msb_signame = "msb_res_" + outputSignalName;
	bool needs_ternary_adder = (second_min < opsize_shifted_word[trail_op_idx]);

	int adder_word_size = *(max_element(
				opsize_shifted_word.begin(),
				opsize_shifted_word.end()
				)
			) + 1 - second_min;

	if (true or needs_ternary_adder) {//TODO check if optimisation is still valid 
									  //with signed input
		vector<string> op_tmp_signal_name(3);
		for (int i = 0 ; i < 3 ; ++i) {
			IntConstMultShiftAdd_BASE& currentInput = *(inputs_info[i]);
			int cur_left_bound = max(second_min, opsize_shifted_word[i]);
			int cur_right_bound = max(second_min, known_zeros[i]);
			int size_head_padd_co = wordsize - cur_left_bound;
			int useful_bits_co = cur_left_bound - cur_right_bound;
			int size_trail_padd_co = cur_right_bound - second_min;

			string trail_zeros = ""; 
			bool concat_head = true;
			bool concat_tail = true;
			if (size_trail_padd_co > 0) {
				trail_zeros = zg(size_trail_padd_co);	
			} else {
				concat_tail = false;
			}

			ostringstream sign_ext_co;
			if (size_head_padd_co > 0) {
				sign_ext_co << "( " << size_head_padd_co- 1 << " downto 0 => " <<
					currentInput.outputSignalName<< of(currentInput.wordsize - 1) <<
					")";
			} else { 
				concat_head = false ;
			} 

			string op_range_select;
			if (useful_bits_co > 0) {
				int start_select_idx = cur_right_bound - t->input_shifts[i];
				op_range_select = currentInput.outputSignalName + 
					range(start_select_idx + useful_bits_co - 1,
						   start_select_idx);	
			} else {
				concat_head = false;
			}

			string concat_head_str = (concat_head) ? " & " : "";
			string concat_tail_str = (concat_tail) ? " & " : "";
			string sig_name = currentInput.getTemporaryName();

			base_op->vhdl << base_op->declare(.0, sig_name, adder_word_size) <<
				" <= " << 
				(
				 	sign_ext_co.str() + 
				 	concat_head_str + 
					op_range_select + 
					concat_tail_str + 
					trail_zeros) <<  ";" << endl;

			op_tmp_signal_name[i] = sig_name;
		}

		GenericAddSub* add3 = new GenericAddSub(
				base_op, 
				target, 
				adder_word_size, 
				GenericAddSub::TERNARY
			);

		base_op->addSubComponent(add3);
		for (int i = 0 ; i < 3 ; ++i) {
			base_op->inPortMap(
					add3,
					add3->getInputName(i),
					op_tmp_signal_name[i]
				);
		}

		base_op->outPortMap(add3, add3->getOutputName(), msb_signame);
		base_op->vhdl << base_op->instance(add3, "ternAdd_"+outputSignalName);
	} else { //We don't need a ternary adder as the rightmost input is aligned with
			 //	zeros
		int min_idx = min_known_zeros_it - known_zeros.begin();
		vector<int> leftmost_op_idx;
		for (int i = 0 ; i < 3 ; ++i) {
			if (i != min_idx)
				leftmost_op_idx.push_back(i);
		}
		
		int shift_a = t->input_shifts[leftmost_op_idx[0]];
		int shift_b = t->input_shifts[leftmost_op_idx[1]];
		int  trunc_a = truncations[leftmost_op_idx[0]];
		int  trunc_b = truncations[leftmost_op_idx[1]];
		auto info_a = inputs_info[leftmost_op_idx[0]];
		auto info_b = inputs_info[leftmost_op_idx[1]];
		string result_name = "bin_adder_res";

		binary_adder(info_a, info_b, shift_a, shift_b, trunc_a, trunc_b, result_name);
		//Eliminating useless zeros
		base_op->vhdl << base_op->declare(.0, msb_signame, adder_word_size) << 
			" <= " << result_name << range(second_min, wordsize) << ";" << endl;
	}

	string final_concat = msb_signame;
	if (second_min > opsize_shifted_word[trail_op_idx] ) {
		int nb_zeros_pad = second_min - opsize_shifted_word[trail_op_idx];
		final_concat += " & " + zg(nb_zeros_pad);
	}

	if (second_min-*(min_known_zeros_it) > 0) {
		int nb_copy = second_min - *(min_known_zeros_it);
		final_concat += " & " + inputs_info[trail_op_idx]->outputSignalName + 
			range(truncations[trail_op_idx] + nb_copy - 1, truncations[trail_op_idx]);
	}

	if (*min_known_zeros_it > 0) {
		final_concat += " & " + zg(*min_known_zeros_it);
	}
	
	base_op->vhdl << declare(outputSignalName, wordsize) << " <= " +  final_concat << ";" << endl;
    return "";
}

void IntConstMultShiftAdd_BASE::handle_sub(
		size_t nb_inputs, 
		int  flag,
		map<adder_graph_base_node_t*, IntConstMultShiftAdd_BASE*> & InfoMap
	)
{
	string msb_signame = "msb_" + outputSignalName;
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);
    vector<int> inputOrder;
    getInputOrder(t->input_is_negative, inputOrder);

	vector<int> opsize_word(nb_inputs);
	vector<int> opsize_shifted_word(nb_inputs);
	vector<int> known_zeros(nb_inputs);
	vector<IntConstMultShiftAdd_BASE*> inputs_info(nb_inputs);

	for (size_t i = 0 ; i < nb_inputs ; ++i) {
		inputs_info[i] = InfoMap[t->inputs[i]];
		IntConstMultShiftAdd_BASE& currentInput = *(inputs_info[i]);
		opsize_word[i] = currentInput.wordsize;
		opsize_shifted_word[i] = currentInput.wordsize + t->input_shifts[i];
		known_zeros[i] = truncations[i] + t->input_shifts[i];
	}

	int min_known_zeros = *(min_element(known_zeros.begin(), known_zeros.end()));
	int  adder_word_size = wordsize - min_known_zeros;
	
	vector<string> operands(nb_inputs);
	for (size_t i = 0 ; i < nb_inputs ; ++i) {
		IntConstMultShiftAdd_BASE& currentInput = *(inputs_info[i]);
		int useful_bits = opsize_word[i] - truncations[i];
		int sign_ext_left = wordsize - opsize_shifted_word[i];
		int pad_zeros_right = adder_word_size - (useful_bits + sign_ext_left);
		
		ostringstream sign_ext;
		if (sign_ext_left > 0) {
			sign_ext << "(" << sign_ext_left - 1 << " downto 0 => " <<
			currentInput.outputSignalName << of(currentInput.wordsize - 1) << ") & ";
		}
		string right_pad{""};
		if (pad_zeros_right > 0) {
			right_pad = " & " + zg(pad_zeros_right);
		}
		string select = inputs_info[i]->outputSignalName + 
			range(inputs_info[i]->wordsize - 1, truncations[i]);

		string signal_name = inputs_info[i]->getTemporaryName();
		
		base_op->vhdl << "\t" << base_op->declare(0., signal_name, adder_word_size) <<
			" <= " << (sign_ext.str() + select + right_pad) << ";" << endl;
		operands[i] = signal_name;
	}

    GenericAddSub* add = new GenericAddSub(
			base_op,
			target,
			wordsize,
			flag
		);
    base_op->addSubComponent(add);
	for (size_t i = 0 ; i < nb_inputs ; ++i) {
		base_op->inPortMap(add,add->getInputName(i), operands[inputOrder[i]]);
	}
	base_op->outPortMap(add, add->getOutputName(), msb_signame);

    base_op->vhdl << base_op->instance(add,"add2_"+outputSignalName);
	string right_padd = 
		(wordsize - adder_word_size > 0) ? " & " + zg(wordsize - adder_word_size) : "";
	
	base_op->vhdl << "\t" << declare(outputSignalName, wordsize) << " <= " << 
		(msb_signame + right_padd) << ";" << endl;
}

string IntConstMultShiftAdd_SUB2_1N::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
	handle_sub(2, GenericAddSub::SUB_RIGHT, InfoMap);
	    return "";
}

string IntConstMultShiftAdd_SUB3_1N::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
	handle_sub(3, GenericAddSub::TERNARY|GenericAddSub::SUB_RIGHT, InfoMap);
    return "";
}

string IntConstMultShiftAdd_SUB3_2N::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
	handle_sub(3, GenericAddSub::TERNARY|GenericAddSub::SUB_MID, InfoMap);
    return "";
}

string IntConstMultShiftAdd_DECODER::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    map<uint,uint > decoder_content;
    if( node->nodeType == NODETYPE_ADDSUB3_2STATE ){
        int i=0;
        for(map<short,vector<int> >::iterator iter=((IntConstMultShiftAdd_ADDSUB3_2STATE*)node)->adder_states.begin();
            iter!=((IntConstMultShiftAdd_ADDSUB3_2STATE*)node)->adder_states.end();++iter)
        {
            for(vector<int>::iterator iter2=(*iter).second.begin();iter2!=(*iter).second.end();++iter2){
                decoder_content[ (*iter2) ] = i;
            }
            i++;
        }
    }else if( node->nodeType == NODETYPE_MUX ){
        IntConstMultShiftAdd_MUX* t = ((IntConstMultShiftAdd_MUX*)node);
        int i=0;
        for( vector<IntConstMultShiftAdd_muxInput>::iterator iter=t->inputs.begin();iter!=t->inputs.end();++iter ){
            IntConstMultShiftAdd_muxInput inp = *iter;

            for(vector<int>::iterator iter2=inp.configurations.begin();iter2!=inp.configurations.end();++iter2){
                decoder_content[ (*iter2) ] = i;
            }
            i++;
        }
        i=0;
        int c = t->inputs.size()-1;
        while(c>0){
            c = c>>1;
            i++;
        }
        decoder_size = i;
        //cerr << "Mux Decoder size " << i << endl;
    }
    else{
        for(map<short,vector<int> >::iterator iter=((IntConstMultShiftAdd_BASE_CONF*)node)->adder_states.begin();
            iter!=((IntConstMultShiftAdd_BASE_CONF*)node)->adder_states.end();++iter)
        {
            for(vector<int>::iterator iter2=(*iter).second.begin();iter2!=(*iter).second.end();++iter2){
                if(decoder_size >= 3 && (*iter).first==3){
                    decoder_size = 4;
                    decoder_content[ (*iter2) ] = 0x8;
                }
                else{
                    decoder_content[ (*iter2) ] = (*iter).first;
                }
            }
        }
    }

    stringstream outname;
    outname << node->outputSignalName << "_decode";
    outputSignalName = outname.str();

    string dec_name = outputSignalName+"r";
    GenericLut* dec = new GenericLut(base_op, node->target,dec_name,decoder_content,configurationSignalWordsize,decoder_size);
    base_op->addSubComponent(dec);
    base_op->inPortMap(dec,"x_in_this_does_not_exist!!","config_no");
    base_op->outPortMap(dec,"x_out",outputSignalName);
    base_op->vhdl << base_op->instance(dec,outputSignalName+"r_i") << endl;

    return "";
}

string IntConstMultShiftAdd_ADDSUB2_CONF::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)(base_node);

    GenericAddSub* addsub = new GenericAddSub(base_op, target,wordsize);
    base_op->addSubComponent(addsub);
    for(int i=0;i<2;i++){
        IntConstMultShiftAdd_BASE* t_in=InfoMap[t->inputs[i]];
        string t_name = t_in->getTemporaryName();
        base_op->declare(t_name,wordsize);
        base_op->vhdl << t_name << " <= std_logic_vector(" << getShiftAndResizeString( t_in,wordsize,t->input_shifts[i]) << ");" << endl;

        base_op->inPortMap(addsub,addsub->getInputName(i),t_name);
    }
    base_op->inPortMap(addsub,addsub->getInputName(1,true),decoder->outputSignalName + "(1)");
    base_op->inPortMap(addsub,addsub->getInputName(0,true),decoder->outputSignalName + "(0)");
    base_op->outPortMap(addsub,addsub->getOutputName(),outputSignalName);
    base_op->vhdl << base_op->instance(addsub,outputSignalName+"_addsub") << endl;
    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t ) << endl;
    return "";
}

string IntConstMultShiftAdd_ADDSUB3_2STATE::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)(base_node);

    short adderStates[2] = {0};
    map<short,vector<int> >::iterator stateIter = adder_states.begin();
    adderStates[0] = ( *stateIter ).first;
    ++stateIter;
    adderStates[1] = ( *stateIter ).first;

    if( IntConstMultShiftAdd_BASE::target_ID == "Virtex5" || IntConstMultShiftAdd_BASE::target_ID == "Virtex6" )
    {
        Xilinx_TernaryAdd_2State* add3 = new Xilinx_TernaryAdd_2State(base_op, target,wordsize,adderStates[0],adderStates[1]);
        base_op->addSubComponent(add3);
        base_op->vhdl << base_op->declare( "add3_" + outputSignalName + "_x",wordsize   ) << " <= "
             << getShiftAndResizeString( InfoMap[ t->inputs[0] ] , wordsize, t->input_shifts[0],false) << ";";
        base_op->vhdl << base_op->declare( "add3_" + outputSignalName + "_y",wordsize   ) << " <= "
             << getShiftAndResizeString( InfoMap[ t->inputs[1] ] , wordsize, t->input_shifts[1],false) << ";";
        base_op->vhdl << base_op->declare( "add3_" + outputSignalName + "_z",wordsize   ) << " <= "
             << getShiftAndResizeString( InfoMap[ t->inputs[2] ] , wordsize, t->input_shifts[2],false) << ";";

        base_op->inPortMap(add3,"x_i","add3_" + outputSignalName + "_x");
        base_op->inPortMap(add3,"y_i","add3_" + outputSignalName + "_y");
        base_op->inPortMap(add3,"z_i","add3_" + outputSignalName + "_z");
        base_op->inPortMap(add3,"sel_i",decoder->outputSignalName + "(0)");
        base_op->outPortMap(add3,"sum_o",outputSignalName);
        base_op->vhdl << base_op->instance(add3,"ternAdd_"+outputSignalName);
    }
    else
    {
        base_op->vhdl << base_op->declare( "add3_" + outputSignalName + "_x",wordsize   ) << " <= "
             << getShiftAndResizeString( InfoMap[ t->inputs[0] ] , wordsize, t->input_shifts[0],false) << ";";
        base_op->vhdl << base_op->declare( "add3_" + outputSignalName + "_y",wordsize   ) << " <= "
             << getShiftAndResizeString( InfoMap[ t->inputs[1] ] , wordsize, t->input_shifts[1],false) << ";";
        base_op->vhdl << base_op->declare( "add3_" + outputSignalName + "_z",wordsize   ) << " <= "
             << getShiftAndResizeString( InfoMap[ t->inputs[2] ] , wordsize, t->input_shifts[2],false) << ";";
        ///TBD
        cerr << "ADDSUB3_2STATE: NODETYPE NOT SUPPORTED FOR NON VIRTEX TARGETS YET" << endl;
    }
    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t );
    return "";
}

string IntConstMultShiftAdd_ADDSUB3_CONF::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)(base_node);

    GenericAddSub* addsub = new GenericAddSub(base_op, target,wordsize);
    GenericAddSub* addsub2 = new GenericAddSub(base_op, target,wordsize);
    base_op->addSubComponent(addsub);
    base_op->addSubComponent(addsub2);
    for(int i=0;i<3;i++){
        IntConstMultShiftAdd_BASE* t_in=InfoMap[t->inputs[i]];
        string t_name = t_in->getTemporaryName();
        base_op->declare(t_name,wordsize);
        base_op->vhdl << t_name << " <= std_logic_vector(" << getShiftAndResizeString( t_in,wordsize,t->input_shifts[i]) << ");" << endl;
        if(i==0)
            base_op->inPortMap(addsub,"x_i",t_name);
        else if(i==1)
            base_op->inPortMap(addsub,"y_i",t_name);
        else
            base_op->inPortMap(addsub2,"y_i",t_name);
    }
    base_op->outPortMap     (addsub,"sum_o",outputSignalName+"_pre");
    base_op->inPortMap      (addsub,"neg_y_i",decoder->outputSignalName + "(1)");
    base_op->inPortMap      (addsub,"neg_x_i",decoder->outputSignalName + "(0)");
    base_op->inPortMap      (addsub2,"x_i",outputSignalName+"_pre");
    base_op->inPortMap      (addsub2,"neg_y_i",decoder->outputSignalName + "(2)");
    if( decoder->decoder_size == 4 ) base_op->inPortMap   (addsub2,"neg_x_i",decoder->outputSignalName + "(3)");
    else    base_op->inPortMapCst   (addsub2,"neg_x_i","'0'");
    base_op->outPortMap(addsub2,"sum_o",outputSignalName);
    base_op->vhdl << base_op->instance(addsub,outputSignalName+"_addsub_pre") << endl;
    base_op->vhdl << base_op->instance(addsub2,outputSignalName+"_addsub") << endl;

    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t ) << endl;
    return "";
}

string IntConstMultShiftAdd_AND::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    mux_node_t* t = (mux_node_t*)(base_node);
    short configurationIndex=-1;
    for(unsigned int i=0;i<t->inputs.size();i++)
    {
        if(t->inputs[i]!=NULL)
        {
            configurationIndex = i;
            break;
        }
    }
    base_op->vhdl << "\t" << base_op->declare(outputSignalName,wordsize) << " <= ";
    base_op->vhdl << getShiftAndResizeString(InfoMap[t->inputs[configurationIndex]]
            ,wordsize
            ,t->input_shifts[configurationIndex],false);
    base_op->vhdl << " when config_no=" << getBinary(configurationIndex,IntConstMultShiftAdd_BASE::configurationSignalWordsize);
    base_op->vhdl << " else (others=>'0')";
    base_op->vhdl << ";" << endl;
    return "";
}

string IntConstMultShiftAdd_MUX::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    GenericMux* new_mux = new GenericMux(base_op, target,wordsize,inputs.size());
    base_op->addSubComponent(new_mux);
    int i=0;
    for( vector<IntConstMultShiftAdd_muxInput>::iterator iter=inputs.begin();iter!=inputs.end();++iter ){
        if( iter->node == NULL )
        {
            if( iter->shift == -1 )
                base_op->inPortMapCst(new_mux,new_mux->getInputName(i),"(others=>'-')");
            else
                base_op->inPortMapCst(new_mux,new_mux->getInputName(i),"(others=>'0')");
        }
        else{
            IntConstMultShiftAdd_BASE* t_in=InfoMap[ iter->node ];
            string t_name = t_in->getTemporaryName();
            base_op->vhdl << base_op->declare(t_name,wordsize) << " <= " <<  getShiftAndResizeString(t_in
                                                    ,wordsize
                                                    ,iter->shift,false) << ";";
            base_op->inPortMap(new_mux,new_mux->getInputName(i),t_name);
        }
        i++;
    }
    base_op->inPortMap(new_mux,new_mux->getSelectName(),decoder->outputSignalName);
    base_op->outPortMap(new_mux,new_mux->getOutputName(),outputSignalName);
    base_op->vhdl << base_op->instance(new_mux,outputSignalName+"_mux");

/*
    map<int,bool> negateConfigMap;

    base_op->vhdl << "\tWITH config_no SELECT" << endl;
    int outputSize=0;
    if(negateConfigMap.size()>0) outputSize = wordsize+1;
    else outputSize = wordsize;

    base_op->vhdl << "\t\t" << base_op->declare(outputSignalName,outputSize) << " <= \n";

    int muxStatesWritten=0;
    for(int inp=0;inp<(int)inputs.size();inp++)
    {
        IntConstMultShiftAdd_muxInput cur_inp = inputs[inp];
        int configsInMux = cur_inp.configurations.size();
        if( inp == ((int)inputs.size()) - 1 )
            configsInMux=1;
        for(int i=0;i<configsInMux;i++)
        {
            muxStatesWritten++;
            base_op->vhdl << "\t\t\t";
            if( cur_inp.node == NULL )
            {
                if(cur_inp.shift == -1)
                    base_op->vhdl << "(others=>'-'";
                else
                    base_op->vhdl << "(others=>'0'";

            }
            else if( negateConfigMap.find( cur_inp.configurations[i] )!=negateConfigMap.end() )
            {
                base_op->vhdl << "'1' & (";
                base_op->vhdl << getShiftAndResizeString(InfoMap[cur_inp.node]
                        ,wordsize
                        ,cur_inp.shift,false);
                base_op->vhdl << " xor \"";
                for( int x=0;x<wordsize;x++ )  base_op->vhdl << "1";
                base_op->vhdl << "\"";
            }
            else
            {
                if(negateConfigMap.size()>0) base_op->vhdl << "'0' & (";
                else base_op->vhdl << "(";

                base_op->vhdl << getShiftAndResizeString(InfoMap[cur_inp.node]
                        ,wordsize
                        ,cur_inp.shift,false);
            }
            base_op->vhdl << ") WHEN ";
            if( inp < ((int)inputs.size()) - 1 )
            {
                base_op->vhdl << getBinary(cur_inp.configurations[i],IntConstMultShiftAdd_BASE::configurationSignalWordsize) << "," << endl;
            }
            else
            {
                base_op->vhdl << "OTHERS;" << endl;
            }
        }
    }
*/
    return "";
}

string IntConstMultShiftAdd_BASE::getShiftAndResizeString(string signalName, int outputWordsize, int inputShift,bool signedConversion)
{
    stringstream tmp;
    if(!signedConversion) tmp << "std_logic_vector(";
    if(inputShift > 0)
    {
        tmp << "unsigned(shift_left(resize(signed(" << signalName << ")," << outputWordsize << ")," << inputShift << "))";
    }
    else
    {
        tmp << "unsigned(resize(signed(" << signalName << ")," << outputWordsize << "))";
    }
    if(!signedConversion) tmp << ")";
    return tmp.str();
}

string IntConstMultShiftAdd_BASE::getShiftAndResizeString(IntConstMultShiftAdd_BASE *input_node, int outputWordsize, int inputShift,bool signedConversion)
{
    int neg_shift=0;
    if( is_a<adder_subtractor_node_t>(*base_node) ){
        adder_subtractor_node_t* t = (adder_subtractor_node_t*)base_node;
        for(uint i=0;i<t->input_shifts.size();i++){
            if(t->input_shifts[i]<neg_shift )
                neg_shift = t->input_shifts[i];
        }
    }
    else if( is_a<conf_adder_subtractor_node_t>(*base_node) ){
        conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)base_node;
        for(uint i=0;i<t->input_shifts.size();i++){
            if(t->input_shifts[i]<neg_shift )
                neg_shift = t->input_shifts[i];
        }
    }

    stringstream tmp;
    if(!signedConversion) tmp << "std_logic_vector(";
    if(input_node->wordsize+inputShift > outputWordsize )
    {
        //log(2) << "WARNING: possible shift overflow @" << base_node->outputSignalName << endl;

        if( (inputShift-neg_shift) > 0)
        {
            tmp << "unsigned(shift_left(resize(signed(" << input_node->outputSignalName <<")," << outputWordsize << ")," << (inputShift-neg_shift) << "))" ;
        }
        else
        {
            tmp << "resize(unsigned(" << input_node->outputSignalName << ")," << outputWordsize << ")";
        }

    }
    else if( input_node->wordsize > outputWordsize )
    {
        if((inputShift-neg_shift) > 0)
        {
            tmp << "resize(unsigned(shift_left(signed(" << input_node->outputSignalName <<")," << (inputShift-neg_shift) << "))," << outputWordsize << ")" ;
        }
        else
        {
            tmp << "resize(unsigned(" << input_node->outputSignalName << ")," << outputWordsize << ")";
        }
    }
    else if( input_node->wordsize < outputWordsize )
    {
        if((inputShift-neg_shift) > 0)
        {
            tmp << "unsigned(shift_left(resize(signed(" << input_node->outputSignalName << ")," << outputWordsize << ")," << (inputShift-neg_shift) << "))";
        }
        else
        {
            tmp << "unsigned(resize(signed(" << input_node->outputSignalName << ")," << outputWordsize << "))";
        }
    }
    else
    {
        if((inputShift-neg_shift) > 0)
        {
            tmp << "unsigned(shift_left(signed(" << input_node->outputSignalName << ")," << (inputShift-neg_shift) << "))";
        }
        else
        {
            tmp << "unsigned(" << input_node->outputSignalName << ")";
        }
    }


    if(!signedConversion) tmp << ")";
    return tmp.str();
}


string IntConstMultShiftAdd_BASE::getNegativeShiftString(string signalName,int outputWordsize, adder_graph_base_node_t* base_node)
{
    int neg_shift=0;
    if( is_a<adder_subtractor_node_t>(*base_node) ){
        adder_subtractor_node_t* t = (adder_subtractor_node_t*)base_node;
        for(uint i=0;i<t->input_shifts.size();i++){
            if(t->input_shifts[i]<neg_shift )
                neg_shift = t->input_shifts[i];
        }
    }
    else if( is_a<conf_adder_subtractor_node_t>(*base_node) ){
        conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)base_node;
        for(uint i=0;i<t->input_shifts.size();i++){
            if(t->input_shifts[i]<neg_shift )
                neg_shift = t->input_shifts[i];
        }
    }

    if(neg_shift < 0)
    {
        stringstream tmp;
        tmp << signalName << "_ns";
        this->wordsize -= abs(neg_shift);
        base_op->declare( tmp.str(),this->wordsize );
        tmp << " <= std_logic_vector( resize(shift_right(signed(" << signalName << "),"<< abs(neg_shift) << "),"<< this->wordsize <<") );"<<endl;
        outputSignalName += "_ns";
        return "\t" + tmp.str();
    }
    return "";
}

string IntConstMultShiftAdd_BASE::getNegativeResizeString(string signalName,int outputWordsize)
{
    if(outputWordsize != this->wordsize)
    {
        stringstream tmp;
        tmp << signalName << "_nr";
        base_op->declare( tmp.str(),this->wordsize );
        tmp << " <= std_logic_vector( resize(signed(" << signalName << "),"<< this->wordsize <<") );"<<endl;
        outputSignalName += "_nr";
        return "\t" + tmp.str();
    }
    return "";
}

void IntConstMultShiftAdd_BASE::getInputOrder(vector<bool> &inputIsNegative, vector<int> &inputOrder)
{
    for(int i=0;i<(int)inputIsNegative.size();i++)
    {
        if( inputIsNegative[i] )
        {
            inputOrder.push_back(i);
        }
        else
        {
            inputOrder.insert(inputOrder.begin(),i);
        }
    }
}

string IntConstMultShiftAdd_BASE::getBinary(int value, int Wordsize)
{
    string tmp;
    while(value>0)
    {
        if(value%2) tmp.insert(0,1,'1');
        else tmp.insert(0,1,'0');
        value = (value>>1);
    }
    while((int)tmp.length() < Wordsize) tmp.insert(0,1,'0');

    tmp.insert(0,1,'\"');
    tmp += '\"';
    return tmp;
}

string IntConstMultShiftAdd_BASE::declare(string signalName, int wordsize)
{
    return base_op->declare(signalName,wordsize);
}

string IntConstMultShiftAdd_BASE::getTemporaryName()
{
    stringstream t_name;
    t_name << outputSignalName << "_t" << ++outputSignalUsageCount;
    return t_name.str();
}
}


#endif // HAVE_PAGSUITE
