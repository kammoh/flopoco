#ifndef INTADDERS_HPP
#define INTADDERS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

namespace flopoco {

	/** The IntAdder class for pipelined integer adders.
	 */
	class IntAdder : public Operator {
	public:

		/** The types of integer addition */
		typedef enum {
			Add, /**< plain adder */
			Sub, /**< subtracter */
			AddSub, /**< X+Y, X-Y */
			SubSub, /**< X-Y, Y-X */
		} Type;
		
		/**
		 * The IntAdder constructor
		 * @param[in] target           the target device
		 * @param[in] wIn              the with of the inputs and output
		 * @param[in] inputDelays      the delays for each input
		 * @param[in] optimizeType     the type optimization we want for our adder.
		 *            0: optimize for logic (LUT/ALUT)
		 *            1: optimize register count
		 *            2: optimize slice/ALM count
		 * @param[in] srl              optimize for use of shift registers
		 **/
		IntAdder ( Target* target, int wIn);

		/**
		 * The IntAdder constructor
		 * @param[in] parentOp         the parent operator of this component
		 * @param[in] target           the target device
		 * @param[in] wIn              the with of the inputs and output
		 * @param[in] inputDelays      the delays for each input
		 * @param[in] optimizeType     the type optimization we want for our adder.
		 *            0: optimize for logic (LUT/ALUT)
		 *            1: optimize register count
		 *            2: optimize slice/ALM count
		 * @param[in] srl              optimize for use of shift registers
		 **/
		IntAdder ( OperatorPtr parentOp, Target* target, int wIn);

		/**
		 *  Destructor
		 */
		~IntAdder();

		/**
		 * The emulate function.
		 * @param[in] tc               a list of test-cases
		 */
		void emulate ( TestCase* tc );

		/**
		 * get the maximum adder size for a given target period
		 */
		int getMaxAdderSizeForPeriod(double TargetPeriod);

		// User-interface stuff
		/** Factory method */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);

		static void registerFactory();

	protected:
		int wIn;                                    /**< the width for X, Y and R*/
	private:
		vector<Operator*> addImplementationList;     /**< this list will be populated with possible adder architectures*/
		int selectedVersion;                         /**< the selected version from the addImplementationList */
	};

}
#endif
