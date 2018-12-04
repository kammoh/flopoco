// general c++ library for manipulating streams
#include "IntConstMultShiftAdd.hpp"

#ifdef HAVE_PAGSUITE
#include <iostream>
#include <sstream>
#include <string>

#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include <map>
#include <vector>
#include <set>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "PrimitiveComponents/Primitive.hpp"
//#include "rpag/rpag.h"

using namespace std;


namespace flopoco {

IntConstMultShiftAdd::IntConstMultShiftAdd(Operator* parentOp, Target* target, int wIn_, string pipelined_realization_str, bool pipelined_, bool syncInOut_, int syncEveryN_, bool syncMux_)
    : Operator(parentOp, target),
      wIn(wIn_),
      syncInOut(syncInOut_),
      pipelined(pipelined_),
      syncEveryN(syncEveryN_)
{
    syncMux=syncMux_;

    ostringstream name;
    name << "IntConstMultShiftAdd_" << wIn;
    setName(name.str());

    if(pipelined_realization_str.empty()) return; //in case the realization string is not defined, don't further process it.

    string pipelined_realisation,t(pipelined_realization_str);

    if( TryRunRPAG(t,pipelined_realisation) )
    {
        size_t pos=0;
        string tmp=t;
        vector<int64_t> inner;
        while(tmp.size()>0)
        {
            pos=tmp.find_first_of(",; ");

            if(pos != string::npos)
            {
                switch (tmp[pos]) {
                case ',':
                    inner.push_back( atol(tmp.substr(0,pos).c_str()) );
                    break;
                case ' ':
                case ';':
                    inner.push_back( atol(tmp.substr(0,pos).c_str()) );
                    output_coefficients.push_back(inner);
                    inner.clear();
                    break;
                default:
                    break;
                }
                tmp = tmp.substr(pos+1);
            }
            else
            {
                inner.push_back( atol(tmp.c_str()) );
                output_coefficients.push_back(inner);
                tmp = "";
            }
        }

        ProcessIntConstMultShiftAdd(target,pipelined_realisation);
    }
    else
    {
        ProcessIntConstMultShiftAdd(target,t);
    }

    /* for(OperatorPtr sc : getSubComponents())
    {
        cerr << "--> sub component: " << sc->getName() << endl;
    }*/
};

IntConstMultShiftAdd::IntConstMultShiftAdd(Operator* parentOp, Target *target, int wIn_, vector<vector<int64_t> >& coefficients, bool pipelined_, bool syncInOut_, int syncEveryN_, bool syncMux_)
    : Operator(parentOp, target),
      wIn(wIn_),
      syncInOut(syncInOut_),
      pipelined(pipelined_),
      syncEveryN(syncEveryN_)
{
    syncMux=syncMux_;
    output_coefficients = coefficients;

    stringstream coefficients_str;
    for(int i=0;i<(int)coefficients.size();i++)
    {
        if(i>0) coefficients_str << ";";
        for(int j=0;j<(int)coefficients[i].size();j++)
        {
            if(j>0) coefficients_str << ",";
            coefficients_str << coefficients[i][j];
        }
    }

    string pipelined_realisation;
    if( !TryRunRPAG(coefficients_str.str(),pipelined_realisation) )
    {
        throw runtime_error("ERROR: Could not run RPAG");
    }

    ProcessIntConstMultShiftAdd(target,pipelined_realisation);
};

void IntConstMultShiftAdd::ProcessIntConstMultShiftAdd(Target* target, string pipelined_realization_str)
{
    REPORT( INFO, "IntConstMultShiftAdd started with syncoptions:")
            REPORT( INFO, "\tsyncInOut: " << (syncInOut?"enabled":"disabled"))
            REPORT( INFO, "\tsyncMux: " << (syncMux?"enabled":"disabled"))
            REPORT( INFO, "\tsync every " << syncEveryN << " stages" << std::endl )

            if(RPAGused)
    {
        std::stringstream o;
        o <<  "Starting with coefficients ";

        for(int j=0;j<(int)output_coefficients.size();j++)
        {
            if(j>0) o << ",";
            o << "[";
            for(int i=0;i<(int)output_coefficients[j].size();i++){
                if(i>0)o <<",";
                o << output_coefficients[j][i];
            }
            o << "]";
        }
        REPORT(DETAILED,o.str())
    }
    needs_unisim = false;
    emu_conf = 0;

    bool validParse;
    srcFileName="IntConstMultShiftAdd";

    useNumericStd();

//    setCopyrightString(UniKs::getAuthorsString(UniKs::AUTHOR_MKLEINLEIN|UniKs::AUTHOR_MKUMM|UniKs::AUTHOR_KMOELLER));

    pipelined_adder_graph.quiet = true; //disable debug output, except errors
    REPORT( INFO, "parse graph...")
            validParse = pipelined_adder_graph.parse_to_graph(pipelined_realization_str,false);

    if(validParse)
    {
        REPORT( INFO,  "check graph...")
		pipelined_adder_graph.check_and_correct(pipelined_realization_str);
		pipelined_adder_graph.print_graph();
        pipelined_adder_graph.drawdot("pag_input_graph.dot");

        noOfConfigurations = (*pipelined_adder_graph.nodes_list.begin())->output_factor.size();
        noOfInputs = (*pipelined_adder_graph.nodes_list.begin())->output_factor[0].size();
        noOfPipelineStages = 0;
        int configurationSignalWordsize = ceil(log2(noOfConfigurations));
        for (std::list<adder_graph_base_node_t*>::iterator iter = pipelined_adder_graph.nodes_list.begin(); iter != pipelined_adder_graph.nodes_list.end(); ++iter)
            if ((*iter)->stage > noOfPipelineStages) noOfPipelineStages=(*iter)->stage;

        REPORT( INFO, "noOfInputs: " << noOfInputs)
                REPORT( INFO, "noOfConfigurations: " << noOfConfigurations)
                REPORT( INFO, "noOfPipelineStages: " << noOfPipelineStages)

                if(noOfConfigurations>1)
                addInput("config_no",configurationSignalWordsize);

        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE::target_ID = target->getID();
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE::noOfConfigurations = noOfConfigurations;
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE::noOfInputs = noOfInputs;
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE::configurationSignalWordsize = configurationSignalWordsize;

        //IDENTIFY NODE
        REPORT(INFO,"identifiing nodes..")

                map<adder_graph_base_node_t*,IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*> additionalNodeInfoMap;
        map<int,list<adder_graph_base_node_t*> > stageNodesMap;
        for (std::list<adder_graph_base_node_t*>::iterator iter = pipelined_adder_graph.nodes_list.begin();
             iter != pipelined_adder_graph.nodes_list.end();
             ++iter)
        {
            if( (*iter)->stage > noOfPipelineStages ) noOfPipelineStages = (*iter)->stage;

            stageNodesMap[(*iter)->stage].push_back(*iter);

            IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* t = identifyNodeType(*iter);
            t->outputSignalName = generateSignalName(*iter);
            t->target = target;

            if      (  t->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_ADDSUB2_CONF
                       || t->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_ADDSUB3_2STATE
                       || t->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_ADDSUB3_CONF
                       ){
                REPORT(DEBUG,"has decoder")

                        conf_adder_subtractor_node_t* cc = new conf_adder_subtractor_node_t();
                cc->stage = (*iter)->stage-1;
                stageNodesMap[(*iter)->stage-1].push_back( cc );
                ((IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE_CONF*)t)->decoder->outputSignalName = t->outputSignalName + "_decode";
                additionalNodeInfoMap.insert(
                            make_pair<adder_graph_base_node_t*,IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*>(cc,((IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE_CONF*)t)->decoder)
                            );
            }else if(t->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_MUX){
                REPORT(DEBUG,"has decoder")

                        mux_node_t* cc = new mux_node_t();
                cc->stage = (*iter)->stage-1;
                stageNodesMap[(*iter)->stage-1].push_back( cc );
                ((IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_MUX*)t)->decoder->outputSignalName = t->outputSignalName + "_decode";
                additionalNodeInfoMap.insert(
                            make_pair<adder_graph_base_node_t*,IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*>(cc,((IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_MUX*)t)->decoder)
                            );
            }
            t->wordsize  = computeWordSize( *iter,wIn );
            t->base_node = (*iter);
            //additionalNodeInfoMap.insert( std::make_pair<adder_graph_base_node_t*,IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*>(*iter,t) );
            additionalNodeInfoMap.insert( {*iter,t} );
        }
        //IDENTIFY CONNECTIONS
        REPORT(INFO,"identifiing node connections..")
                for (std::list<adder_graph_base_node_t*>::iterator iter = pipelined_adder_graph.nodes_list.begin();
                iter != pipelined_adder_graph.nodes_list.end();
        ++iter)
        {
            identifyOutputConnections(*iter,additionalNodeInfoMap);
        }
        printAdditionalNodeInfo(additionalNodeInfoMap);

        /*
            //IDENTIFY STAGE MERGES
        for( int i=1;i<=noOfPipelineStages;i++ )
        {
            cout << "try stagemerge for stage " << i << " node count "<< stageNodesMap[i].size() << endl;
            bool mergePossible = true;
            int current_id = 0;
            for( list<adder_graph_base_node_t*>::iterator iter = stageNodesMap[i].begin();iter!=stageNodesMap[i].end();++iter )
            {
                IntConstMultShiftAdd_AdditionalNodeInfo operationNodeInfo = additionalNodeInfoMap[(*iter)];
                if( is_a<adder_subtractor_node_t>(*(*iter)) )
                {
                    if( operationNodeInfo.nodeType == IntConstMultShiftAdd_NODETYPE_ADD2 ||
                        operationNodeInfo.nodeType == IntConstMultShiftAdd_NODETYPE_SUB2_1N)
                    {
                        adder_subtractor_node_t* t = (adder_subtractor_node_t*)*iter;
                        int lut_inp_count = 0;
                        for(unsigned int j =0;j<t->inputs.size();j++)
                        {
                            if( is_a<mux_node_t>(*t->inputs[j]) )
                            {
                                vector<IntConstMultShiftAdd_muxInfo>* muxInfoMap = (vector<IntConstMultShiftAdd_muxInfo>*)additionalNodeInfoMap[t->inputs[j]].extendedInfo;
                                lut_inp_count += muxInfoMap->size();
                            }
                            else if(  is_a<register_node_t>(*t->inputs[j]) )
                            {
                                lut_inp_count ++;
                            }
                            else
                            {
                                cout << "stage "<< i << " node " << current_id << " input "<< j <<" break node missfit " << endl;
                                lut_inp_count = 10000;
                                break;
                            }
                        }
                        cout << "stage "<<i << " node " << current_id << " lut_size " << lut_inp_count << endl;
                        if(lut_inp_count > 5)
                        {
                            mergePossible = false;
                            break;
                        }
                    }
                    else
                    {
                        cout << "stage "<< i << " node " << current_id << " break node missfit " << endl;
                        mergePossible = false;
                        break;
                    }
                }
                else if( is_a<register_node_t>(*(*iter)) )
                {

                }
                else
                {
                    cout << "stage "<< i << " node " << current_id << " break node missfit " << endl;
                    mergePossible = false;
                    break;
                }
                current_id++;
            }

            if(mergePossible)
            {
                IF_VERBOSE(1) cout << "Found possible stagemerge between " << i-1 << " and " << i << endl;
            }
        }
        */

        //START BUILDING NODES
        REPORT(INFO,"building nodes..")
                int unpiped_stage_count=0;
        bool isMuxStage = false;
        for(unsigned int currentStage=0;currentStage<=(unsigned int)noOfPipelineStages;currentStage++)
        {
            isMuxStage = false;
            for (std::list<adder_graph_base_node_t*>::iterator operationNode = stageNodesMap[currentStage].begin();
                 operationNode != stageNodesMap[currentStage].end();
                 ++operationNode)
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* op_node = additionalNodeInfoMap[(*operationNode)];
                REPORT( INFO, op_node->outputSignalName << " as " << op_node->get_name())

                if(op_node->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_INPUT)
                {
                    input_node_t* t = (input_node_t*)op_node->base_node;

                    stringstream inputSignalName;
                    int id=0;
                    while( t->output_factor[0][id]==0 && id < noOfInputs )
                    {
                        id++;
                    }

                    inputSignalName << "x_in" << id;
                    addInput( inputSignalName.str(),wIn );
                    vhdl << "\t" << declare(op_node->outputSignalName,wIn) << " <= " << inputSignalName.str() << ";" << endl;
                    input_signals.push_back(inputSignalName.str());
                }
                else
                vhdl << op_node->get_realisation( additionalNodeInfoMap );

                if(op_node->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_MUX ||
                        op_node->nodeType == IntConstMultShiftAdd_TYPES::NODETYPE_AND)
                    isMuxStage = true;

                for(list<pair<string,int> >::iterator declare_it=op_node->declare_list.begin();declare_it!=op_node->declare_list.end();++declare_it  )
                    declare( (*declare_it).first,(*declare_it).second );

                for(list<Operator* >::iterator declare_it=op_node->opList.begin();declare_it!=op_node->opList.end();++declare_it  )
                    addSubComponent(  *declare_it);
            }


            if( !(syncEveryN>1 && !syncMux && isMuxStage) )
                unpiped_stage_count++;

            bool doPipe=false;
            if( pipelined && (!isMuxStage || (isMuxStage && syncMux) ) )
            {
                if( (currentStage == 0 || (int)currentStage == noOfPipelineStages) )
                {
                    //IN OUT STAGE
                    if( syncInOut )
                    {
                        doPipe = true;
                    }
                }
                else if( (unpiped_stage_count%syncEveryN)==0 )
                {
                    doPipe = true;
                }
            }
            else if( !pipelined )
            {
                if( syncInOut && (currentStage == 0 || (int)currentStage == noOfPipelineStages) )
                {
                    doPipe = true;
                }
                else if( (unpiped_stage_count%syncEveryN)==0 )
                {
                    doPipe = true;
                }
            }

            if(doPipe)
            {
                REPORT( DETAILED, "----pipeline----")
//                        nextCycle(); //!!!
                unpiped_stage_count = 0;
            }

        }

        if( !RPAGused )
        {
            output_signal_info sig_info;
            short realizedOutputNodes = 0;
            for (std::list<adder_graph_base_node_t*>::iterator operationNode = stageNodesMap[noOfPipelineStages].begin();
                 operationNode != stageNodesMap[noOfPipelineStages].end();
                 ++operationNode)
            {
                if (is_a<output_node_t>(*(*operationNode))){
                    IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* op_node = additionalNodeInfoMap[(*operationNode)];
                    stringstream outputSignalName;
                    outputSignalName << "x_out" << realizedOutputNodes;
                    realizedOutputNodes++;
                    for(int j=0; j < noOfConfigurations; j++) {
                        outputSignalName << "_c";
                        for(int i=0; i < noOfInputs; i++)
                        {
                            if(i>0) outputSignalName << "v" ;
                            outputSignalName << (((*operationNode)->output_factor[j][i] < 0) ? "m" : "");
                            outputSignalName << abs((*operationNode)->output_factor[j][i]);
                        }
                    }
                    addOutput(outputSignalName.str(), op_node->wordsize);

                    sig_info.output_factors = (*operationNode)->output_factor;
                    sig_info.signal_name = outputSignalName.str();
                    sig_info.wordsize = op_node->wordsize;
                    output_signals.push_back( sig_info );
                    vhdl << "\t" << outputSignalName.str() << " <= " << op_node->outputSignalName << ";" << endl;
                }
            }
        }
        else
        {
            output_signal_info sig_info;
            REPORT( INFO, "Matching outputs... ")
                    short realizedOutputNodes = 0;
            for(vector<vector<int64_t> >::iterator iter=output_coefficients.begin();iter!=output_coefficients.end();++iter )
            {
                {
                    std::stringstream o;
                    o << "Searching [";
                    for(int i=0;i<(int)(*iter).size();i++){
                        if(i>0)o <<",";
                        o << (*iter)[i];
                    }
                    o << "]... ";
                    REPORT(DETAILED,o.str());
                }

                vector<int64_t> node_output(*iter);
                int norm_factor=0;
                bool normalize=true;
                do
                {
                    normalize = true;
                    cout << "!!! node_output = ";
                    for(unsigned int i=0;i<node_output.size();i++)
                    {
                        cout << node_output[i] << " ";
                        if(node_output[i] == 0) THROWERROR( "Node output is zero! " );
                        if( abs(node_output[i]) % 2 == 1) normalize=false;
                    }
                    if(normalize)
                    {
                        norm_factor++;
                        for(unsigned int i=0;i<node_output.size();i++)
                        {
                            node_output[i] /= 2;
                        }
                    }
                    cout << endl;
                    cout << "!!! while( normalize )" << endl;
                }while( normalize );

                if(norm_factor > 0){
                    std::stringstream o;
                    o << "Fundamental [";
                    for(int i=0;i<(int)node_output.size();i++){
                        if(i>0)o <<",";
                        o << node_output[i];
                    }
                    o << "] norm "<< norm_factor;
                    REPORT(DETAILED,o.str())
                }

                stringstream outputSignalName;
                outputSignalName << "x_out" << realizedOutputNodes << "_";
                realizedOutputNodes++;
                for(int i=0; i < (int)(*iter).size(); i++)
                {
                    if(i>0) outputSignalName << "v" ;
                    outputSignalName << (((*iter)[i] < 0) ? "m" : "");
                    outputSignalName << abs((*iter)[i]);
                }
                bool found = false;
                for (std::list<adder_graph_base_node_t*>::iterator operationNode = stageNodesMap[noOfPipelineStages].begin();
                     operationNode != stageNodesMap[noOfPipelineStages].end();
                     ++operationNode)
                {
                    if( (*operationNode)->output_factor[0] == node_output )
                    {

                        REPORT(DETAILED, "FOUND!")
                                found = true;
                        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* op_node = additionalNodeInfoMap[(*operationNode)];
                        addOutput(outputSignalName.str(), op_node->wordsize + norm_factor);
                        vector<vector<int64_t> > outer;
                        outer.push_back(*iter);
                        sig_info.output_factors = outer;
                        sig_info.signal_name = outputSignalName.str();
                        sig_info.wordsize = op_node->wordsize + norm_factor;
                        output_signals.push_back( sig_info );
                        if( norm_factor == 0 )
                        {
                            vhdl << "\t" << outputSignalName.str() << " <= " << op_node->outputSignalName << ";" << endl;
                        }
                        else
                        {
                            vhdl << "\t" << outputSignalName.str() << " <= " << getShiftAndResizeString(op_node->outputSignalName,op_node->wordsize + norm_factor,norm_factor,false) << ";" << endl;
                        }
                    }
                }
                if(!found)
                    THROWERROR( "MISSING!" );

            }

        }
    }

