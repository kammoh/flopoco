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
                int shape
        ) : BaseMultiplierCategory{
        get_wX(shape),
        get_wY(shape),
        false,
        false,
        shape,
        "BaseMultiplierDSPKarazuba_" + string(1,((char) shape) + 'A' - 1),
        false
        }{
            this->wX = get_wX(shape);
            this->wY = get_wY(shape);
            this->wR = get_wR(shape);
        }



        static int get_wX(int shape) {return 16;}
        static int get_wY(int shape) {return 24;}
        static int get_wR(int shape) {return 16+24;}
        static int getRelativeResultMSBWeight(int shape) {return 1;}
        static int getRelativeResultLSBWeight(int shape) {return 1;}

        Operator *generateOperator(Operator *parentOp, Target *target, Parametrization const & params) const final;

        /** Factory method */
        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
        /** Register the factory */
        static void registerFactory();

    private:
        int wX, wY, wR, n;
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
