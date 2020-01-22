#include "BaseMultiplierDSPKarazuba.hpp"


namespace flopoco {


    Operator *BaseMultiplierDSPKarazuba::generateOperator(
            Operator *parentOp,
            Target *target,
            Parametrization const &parameters) const {
        return new BaseMultiplierDSPKarazubaOp(
                parentOp,
                target,
                parameters.getTileXWordSize(),
                parameters.getTileYWordSize(),
                parameters.getShapePara()
        );
    }



    OperatorPtr BaseMultiplierDSPKarazuba::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
    {
        int wX, wY, n;
        UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
        UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
        UserInterface::parseStrictlyPositiveInt(args, "k", &n);

        return new BaseMultiplierDSPKarazubaOp(parentOp,target, wX, wY, n);
    }

    void BaseMultiplierDSPKarazuba::registerFactory()
    {
        UserInterface::add("BaseMultiplierDSPKarazuba", // name
                           "Implements the Karazuba pattern with DSPs where certain multipliers can be omitted to save DSPs",
                           "BasicInteger", // categories
                           "",
                           "wX(int): size of input X of a single DSP-block;\
                        wY(int): size of input Y of a single DSP-block;\
						n(int): size of pattern and number of DSP substitutions;",
                           "",
                           BaseMultiplierDSPKarazuba::parseArguments//,
                           //BaseMultiplierDSPKarazuba::unitTest
        ) ;
    }

    void BaseMultiplierDSPKarazubaOp::emulate(TestCase* tc)
    {
        int gcd = BaseMultiplierDSPKarazubaOp::gcd(wX, wY);
        long kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);

        mpz_class svX = tc->getInputValue("X");
        mpz_class svY = tc->getInputValue("Y");

        unsigned x_mask = 0xffffffff >> (32-wX);
        unsigned y_mask = 0xffffffff >> (32-wY);

        mpz_class a[n+1];
        mpz_class b[n+1];
        mpz_class svR = 0;
        for(int i = 0; i <= n; i++){     //diagonal
            a[i] = (svX >> i*kxy) & x_mask;
            b[i] = (svY >> i*kxy) & y_mask;
            svR += (a[i]*b[i] << 2*i*kxy);
            for(int j = 0; j < i; j++) {     //karazuba substitution
                svR += (((a[j]-a[i])*(b[i]-b[j]) + a[j]*b[j] + a[i]*b[i]) << (j+i)*kxy);
                cout << i << " " << " " << j << " " << ((a[j]-a[i])*(b[i]-b[j]) + a[j]*b[j] + a[i]*b[i]) << endl;
            }
        }

