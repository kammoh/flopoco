#ifndef BaseMultiplierDSPSuperTilesXilinx_HPP
#define BaseMultiplierDSPSuperTilesXilinx_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {

	/**
	* \brief Implementation of all super tiles of size 2 for Xilinx FPGAs according to "Resource Optimal Design of Large Multipliers for FPGAs", Martin Kumm & Johannes Kappauf 
    **/

    class BaseMultiplierDSPSuperTilesXilinx : public BaseMultiplier
    {
    public:
    	/**
    	* \brief The shape enum. For their definition, see Fig. 5 of "Resource Optimal Design of Large Multipliers for FPGAs", Martin Kumm & Johannes Kappauf 
        **/
        enum TILE_SHAPE{
            SHAPE_A =  1, //shape (a)
            SHAPE_B =  2, //shape (b)
            SHAPE_C =  3, //shape (c)
            SHAPE_D =  4, //shape (d)
            SHAPE_E =  5, //shape (e)
            SHAPE_F =  6, //shape (f)
            SHAPE_G =  7, //shape (g)
            SHAPE_H =  8, //shape (h)
            SHAPE_I =  9, //shape (i)
            SHAPE_J = 10, //shape (j)
            SHAPE_K = 11, //shape (k)
            SHAPE_L = 12  //shape (l)
        };
	public:
        BaseMultiplierDSPSuperTilesXilinx(bool isSignedX, bool isSignedY, TILE_SHAPE shape, bool pipelineDSPs);

        virtual Operator *generateOperator(Target *target);

        virtual bool shapeValid(int x, int y);

    private:
        TILE_SHAPE shape;
        bool pipelineDSPs;
	};

    class BaseMultiplierDSPSuperTilesXilinxOp : public Operator
    {
    public:
        BaseMultiplierDSPSuperTilesXilinxOp(Target* target, bool isSignedX, bool isSignedY, int wX, int wY, int wR, BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE shape, bool pipelineDSPs);
    private:
        int wX, wY, wR;
    };

}
#endif
