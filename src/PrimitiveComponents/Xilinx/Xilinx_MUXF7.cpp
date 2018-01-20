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
#include "Xilinx_MUXF7.hpp"

using namespace std;
namespace flopoco {
    Xilinx_MUXF7_base::Xilinx_MUXF7_base( Target *target ) : Xilinx_Primitive( target ) {}

    Xilinx_MUXF7::Xilinx_MUXF7( Target *target ) : Xilinx_MUXF7_base( target ) {
        setName( "MUXF7" );
        addOutput( "o", 1 );
        base_init();
    }

    Xilinx_MUXF7_L::Xilinx_MUXF7_L( Target *target ) : Xilinx_MUXF7_base( target ) {
        setName( "MUXF7_L" );
        addOutput( "lo", 1 );
        base_init();
    }

    Xilinx_MUXF7_D::Xilinx_MUXF7_D( Target *target ) : Xilinx_MUXF7_base( target ) {
        setName( "MUXF7_D" );
        addOutput( "o", 1 );
        addOutput( "lo", 1 );
        base_init();
    }

    void Xilinx_MUXF7_base::base_init() {
        srcFileName = "Xilinx_MUXF7";
        addInput( "i0", 1 );
        addInput( "i1", 1 );
        addInput( "s", 1 );
    }
}//namespace
