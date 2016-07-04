#ifndef Tester_hpp
#define Tester_hpp

#include <stdio.h>
#include <string>
#include "../UserInterface.hpp"

using namespace std;

namespace flopoco{

	class Tester
	{

	public:

		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static void registerFactory();

		Tester(string opName, bool testDependences = false);

	};
};
#endif
