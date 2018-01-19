#ifndef Xilinx_LUT5_H
#define Xilinx_LUT5_H

#include "Xilinx_Primitive.hpp"

#include "Xilinx_LUT_compute.h"

namespace flopoco {
    class Xilinx_LUT5_base : public Xilinx_Primitive {
      public:
        Xilinx_LUT5_base( Target *target );
        ~Xilinx_LUT5_base() {};
        void base_init();
    };


    class Xilinx_LUT5 : public Xilinx_LUT5_base {
      public:
        Xilinx_LUT5( Target *target );
        ~Xilinx_LUT5() {};
    };

    class Xilinx_LUT5_L : public Xilinx_LUT5_base {
      public:
        Xilinx_LUT5_L( Target *target );
        ~Xilinx_LUT5_L() {};
    };

    class Xilinx_LUT5_D : public Xilinx_LUT5_base {
      public:
        Xilinx_LUT5_D( Target *target );
        ~Xilinx_LUT5_D() {};
    };

    class Xilinx_CFGLUT5 : public Xilinx_LUT5_base {
      public:
        Xilinx_CFGLUT5( Target *target );
        ~Xilinx_CFGLUT5() {};
    };

}//namespace

#endif
