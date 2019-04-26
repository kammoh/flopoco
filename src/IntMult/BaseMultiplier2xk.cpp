#include "BaseMultiplier2xk.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"

namespace flopoco {


BaseMultiplier2xk::BaseMultiplier2xk(bool isSignedX, bool isSignedY, int width, bool flipXY) : BaseMultiplier(isSignedX,isSignedY)
{

    srcFileName = "BaseMultiplier2xk";
    uniqueName_ = "BaseMultiplier2xk";

    this->flipXY = flipXY;
    this->width = width;

    if(!flipXY)
    {
        wX = 2;
        wY = width;
        uniqueName_ = string("BaseMultiplier2x") + std::to_string(width);
    }
    else
    {
        wX = width;
        wY = 2;
        uniqueName_ = string("BaseMultiplier") + std::to_string(width) + string("x2");
    }

}

Operator* BaseMultiplier2xk::generateOperator(Operator *parentOp, Target* target)
{
	return new BaseMultiplier2xkOp(parentOp, target, isSignedX, isSignedY, width, flipXY);
}
	

BaseMultiplier2xkOp::BaseMultiplier2xkOp(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int width, bool flipXY) : Operator(parentOp,target)
{
    ostringstream name;

    string in1,in2;

    if(!flipXY)
    {
        wX = 2;
        wY = width;
        in1 = "Y";
        in2 = "X";
        name << "BaseMultiplier2x" << width;
    }
    else
    {
        wX = width;
        wY = 2;
        in1 = "X";
        in2 = "Y";
        name << "BaseMultiplier" << width << "x2";
    }
    setNameWithFreqAndUID(name.str());

    addInput("X", wX, true);
    addInput("Y", wY, true);


    addOutput("R", width+2, 1, true);

    if((isSignedX == true) || (isSignedY == true)) throw string("unsigned inputs currently not supported by BaseMultiplier2xkOp, sorry");

    int needed_luts = width+1;//no. of required LUTs
    int needed_cc = ( needed_luts / 4 ) + ( needed_luts % 4 > 0 ? 1 : 0 ); //no. of required carry chains

    declare( "cc_s", needed_cc * 4 );
    declare( "cc_di", needed_cc * 4 );
    declare( "cc_co", needed_cc * 4 );
    declare( "cc_o", needed_cc * 4 );

    //create the LUTs:
    for(int i=0; i < needed_luts; i++)
    {
        //LUT content of the LUTs:
        lut_op lutop_o6 = (lut_in(0) & lut_in(1)) ^ (lut_in(2) & lut_in(3)); //xor of two partial products
        lut_op lutop_o5 = lut_in(0) & lut_in(1); //and of first partial product
        lut_init lutop( lutop_o5, lutop_o6 );

		Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2( parentOp,target );
        cur_lut->setGeneric( "init", lutop.get_hex(), 64 );

        inPortMap("i0",in2 + of(1));
        inPortMap("i2",in2 + of(0));

        if(i==0)
            inPortMapCst("i1","'0'"); //connect 0 at LSB position
        else
            inPortMap("i1",in1 + of(i-1));

        if(i==needed_luts-1)
            inPortMapCst("i3","'0'"); //connect 0 at MSB position
        else
            inPortMap("i3",in1 + of(i));

        inPortMapCst("i4","'0'");
        inPortMapCst("i5","'1'");

        outPortMap("o5","cc_di" + of(i));
        outPortMap("o6","cc_s" + of(i));

        vhdl << cur_lut->primitiveInstance( join("lut",i)) << endl;
    }

    //create the carry chain:
    for( int i = 0; i < needed_cc; i++ ) {
		Xilinx_CARRY4 *cur_cc = new Xilinx_CARRY4( parentOp,target );

        inPortMapCst("cyinit", "'0'" );
        if( i == 0 ) {
            inPortMapCst("ci", "'0'" ); //carry-in can not be used as AX input is blocked!!
        } else {
            inPortMap("ci", "cc_co" + of( i * 4 - 1 ) );
        }
        inPortMap("di", "cc_di" + range( i * 4 + 3, i * 4 ) );
        inPortMap("s", "cc_s" + range( i * 4 + 3, i * 4 ) );
        outPortMap("co", "cc_co" + range( i * 4 + 3, i * 4 ));
        outPortMap("o", "cc_o" + range( i * 4 + 3, i * 4 ));

        stringstream cc_name;
        cc_name << "cc_" << i;
        vhdl << cur_cc->primitiveInstance( cc_name.str());
    }
    vhdl << endl;

    vhdl << tab << "R <= cc_co(" << width << ") & cc_o(" << width << " downto 0);" << endl;
}

}   //end namespace flopoco

