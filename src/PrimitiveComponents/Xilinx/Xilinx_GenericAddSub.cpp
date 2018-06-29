// general c++ library for manipulating streams
#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "Xilinx_GenericAddSub.hpp"
#include "Xilinx_GenericAddSub_slice.hpp"

using namespace std;
namespace flopoco {

    Xilinx_GenericAddSub::Xilinx_GenericAddSub(Operator* parentOp, Target *target, int wIn, int fixed_signs ) : Operator( parentOp, target ),wIn_(wIn),signs_(fixed_signs),dss_(false) {
        setCopyrightString( "Marco Kleinlein" );

        srcFileName = "Xilinx_GenericAddSub";
        stringstream name;
        name << "Xilinx_GenericAddSub_" << wIn;

        if( fixed_signs != -1 ) {
            name << "_fixed_" << ( fixed_signs & 0x1 ) << ( ( fixed_signs & 0x2 ) >> 1 ) ;
        }

        setNameWithFreqAndUID( name.str() );
        setCombinatorial();
        addLUT( wIn + 1 );

        if( fixed_signs != -1 ) {
            build_with_fixed_sign( target, wIn, fixed_signs );
        } else {
            build_normal( target, wIn );
        }
    }

    Xilinx_GenericAddSub::Xilinx_GenericAddSub(Operator* parentOp, Target *target, int wIn, bool dss ) : Operator( parentOp, target ),wIn_(wIn),signs_(-1),dss_(dss) {
        setCopyrightString( "Marco Kleinlein" );
        srcFileName = "Xilinx_GenericAddSub";
        stringstream name;
        name << "Xilinx_GenericAddSub_" << wIn;

        if( dss ) {
            name << "_dss";
        }

        setNameWithFreqAndUID( name.str() );
		setCombinatorial();
        addLUT( wIn + 1 );

        if( dss ) {
            build_with_dss( target, wIn );
        } else {
            build_normal( target, wIn );
        }
    }

    void Xilinx_GenericAddSub::build_normal( Target *target, int wIn ) {
        addInput( "x_i", wIn );
        addInput( "y_i", wIn );
        addInput( "neg_x_i" );
        addInput( "neg_y_i" );
        addOutput( "sum_o", wIn );
        addOutput( "c_o" );
        const int effective_ws = wIn + 1;
        const int ws_remain = ( effective_ws ) % 4;
        const int num_full_slices = floor( ( effective_ws - ws_remain ) / 4 );
        declare( "carry", num_full_slices + 1 );
        declare( "sum_t", effective_ws );
        declare( "x", effective_ws );
        declare( "y", effective_ws );
        declare( "neg_x");
        declare( "neg_y");
        vhdl << tab << "x" << range( effective_ws - 1, 1 ) << " <= x_i;" << std::endl;
        vhdl << tab << "y" << range( effective_ws - 1, 1 ) << " <= y_i;" << std::endl;
        vhdl << tab << "neg_x <= neg_x_i;" << std::endl;
        vhdl << tab << "neg_y <= neg_y_i;" << std::endl;
        int i = 0;

        for( ; i < num_full_slices; i++ ) {
            stringstream slice_name;
            slice_name << "slice_" << i;
            Xilinx_GenericAddSub_slice *slice_i = new Xilinx_GenericAddSub_slice( getParentOp(), target, 4, ( i == 0 ? true : false ), false, false, this->getName() );
            addSubComponent( slice_i );
            inPortMap( slice_i, "x_in", "x" + range( 4 * i + 3, 4 * i ) );
            inPortMap( slice_i, "y_in", "y" + range( 4 * i + 3, 4 * i ) );
            inPortMap( slice_i, "neg_x_in", "neg_x" );
            inPortMap( slice_i, "neg_y_in", "neg_y" );

            if( i == 0 ) {
                inPortMapCst( slice_i, "carry_in", "'0'" );
            } else {
                inPortMap( slice_i, "carry_in" , "carry" + of( i - 1 ) );
            }

            outPortMap( slice_i, "carry_out", "carry" + of( i ));
            outPortMap( slice_i, "sum_out", "sum_t" + range( 4 * i + 3, 4 * i ));
            vhdl << instance( slice_i, slice_name.str() );
        }

        if( ws_remain > 0 ) {
            stringstream slice_name;
            slice_name << "slice_" << i;
            Xilinx_GenericAddSub_slice *slice_i = new Xilinx_GenericAddSub_slice( getParentOp(), target, ws_remain, ( i == 0 ? true : false ), false, false , this->getName() );
            addSubComponent( slice_i );
            inPortMap( slice_i, "x_in", "x" + range( effective_ws - 1, 4 * i ) );
            inPortMap( slice_i, "y_in", "y" + range( effective_ws - 1, 4 * i ) );
            inPortMap( slice_i, "neg_x_in", "neg_x" );
            inPortMap( slice_i, "neg_y_in", "neg_y" );

            if( i == 0 ) {
                inPortMapCst( slice_i, "carry_in", "'0'" );
            } else {
                inPortMap( slice_i, "carry_in" , "carry" + of( i - 1 ) );
            }

            outPortMap( slice_i, "carry_out", "carry" + of( i ));
            outPortMap( slice_i, "sum_out", "sum_t" + range( effective_ws - 1, 4 * i ));
            vhdl << instance( slice_i, slice_name.str() );
        }

        vhdl << tab << "sum_o <= sum_t" << range( effective_ws - 1, 1 ) << ";" << std::endl;
        vhdl << tab << "c_o <= carry" << of( i ) << ";" << std::endl;
    }

