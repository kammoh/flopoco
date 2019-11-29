#include "BaseMultiplierXilinx2xk.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"

namespace flopoco {

Operator* BaseMultiplierXilinx2xk::generateOperator(
		Operator *parentOp,
		Target* target,
		Parametrization const & parameters) const
{
	return new BaseMultiplierXilinx2xkOp(
			parentOp,
			target,
            parameters.isSignedMultX(),
			parameters.isSignedMultY(),
            parameters.getMultXWordSize(),
			parameters.isFlippedXY()
		);
}

double BaseMultiplierXilinx2xk::getLUTCost(uint32_t wX, uint32_t wY) const
{
	if (wX <= 2)
		return double(wY + 1);
	else
		return double(wX + 1);
}

OperatorPtr BaseMultiplierXilinx2xk::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
    int wY;
	bool xIsSigned,yIsSigned;
    UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
	UserInterface::parseBoolean(args,"xIsSigned",&xIsSigned);
	UserInterface::parseBoolean(args,"yIsSigned",&yIsSigned);

	return new BaseMultiplierXilinx2xkOp(parentOp,target,xIsSigned,yIsSigned,wY, false);
}

void BaseMultiplierXilinx2xk::registerFactory()
{
    UserInterface::add("BaseMultiplierXilinx2xk", // name
                        "Implements a 2xY-LUT-Multiplier that can be realized efficiently on some Xilinx-FPGAs",
                       "BasicInteger", // categories
                        "",
                       "wY(int): size of input Y;\
						xIsSigned(bool)=0: input X is signed;\
						yIsSigned(bool)=0: input Y is signed;",
                       "",
                       BaseMultiplierXilinx2xk::parseArguments,
                       BaseMultiplierXilinx2xk::unitTest
    ) ;
}

void BaseMultiplierXilinx2xk::emulate(TestCase* tc)
{
    mpz_class svX = tc->getInputValue("X");
    mpz_class svY = tc->getInputValue("Y");
    mpz_class svR = svX * svY;
    tc->addExpectedOutput("O", svR);
}

TestList BaseMultiplierXilinx2xk::unitTest(int index)
{
    // the static list of mandatory tests
    TestList testStateList;
    vector<pair<string,string>> paramList;

    //test square multiplications:
    for(int w=1; w <= 6; w++)
    {
        paramList.push_back(make_pair("wX", to_string(w)));
        paramList.push_back(make_pair("wY", to_string(w)));
        testStateList.push_back(paramList);
        paramList.clear();
    }

    return testStateList;
}

BaseMultiplierXilinx2xkOp::BaseMultiplierXilinx2xkOp(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int width, bool flipXY) : Operator(parentOp,target)
{
    ostringstream name;

    string in1,in2;

    if(!flipXY)
    {
        wX = 2;
        wY = width;
        in1 = "Y";
        in2 = "X";
        name << "BaseMultiplierXilinx2x" << width;
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

    if((isSignedX == true) || (isSignedY == true)) throw string("unsigned inputs currently not supported by BaseMultiplierXilinx2xkOp, sorry");

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

		Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2( this,target );
        cur_lut->setGeneric( "init", lutop.get_hex(), 64 );

        inPortMap("i0",in2 + of(1));
        inPortMap("i2",in2 + of(0));

        if(i==0)
            //inPortMapCst("i1","'0'"); //connect 0 at LSB position
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
		Xilinx_CARRY4 *cur_cc = new Xilinx_CARRY4( this,target );

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

