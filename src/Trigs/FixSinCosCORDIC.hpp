#ifndef FIXSINCOSCORDIC_HPP
#define FIXSINCOSCORDIC_HPP

#include "Operator.hpp"
#include "utils.hpp"


#include <vector>

namespace flopoco{ 


	class FixSinCosCORDIC : public FixSinCos {
	  
	  public:
	  
		// constructor, defined there with two parameters (default value 0 for each)
		FixSinCosCORDIC(OperatorPtr parentOp, Target* target, int wIn, int wOut, int reducedIterations = 0);

		// destructor
		~FixSinCosCORDIC();
	

	private:
		int wIn;                   /**< input precision, input being in [-1,1) (includes a sign bit of weight 2^0) */
		int wOut;                  /**< output precisions; output being in (-1,1) (includes a sign bit of weight 2^0) */
		int reducedIterations;     /**< 0 = normal CORDIC, 1=halved number of iterations */
		int w;                     /**< internal precision */
		int	maxIterations;         /**< index at which iterations stop */
		int g;                     /**< number of guard bits*/
		mpfr_t kfactor;            /**< */
		vector<mpfr_t> atani;      /**< */

	};

}

#endif