    //cout << endl;

    pipelined_adder_graph.clear();
}

list<IntConstMultShiftAdd::output_signal_info> &IntConstMultShiftAdd::GetOutputList()
{
    return output_signals;
}

void IntConstMultShiftAdd::emulate(TestCase * tc)
{
    vector<mpz_class> input_vec;

    unsigned int confVal=emu_conf;

    if( noOfConfigurations > 1 )
        tc->addInput("config_no",emu_conf);

    for(int i=0;i<noOfInputs;i++ )
    {
        stringstream inputName;
        inputName << "x_in" << i;

        mpz_class inputVal = tc->getInputValue(inputName.str());

        mpz_class big1 = (mpz_class(1) << (wIn));
        mpz_class big1P = (mpz_class(1) << (wIn-1));

        if ( inputVal >= big1P)
            inputVal = inputVal - big1;

        input_vec.push_back(inputVal);
    }

    mpz_class expectedResult;


    for(list<output_signal_info>::iterator out_it= output_signals.begin();out_it!=output_signals.end();++out_it  )
    {
		expectedResult = 0;
        stringstream comment;
        for(int i=0; i < noOfInputs; i++)
        {
            mpz_class output_factor;
            if ((int)confVal<noOfConfigurations)
                output_factor= (long signed int) (*out_it).output_factors[confVal][i];
            else
                output_factor= (long signed int) (*out_it).output_factors[noOfConfigurations-1][i];

            expectedResult += input_vec[i] * output_factor;
            if(i != 0) comment << " + ";
            else comment << "\t";
            comment << input_vec[i] << " * " << output_factor;
        }
        comment << " == " << expectedResult << endl;

        int output_ws = computeWordSize((*out_it).output_factors,wIn);
        if ( expectedResult < mpz_class(0) )
        {
            mpz_class min_val = -1 * (mpz_class(1) << (output_ws-1));
            if( expectedResult < min_val )
            {
                std::stringstream err;
                err << "ERROR in testcase <" << comment.str() << ">" << std::endl;
                err << "Wordsize of outputfactor ("<< output_ws << ") does not match given (" << computeWordSize((*out_it).output_factors,wIn) << ")";
                THROWERROR( err.str() )
            }
        }
        else
        {
            mpz_class max_val = (mpz_class(1) << (output_ws-1)) -1;
            if( expectedResult > max_val )
            {
                std::stringstream err;
                err << "ERROR in testcase <" << comment.str() << ">\n";
                err << "Outputfactor does not fit in given wordsize" << endl;
                THROWERROR( err.str() )
            }
        }

        try
        {
            tc->addComment(comment.str());
            tc->addExpectedOutput((*out_it).signal_name,expectedResult);
        }
        catch(string errorstr)
        {
            cout << errorstr << endl;
        }
    }

    if(emu_conf < noOfConfigurations-1 && noOfConfigurations!=1)
        emu_conf++;
    else
        emu_conf=0;
}

