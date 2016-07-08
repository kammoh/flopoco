#ifndef FPADD_HPP
#define FPADD_HPP
#include "../Operator.hpp"
#include "FPAddSinglePath.hpp"
#include "FPAddDualPath.hpp"

namespace flopoco{
	class FPAdd {
	public:
		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		/** Create the next TestState based on the previous TestState */
		static void nextTestState(TestState * previousTestState);

		/** Factory register method */ 
		static void registerFactory();
		
	};
}
#endif
