#ifndef  IntConstMultShiftAdd_HPP
#define IntConstMultShiftAdd_HPP

#include "Operator.hpp"

#ifdef HAVE_PAGSUITE

#include "utils.hpp"
#include "paglib/adder_graph.h"
#include "IntConstMultShiftAddTypes.hpp"

using namespace std;

#endif // HAVE_PAGSUITE
namespace flopoco {
  class IntConstMultShiftAdd : public Operator {
#ifdef HAVE_PAGSUITE
    public:
     static ostream nostream;
     int noOfPipelineStages;

     IntConstMultShiftAdd(Operator* parentOp, Target* target,int wIn_, string pipelined_realization_str, bool pipelined_=true, bool syncInOut_=true, int syncEveryN_=1,bool syncMux_=true);
     IntConstMultShiftAdd(Operator* parentOp, Target* target, int wIn_, vector<vector<int64_t> > &coefficients, bool pipelined_=true, bool syncInOut_=true, int syncEveryN_=1, bool syncMux_=true);

      ~IntConstMultShiftAdd() {}

     void emulate(TestCase * tc);
     void buildStandardTestCases(TestCaseList* tcl);
     struct output_signal_info{
         string signal_name;
         vector<vector<int64_t> > output_factors;
         int wordsize;};
     list<output_signal_info>& GetOutputList();

     static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args );

#endif // HAVE_PAGSUITE
     static void registerFactory();
#ifdef HAVE_PAGSUITE

  protected:
     int wIn;
     bool syncInOut;
     bool pipelined;
     int syncEveryN;
     bool syncMux;

     bool RPAGused;
     int emu_conf;
     vector<vector<int64_t> > output_coefficients;
     adder_graph_t pipelined_adder_graph;
     list<string> input_signals;

     list<output_signal_info> output_signals;

     int noOfInputs;
     int noOfConfigurations;
     bool needs_unisim;

     void ProcessIntConstMultShiftAdd(Target* target, string pipelined_realization_str);


     string generateSignalName(adder_graph_base_node_t* node);
     IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* identifyNodeType(adder_graph_base_node_t* node);
     bool TryRunRPAG(string pipelined_realization_str,string& out);

     void identifyOutputConnections(adder_graph_base_node_t* node, map<adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE *> &infoMap);
     void printAdditionalNodeInfo(map<adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE *> &infoMap );
     string getShiftAndResizeString(string signalName, int outputWordsize, int inputShift, bool signedConversion=true);
     string getBinary(int value, int wordsize);


#endif // HAVE_PAGSUITE
 };
}//namespace
#endif
