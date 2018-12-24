#ifndef FIXHALFSINE_HPP
#define FIXHALFSINE_HPP

#include "Operator.hpp"
#include "FixFIR.hpp"
#include "utils.hpp"

#include <iostream>
#include <sstream>
#include <vector>

#include <string.h>


namespace flopoco{

	class FixHalfSine : public FixFIR
	{
	public:

		FixHalfSine(OperatorPtr parentOp, Target* target, int lsb_, int N_);

		virtual ~FixHalfSine();

		/** Factory method */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
		static void registerFactory();

	private: 
		int N; /* FixFIR::n = 2*N */
	};

}


#endif
