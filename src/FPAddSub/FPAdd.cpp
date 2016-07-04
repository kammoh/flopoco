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

	void FPAdd::nextTestState(TestState * previousTestState)
	{
		static vector<vector<pair<string,string>>> testStateList;
		vector<pair<string,string>> paramList;

		if(previousTestState->getIterationIndex() == 0)
		{
			previousTestState->setTestBenchSize(100);
			for(int j = 0; j<2; j++)
			{
				paramList.push_back(make_pair("wE","8"));
				paramList.push_back(make_pair("wF","23"));
				if(j==1)
				{
					paramList.push_back(make_pair("dualPath","true"));
				}
				testStateList.push_back(paramList);
				paramList.clear();
				paramList.push_back(make_pair("wE","11"));
				paramList.push_back(make_pair("wF","52"));
				if(j==1)
				{
					paramList.push_back(make_pair("dualPath","true"));
				}
				testStateList.push_back(paramList);


				for(int i = 5; i<53; i++)
				{
					int nbByteWE = 6+(i/10);
					while(nbByteWE>i)
					{
						nbByteWE -= 2;
					}
					paramList.clear();
					paramList.push_back(make_pair("wF",to_string(i)));
					paramList.push_back(make_pair("wE",to_string(nbByteWE)));
					if(j==1)
					{
						paramList.push_back(make_pair("dualPath","true"));
					}
					testStateList.push_back(paramList);
				}
			}
			previousTestState->setIterationNumber(testStateList.size());
		}

		vector<pair<string,string>>::iterator itVector;
		int indexIteration = previousTestState->getIterationIndex();

		for(itVector = testStateList[indexIteration].begin(); itVector != testStateList[indexIteration].end(); ++itVector)
		{
			previousTestState->changeValue((*itVector).first,(*itVector).second);
		}
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
			FPAdd::nextTestState
			) ;
	}
}
