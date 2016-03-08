#ifndef INTADDERS_HPP
#define INTADDERS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

namespace flopoco {

#define LOGIC      0
#define REGISTER   1
#define SLICE      2
#define LATENCY    3

	/** The IntAdder class for experimenting with adders.
	 */
	class IntAdder : public Operator {
	public:

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
		IntAdder ( Target* target, int wIn, int optimizeType = SLICE, bool srl = true, int implementation = -1 );

		/**
		 *  Destructor
		 */
		~IntAdder();

		/**
		 * The emulate function.
		 * @param[in] tc               a list of test-cases
		 */
		void emulate ( TestCase* tc );

		// User-interface stuff
		/** Factory method */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static void registerFactory();

	protected:
		int wIn;                                    /**< the width for X, Y and R*/
	private:
		vector<Operator*> addImplementationList;     /**< this list will be populated with possible adder architectures*/
		int selectedVersion;                         /**< the selected version from the addImplementationList */
	};

}
#endif