        //mpz_class a0 = svX & x_mask, a6 = (svX >> kxy) & x_mask;
        //mpz_class b0 = svY & y_mask, b6 = (svY >> kxy) & y_mask;
        //mpz_class svR = (a6*b6 << (1+1)*kxy) + (((a0-a6)*(b6-b0) + a6*b6 + a0*b0) << (0+1)*kxy) + (a0*b0 << (0+0)*kxy);
        tc->addExpectedOutput("R", svR);
    }

    BaseMultiplierDSPKarazubaOp::BaseMultiplierDSPKarazubaOp(Operator *parentOp, Target* target, int wX, int wY, int n) : Operator(parentOp,target), wX(wX), wY(wY), wR(wX+wY), n(n)
    {
        int gcd = BaseMultiplierDSPKarazubaOp::gcd(wX, wY);
        if(gcd == 1) THROWERROR("Input word sizes " << wX << " " << wY << " do not have a common divider >1");

        int kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);
        cout << "first possible position " << kxy << endl;

        srcFileName = "BaseMultiplierDSPKarazuba";
        ostringstream name;
        name << "IntKaratsuba_" << wX << "x" << wY << "_order_" << n;
        setNameWithFreqAndUID(name.str() );
        useNumericStd();

        addInput ("X", kxy*n+wX);
        addInput ("Y", kxy*n+wY);
        addOutput("R", 2*kxy*n+wX+wY);

        for(int x=0; x <= n; x++)
        {
            vhdl << tab << declare("a" + to_string(kxy*x/gcd),wX) << " <= X(" << x*kxy+wX-1 << " downto " << x*kxy << ");" << endl;
        }
        for(int y=0; y <= n; y++)
        {
            vhdl << tab << declare("b" + to_string(kxy*y/gcd),wY) << " <= Y(" << y*kxy+wY-1 << " downto " << y*kxy << ");" << endl;
        }

        //BitHeap *bitHeap;
        bitHeap = new BitHeap(this, 2*kxy*n+wX+wY+3);

        for(int xy = 0; xy <= n; xy++){     //diagonal
            //newInstance( "DSPBlock", "dsp" + to_string(kxy*xy/gcd) + "_" + to_string(kxy*xy/gcd), "wX=" + to_string(wX) + " wY=" + to_string(wY) + " isPipelined=0 xIsSigned=0 yIsSigned=0","X=>a" + to_string(kxy*xy/gcd) + ", Y=>b" + to_string(kxy*xy/gcd) + "", "R=>c" + to_string(kxy*xy/gcd) + "_" + to_string(kxy*xy/gcd));
            //bitHeap->addSignal("c" + to_string(kxy*xy/gcd) + "_" + to_string(kxy*xy/gcd), 2*kxy*xy);
        }

        TileBaseMultiple = gcd;

        for(int xy = 0; xy <= n; xy++){     //diagonal
            createMult(kxy*xy/gcd, kxy*xy/gcd);
            for(int nr = 0; nr < xy; nr++) {     //karazuba substitution
                createRectKaratsuba(kxy*nr/gcd,kxy*xy/gcd,kxy*xy/gcd,kxy*nr/gcd);
            }
        }


/*        vhdl << tab << declare("k00", wX+wY) << " <= a0*b0;" << endl;
        vhdl << tab << declare("k66", wX+wY) << " <= a6*b6;" << endl;

        //vhdl << tab << declare("da", wX) << " <= (signed(a0) - signed(a6));" << endl;
        //vhdl << tab << declare("db", wY) << " <= (signed(b6) - signed(b0));" << endl;
        //vhdl << tab << declare("k6_0_0_6", wX+wY) << " <= da*db;" << endl;
        vhdl << tab << declare("k6_0_0_6", wX+wY+2) << " <= (signed(\"0\" & a0) - signed(\"0\" & a6))*(signed(\"0\" & b6) - signed(\"0\" & b0))+signed(k66)+signed(k00);" << endl;
*/        //vhdl << tab << declare("k6_0_0_6", wX+wY+1) << " <= (\"0\" & a6*b0)+(\"0\" & a0*b6);" << endl;
/*        newInstance("DSPBlock", "dsp6_0_0_6",
                    "wX=" + to_string(wX) + " wY=" + to_string(wY) + " isPipelined=0 xIsSigned=0 yIsSigned=0",
                    "X=>da, Y=>db","R=>k6_0_0_6");
*/
/*        vhdl << tab << declare("res", 2*48+wX+wY) << " <= k66 & \"000000\" & k6_0_0_6 & \"00000000\" & k00 ;" << endl;
        bitHeap->addSignal("res", 0);
*/
        //bitHeap->subtractSignal("k6_0_0_6", 6*gcd);
//        bitHeap->addSignal("c0_0", 0);
//        bitHeap->addSignal("c6_6", (6+6)*gcd );



 //       for(int xy = 1; xy <= n; xy++) {