void IntConstMultShiftAdd::buildStandardTestCases(TestCaseList * tcl)
{
    TestCase* tc;

    int min_val = -1 * (1 << (wIn-1));
    int max_val = (1 << (wIn-1)) -1;

    stringstream max_str;
    max_str << "Test MAX: " << max_val;

    stringstream min_str;
    min_str << "Test MIN: " << min_val;

    for(int i=0;i<noOfConfigurations;i++)
    {
        tc = new TestCase (this);
        tc->addComment("Test ZERO");
        if( noOfConfigurations > 1 )
        {
            tc->addInput("config_no",i);
        }
        for(list<string>::iterator inp_it = input_signals.begin();inp_it!=input_signals.end();++inp_it )
        {
            tc->addInput(*inp_it,0 );
        }
        emulate(tc);
        tcl->add(tc);
    }

    for(int i=0;i<noOfConfigurations;i++)
    {
        tc = new TestCase (this);
        tc->addComment("Test ONE");
        if( noOfConfigurations > 1 )
        {
            tc->addInput("config_no",i);
        }
        for(list<string>::iterator inp_it = input_signals.begin();inp_it!=input_signals.end();++inp_it )
        {
            tc->addInput(*inp_it,1 );
        }
        emulate(tc);
        tcl->add(tc);
    }

    for(int i=0;i<noOfConfigurations;i++)
    {
        tc = new TestCase (this);
        tc->addComment(min_str.str());
        if( noOfConfigurations > 1 )
        {
            tc->addInput("config_no",i);
        }
        for(list<string>::iterator inp_it = input_signals.begin();inp_it!=input_signals.end();++inp_it )
        {
            tc->addInput(*inp_it,min_val );
        }
        emulate(tc);
        tcl->add(tc);
    }

    for(int i=0;i<noOfConfigurations;i++)
    {
        tc = new TestCase (this);
        tc->addComment(max_str.str());
        if( noOfConfigurations > 1 )
        {
            tc->addInput("config_no",i);
        }
        for(list<string>::iterator inp_it = input_signals.begin();inp_it!=input_signals.end();++inp_it )
        {
            tc->addInput(*inp_it,max_val );
        }
        emulate(tc);
        tcl->add(tc);
    }
}

