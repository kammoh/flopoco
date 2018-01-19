#ifndef Xilinx_GenericAddSub_H
#define Xilinx_GenericAddSub_H

#include "Operator.hpp"
#include "utils.hpp"

namespace flopoco {

	// new operator class declaration
    class Xilinx_GenericAddSub : public Operator {
        const int wIn_;
        const int signs_;
        const bool dss_;
      public:
        // definition of some function for the operator

		// constructor, defined there with two parameters (default value 0 for each)
        Xilinx_GenericAddSub(Operator* parentOp, Target *target, int wIn = 10, bool dss = false );
        Xilinx_GenericAddSub(Operator* parentOp, Target *target, int wIn = 10, int fixed_signs = -1 );

        void
        build_normal( Target *target, int wIn );

        void
        build_with_dss( Target *target, int wIn );

        void
        build_with_fixed_sign( Target *target, int wIn, int fixed_signs );

		// destructor
        ~Xilinx_GenericAddSub() {}

        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args ){
            if( target->getVendor() != "Xilinx" )
                throw std::runtime_error( "Can't build xilinx primitive on non xilinx target" );

            int wIn,mode;
            bool dss;
            UserInterface::parseInt(args,"wIn",&wIn );
            UserInterface::parseInt(args,"mode",&mode);
            UserInterface::parseBoolean(args,"dss",&dss);

            if( dss ){
                return new Xilinx_GenericAddSub(parentOp, target,wIn,dss);
            }else if( mode > 0 ){
                return new Xilinx_GenericAddSub(parentOp, target,wIn,mode);
            }else{
                return new Xilinx_GenericAddSub(parentOp, target,wIn,false);
            }
        }
        static void registerFactory(){
            UserInterface::add( "XilinxAddSub", // name
                                "An adder/subtractor build of xilinx primitives.", // description, string
                                "Primitives", // category, from the list defined in UserInterface.cpp
                                "",
                                "wIn(int): The wordsize of the adder; \
                                mode(int)=0: Bitmask for input negation, removes configurability; \
                                dss(bool)=false: Creates configurable adder with possibility to substract both inputs at same time;",
                                "",
                                Xilinx_GenericAddSub::parseArguments
                              );
        }

        // Operator interface
    public:
        virtual void emulate(TestCase *tc)
        {
            bool neg_a=false,neg_b=false;
            mpz_class x = tc->getInputValue("x_i");
            mpz_class y = tc->getInputValue("y_i");
            mpz_class s = 0;
            if( signs_ > 0 ){
                if( signs_ & 0x1 )
                    neg_a = true;

                if( signs_ & 0x2 )
                    neg_b = true;
            } else {
                mpz_class na = tc->getInputValue("neg_x_i");
                mpz_class nb = tc->getInputValue("neg_y_i");

                if( na > 0 )
                    neg_a = true;

                if( nb > 0 )
                    neg_b = true;
            }

            if( neg_a )
                s -= x;
            else
                s += x;

            if( neg_b )
                s -= y;
            else
                s += y;

            mpz_class mask = ((1<<wIn_)-1);
            s = s & mask;

            tc->addExpectedOutput("sum_o", s);
        }
    };


}//namespace

#endif
