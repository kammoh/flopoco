#include "PrimitiveComponents/Xilinx/XilinxGPC.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"
#include <string>

using namespace std;

namespace flopoco{

XilinxGPC::XilinxGPC(Operator* parentOp, Target * target, vector<int> heights) : Compressor(parentOp, target)
{
    this->heights = heights;
    ostringstream name;

    //compressors are supposed to be combinatorial
    setCombinatorial();
    setShared();

    //remove the zero columns at the lsb
    while(heights[0] == 0)
    {
        heights.erase(heights.begin());
    }

    setWordSizes();
    createInputsAndOutputs();

    //set name:
    name << "XilinxGPC_";
    for(int i=heights.size()-1; i>=0; i--)
        name << heights[i];

    name << "_" << wOut;
    setName(name.str());

//    vhdl << tab << "R <= (others => '0');" << endl;
/*
    declare("s",5);
    declare("cc_co",4);
    declare("cc_o",4);
*/
	declare("cc_di",4);
	declare("cc_s",4);

	vector<string> lutOp(4);
	vector<vector<string> > lutInputMappings(4,vector<string>(4));
	vector<string> ccDInputMappings(4);

	if((heights[2] == 6) && (heights[1] == 0) && (heights[0] == 6))
	{
		lutOp[0] = "6996966996696996"; //config h
		lutOp[1] = "177E7EE8FFF0F000"; //config i
		lutOp[2] = "6996966996696996"; //config h
		lutOp[3] = "177E7EE8FFF0F000"; //config i

		lutInputMappings[0] = {"X0" + of(0),"X0" + of(1),"X0" + of(2),"X0" + of(3),"X0" + of(4),"X0" + of(5)};
		lutInputMappings[1] = {"X0" + of(1),"X0" + of(2),"X0" + of(3),"X0" + of(4),"X0" + of(5),"'1'"};
		lutInputMappings[2] = {"X2" + of(0),"X2" + of(1),"X2" + of(2),"X2" + of(3),"X2" + of(4),"X2" + of(5)};
		lutInputMappings[3] = {"X2" + of(1),"X2" + of(2),"X2" + of(3),"X2" + of(4),"X2" + of(5),"'1'"};

		ccDInputMappings = {"X0" + of(0),"O5","X2" + of(0),"O5"};
	}
	else
	{
		stringstream s;
		s << "Unsupported GPC with column heights: ";
		for(int i=heights.size()-1; i >= 0; i--)
		{
			s << heights[i] << " ";
		}
		THROWERROR(s.str());
	}
	//build LUTs for one slice
	for(int i=0; i <= 3; i++)
    {
        //LUT content of the LUTs exept the last LUT:
//        lut_op lutop_o6 = lut_in(0) ^ lut_in(1) ^ lut_in(2) ^ lut_in(3); //sum out of full adder xor input 3
//        lut_op lutop_o5 = (lut_in(0) & lut_in(1)) | (lut_in(0) & lut_in(2)) | (lut_in(1) & lut_in(2)); //carry out of full adder

        Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2(this,target);
		cur_lut->setGeneric( "init", "x\"" + lutOp[i] + "\"", 64 );

		for(int j=0; j <= 5; j++)
		{
			if(lutInputMappings[i][j].find("'") == string::npos)
			{
				//input is a dynamic signal
				inPortMap(cur_lut,"i" + to_string(j),lutInputMappings[i][j]);
			}
			else
			{
				//input is a constant
				inPortMapCst(cur_lut,"i" + to_string(j),lutInputMappings[i][j]);
			}

		}

		if(ccDInputMappings[i] == "O5")
		{
			outPortMap(cur_lut,"o5","cc_di" + of(i));
		}
		else
		{
			outPortMap(cur_lut,"o5","open");
			vhdl << tab << "cc_di(" << i << ") <= " << ccDInputMappings[i] << ";" << endl;
		}
        outPortMap(cur_lut,"o6","cc_s" + of(i));

        vhdl << cur_lut->primitiveInstance(join("lut",i)) << endl;
    }

    Xilinx_CARRY4 *cur_cc = new Xilinx_CARRY4(this,target);

    inPortMapCst( cur_cc, "cyinit", "'0'" );
    inPortMapCst( cur_cc, "ci", "'0'" ); //carry-in can not be used as AX input is blocked!!
//    inPortMap( cur_cc, "ci", "cc_co" + of( i * 4 - 1 ) );

    inPortMap( cur_cc, "di", "cc_di");
    inPortMap( cur_cc, "s", "cc_s");
    outPortMap( cur_cc, "co", "cc_co");
    outPortMap( cur_cc, "o", "cc_o");

    vhdl << cur_cc->primitiveInstance("cc") << endl;

	vhdl << tab << "R <= cc_co(3) & cc_o;" << endl;
}

OperatorPtr XilinxGPC::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
    string in;
    vector<int> heights;

    UserInterface::parseString(args, "columnHeights", &in);

    // tokenize the string, with ':' as a separator
    stringstream ss(in);
    while(ss.good())
    {
        string substr;

        getline(ss, substr, ',');
        heights.insert(heights.begin(), stoi(substr));
    }

    return new XilinxGPC(parentOp,target,heights);
}

void XilinxGPC::registerFactory(){
    UserInterface::add("XilinxGPC", // name
                       "Implements Xilinx optimized GPCs\\ \
                       Available GPC sizes are: \
                       (6,0,6;5)",
                       "Primitives", // categories
                       "",
                       "columnHeights(string): comma separated list of heights for the columns of the compressor, \
in decreasing order of the weight. For example, columnHeights=\"6,0,6\" produces a (6,0,6:5) GPC",
                       "",
                       XilinxGPC::parseArguments
                       ) ;
}

double BasicXilinxGPC::getEfficiency(unsigned int middleLength){
    if(type.compare("variable") != 0){
        int inputBits = 0;
        int outputBits = 0;
        double ratio = 0.0;
        for(unsigned int j = 0; j < heights.size(); j++){
            inputBits += heights[j];
        }
        for(unsigned int j = 0; j < outHeights.size(); j++){
            outputBits += outHeights[j];
        }
        ratio = (double)inputBits - (double)outputBits;
        if(area != 0.0){
            ratio /= (double)area;
        }
        else{
            //area (and therefore cost) is zero. Therefore set ratio to zero.
            ratio = 0.0;
        }
        return ratio;
    }
    else{
        //TODO
        return 0.0;
    }
}

}
