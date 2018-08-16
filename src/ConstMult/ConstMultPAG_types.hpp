#ifndef CONSTMULTPAG_TYPES_HPP
#define CONSTMULTPAG_TYPES_HPP
/*  CONSTMULTPAG_TYPES.hpp
 *  Version:
 *  Datum: 20.11.2014
 */

#ifdef HAVE_PAGSUITE

#include "pag_lib/adder_graph.h"
#include "utils.hpp"
#include "Target.hpp"
#include "Operator.hpp"

#include <string>
#include <sstream>
#include <list>
#include <map>
#include <vector>

using namespace std;
using namespace flopoco;

namespace ConstMultPAG_TYPES {
    enum NODETYPE{
        NODETYPE_INPUT=1,
        NODETYPE_REG=12, // Standart register

        NODETYPE_ADD2=21, // Adder with 2 inputs
        NODETYPE_SUB2_1N=22, // Subtractor with 2 inputs
        NODETYPE_ADDSUB2_CONF=23, // Configurable adder/subtractor with 2 inputs

        NODETYPE_ADD3=31, // Adder with 3 Inputs
        NODETYPE_SUB3_1N=32, // Subtractor with 3 inputs; one fixed negative input
        NODETYPE_SUB3_2N=33, // Subtractor with 3 inputs; two fixed negative inputs
        NODETYPE_ADDSUB3_2STATE=34, // Switchable adder/subtractor with 3 inputs; two states
        NODETYPE_ADDSUB3_CONF=35, // Configurable adder/subtractor with 2 inputs

        NODETYPE_AND=51, // Multiplexer with one not zero input
        NODETYPE_MUX=52, // Standart multiplexer
        NODETYPE_MUX_Z=53, // Multiplexer with zero input

        NODETYPE_DECODER=61, // Decoder for configuration inputs

        ConstMultPAG_NODETYPE_UNKNOWN=0
    };

    struct ConstMultPAG_muxInput{
        adder_graph_base_node_t* node;
        int shift;
        vector<int> configurations;
    };


class ConstMultPAG_BASE{
public:
    Target* target;
    Operator* base_op;
    adder_graph_base_node_t* base_node;
    NODETYPE nodeType;
    int wordsize;
    string outputSignalName;
    int outputSignalUsageCount;
    list<adder_graph_base_node_t*> outputConnectedTo;
    list<pair<string,int> > declare_list;
    list<Operator*> opList;

    static string target_ID;
    static int noOfConfigurations;
    static int configurationSignalWordsize;
    static int noOfInputs;

    virtual string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap )
    { return ""; }

    virtual string get_name()
    { return ""; }

    ConstMultPAG_BASE(Operator* base){
        base_op = base;
        outputSignalUsageCount = 0;
        wordsize = 0;
    };

    void addSubComponent( Operator* op ){
        opList.push_back(op);
    }

    string declare(string signalName,int wordsize = 1 );
    void getInputOrder(vector<bool>& inputIsNegative,vector<int>& inputOrder );
    string getNegativeShiftString(string signalName , int wordsize, adder_graph_base_node_t *base_node );
    string getNegativeResizeString(string signalName,int outputWordsize);
    static string getShiftAndResizeString(string signalName, int wordsize, int inputShift, bool signedConversion=true);
    string getShiftAndResizeString(ConstMultPAG_BASE *input_node, int wordsize, int inputShift, bool signedConversion=true);
    static string getBinary(int value, int wordsize);
    string getTemporaryName();
};

class ConstMultPAG_INPUT : public ConstMultPAG_BASE{
public:
    ConstMultPAG_INPUT(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_INPUT;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "INPUT";}
};

class ConstMultPAG_REG : public ConstMultPAG_BASE{
public:
    ConstMultPAG_REG(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_REG;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "REG";}
};

class ConstMultPAG_ADD2 : public ConstMultPAG_BASE{
public:
    ConstMultPAG_ADD2(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_ADD2;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "ADD2";}
};

class ConstMultPAG_SUB2_1N : public ConstMultPAG_BASE{
public:
    ConstMultPAG_SUB2_1N(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_SUB2_1N;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "SUB2_1N";}
};

class ConstMultPAG_ADD3 : public ConstMultPAG_BASE{
public:
    ConstMultPAG_ADD3(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_ADD3;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "ADD3";}
};

class ConstMultPAG_SUB3_1N : public ConstMultPAG_BASE{
public:
    ConstMultPAG_SUB3_1N(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_SUB3_1N;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "SUB3_1N";}
};

class ConstMultPAG_SUB3_2N : public ConstMultPAG_BASE{
public:
    ConstMultPAG_SUB3_2N(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_SUB3_2N;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "SUB3_2N";}
};
class ConstMultPAG_BASE_CONF;
class ConstMultPAG_DECODER : public ConstMultPAG_BASE{
public:
    int decoder_size;
    ConstMultPAG_BASE *node;
    ConstMultPAG_DECODER(Operator* base):ConstMultPAG_BASE(base){nodeType=NODETYPE_DECODER;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "DECODER";}
};

class ConstMultPAG_BASE_CONF: public ConstMultPAG_BASE{
public:
    ConstMultPAG_BASE_CONF(Operator* base):ConstMultPAG_BASE(base){}
    ConstMultPAG_DECODER *decoder;
    int highCountState;
    map<short,vector<int> > adder_states;
};

class ConstMultPAG_ADDSUB2_CONF : public ConstMultPAG_BASE_CONF{
public:
    ConstMultPAG_ADDSUB2_CONF(Operator* base):ConstMultPAG_BASE_CONF(base){nodeType=NODETYPE_ADDSUB2_CONF;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "ADDSUB2_CONF";}
};

class ConstMultPAG_ADDSUB3_2STATE : public ConstMultPAG_BASE_CONF{
public:
    ConstMultPAG_ADDSUB3_2STATE(Operator* base):ConstMultPAG_BASE_CONF(base){nodeType=NODETYPE_ADDSUB3_2STATE;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "ADDSUB3_2STATE";}
};

class ConstMultPAG_ADDSUB3_CONF : public ConstMultPAG_BASE_CONF{
public:
    ConstMultPAG_ADDSUB3_CONF(Operator* base):ConstMultPAG_BASE_CONF(base){nodeType=NODETYPE_ADDSUB3_CONF;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "ADDSUB3_CONF";}
};

class ConstMultPAG_BASE_MUX: public ConstMultPAG_BASE{
public:
    ConstMultPAG_BASE_MUX(Operator* base):ConstMultPAG_BASE(base){}
    vector<ConstMultPAG_muxInput> inputs;
};

class ConstMultPAG_AND : public ConstMultPAG_BASE_MUX{
public:
    ConstMultPAG_AND(Operator* base):ConstMultPAG_BASE_MUX(base){nodeType=NODETYPE_AND;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "AND";}
};

class ConstMultPAG_MUX : public ConstMultPAG_BASE_MUX{
public:
    ConstMultPAG_DECODER *decoder;
    ConstMultPAG_MUX(Operator* base):ConstMultPAG_BASE_MUX(base){nodeType=NODETYPE_MUX;}
    string get_realisation(map<adder_graph_base_node_t*,ConstMultPAG_BASE*>& InfoMap);
    string get_name(){return "MUX";}
};
}
#endif // HAVE_PAGSUITE
#endif