string IntConstMultShiftAdd::generateSignalName(adder_graph_base_node_t *node)
{
    stringstream signalName;
    signalName << "x";
    for( int i=0;i<noOfConfigurations;i++ )
    {
        signalName << "_c";
        for( int j=0;j<noOfInputs;j++)
        {
            if(j>0) signalName << "v";
            if( node->output_factor[i][j] != DONT_CARE )
            {
                if( node->output_factor[i][j] < 0 )
                {
                    signalName << "m";
                }
                signalName << abs( node->output_factor[i][j] );
            }
            else
                signalName << "d";
        }
    }
    signalName << "_s" << node->stage;
    if (is_a<output_node_t>(*node))
        signalName << "_o";
    return signalName.str();
}

IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* IntConstMultShiftAdd::identifyNodeType(adder_graph_base_node_t *node)
{
    if(is_a<adder_subtractor_node_t>(*node))
    {
        adder_subtractor_node_t* t = (adder_subtractor_node_t*)node;
        if( t->inputs.size() == 2)
        {
            if(t->input_is_negative[0] || t->input_is_negative[1])
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_SUB2_1N* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_SUB2_1N(this);
                return new_node;
            }
            else
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADD2* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADD2(this);
                return new_node;
            }
        }
        else if( t->inputs.size() == 3)
        {
            needs_unisim = true;
            int negative_count=0;
            if(t->input_is_negative[0]) negative_count++;
            if(t->input_is_negative[1]) negative_count++;
            if(t->input_is_negative[2]) negative_count++;

            if(negative_count == 0)
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADD3* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADD3(this);
                return new_node;
            }
            else if(negative_count == 1)
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_SUB3_1N* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_SUB3_1N(this);
                return new_node;
            }
            else if(negative_count == 2)
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_SUB3_2N* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_SUB3_2N(this);
                return new_node;
            }
        }
    }
    else if(is_a<conf_adder_subtractor_node_t>(*node))
    {
        conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)node;
        int highCountState;
        map<short,vector<int> > adder_states;

        highCountState = -1;
        int highCount=0;
        for( unsigned int i = 0;i<t->input_is_negative.size();i++)
        {
            short cur_state=0;
            for( unsigned int j = 0;j<t->input_is_negative[i].size();j++)
            {
                if(t->input_is_negative[i][j])
                    cur_state |= (1<<j);
            }
            map<short,vector<int> >::iterator found = adder_states.find(cur_state);
            if( found == adder_states.end())
            {
                vector<int> newvec;
                newvec.push_back(i);
                adder_states.insert( {cur_state,newvec}  );
            }
            else
            {
                (*found).second.push_back(i);
                if((int)(*found).second.size()>highCount)
                {
                    highCount = (*found).second.size();
                    highCountState = cur_state;
                }
            }
        }
        if(highCountState==-1)
        {
            highCountState = (*adder_states.rbegin()).first;
        }

        if(t->inputs.size() == 2)
        {
            IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADDSUB2_CONF* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADDSUB2_CONF(this);
            IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER* new_dec = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER(this);
            new_dec->decoder_size = 2;
            new_node->decoder = new_dec;
            new_dec->node = new_node;
            new_node->adder_states = adder_states;
            new_node->highCountState = highCountState;
            return new_node;
        }
        else if(t->inputs.size() == 3)
        {
            needs_unisim = true;
            if(adder_states.size() == 2 && this->getTarget()->getVendor() == "Xilinx" )
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADDSUB3_2STATE* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADDSUB3_2STATE(this);
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER* new_dec = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER(this);
                new_dec->decoder_size = 1;
                new_node->decoder = new_dec;
                new_dec->node = new_node;
                new_node->adder_states = adder_states;
                new_node->highCountState = highCountState;
                return new_node;
            }
            else
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADDSUB3_CONF* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_ADDSUB3_CONF(this);
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER* new_dec = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER(this);
                new_dec->decoder_size = 3;
                new_node->decoder = new_dec;
                new_dec->node = new_node;
                new_node->adder_states = adder_states;
                new_node->highCountState = highCountState;
                return new_node;
            }
        }
    }
    else if(is_a<register_node_t>(*node) || is_a<output_node_t>(*node) )
    {
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_REG* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_REG(this);
        return new_node;
    }
    else if(is_a<mux_node_t>(*node))
    {
        mux_node_t* t = (mux_node_t*)node;
        vector<int> dontCareConfig;
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_muxInput zeroInput;
        zeroInput.node = NULL;
        zeroInput.shift = 0;
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_muxInput dontCareInput;
        dontCareInput.node = NULL;
        dontCareInput.shift = -1;
        vector<IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_muxInput> muxInfoMap;
        for(unsigned int i=0;i<t->inputs.size();i++)
        {
            bool found=false;
            if(t->output_factor[i][0] == DONT_CARE)
            {
                dontCareInput.configurations.push_back(i);
                continue;
            }

            if( t->output_factor[i] == vector<int64_t>(t->output_factor[i].size(),0) )
            {
                zeroInput.configurations.push_back(i);
                continue;
            }

            for( vector<IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_muxInput>::iterator iter = muxInfoMap.begin();
                 iter!=muxInfoMap.end();
                 ++iter)
            {
                if( (*iter).node == t->inputs[i] && (*iter).shift == t->input_shifts[i] )
                {
                    found = true;
                    (*iter).configurations.push_back(i);
                }
            }

            if( !found )
            {
                IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_muxInput newInfo;
                newInfo.node = t->inputs[i];
                newInfo.shift = t->input_shifts[i];
                newInfo.configurations.push_back(i);
                muxInfoMap.push_back(newInfo);
            }
        }
        if(zeroInput.configurations.size() > 0)
            muxInfoMap.push_back(zeroInput);

        if(dontCareInput.configurations.size() > 0)
        {
            muxInfoMap.push_back(dontCareInput);
        }

        for (unsigned int n=muxInfoMap.size(); n>1; n=n-1){
            for (unsigned int i=0; i<n-1; i=i+1){
                if (muxInfoMap.at(i).configurations.size() > muxInfoMap.at(i+1).configurations.size()){
                    IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_muxInput tmpInfo = muxInfoMap[i];
                    muxInfoMap[i] = muxInfoMap[i+1];
                    muxInfoMap[i+1] = tmpInfo;
                }
            }
        }

        short nonZeroOutputCount=0;
        for( int i=0;i<noOfConfigurations;i++ )
        {
            if( t->output_factor[i] != vector<int64_t>(t->output_factor[i].size(),0) && t->output_factor[i][0] != DONT_CARE )
            {
                nonZeroOutputCount++;
            }
        }

        if( nonZeroOutputCount == 1 )
        {
            IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_AND* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_AND(this);
            new_node->inputs = muxInfoMap;
            return new_node;
        }
        else
        {
            IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_MUX* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_MUX(this);
            IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER* new_dec = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_DECODER(this);
            new_node->decoder = new_dec;
            new_dec->node = new_node;
            new_node->inputs = muxInfoMap;
            return new_node;
        }
    }
    else if(is_a<input_node_t>(*node))
    {
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_INPUT* new_node = new IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_INPUT(this);
        return new_node;
    }
    else
    {
        return NULL;
    }
    return NULL;
}