    void Xilinx_GenericAddSub::build_with_dss( Target *target, int wIn ) {
        addInput( "x_i", wIn );
        addInput( "y_i", wIn );
        addInput( "neg_x_i" );
        addInput( "neg_y_i" );
        addOutput( "sum_o", wIn );
        addOutput( "c_o" );
        const int effective_ws = wIn;
        const int ws_remain = ( effective_ws ) % 4;
        const int num_full_slices = ( effective_ws - ws_remain ) / 4;
        declare( "carry", num_full_slices + 1 );
        declare( "sum_t", effective_ws );
        declare( "x", effective_ws );
        declare( "y", effective_ws );
        declare( "neg_x" );
        declare( "neg_y" );
        declare( "bbus", wIn + 1 );
        vhdl << tab << "x" << range( effective_ws - 1, 0 ) << " <= x_i;" << std::endl;
        vhdl << tab << "y" << range( effective_ws - 1, 0 ) << " <= y_i;" << std::endl;
        vhdl << tab << "neg_x <= neg_x_i;" << std::endl;
        vhdl << tab << "neg_y <= neg_y_i;" << std::endl;
        vhdl << tab << "bbus" << of( 0 ) << " <= '0';" << std::endl;
        int i = 0;

        for( ; i < num_full_slices; i++ ) {
            stringstream slice_name;
            slice_name << "slice_" << i;
            Xilinx_GenericAddSub_slice *slice_i = new Xilinx_GenericAddSub_slice( getParentOp(), target, 4, ( i == 0 ? true : false ), false, true , this->getName() );
            addSubComponent( slice_i );
            inPortMap( slice_i, "x_in", "x" + range( 4 * i + 3, 4 * i ) );
            inPortMap( slice_i, "y_in", "y" + range( 4 * i + 3, 4 * i ) );
            inPortMap( slice_i, "neg_x_in", "neg_x" );
            inPortMap( slice_i, "neg_y_in", "neg_y" );
            inPortMap( slice_i, "bbus_in", "bbus" + range( 4 * i + 3, 4 * i ) );
            outPortMap( slice_i, "bbus_out" , "bbus" + range( 4 * i + 4, 4 * i + 1 ));

            if( i == 0 ) {
                inPortMapCst( slice_i, "carry_in", "'0'" );
            } else {
                inPortMap( slice_i, "carry_in" , "carry" + of( i-1 ) );
            }

            outPortMap( slice_i, "carry_out", "carry" + of( i ));
            outPortMap( slice_i, "sum_out", "sum_t" + range( 4 * i + 3, 4 * i ));
            vhdl << instance( slice_i, slice_name.str() );
        }

        if( ws_remain > 0 ) {
            stringstream slice_name;
            slice_name << "slice_" << i;
            Xilinx_GenericAddSub_slice *slice_i = new Xilinx_GenericAddSub_slice( getParentOp(), target, ws_remain, ( i == 0 ? true : false ), false, true , this->getName() );
            addSubComponent( slice_i );
            inPortMap( slice_i, "x_in", "x" + range( effective_ws - 1, 4 * i ) );
            inPortMap( slice_i, "y_in", "y" + range( effective_ws - 1, 4 * i ) );
            inPortMap( slice_i, "neg_x_in", "neg_x" );
            inPortMap( slice_i, "neg_y_in", "neg_y" );
            inPortMap( slice_i, "bbus_in", "bbus" + range( effective_ws - 1, 4 * i ) );
            outPortMap( slice_i, "bbus_out" , "bbus" + range( effective_ws, 4 * i + 1 ));

            if( i == 0 ) {
                inPortMapCst( slice_i, "carry_in", "'0'" );
            } else {
                inPortMap( slice_i, "carry_in" , "carry" + of( i-1 ) );
            }

            if( i == 0 ) {
                outPortMap( slice_i, "carry_out", "carry");
            } else {
                outPortMap( slice_i, "carry_out", "carry" + of( i ));
            }

            outPortMap( slice_i, "sum_out", "sum_t" + range( effective_ws - 1, 4 * i ));
            vhdl << instance( slice_i, slice_name.str() );
        }

        vhdl << tab << "sum_o <= sum_t" << range( effective_ws - 1, 0 ) << ";" << std::endl;

        if( i == 0 ) {
            vhdl << tab << "c_o <= carry;" << std::endl;
        } else {
            vhdl << tab << "c_o <= carry" << of( i ) << ";" << std::endl;
        }
    }

