#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "FixHalfSine.hpp"

using namespace std;

namespace flopoco{


	FixHalfSine::FixHalfSine(Target* target, int lsb_, int N_) :
		FixFIR(target, lsb_), N(N_)
	{
		srcFileName="FixHalfSine";
		
		ostringstream name;
		name << "FixHalfSine_" << p << "_" << N << "_uid" << getNewUId();
		setName(name.str());

		setCopyrightString("Florent de Dinechin, Matei Istoan (2014)");

		// define the coefficients
		for (int i=1; i<2*N; i++) {
			ostringstream c;
			c <<  "sin(pi*" << i << "/" << 2*N << ")";
			coeff.push_back(c.str());
		}

		buildVHDL();
	};


}

