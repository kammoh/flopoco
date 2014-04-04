#ifndef _FIXFUNCTION_HPP_
#define _FIXFUNCTION_HPP_

#include <string>
#include <iostream>

#include <sollya.h>
#include <gmpxx.h>
#include "../TestCase.hpp"

using namespace std;

/* Stylistic convention here: all the sollya_obj_t have names that end with a capital S */
namespace flopoco{
	
	/** The FixFunction objects holds a fixed-point function. 
			It provides an interface to Sollya services such as 
			parsing it,
			evaluating it at arbitrary precision,
			providing a polynomial approximation on an interval
	*/

	class FixFunction {
	public:


		/**
			 The FixFunctionByTable constructor
			 @param[string] func    a string representing the function, input range should be [0,1]
			 @param[int]    wInX    input size, also opposite of input LSB weight
			 @param[int]    lsbOut  output LSB weight
			 @param[int]    msbOut  output MSB weight, used to determine wOut
			 
			 One could argue that MSB weight is redundant, as it can be deduced from an analysis of the function. 
			 This would require quite a lot of work for non-trivial functions (isolating roots of the derivative etc).
			 So this is currently left to the user.
			 There are defaults for lsbOut and msbOut for situations when they are computed afterwards.
		 */
		FixFunction(string sollyaString, int wIn=0, int lsbOut=0, int msbOut=0);
		FixFunction(sollya_obj_t fS);

		virtual ~FixFunction();

		string getDescription() const;
		double eval(double x) const;
		void eval(mpfr_t r, mpfr_t x) const;
		void eval(mpz_class x, mpz_class &rd, mpz_class &ru, bool correctlyRounded=false) const;

		sollya_obj_t getSollyaObj() const;

		void emulate(TestCase * tc,	bool correctlyRounded=false /**< if false, faithful function */);
	private:

		int wIn;   
		int msbOut;
		int lsbOut;
		int wOut;
		string description;
		sollya_obj_t fS;

	};

}
#endif // _FIXFUNCTION_HH_