/*            vhdl << tab << declare("d" + to_string((xy-1)*kxy/gcd) + "_" + to_string(xy*kxy/gcd),wY) << " <= std_logic_vector(signed(resize(unsigned(b" << xy*kxy/gcd << "),24))) - signed(resize(unsigned((b" << (xy-1)*kxy/gcd << "),24)));" << endl;
            newInstance("DSPBlock", "dsp" + to_string(kxy*xy/gcd) + "_" + to_string(0) + "_" + to_string(0) + "_" + to_string(kxy*xy/gcd),
                        "wX=" + to_string(wX) + " wY=" + to_string(wY) + " usePreAdder=1 preAdderSubtracts=1 isPipelined=0 xIsSigned=0 yIsSigned=0",
                        "X1=>a" + to_string((xy-1)*kxy/gcd) + ", X2=>a" + to_string(xy*kxy/gcd) + ", Y=>d" + to_string((xy-1)*kxy/gcd) + "_" +
                        to_string(kxy*xy/gcd),
                        "R=>k" + to_string(xy*kxy/gcd) + "_" + to_string((xy-1)*kxy/gcd) + "_" + to_string((xy-1)*kxy/gcd) + "_" + to_string(xy*kxy/gcd));

            bitHeap->addSignal("k" + to_string(xy*kxy/gcd) + "_" + to_string((xy-1)*kxy/gcd) + "_" + to_string((xy-1)*kxy/gcd) + "_" + to_string(xy*kxy/gcd));
            bitHeap->addSignal("c" + to_string((xy-1)*kxy/gcd) + "_" + to_string((xy-1)*kxy/gcd),((xy-1)*kxy/gcd+xy*kxy/gcd)*gcd);
            bitHeap->addSignal("c" + to_string(xy*kxy/gcd) + "_" + to_string(xy*kxy/gcd),((xy-1)*kxy/gcd+xy*kxy/gcd)*gcd);

*/
 /*           int i = kxy*xy/gcd, j = 0, k = 0, l = kxy*xy/gcd;
            vhdl << tab << declare("d" + to_string(i) + "_" + to_string(k),24) << " <= std_logic_vector((b" << j << ") - (b" << l << "));" << endl;
            declare("k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),16+24);

            if(!isSignalDeclared("a" + to_string(l) + "se"))
                vhdl << tab << declare("a" + to_string(l) + "se",16) << " <= std_logic_vector(unsigned(a" << i << "));" << endl;
            if(!isSignalDeclared("a" + to_string(j) + "se"))
                vhdl << tab << declare("a" + to_string(j) + "se",16) << " <= std_logic_vector(unsigned(a" << k << "));" << endl;

            int wDSPOut=16+24;

            newInstance("DSPBlock", "dsp" + to_string(kxy*xy/gcd) + "_" + to_string(0) + "_" + to_string(0) + "_" + to_string(kxy*xy/gcd),
                        "wX=" + to_string(wX) + " wY=" + to_string(wY) + " usePreAdder=1 preAdderSubtracts=1 isPipelined=0 xIsSigned=0 yIsSigned=0",
                        "X1=>a" + to_string(j) + "se, X2=>a" + to_string(l) + "se, Y=>d" + to_string(i) + "_" +
                        to_string(k),
                        "R=>k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l));

            getSignalByName(declare("kr" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),wDSPOut))->setIsSigned();
            vhdl << tab << "kr" << i << "_" << j << "_" << k << "_" << l << " <= k" << i << "_" << j << "_" << k << "_" << l << "(" << wDSPOut-1 << " downto 0);" << endl;
            bitHeap->addSignal("kr" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),(i+j)*gcd);
            bitHeap->addSignal("c" + to_string(i) + "_" + to_string(l),(i+j)*gcd);
            bitHeap->addSignal("c" + to_string(k) + "_" + to_string(j),(i+j)*gcd);*/
//        }

