#include "PrimitiveComponents/Xilinx/XilinxGPC.hpp"
#include <string>

using namespace std;

namespace flopoco{

XilinxGPC::XilinxGPC(Target * target, vector<int> heights) : Compressor(target)
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

    vhdl << tab << "R <= (others => '0');" << endl;
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

    return new XilinxGPC(target,heights);
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