void IntConstMultShiftAdd::identifyOutputConnections(adder_graph_base_node_t *node, map<adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*> &infoMap)
{
    if(is_a<adder_subtractor_node_t>(*node))
    {
        adder_subtractor_node_t* t = (adder_subtractor_node_t*)node;
        for(int i=0;i<(int)t->inputs.size();i++)
        {
            if( t->inputs[i] != NULL )
            {
                infoMap[t->inputs[i]]->outputConnectedTo.push_back(node);
            }
        }
    }
    else if(is_a<conf_adder_subtractor_node_t>(*node))
    {
        conf_adder_subtractor_node_t* t = (conf_adder_subtractor_node_t*)node;
        for(int i=0;i<(int)t->inputs.size();i++)
        {
            if( t->inputs[i] != NULL )
            {
                infoMap[t->inputs[i]]->outputConnectedTo.push_back(node);
            }
        }
    }
    else if(is_a<register_node_t>(*node) || is_a<output_node_t>(*node))
    {
        register_node_t* t = (register_node_t*)node;
        infoMap[t->input]->outputConnectedTo.push_back(node);
    }
    else if(is_a<mux_node_t>(*node))
    {
        mux_node_t* t = (mux_node_t*)node;
        for(int i=0;i<(int)t->inputs.size();i++)
        {
            if( t->inputs[i] != NULL )
            {
                infoMap[t->inputs[i]]->outputConnectedTo.push_back(node);
            }
        }
    }
}