/*
        long X = 0xeafe, Y = 0xbead;
        long a0 = X&0xFF, a1 = (X>>8)&0xFF, b0 = Y&0xFF, b1 = (Y>>8)&0xFF;
        long M00 = a0*b0, M11 = a1*b1, M10 = a1*b0, M01 = a0*b1;
        long a = a1 - a0;
        long b = b1 - b0;
        cout << " " << a << " " << b;
        long MKA = (a0-a1)*(b1 - b0) + M00 + M11;
        long MKB = (a1 + a0)*(b1 + b0) - M00 - M11;
        long RKA = (M11 << 16) + (MKA << 8) + M00;
        long RKB = (M11 << 16) + (MKB << 8) + M00;
        long RNO = (M11 << 16) + (M10 << 8) + (M01 << 8) + M00;
        cout << " org " << RNO << " karazuba " << RKA << " karazuba B " << RKB << " org " << X*Y << endl;
*/

 /*       unsigned x_mask = 0xffffffff >> (32-wX);
        unsigned y_mask = 0xffffffff >> (32-wY);

        mpz_class svX("bbbbffffffffffffeeeeaaaa33332221", 16);
        //mpz_class svX("7fffffffffffffffffffffffffffffff", 16);
        mpz_class svY("7b7befef7b7befef7b7befef7b7befef", 16);

        mpz_class a0 = svX & x_mask, a6 = (svX >> kxy) & x_mask, a12 = (svX >> 2*kxy) & x_mask;
        mpz_class b0 = svY & y_mask, b6 = (svY >> kxy) & y_mask, b12 = (svY >> 2*kxy) & y_mask;

        mpz_class erg = (a12*b12 << (2+2)*kxy) + (a12*b6 << (2+1)*kxy) + (a6*b12 << (1+2)*kxy) + (a12*b0 << (2+0)*kxy) + (a0*b12 << (0+2)*kxy) + (a6*b6 << (1+1)*kxy)  + (a6*b0 << (1+0)*kxy) + (a0*b6 << (0+1)*kxy) + (a0*b0 << (0+0)*kxy);
        //mpz_class erg_ka = (a12*b12 << (2+2)*kxy) + (((a6-a12)*(b12-b6) + a12*b12 + a6*b6) << (1+2)*kxy) +  ((a0*b12 + a12*b0 + a6*b6) << (1+1)*kxy) + (((a0-a6)*(b6-b0) + a6*b6 + a0*b0) << (0+1)*kxy) + (a0*b0 << (0+0)*kxy);
        mpz_class erg_ka = (a12*b12 << (2+2)*kxy) + (((a6-a12)*(b12-b6) + a12*b12 + a6*b6) << (1+2)*kxy) +  (((a0 - a12) * (b12 - b0) + a0 * b0 + a12 * b12 + a6*b6) << (1+1)*kxy) + (((a0-a6)*(b6-b0) + a6*b6 + a0*b0) << (0+1)*kxy) + (a0*b0 << (0+0)*kxy);

        cout << svX.get_str(16) << " * " << svY.get_str(16)  << " = " << erg.get_str(16) << endl;
        cout << svX.get_str(16) << " * " << svY.get_str(16)  << " = " << erg_ka.get_str(16) << endl;
*/

        //compress the bitheap
        bitHeap -> startCompression();

        vhdl << tab << "R" << " <= " << bitHeap->getSumName() <<
             range(2*kxy*n+wX+wY-1, 0) << ";" << endl;

