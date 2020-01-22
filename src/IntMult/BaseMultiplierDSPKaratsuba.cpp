#include "BaseMultiplierDSPKaratsuba.hpp"


namespace flopoco {


    Operator *BaseMultiplierDSPKaratsuba::generateOperator(
            Operator *parentOp,
            Target *target,
            Parametrization const &parameters) const {
        return new BaseMultiplierDSPKaratsubaOp(
                parentOp,
                target,
                parameters.getTileXWordSize(),
                parameters.getTileYWordSize(),
                parameters.getShapePara()
        );
    }

    float BaseMultiplierDSPKaratsuba::shape_utilisation(int shape_x, int shape_y, int wX, int wY, bool signedIO){
        return 1;
    }

    int BaseMultiplierDSPKaratsuba::getDSPCost() const {
        int dsps = 0;
        int gcd = BaseMultiplierDSPKaratsuba::gcd(wX, wY);
        long kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);
        for(int j = 0; j <= n; j++){
            for(int i = 0; i <= j; i++){
                dsps++;
            }
        }
        return dsps;
    }

    double BaseMultiplierDSPKaratsuba::getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY){
        int bits = 0;
        int gcd = BaseMultiplierDSPKaratsuba::gcd(wX, wY);
        long kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);
        for(int j = 0; j <= n; j++){
            for(int i = 0; i <= j; i++){
                bits += (wX+wY);
            }
        }                   //TODO: include costs for pre substraction
        return bits*0.65;   //TODO: consider protrusion of multiplier over the bounds of the complete multiplier
    }

    bool BaseMultiplierDSPKaratsuba::shapeValid(Parametrization const& param, unsigned x, unsigned y) const
    {
        int gcd = BaseMultiplierDSPKaratsuba::gcd(wX, wY);
        long kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);
        for(int j = 0; j <= param.getShapePara(); j++){
            for(int i = 0; i <= param.getShapePara(); i++){
                if(i*kxy <= x && x < i*kxy+wX && j*kxy <= y && y < j*kxy+wY )
                    return true;
            }
        }
        return false;
    }

    bool BaseMultiplierDSPKaratsuba::shapeValid(int x, int y)
    {
        int gcd = BaseMultiplierDSPKaratsuba::gcd(wX, wY);
        long kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);
        for(int j = 0; j <= n; j++){
            for(int i = 0; i <= n; i++){
                if(i*kxy <= x && x < i*kxy+wX && j*kxy <= y && y < j*kxy+wY )
                    return true;
            }
        }
        return false;
    }

    OperatorPtr BaseMultiplierDSPKaratsuba::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
    {
        int wX, wY, n;
        UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
        UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
        UserInterface::parseStrictlyPositiveInt(args, "k", &n);

        return new BaseMultiplierDSPKaratsubaOp(parentOp,target, wX, wY, n);
    }

    void BaseMultiplierDSPKaratsuba::registerFactory()
    {
        UserInterface::add("BaseMultiplierDSPKaratsuba", // name
                           "Implements the Karatsuba pattern with DSPs where certain multipliers can be omitted to save DSPs",
                           "BasicInteger", // categories
                           "",
                           "wX(int): size of input X of a single DSP-block;\
                        wY(int): size of input Y of a single DSP-block;\
						n(int): size of pattern and number of DSP substitutions;",
                           "",
                           BaseMultiplierDSPKaratsuba::parseArguments//,
                           //BaseMultiplierDSPKaratsuba::unitTest
        ) ;
    }

    void BaseMultiplierDSPKaratsubaOp::emulate(TestCase* tc)
    {
        int gcd = BaseMultiplierDSPKaratsubaOp::gcd(wX, wY);
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
            for(int j = 0; j < i; j++) {     //karatsuba substitution
                svR += (((a[j]-a[i])*(b[i]-b[j]) + a[j]*b[j] + a[i]*b[i]) << (j+i)*kxy);
                //cout << i << " " << " " << j << " " << ((a[j]-a[i])*(b[i]-b[j]) + a[j]*b[j] + a[i]*b[i]) << endl;
            }
        }

        tc->addExpectedOutput("R", svR);
    }

    BaseMultiplierDSPKaratsubaOp::BaseMultiplierDSPKaratsubaOp(Operator *parentOp, Target* target, int wX, int wY, int n) : Operator(parentOp,target), wX(wX), wY(wY), wR(wX+wY), n(n)
    {
        int gcd = BaseMultiplierDSPKaratsubaOp::gcd(wX, wY);
        if(gcd == 1) THROWERROR("Input word sizes " << wX << " " << wY << " do not have a common divider >1");

        int kxy = gcd;
        for(; kxy % wX || kxy % wY; kxy += gcd);
        cout << "first possible position " << kxy << endl;

        srcFileName = "BaseMultiplierDSPKaratsuba";
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

        TileBaseMultiple = gcd;

        for(int xy = 0; xy <= n; xy++){     //diagonal
            createMult(kxy*xy/gcd, kxy*xy/gcd);
            for(int nr = 0; nr < xy; nr++) {     //karatsuba substitution
                createRectKaratsuba(kxy*nr/gcd,kxy*xy/gcd,kxy*xy/gcd,kxy*nr/gcd);
            }
        }

        //compress the bitheap
        bitHeap -> startCompression();

        vhdl << tab << "R" << " <= " << bitHeap->getSumName() <<
             range(2*kxy*n+wX+wY-1, 0) << ";" << endl;

    }


    void BaseMultiplierDSPKaratsubaOp::createMult(int i, int j)
    {
        REPORT(DEBUG, "implementing a" << i << " * b" << j << " with weight " << (i+j)*TileBaseMultiple << " (" << (i+j) << " x " << TileBaseMultiple << ")");
        if(!isSignalDeclared("a" + to_string(i) + "se"))
            vhdl << tab << declare("a" + to_string(i) + "se",18) << " <= std_logic_vector(resize(unsigned(a" << i << "),18));" << endl;
        if(!isSignalDeclared("b" + to_string(j) + "se"))
            vhdl << tab << declare("b" + to_string(j) + "se",25) << " <= std_logic_vector(resize(unsigned(b" << j << "),25));" << endl;

        newInstance( "DSPBlock", "dsp" + to_string(i) + "_" + to_string(j), "wX=25 wY=18 usePreAdder=0 preAdderSubtracts=0 isPipelined=0 xIsSigned=1 yIsSigned=1","X=>b" + to_string(j) + "se, Y=>a" + to_string(i) + "se", "R=>c" + to_string(i) + "_" + to_string(j));

        bitHeap->addSignal("c" + to_string(i) + "_" + to_string(j),(i+j)*TileBaseMultiple);
    }

    void BaseMultiplierDSPKaratsubaOp::createRectKaratsuba(int i, int j, int k, int l)
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