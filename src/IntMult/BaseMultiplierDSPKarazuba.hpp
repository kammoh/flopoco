#ifndef FLOPOCO_BASEMULTIPLIERDSPKARAZUBA_HPP
#define FLOPOCO_BASEMULTIPLIERDSPKARAZUBA_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "mpfr.h"
#include "Target.hpp"
#include "Operator.hpp"
#include "BitHeap/BitHeap.hpp"
#include "Table.hpp"
#include "BaseMultiplierCategory.hpp"

namespace flopoco {


    class BaseMultiplierDSPKarazuba : public BaseMultiplierCategory
    {
    public:

        BaseMultiplierDSPKarazuba(
                int size
        ) : BaseMultiplierCategory{
        get_wX(size),
        get_wY(size),
        false,
        false,
        size,
        "BaseMultiplierDSPKarazuba_size" + string(1,((char) size) + '0'),
        false
        }{
            wX = 16;
            wY = 24;
            wR = 16+24;
            n = size;
        }

        static int get_wX(int n) {return fpr(16, 24, gcd(16, 24))*n+16;}
        static int get_wY(int n) {return fpr(16, 24, gcd(16, 24))*n+24;}
        static int get_wR(int n) {return get_wX(n) + get_wY(n);}
        static int getRelativeResultMSBWeight(int n) {return get_wR(n);}
        static int getRelativeResultLSBWeight(int n) {return 0;}
        int getDSPCost() const final;
        double getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY);
        bool shapeValid(const Parametrization &param, unsigned int x, unsigned int y) const;
        bool shapeValid(int x, int y);
        bool isIrregular() const override { return true;}



        Operator *generateOperator(Operator *parentOp, Target *target, Parametrization const & params) const final;

        /** Factory method */
        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
        /** Register the factory */
        static void registerFactory();

    private:
        int wX, wY, wR, n;
        static int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b);}
        static long fpr(int wX, int wY, int gcd) {long kxy = gcd; for(; kxy % wX || kxy % wY; kxy += gcd); return kxy;}

    };

    class BaseMultiplierDSPKarazubaOp : public Operator
    {
    public:

        BaseMultiplierDSPKarazubaOp(Operator *parentOp, Target* target, int wX, int wY, int k);
        void emulate(TestCase * tc);
    protected:
        BitHeap *bitHeap;
    private:
        int wX, wY, wR, n;
        static int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b);}

        int TileBaseMultiple;
        void createRectKaratsuba(int i, int j, int k, int l);
        void createMult(int i, int j);
    };

}
#endif //FLOPOCO_BASEMULTIPLIERDSPKARAZUBA_HPP
