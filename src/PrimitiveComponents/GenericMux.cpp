#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"
#include "GenericMux.hpp"
#include "Primitive.hpp"

#include "Xilinx/Xilinx_GenericMux.hpp"

using namespace std;
namespace flopoco {
    GenericMux::GenericMux(Operator* parentOp, Target* target, const uint32_t &wIn, const uint32_t &inputCount) : Operator(parentOp,target) {
        setCopyrightString( "Marco Kleinlein" );
        srcFileName="GenericMux";
		ostringstream name;
        name << "GenericMux_w" << wIn << "_s" << inputCount;
        setNameWithFreqAndUID(name.str());

        for( uint i=0;i<inputCount;++i )
            addInput(getInputName(i),wIn);

        addInput(getSelectName(), intlog2( inputCount-1 ) );
        addOutput(getOutputName(),wIn);

        if( target->useTargetOptimizations() && target->getVendor() == "Xilinx" )
            buildXilinx(parentOp, target,wIn,inputCount);
        else if( target->useTargetOptimizations() && target->getVendor()=="Altera" )
            buildAltera(parentOp, target,wIn,inputCount);
        else
            buildCommon(target,wIn,inputCount);

    }

    void GenericMux::buildXilinx(Operator* parentOp, Target* target, const uint32_t &wIn, const uint32_t &inputCount){
        Xilinx_GenericMux *mux = new Xilinx_GenericMux(parentOp, target,inputCount,wIn );
        inPortMap("s_in",getSelectName());
        for( uint i=0;i<inputCount;++i )
            inPortMap(join( "x", i, "_in" ), getInputName(i));

        outPortMap("x_out",getOutputName() );
    }

    void GenericMux::buildAltera(Operator* parentOp, Target *target, const uint32_t &wIn, const uint32_t &inputCount){
        REPORT(LIST,"Altera junction not fully implemented, fall back to common.");
        buildCommon(target,wIn,inputCount);
    }

    void GenericMux::buildCommon(Target* target, const uint32_t &wIn, const uint32_t &inputCount){
        const uint16_t select_ws = intlog2( inputCount-1 );

        vhdl << tab << "with " << getSelectName() << " select" << endl
             << tab << tab << getOutputName() << " <= " << endl;
        for(unsigned int i=0; i< inputCount; i++)
        {
            vhdl << tab << tab << tab << getInputName(i) << " when \"";
            cout << "GenericMux: getInputName(" << i << ")=" << getInputName(i) << endl;
            for(int j=select_ws-1; j>=0; --j)
            {
                vhdl << (i&(1<<j)?"1":"0");
            }
            vhdl << "\"," << endl;
        }
        vhdl << "(others=>'X') when others;" << endl;
    }

    std::string GenericMux::getSelectName() const{
        return "iSel";
    }

    std::string GenericMux::getInputName(const uint32_t &index) const{
        std::stringstream o;
        o << "iS_" << index;
        return o.str();
    }

    std::string GenericMux::getOutputName() const{
        return "oMux";
    }

    void GenericMux::emulate(TestCase * tc) {

	}


    void GenericMux::buildStandardTestCases(TestCaseList * tcl) {
		// please fill me with regression tests or corner case tests!
	}





    OperatorPtr GenericMux::parseArguments(Target *target, vector<string> &args) {
        /*
		int param0, param1;
		UserInterface::parseInt(args, "param0", &param0); // param0 has a default value, this method will recover it if it doesnt't find it in args, 
		UserInterface::parseInt(args, "param1", &param1); 
        return new UserDefinedOperator(target, param0, param1);*/
        return nullptr;
	}
	
    void GenericMux::registerFactory(){
        /*UserInterface::add("UserDefinedOperator", // name
											 "An heavily commented example operator to start with FloPoCo.", // description, string
											 "Miscellaneous", // category, from the list defined in UserInterface.cpp
											 "", //seeAlso
											 // Now comes the parameter description string.
											 // Respect its syntax because it will be used to generate the parser and the docs
											 // Syntax is: a semicolon-separated list of parameterDescription;
											 // where parameterDescription is parameterName (parameterType)[=defaultValue]: parameterDescriptionString 
											 "param0(int)=16: A first parameter, here used as the input size; \
                        param1(int): A second parameter, here used as the output size",
											 // More documentation for the HTML pages. If you want to link to your blog, it is here.
											 "Feel free to experiment with its code, it will not break anything in FloPoCo. <br> Also see the developper manual in the doc/ directory of FloPoCo.",
											 UserDefinedOperator::parseArguments
                                             ) ;*/
	}

}//namespace
