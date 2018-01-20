#ifndef Xilinx_LUT6_H
#define Xilinx_LUT6_H

#include "Xilinx_Primitive.hpp"

#include "Xilinx_LUT_compute.h"

namespace flopoco {
    enum LUT6_VARIANT {
        LUT6_GENERAL_OUT,
        LUT6_TWO_OUT,
        LUT6_LOCAL_OUT,
        LUT6_GENERAL_AND_LOCAL_OUT
    };
    // new operator class declaration
    class Xilinx_LUT6_base : public Xilinx_Primitive {
      public:
        Xilinx_LUT6_base( Target *target );
        ~Xilinx_LUT6_base() {};

        void base_init();
    };

    class Xilinx_LUT6 : public Xilinx_LUT6_base {
      public:
        Xilinx_LUT6( Target *target );
        ~Xilinx_LUT6() {};
    };

    class Xilinx_LUT6_2 : public Xilinx_LUT6_base {
      public:
        Xilinx_LUT6_2( Target *target );
        ~Xilinx_LUT6_2() {};
    };

    class Xilinx_LUT6_L : public Xilinx_LUT6_base {
      public:
        Xilinx_LUT6_L( Target *target );
        ~Xilinx_LUT6_L() {};
    };

    class Xilinx_LUT6_D : public Xilinx_LUT6_base {
      public:
        Xilinx_LUT6_D( Target *target );
        ~Xilinx_LUT6_D() {};
    };
}//namespace

#endif