void IntConstMultShiftAdd::printAdditionalNodeInfo(map<adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*> &infoMap)
{
    stringstream nodeInfoString;
    for( map<adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE*>::iterator nodeInfo = infoMap.begin();
         nodeInfo != infoMap.end();
         ++nodeInfo)
    {
        nodeInfoString << "Node (" << (*nodeInfo).first << ") identified as <"<< (*nodeInfo).second->nodeType << ":" << (*nodeInfo).second->get_name() <<"> in stage "<< (*nodeInfo).first->stage <<endl;
        nodeInfoString << "\tOutputsignal: "<<(*nodeInfo).second->outputSignalName<< endl;
        nodeInfoString << "\tOutputvalues: ";
        for(int i=0;i<(int)(*nodeInfo).first->output_factor.size();i++)
        {

            if(i>0) nodeInfoString << ";";
            for(int j=0;j<(int)(*nodeInfo).first->output_factor[i].size();j++)
            {

                if(j>0)
                    nodeInfoString << ",";
                if( (*nodeInfo).first->output_factor[i][j]!=DONT_CARE )
                    nodeInfoString<<(*nodeInfo).first->output_factor[i][j];
                else
                    nodeInfoString << "-";
            }
        }

        nodeInfoString << " (ws:" << (*nodeInfo).second->wordsize << ")" << endl;

        if( (*nodeInfo).second->outputConnectedTo.size() > 0 )
        {
            list<adder_graph_base_node_t*>::iterator iter = (*nodeInfo).second->outputConnectedTo.begin();
            nodeInfoString << "\tOutputConnectedTo: " << endl << "\t\t" << *iter  << "("<< infoMap[*iter ]->outputSignalName << ")";
            for( ++iter;
                 iter!= (*nodeInfo).second->outputConnectedTo.end();
                 ++iter)
            {
                nodeInfoString << endl << "\t\t" << *iter  << "("<< infoMap[*iter ]->outputSignalName << ")";
            }

            nodeInfoString << endl;
        }
    }

    nodeInfoString << endl;

    REPORT( DETAILED, nodeInfoString.str())
}

