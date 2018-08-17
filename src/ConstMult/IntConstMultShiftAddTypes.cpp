/*  IntConstMultShiftAdd_TYPES.cpp
 *  Version:
 *  Datum: 20.11.2014
 */
#ifdef HAVE_PAGSUITE
#include "IntConstMultShiftAddTypes.hpp"

//#include "FloPoCo.hpp"
#include "PrimitiveComponents/GenericAddSub.hpp"
#include "PrimitiveComponents/GenericLut.hpp"
#include "PrimitiveComponents/GenericMux.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_TernaryAdd_2State.hpp"
//#include "PrimitiveComponents/Altera/Altera_TernaryAdd.hpp"

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

string IntConstMultShiftAdd_ADD2::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);

    GenericAddSub* addsub = new GenericAddSub(base_op,target,wordsize);
    //cerr << "-->" << addsub->getName() << endl;
    base_op->addSubComponent(addsub);
    for(int i=0;i<2;i++){
        IntConstMultShiftAdd_BASE* t_in=InfoMap[t->inputs[i]];
        string t_name = t_in->getTemporaryName();
        base_op->declare(t_name,wordsize);
        base_op->vhdl << t_name << " <= std_logic_vector(" << getShiftAndResizeString( t_in,wordsize,t->input_shifts[i]) << ");" << endl;

        base_op->inPortMap(addsub,addsub->getInputName(i),t_name);
    }
//    base_op->declare( outputSignalName,wordsize );
    base_op->outPortMap(addsub,"sum_o",outputSignalName);
    base_op->vhdl << base_op->instance(addsub,outputSignalName+"_add2") << endl;
    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t ) << endl;

    return "";//base_op->vhdl.str();
}

string IntConstMultShiftAdd_ADD3::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_x",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[0] ] , wordsize, t->input_shifts[0],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_y",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[1] ] , wordsize, t->input_shifts[1],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_z",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[2] ] , wordsize, t->input_shifts[2],false) << ";" << endl;

        GenericAddSub* add3 = new GenericAddSub(base_op, target,wordsize,GenericAddSub::TERNARY);
        base_op->addSubComponent(add3);
        base_op->inPortMap(add3,add3->getInputName(0),"add3_" + outputSignalName + "_x");
        base_op->inPortMap(add3,add3->getInputName(1),"add3_" + outputSignalName + "_y");
        base_op->inPortMap(add3,add3->getInputName(2),"add3_" + outputSignalName + "_z");
        base_op->outPortMap(add3,add3->getOutputName(),outputSignalName);
        base_op->vhdl << base_op->instance(add3,"ternAdd_"+outputSignalName);


    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t );
    return "";
}

string IntConstMultShiftAdd_SUB2_1N::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);
    vector<int> inputOrder;
    getInputOrder(t->input_is_negative,inputOrder);

    base_op->vhdl << "\t" << base_op->declare( "add2_" + outputSignalName + "_x",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[0] ] , wordsize, t->input_shifts[0],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add2_" + outputSignalName + "_y",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[1] ] , wordsize, t->input_shifts[1],false) << ";" << endl;

    GenericAddSub* add = new GenericAddSub(base_op, target,wordsize,GenericAddSub::SUB_RIGHT);
    base_op->addSubComponent(add);
    base_op->inPortMap(add,add->getInputName(0),"add2_" + outputSignalName + "_x");
    base_op->inPortMap(add,add->getInputName(1),"add2_" + outputSignalName + "_y");
    base_op->outPortMap(add,add->getOutputName(),outputSignalName);
    base_op->vhdl << base_op->instance(add,"add2_"+outputSignalName);

    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t );
    return "";
}

string IntConstMultShiftAdd_SUB3_1N::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);
    vector<int> inputOrder;
    getInputOrder(t->input_is_negative,inputOrder);

    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_x",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[inputOrder[0]] ] , wordsize, t->input_shifts[inputOrder[0]],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_y",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[inputOrder[1]] ] , wordsize, t->input_shifts[inputOrder[1]],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_z",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[inputOrder[2]] ] , wordsize, t->input_shifts[inputOrder[2]],false) << ";" << endl;

    GenericAddSub* add3 = new GenericAddSub(base_op, target,wordsize,GenericAddSub::TERNARY|GenericAddSub::SUB_RIGHT);
    base_op->addSubComponent(add3);
    base_op->inPortMap(add3,add3->getInputName(0),"add3_" + outputSignalName + "_x");
    base_op->inPortMap(add3,add3->getInputName(1),"add3_" + outputSignalName + "_y");
    base_op->inPortMap(add3,add3->getInputName(2),"add3_" + outputSignalName + "_z");
    base_op->outPortMap(add3,add3->getOutputName(),outputSignalName);
    base_op->vhdl << base_op->instance(add3,"ternAdd_"+outputSignalName);

    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t );
    return "";
}

string IntConstMultShiftAdd_SUB3_2N::get_realisation(map<adder_graph_base_node_t *, IntConstMultShiftAdd_BASE *> &InfoMap)
{
    adder_subtractor_node_t* t = (adder_subtractor_node_t*)(base_node);
    vector<int> inputOrder;
    getInputOrder(t->input_is_negative,inputOrder);

    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_x",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[inputOrder[0]] ] , wordsize, t->input_shifts[inputOrder[0]],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_y",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[inputOrder[1]] ] , wordsize, t->input_shifts[inputOrder[1]],false) << ";" << endl;
    base_op->vhdl << "\t" << base_op->declare( "add3_" + outputSignalName + "_z",wordsize   ) << " <= "
         << getShiftAndResizeString( InfoMap[ t->inputs[inputOrder[2]] ] , wordsize, t->input_shifts[inputOrder[2]],false) << ";" << endl;

    GenericAddSub* add3 = new GenericAddSub(base_op, target,wordsize,GenericAddSub::TERNARY|GenericAddSub::SUB_RIGHT|GenericAddSub::SUB_MID);
    base_op->addSubComponent(add3);
    base_op->inPortMap(add3,add3->getInputName(0),"add3_" + outputSignalName + "_x");
    base_op->inPortMap(add3,add3->getInputName(1),"add3_" + outputSignalName + "_y");
    base_op->inPortMap(add3,add3->getInputName(2),"add3_" + outputSignalName + "_z");
    base_op->outPortMap(add3,add3->getOutputName(),outputSignalName);
    base_op->vhdl << base_op->instance(add3,"ternAdd_"+outputSignalName);

    base_op->vhdl << getNegativeShiftString( outputSignalName,wordsize,t );
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
