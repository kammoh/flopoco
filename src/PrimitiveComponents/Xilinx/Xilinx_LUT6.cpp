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
    Xilinx_LUT6_base::Xilinx_LUT6_base( Target *target ) : Xilinx_Primitive( target ) {
    }

    Xilinx_LUT6::Xilinx_LUT6( Target *target ) : Xilinx_LUT6_base( target ) {
        setName( "LUT6" );
        base_init();
        addOutput( "o", 1 );
    }

    Xilinx_LUT6_2::Xilinx_LUT6_2( Target *target ) : Xilinx_LUT6_base( target ) {
        setName( "LUT6_2" );
        base_init();
        addOutput( "o5", 1 );
        addOutput( "o6", 1 );
    }

    Xilinx_LUT6_L::Xilinx_LUT6_L( Target *target ) : Xilinx_LUT6_base( target ) {
        setName( "LUT6_L" );
        base_init();
        addOutput( "lo", 1 );
    }

    Xilinx_LUT6_D::Xilinx_LUT6_D( Target *target ) : Xilinx_LUT6_base( target ) {
        setName( "LUT6_D" );
        base_init();
        addOutput( "o", 1 );
        addOutput( "lo", 1 );
    }

    void Xilinx_LUT6_base::base_init() {
        // definition of the source file name, used for info and error reporting using REPORT
        srcFileName = "Xilinx_LUT6";

        for( int i = 0; i < 6; i++ )
            addInput( join( "i", i ), 1 );
    }
}//namespace
