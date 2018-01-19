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
#include "Xilinx_LUT5.hpp"

using namespace std;
namespace flopoco {
    Xilinx_LUT5_base::Xilinx_LUT5_base( Target *target ) : Xilinx_Primitive( target ) {}

    Xilinx_LUT5::Xilinx_LUT5( Target *target ) : Xilinx_LUT5_base( target ) {
        setName( "LUT5" );
        addOutput( "o", 1 );
        base_init();
    }

    Xilinx_LUT5_L::Xilinx_LUT5_L( Target *target ) : Xilinx_LUT5_base( target ) {
        setName( "LUT5_L" );
        addOutput( "lo", 1 );
        base_init();
    }

    Xilinx_LUT5_D::Xilinx_LUT5_D( Target *target ) : Xilinx_LUT5_base( target ) {
        setName( "LUT5_D" );
        addOutput( "lo", 1 );
        addOutput( "o", 1 );
        base_init();
    }

    Xilinx_CFGLUT5::Xilinx_CFGLUT5( Target *target ) : Xilinx_LUT5_base( target ) {
        setName( "CFGLUT5" );
        addInput("clk");
        addInput("ce"); //clk enable
        addInput("cdi");//configuration data in
        addOutput( "o5", 1 ); //4 LUT output
        addOutput( "o6", 1 ); //5 LUT output
        addOutput( "cdo", 1 ); //configuration data out
        base_init();
    }

    void Xilinx_LUT5_base::base_init() {
        // definition of the source file name, used for info and error reporting using REPORT
        srcFileName = "Xilinx_LUT5";

        for( int i = 0; i < 5; i++ )
            addInput( join( "i", i ), 1 );
    }
}//namespace