string IntConstMultShiftAdd::getShiftAndResizeString(string signalName, int outputWordsize, int inputShift,bool signedConversion)
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

string IntConstMultShiftAdd::getBinary(int value, int Wordsize)
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

bool IntConstMultShiftAdd::TryRunRPAG(string realisation, string& out)
{
    /// \todo Add rpag library call instead of running rpag.


    string RPAG_ENV_VAR = "RPAG_ARGS";
    if(realisation.find("{")!=string::npos) //!!! there should be a better way doing this!
    {
        RPAGused = false;
        return false;
    }
    else
    {
        RPAGused = true;
        REPORT( INFO, "INFO: Recognized coefficient input, try running RPAG")
                string args,cmd;
        char* env_buf = NULL;
        env_buf = getenv( RPAG_ENV_VAR.c_str() );
        if( env_buf != NULL )
            args = string(env_buf);

        if( realisation.find(",") != string::npos )
        {
            args += " --cmm";
        }
        cmd = "rpag " + args + " " + realisation + " --file_output=true";
        cout << "running->" << cmd << endl;
        FILE* cmd_ex = popen(cmd.c_str(),"r");

        if( cmd_ex == NULL )
        {
            THROWERROR( "ERROR: Could not run RPAG (PATH set?)" )
        }
        else
        {
            REPORT( INFO, "RPAG running")
                    REPORT( DETAILED, cmd )
                    string output;
            char buf[256];

            while( fgets(buf,256,cmd_ex) != NULL )
                output += string(buf);


            int pos_a=output.find("=",output.find("pipelined_adder_graph="))+1;
            int pos_b=output.find_first_of("\r\n",pos_a);

            output = output.substr(pos_a,pos_b-pos_a);

            if( output.size() > 0 )
                REPORT( INFO, "RESULT: " << output)
                        else
                {
                    THROWERROR("RPAG failed, could not create graph")
                }
                    out=output;

            pclose(cmd_ex);
        }
        return true;
    }
}