    void Xilinx_GenericAddSub::build_with_fixed_sign( Target *target, int wIn, int fixed_signs ) {
        addInput( "x_i", wIn );
        addInput( "y_i", wIn );
        addOutput( "sum_o", wIn );
        addOutput( "c_o" );
        const int effective_ws = wIn;
        const int ws_remain = ( effective_ws ) % 4;
        const int num_full_slices = ( effective_ws - ws_remain ) / 4;
        declare( "carry", num_full_slices + 1 );
        declare( "sum_t", effective_ws );
        declare( "x", effective_ws );
        declare( "y", effective_ws );
        vhdl << tab << "x" << range( effective_ws - 1 , 0 ) << " <= x_i;" << std::endl;
        vhdl << tab << "y" << range( effective_ws - 1, 0 ) << " <= y_i;" << std::endl;
        std::string neg_x, neg_y;

        if( fixed_signs != 3 ) {
            if( fixed_signs & 0x1 ) {
                neg_x = "'1'";
            } else {
                neg_x = "'0'";
            }

            if( fixed_signs & 0x2 ) {
                neg_y = "'1'";
            } else {
                neg_y = "'0'";
            }
        } else {
            throw "up";
        }

        int i = 0;

        //for(;(4*i+3)<=wIn;i++){
        for( ; i < num_full_slices; i++ ) {
            stringstream slice_name;
            slice_name << "slice_" << i;
            Xilinx_GenericAddSub_slice *slice_i = new Xilinx_GenericAddSub_slice( getParentOp(), target, 4, ( i == 0 ? true : false ), true, false , this->getName() );
            addSubComponent( slice_i );
            inPortMap( slice_i, "x_in", "x" + range( 4 * i + 3, 4 * i ) );
            inPortMap( slice_i, "y_in", "y" + range( 4 * i + 3, 4 * i ) );
            inPortMapCst( slice_i, "neg_x_in", neg_x );
            inPortMapCst( slice_i, "neg_y_in", neg_y );

            if( i == 0 ) {
                if( fixed_signs > 0 ) {
                    inPortMapCst( slice_i, "carry_in", "'1'" );
                } else {
                    inPortMapCst( slice_i, "carry_in", "'0'" );
                }
            } else {
                inPortMap( slice_i, "carry_in" , "carry" + range(i-1, i - 1 ) );
            }

            outPortMap( slice_i, "carry_out", "carry" + range(i, i ));
            outPortMap( slice_i, "sum_out", "sum_t" + range( 4 * i + 3, 4 * i ));
            vhdl << instance( slice_i, slice_name.str() );
        }

        if( ws_remain > 0 ) {
            stringstream slice_name;
            slice_name << "slice_" << i;
            Xilinx_GenericAddSub_slice *slice_i = new Xilinx_GenericAddSub_slice( getParentOp(), target, ws_remain, ( i == 0 ? true : false ), true, false , this->getName() );
            addSubComponent( slice_i );
            inPortMap( slice_i, "x_in", "x" + range( effective_ws - 1, 4 * i ) );
            inPortMap( slice_i, "y_in", "y" + range( effective_ws - 1, 4 * i ) );
            inPortMapCst( slice_i, "neg_x_in", neg_x );
            inPortMapCst( slice_i, "neg_y_in", neg_y );

            if( i == 0 ) {
                if( fixed_signs > 0 ) {
                    inPortMapCst( slice_i, "carry_in", "'1'" );
                } else {
                    inPortMapCst( slice_i, "carry_in", "'0'" );
                }
            } else {
                inPortMap( slice_i, "carry_in" , "carry" + range(i-1, i - 1 ) );
            }

            if( i == 0 ) {
                outPortMap( slice_i, "carry_out", "carry");
            } else {
                outPortMap( slice_i, "carry_out", "carry" + range(i, i ));
            }

            outPortMap( slice_i, "sum_out", "sum_t" + range( effective_ws - 1, 4 * i ));
            vhdl << instance( slice_i, slice_name.str() );
        }

        vhdl << tab << "sum_o <= sum_t" << range( effective_ws - 1, 0 ) << ";" << std::endl;

        if( i == 0 ) {
            vhdl << tab << "c_o <= carry;" << std::endl;
        } else {
            vhdl << tab << "c_o <= carry" << of( i ) << ";" << std::endl;
        }
    }
}//namespace


