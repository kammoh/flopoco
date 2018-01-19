#include "BaseMultiplierDSPSuperTilesXilinx.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"

namespace flopoco {


BaseMultiplierDSPSuperTilesXilinx::BaseMultiplierDSPSuperTilesXilinx(bool isSignedX, bool isSignedY, TILE_SHAPE shape, bool pipelineDSPs) : BaseMultiplier(isSignedX,isSignedY)
{
    char shapeAsChar = ((char) shape) + 'a' - 1; //convert enum to char

    srcFileName = "BaseMultiplierDSPSuperTilesXilinx";
    uniqueName_ = string("BaseMultiplierDSPSuperTilesXilinxShape_") + string(1,shapeAsChar);

    this->shape = shape;
    this->pipelineDSPs = pipelineDSPs;

    switch(shape)
    {
        case SHAPE_A:
            wX = 41;
            wY = 34;
            wR = 42; //not counting the zeros!
            break;
        case SHAPE_B:
            wX = 41;
            wY = 41;
            wR = 42;
            break;
        case SHAPE_C:
            wX = 41;
            wY = 41;
            wR = 42;
            break;
        case SHAPE_D:
            wX = 34;
            wY = 41;
            wR = 42;
            break;
        case SHAPE_E:
            wX = 24;
            wY = 34;
            wR = 58;
//            wR = 41;
            break;
        case SHAPE_F:
            wX = 48;
            wY = 24;
            wR = 58;
            break;
        case SHAPE_G:
            wX = 24;
            wY = 41;
            wR = 58;
            break;
        case SHAPE_H:
            wX = 41;
            wY = 24;
            wR = 58;
            break;
        case SHAPE_I:
            wX = 41;
            wY = 24;
            wR = 58;
            break;
        case SHAPE_J:
            wX = 24;
            wY = 41;
            wR = 58;
            break;
        case SHAPE_K:
            wX = 34;
            wY = 24;
            wR = 58;
            break;
        case SHAPE_L:
            wX = 24;
            wY = 48;
            wR = 58;
            break;
        default:
            throw string("Error in ") + srcFileName + string(": shape unknown");
    }

}

bool BaseMultiplierDSPSuperTilesXilinx::shapeValid(int x, int y)
{
    //check if (x,y) coordinate lies inside of the super tile shape:
    switch(shape)
    {
        case SHAPE_A:
            if((x <= 16) && (y <= 16)) return false;
            if((x >= 24) && (y >= 17)) return false;
            break;
        case SHAPE_B:
            if((x <= 23) && (y <= 23)) return false;
            if((x >= 24) && (y >= 24)) return false;
            break;
        case SHAPE_C:
            if((x <= 16) && (y <= 16)) return false;
            if((x >= 17) && (y >= 17)) return false;
            break;
        case SHAPE_D:
            if((x <= 16) && (y <= 16)) return false;
            if((x >= 24) && (y >= 24)) return false;
            break;
        case SHAPE_E:
              //rectangular shape, nothing to check
            break;
        case SHAPE_F:
            if((x <= 23) && (y <= 6)) return false;
            if((x >= 24) && (y >= 17)) return false;
            break;
        case SHAPE_G:
            if((x >= 17) && (y >= 17)) return false;
            break;
        case SHAPE_H:
            if((x <= 23) && (y <= 6)) return false;
            break;
        case SHAPE_I:
            if((x >= 17) && (y >= 17)) return false;
            break;
        case SHAPE_J:
            if((x <= 6) && (y <= 23)) return false;
            break;
        case SHAPE_K:
            //rectangular shape, nothing to check
            break;
        case SHAPE_L:
            if((x <= 6) && (y <= 23)) return false;
            if((x >= 17) && (y >= 24)) return false;
            break;
        default:
            throw string("Error in ") + srcFileName + string(": shape unknown");
    }

    //check outer bounds:
    if(!((x >= 0) && (x < wX) && (y >= 0) && (y < wY))) return false;

    return true;
}


Operator* BaseMultiplierDSPSuperTilesXilinx::generateOperator(Target* target)
{
    return new BaseMultiplierDSPSuperTilesXilinxOp(target, isSignedX, isSignedY, wX, wY, wR, shape, pipelineDSPs);
}


BaseMultiplierDSPSuperTilesXilinxOp::BaseMultiplierDSPSuperTilesXilinxOp(Target* target, bool isSignedX, bool isSignedY, int wX, int wY, int wR, BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE shape, bool pipelineDSPs) : Operator(target)
{
    useNumericStd();

    char shapeAsChar = ((char) shape) + 'a' - 1; //convert enum to char
    setName(string("BaseMultiplierDSPSuperTilesXilinxShape_") + string(1,shapeAsChar));

    if((isSignedX == true) || (isSignedY == true)) throw string("unsigned inputs currently not supported by BaseMultiplierDSPSuperTilesXilinxOp, sorry");

    declare("D1", 41); //temporary output of the first DSP
    declare("D2", 41); //temporary output of the second DSP

    switch(shape)
    {
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_A:
            //total operation is: (X(23 downto 0) * Y(33 downto 17) << 17) + (X(40 downto 17) * Y(16 downto 0) << 17)
            //output shift is excluded to avoid later increase of bitheap
            //realized as:        ((X(23 downto 0) * Y(33 downto 17) + X(40 downto 17) * Y(16 downto 0))
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(33 downto 17)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(40 downto 17)) * unsigned(Y(16 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_B:
            //total operation is: (X(23 downto 0) * Y(40 downto 24) << 24) + (X(40 downto 24) * Y(23 downto 0) << 24)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(40 downto 24)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(40 downto 24)) * unsigned(Y(23 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_C:
            //total operation is: (X(16 downto 0) * Y(40 downto 17) << 17) + (X(40 downto 17) * Y(16 downto 0) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(16 downto 0)) * unsigned(Y(40 downto 17)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(40 downto 17)) * unsigned(Y(16 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_D:
            //total operation is: (X(16 downto 0) * Y(40 downto 17) << 17) + (X(33 downto 17) * Y(23 downto 0) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(16 downto 0)) * unsigned(Y(40 downto 17)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(33 downto 17)) * unsigned(Y(23 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_E:
            //total operation is: (X(23 downto 0) * Y(16 downto 0)) + (X(23 downto 0) * Y(33 downto 17) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(16 downto 0)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(33 downto 17)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_F:
            //total operation is: ((X(23 downto 0) * Y(23 downto 7)) << 7) + (X(47 downto 24) * Y(16 downto 0) << 24)
            //realized as:        (X(23 downto 0) * Y(23 downto 7) + ((X(47 downto 24) * Y(16 downto 0)) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(23 downto 7)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(47 downto 24)) * unsigned(Y(16 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_G:
            //total operation is: (X(23 downto 0) * Y(16 downto 0)) + ((X(16 downto 0) * Y(40 downto 17)) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(16 downto 0)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(16 downto 0)) * unsigned(Y(40 downto 17)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_H:
            //total operation is: ((X(23 downto 0) * Y(23 downto 7)) << 7) + (X(40 downto 24) * Y(16 downto 0) << 24)
            //realized as:        (X(23 downto 0) * Y(23 downto 7)) + (X(40 downto 24) * Y(16 downto 0) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(23 downto 7)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(40 downto 24)) * unsigned(Y(23 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_I:
            //total operation is: (X(16 downto 0) * Y(23 downto 0) + (X(40 downto 17) * Y(16 downto 0) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(16 downto 0)) * unsigned(Y(23 downto 0)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(40 downto 17)) * unsigned(Y(16 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_J:
            //total operation is: ((X(23 downto 7) * Y(23 downto 0) << 7) + (X(23 downto 0) * Y(40 downto 24) << 24)
            //realized as:        (X(23 downto 7) * Y(23 downto 0)) + (X(23 downto 0) * Y(40 downto 24) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 7)) * unsigned(Y(23 downto 0)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(40 downto 24)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_K:
            //total operation is: (X(16 downto 0) * Y(23 downto 0) + (X(33 downto 17) * Y(23 downto 0) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(16 downto 0)) * unsigned(Y(23 downto 0)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(33 downto 17)) * unsigned(Y(23 downto 0)));" << endl;
            break;
        case BaseMultiplierDSPSuperTilesXilinx::SHAPE_L:
            //total operation is: ((X(23 downto 7) * Y(23 downto 0) << 7) + (X(16 downto 0) * Y(47 downto 24) << 24)
            //realized as:        (X(23 downto 7) * Y(23 downto 0)) + (X(16 downto 0) * Y(47 downto 24) << 17)
            vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 7)) * unsigned(Y(23 downto 0)));" << endl;
            vhdl << tab << "D2 <= std_logic_vector(unsigned(X(16 downto 0)) * unsigned(Y(47 downto 24)));" << endl;
            break;
        default:
            throw string("Error in ") + srcFileName + string(": shape unknown");
    }

    if(pipelineDSPs) //ToDo: decide on target frequency whether to pipeline or not (and how depth)
    {
        nextCycle();
    }

    declare("T",wR);
    if((shape >= BaseMultiplierDSPSuperTilesXilinx::SHAPE_A) && (shape <= BaseMultiplierDSPSuperTilesXilinx::SHAPE_D))
    {
        //tilings (a) to (d) doesn't have a shift
        vhdl << tab << "T <= std_logic_vector(unsigned('0' & D1) + unsigned('0' & D2));" << endl;
    }
    else
    {
        //tilings (e) to (l) have a 17 bit shift
        vhdl << tab << "T(57 downto 17) <= std_logic_vector(unsigned(D2) + unsigned(\"00000000000000000\" & D1(40 downto 17)));" << endl;
        vhdl << tab << "T(16 downto 0) <= D1(16 downto 0);" << endl;
    }

/*
    if(pipelineDSPs) //ToDo: decide on target frequency whether to pipeline or not (and how depth)
    {
        nextCycle();
    }
*/

    vhdl << tab << "R <= T;" << endl;

    addOutput("R", wR);

    addInput("X", wX, true);
    addInput("Y", wY, true);
}

}   //end namespace flopoco