OperatorPtr flopoco::IntConstMultShiftAdd::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args )
{
  if( target->getVendor() != "Xilinx" )
    throw std::runtime_error( "Can't build xilinx primitive on non xilinx target" );

  int wIn, sync_every = 0;
  std::string graph;
  bool sync_inout,sync_muxes,pipeline;

  UserInterface::parseInt( args, "wIn", &wIn );
  UserInterface::parseString( args, "graph", &graph );
  UserInterface::parseBoolean( args, "pipeline", &pipeline );
  UserInterface::parseBoolean( args, "sync_inout", &sync_inout );
  UserInterface::parseBoolean( args, "sync_muxes", &sync_muxes );
  UserInterface::parseInt( args, "sync_every", &sync_every );

  return new IntConstMultShiftAdd(parentOp, target, wIn, graph, pipeline,sync_inout,sync_every,sync_muxes );
}


}//namespace
#endif // HAVE_PAGSUITE


namespace flopoco {
    void flopoco::IntConstMultShiftAdd::registerFactory()
    {
#ifdef HAVE_PAGSUITE
      UserInterface::add( "IntConstMultShiftAdd", // name
                          "A component for building constant multipliers based on pipelined adder graphs (PAGs).", // description, string
                          "BasicInteger", // category, from the list defined in UserInterface.cpp
                          "",
                          "wIn(int): Wordsize of pag inputs; \
                          graph(string): Realization string of the pag; \
                          pipeline(bool)=true: Enable pipelining of the pag; \
                          sync_inout(bool)=true: Enable pipeline registers for input and output stage; \
                          sync_muxes(bool)=true: Enable counting mux-only stages as full stage; \
                          sync_every(int)=1: Count of stages after which will be pipelined",
                          "",
                          IntConstMultShiftAdd::parseArguments
      );
#endif // HAVE_PAGSUITE
    }
}//namespace
