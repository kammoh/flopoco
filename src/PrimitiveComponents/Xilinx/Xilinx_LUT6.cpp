// general c++ library for manipulating streams
#include <iostream>
#include <sstream>
#include <stdexcept>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "Xilinx_LUT6.hpp"

using namespace std;
namespace flopoco {
	Xilinx_LUT6_base::Xilinx_LUT6_base(Operator *parentOp, Target *target ) : Xilinx_Primitive(parentOp, target ) {
        srcFileName = "Xilinx_LUT6";

        for( int i = 0; i < 6; i++ )
            addInput( join( "i", i ), 1 );
    }

	Xilinx_LUT6::Xilinx_LUT6(Operator *parentOp, Target *target ) : Xilinx_LUT6_base( parentOp, target ) {
        setName( "LUT6" );
        addOutput( "o", 1 );
        vhdl << "o <= i0" << endl;
    }

	Xilinx_LUT6_2::Xilinx_LUT6_2(Operator* parentOp, Target *target ) : Xilinx_LUT6_base( parentOp,target ) {
        setName( "LUT6_2" );
        addOutput( "o5", 1 );
        addOutput( "o6", 1 );
    }

	Xilinx_LUT6_L::Xilinx_LUT6_L(Operator* parentOp, Target *target ) : Xilinx_LUT6_base( parentOp,target ) {
        setName( "LUT6_L" );
        addOutput( "lo", 1 );
    }

	Xilinx_LUT6_D::Xilinx_LUT6_D(Operator* parentOp, Target *target ) : Xilinx_LUT6_base( parentOp,target ) {
        setName( "LUT6_D" );
        addOutput( "o", 1 );
        addOutput( "lo", 1 );
    }

}//namespace
