#ifndef _FIXSINCOS_H
#define _FIXSINCOS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gmpxx.h>

#include "Operator.hpp"

#include "utils.hpp"


namespace flopoco{



	class FixSinCos: public Operator {
		FixSinCos(OperatorPtr parentOp, Target * target, int w);
	
		~FixSinCos();

	private:
		/** Builds a table returning  sin(Addr) and cos(Addr) accurate to 2^(-lsbOut-g)
				(beware, lsbOut is positive) where Addr is on a bits. 
				If g is not null, a rounding bit is added at weight 2^((-lsbOut-1)
				argRedCase=1 for full table, 4 for quadrant reduction, 8 for octant reduction.
				Result is signed if argRedCase=1 (msbWeight=0=sign bit), unsigned positive otherwise (msbWeight=-1).
		*/
		vector<mpz_class> buildSinCosTable(int a, int lsbOut, int g, int argRedCase);
	

	public:	
	
	
		void emulate(TestCase * tc);
	
		void buildStandardTestCases(TestCaseList * tcl);
	
		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);

		/** Factory register method */ 
		static void registerFactory();
	

	private:
		int w;
		mpfr_t scale;              /**< 1-2^(wOut-1)*/
		mpfr_t constPi;
	};

}
#endif // header guard

