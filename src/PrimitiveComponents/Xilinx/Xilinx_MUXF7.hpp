#ifndef Xilinx_MUXF7_H
#define Xilinx_MUXF7_H

#include "Xilinx_Primitive.hpp"

namespace flopoco {
    class Xilinx_MUXF7_base : public Xilinx_Primitive {
      public:
        Xilinx_MUXF7_base( Target *target );
        ~Xilinx_MUXF7_base() {};

        void base_init();
    };

    class Xilinx_MUXF7 : public Xilinx_MUXF7_base {
      public:
        Xilinx_MUXF7( Target *target );
        ~Xilinx_MUXF7() {};
    };

    class Xilinx_MUXF7_L : public Xilinx_MUXF7_base {
      public:
        Xilinx_MUXF7_L( Target *target );
        ~Xilinx_MUXF7_L() {};
    };

    class Xilinx_MUXF7_D : public Xilinx_MUXF7_base {
      public:
        Xilinx_MUXF7_D( Target *target );
        ~Xilinx_MUXF7_D() {};
    };
}//namespace

#endif
