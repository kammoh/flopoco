#ifndef Xilinx_MUXF8_H
#define Xilinx_MUXF8_H

#include "Xilinx_Primitive.hpp"

namespace flopoco {
    class Xilinx_MUXF8_base : public Xilinx_Primitive {
      public:
        Xilinx_MUXF8_base( Target *target );
        ~Xilinx_MUXF8_base() {};

        void base_init();
    };

    class Xilinx_MUXF8 : public Xilinx_MUXF8_base {
      public:
        Xilinx_MUXF8( Target *target );
        ~Xilinx_MUXF8() {};
    };

    class Xilinx_MUXF8_L : public Xilinx_MUXF8_base {
      public:
        Xilinx_MUXF8_L( Target *target );
        ~Xilinx_MUXF8_L() {};
    };

    class Xilinx_MUXF8_D : public Xilinx_MUXF8_base {
      public:
        Xilinx_MUXF8_D( Target *target );
        ~Xilinx_MUXF8_D() {};
    };
}//namespace

#endif
