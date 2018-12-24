#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "FixHalfSine.hpp"

using namespace std;

namespace flopoco{


	FixHalfSine::FixHalfSine(OperatorPtr parentOp, Target* target, int lsb_, int N_) :
		FixFIR(parentOp, target, lsb_, true /*rescale*/),    N(N_)
	{
		srcFileName="FixHalfSine";
		
		ostringstream name;
		name << "FixHalfSine_" << -lsb_  << "_" << N << "_uid" << getNewUId();
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Louis Besème, Florent de Dinechin, Matei Istoan (2014)");

		// define the coefficients
		for (int i=1; i<2*N; i++) {
			ostringstream c;
			c <<  "sin(pi*" << i << "/" << 2*N << ")";
			coeff.push_back(c.str());
		}

		buildVHDL();
	};

	FixHalfSine::~FixHalfSine(){}

	OperatorPtr FixHalfSine::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int lsbInOut;
		UserInterface::parseInt(args, "lsbInOut", &lsbInOut);
		int n;
		UserInterface::parseStrictlyPositiveInt(args, "n", &n);
		OperatorPtr tmpOp = new FixHalfSine(parentOp, target, lsbInOut, n);
		return tmpOp;
	}

	void FixHalfSine::registerFactory(){
		UserInterface::add("FixHalfSine", // name
											 "A generator of fixed-point Half-Sine filters",
											 "FiltersEtc", // categories
											 "",
											 "lsbInOut(int): integer size in bits;\
                        n(int): number of taps",
											 "For more details, see <a href=\"bib/flopoco.html#DinIstoMas2014-SOPCJR\">this article</a>.",
											 FixHalfSine::parseArguments
											 ) ;
	}

}


