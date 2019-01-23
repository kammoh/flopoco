#ifndef  IntConstMultShiftAdd_HPP
#define IntConstMultShiftAdd_HPP

#include "Operator.hpp"

#ifdef HAVE_PAGLIB

#include "utils.hpp"
#include "pagsuite/adder_graph.h"
#include "IntConstMultShiftAddTypes.hpp"

using namespace std;

#endif // HAVE_PAGLIB
namespace flopoco {
    class IntConstMultShiftAdd : public Operator {
#ifdef HAVE_PAGLIB
    public:
        static ostream nostream;
        int noOfPipelineStages;

        IntConstMultShiftAdd(Operator* parentOp, Target* target,int wIn_, string pipelined_realization_str, bool pipelined_=true, bool syncInOut_=true, int syncEveryN_=1,bool syncMux_=true, int epsilon_=0);

        ~IntConstMultShiftAdd() {}

        void emulate(TestCase * tc);
        void buildStandardTestCases(TestCaseList* tcl);
        struct output_signal_info{
        string signal_name;
        vector<vector<int64_t> > output_factors;
        int wordsize;};
        list<output_signal_info>& GetOutputList();

        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args );

#endif // HAVE_PAGLIB
        static void registerFactory();
#ifdef HAVE_PAGLIB

    protected:
        int wIn;
        bool syncInOut;
        bool pipelined;
        int syncEveryN;
        bool syncMux;
        double epsilon;

        bool RPAGused;
        int emu_conf;
        vector<vector<int64_t> > output_coefficients;
        PAGSuite::adder_graph_t pipelined_adder_graph;
        list<string> input_signals;

        list<output_signal_info> output_signals;

        int noOfInputs;
        int noOfConfigurations;
        bool needs_unisim;

        void ProcessIntConstMultShiftAdd(Target* target, string pipelined_realization_str);


        string generateSignalName(PAGSuite::adder_graph_base_node_t* node);
        IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE* identifyNodeType(PAGSuite::adder_graph_base_node_t* node);

        void identifyOutputConnections(PAGSuite::adder_graph_base_node_t* node, map<PAGSuite::adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE *> &infoMap);
        void printAdditionalNodeInfo(map<PAGSuite::adder_graph_base_node_t *, IntConstMultShiftAdd_TYPES::IntConstMultShiftAdd_BASE *> &infoMap );
        string getShiftAndResizeString(string signalName, int outputWordsize, int inputShift, bool signedConversion=true);
        string getBinary(int value, int wordsize);


#endif // HAVE_PAGLIB
    };
}//namespace
#endif
