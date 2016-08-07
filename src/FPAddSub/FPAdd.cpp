#include "FPAdd.hpp"
#include <Operator.hpp>


using namespace std;
namespace flopoco{

	OperatorPtr FPAdd::parseArguments(Target *target, vector<string> &args) {
		int wE, wF;
		bool sub, dualPath;
		UserInterface::parseStrictlyPositiveInt(args, "wE", &wE); 
		UserInterface::parseStrictlyPositiveInt(args, "wF", &wF);
		UserInterface::parseBoolean(args, "sub", &sub);
		UserInterface::parseBoolean(args, "dualPath", &dualPath);
		if(dualPath)
			return new FPAddDualPath(target, wE, wF, sub);
		else
			return new FPAddSinglePath(target, wE, wF, sub);
	}

	TestList FPAdd::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int wF=5; wF<53; wF+=1) // test various input widths
			{
				for(int dualPath = 0; dualPath <2; dualPath++)
				{
					int nbByteWE = 6+(wF/10);
					while(nbByteWE>wF)
					{
						nbByteWE -= 2;
					}

					paramList.push_back(make_pair("wF",to_string(wF)));
					paramList.push_back(make_pair("wE",to_string(nbByteWE)));
					if(dualPath == 1)
						paramList.push_back(make_pair("dualPath","true"));
					else
						paramList.push_back(make_pair("dualPath","false"));
					testStateList.push_back(paramList);
					paramList.clear();
				}
			}
		}
		else     
		{
				// finite number of random test computed out of index
		}	

		return testStateList;
	}

	void FPAdd::registerFactory(){
		UserInterface::add("FPAdd", // name
			"A correctly rounded floating-point adder.",
			"BasicFloatingPoint",
			"", //seeAlso
			"wE(int): exponent size in bits; \
			wF(int): mantissa size in bits; \
			sub(bool)=false: implement a floating-point subtractor instead of an adder;\
			dualPath(bool)=false: use a dual-path algorithm, more expensive but shorter latency;",
			"Single-path is lower hardware, longer latency than dual-path.<br> The difference between single-path and dual-path is well explained in textbooks such as Ercegovac and Lang's <em>Digital Arithmetic</em>, or Muller et al's <em>Handbook of floating-point arithmetic.</em>",
			FPAdd::parseArguments,
			FPAdd::unitTest
			) ;
	}
}