/*        declare("D1", 41); //temporary output of the first DSP
        declare("D2", 41); //temporary output of the second DSP
        vhdl << tab << "D1 <= std_logic_vector(unsigned(X(23 downto 0)) * unsigned(Y(33 downto 17)));" << endl;
        vhdl << tab << "D2 <= std_logic_vector(unsigned(X(40 downto 17)) * unsigned(Y(16 downto 0)));" << endl;
        declare("T",BaseMultiplierDSPKarazuba::get_wR(k));
        vhdl << tab << "T(57 downto 17) <= std_logic_vector(unsigned(D2) + unsigned(\"00000000000000000\" & D1(40 downto 17)));" << endl;
        vhdl << tab << "T(16 downto 0) <= D1(16 downto 0);" << endl;
        vhdl << tab << "R <= T;" << endl;
        addOutput("R", BaseMultiplierDSPKarazuba::get_wR(k));
        addInput("X", BaseMultiplierDSPKarazuba::get_wX(k), true);
        addInput("Y", BaseMultiplierDSPKarazuba::get_wY(k), true);*/
    }


    void BaseMultiplierDSPKarazubaOp::createMult(int i, int j)
    {
        REPORT(DEBUG, "implementing a" << i << " * b" << j << " with weight " << (i+j)*TileBaseMultiple << " (" << (i+j) << " x " << TileBaseMultiple << ")");
        if(!isSignalDeclared("a" + to_string(i) + "se"))
            vhdl << tab << declare("a" + to_string(i) + "se",18) << " <= std_logic_vector(resize(unsigned(a" << i << "),18));" << endl;
        if(!isSignalDeclared("b" + to_string(j) + "se"))
            vhdl << tab << declare("b" + to_string(j) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << j << "),25));" << endl;

        newInstance( "DSPBlock", "dsp" + to_string(i) + "_" + to_string(j), "wX=25 wY=18 usePreAdder=0 preAdderSubtracts=0 isPipelined=0 xIsSigned=1 yIsSigned=1","X=>b" + to_string(j) + "se, Y=>a" + to_string(i) + "se", "R=>c" + to_string(i) + "_" + to_string(j));

        bitHeap->addSignal("c" + to_string(i) + "_" + to_string(j),(i+j)*TileBaseMultiple);
    }

    void BaseMultiplierDSPKarazubaOp::createRectKaratsuba(int i, int j, int k, int l)
    {
        REPORT(FULL, "createRectKaratsuba(" << i << "," << j << "," << k << "," << l << ")");

        REPORT(INFO, "implementing a" << i << " * b" << j << " + a" << k << " * b" << l << " with weight " << (i+j) << " as (a" << i << " - a" << k << ") * (b" << j << " - b" << l << ") + a" << i << " * b" << l << " + a" << k << " * b" << j);

        if(!isSignalDeclared("d" + to_string(i) + "_" + to_string(k)))
            vhdl << tab << declare("d" + to_string(i) + "_" + to_string(k),18) << " <= std_logic_vector(signed(resize(unsigned(a" << i << ")," << 18 << ")) - signed(resize(unsigned(a" << k << ")," << 18 << ")));" << endl;
        declare("k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),43);

        if(!isSignalDeclared("b" + to_string(l) + "se"))
            vhdl << tab << declare("b" + to_string(l) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << l << "),25));" << endl;
        if(!isSignalDeclared("b" + to_string(j) + "se"))
            vhdl << tab << declare("b" + to_string(j) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << j << "),25));" << endl;

        newInstance( "DSPBlock", "dsp" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l), "wX=25 wY=18 usePreAdder=1 preAdderSubtracts=1 isPipelined=0 xIsSigned=1 yIsSigned=1","X1=>b" + to_string(j) + "se, X2=>b" + to_string(l) + "se, Y=>d" + to_string(i) + "_" + to_string(k), "R=>k" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l));

        int wDSPOut=41; //18+24-1=41 bits necessary

        getSignalByName(declare("kr" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),wDSPOut))->setIsSigned();
        vhdl << tab << "kr" << i << "_" << j << "_" << k << "_" << l << " <= k" << i << "_" << j << "_" << k << "_" << l << "(" << wDSPOut-1 << " downto 0);" << endl;
        bitHeap->addSignal("kr" + to_string(i) + "_" + to_string(j) + "_" + to_string(k) + "_" + to_string(l),(i+j)*TileBaseMultiple);
        bitHeap->addSignal("c" + to_string(i) + "_" + to_string(l),(i+j)*TileBaseMultiple);
        bitHeap->addSignal("c" + to_string(k) + "_" + to_string(j),(i+j)*TileBaseMultiple);

    }


}