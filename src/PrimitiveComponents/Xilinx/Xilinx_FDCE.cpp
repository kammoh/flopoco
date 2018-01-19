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
#include "Xilinx_FDCE.hpp"

using namespace std;
namespace flopoco {
    Xilinx_FDCE::Xilinx_FDCE( Target *target ) : Xilinx_Primitive( target ) {
        setName( "FDCE" );
        srcFileName = "Xilinx_FDCE";
        addInput( "clr", 1 );
        addInput( "ce", 1 );
        addInput( "d", 1 );
        addInput( "c", 1 );
        addOutput( "q", 1 );
    }
}//namespace
