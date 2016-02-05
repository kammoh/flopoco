#ifndef __SIGNAL_HPP
#define __SIGNAL_HPP

#include <iostream>
#include <sstream>
#include <gmpxx.h>

#include <vector>

#include <inttypes.h>

#include "utils.hpp"
//#include "Operator.hpp"


namespace flopoco{

//forward reference to Operator, in order to avoid cyclic references
class Operator;

	/**
	 * A class representing a signal.
	 */
	class Signal
	{
	public:
		/** The possible types of a signal*/
		typedef enum {
			in,                              /**< if the signal is an input signal */
			out,                             /**< if the signal is an output signal */
			wire,                            /**< if the signal is a wire (not registered) */
			constant,                        /**< if the signal is a constant */
			table,                           /**< if the signal is a table (either logic, or RAM-based) */
			registeredWithoutReset,          /**< if the signal is registered, but does not have a reset */
			registeredWithAsyncReset,        /**< if the signal is registered, and has an asynchronous reset */
			registeredWithSyncReset,         /**< if the signal is registered, and has an synchronous reset */
			registeredWithZeroInitialiser    /**< if the signal is registered, the it has a zero initializer (but no reset) */
		} SignalType;

#if 0 // all these flags could be replaced with something like that
		/** The possible arithmetic types of a bit vector*/
		typedef enum {
			unsigned_integer,          /**<  */
			signed_integer,          /**<  */
			unsigned_fixpoint,          /**<  */
			signed_fixpoint,          /**<  */
			floating_point          /**<  */
		} ArithType;
#endif

		/**
		 * Signal constructor.
		 * The standard constructor
		 * @param parentOp  the operator containing the signal
		 * @param name      the name of the signal
		 * @param type      the type of the signal, @see SignalType
		 * @param width     the width of the signal
		 * @param isBus     the flag which signals if the signal is a bus (std_logic_vector)
		 */
		Signal(Operator* parentOp, const std::string name, const Signal::SignalType type, const int width = 1, const bool isBus=false);

		/**
		 * Signal constructor.
		 * The standard constructor for signals that are constants
		 * @param parentOp       the operator containing the signal
		 * @param name           the name of the signal
		 * @param type           the type of the signal, @see SignalType
		 * @param constValue     the value of the constant
		 */
		Signal(Operator* parentOp, const std::string name, const Signal::SignalType type, const double constValue);

		/**
		 * Signal constructor.
		 * The standard constructor for signals that are used for tables.
		 * @param parentOp       the operator containing the signal
		 * @param name           the name of the signal
		 * @param type           the type of the signal, @see SignalType
		 * @param width          the width of the signal
		 * @param tableValue     the values stored in the table, as well as the possible required type declarations
		 */
		Signal(Operator* parentOp, const std::string name, const Signal::SignalType type, const int width = 1, const std::string tableValue = "");

		/**
		 * Signal constructor.
		 * The standard constructor for signals which are fixed-point.
		 * @param parentOp  the operator containing the signal
		 * @param name      the name of the signal
		 * @param type      the type of the signal, @see SignalType
		 * @param isSigned  true for signed fixed-point, false otherwise
		 * @param MSB       the weight of the MSB
		 * @param LSB       the weight of the LSB
		 */
		Signal(Operator* parentOp, const std::string name, const SignalType type, const bool isSigned, const int MSB, const int LSB);

		/**
		 * Signal constructor.
		 * The standard constructor for signals which are floating-point.
		 * @param parentOp  the operator containing the signal
		 * @param name      the name of the signal
		 * @param type      the type of the signal, @see SignalType
		 * @param wE        the exponent width
		 * @param wF        the significand width
		 */
		Signal(Operator* parentOp, const std::string name, const SignalType type, const int wE, const int wF, const bool ieeeFormat=false);

		/**
		 * Signal constructor
		 * The copy constructor
		 */
		Signal(Signal* originalSignal);

		/**
		 * Signal constructor
		 * Copy the signal to a different operator
		 */
		Signal(Operator* newParentOp, Signal* originalSignal);

		/** Signal destructor.
		 */
		~Signal();


		/**
		 * When a signal was automatically created as a std_logic_vector, this enables to declare it as Fix
		 */
		void promoteToFix(const bool isSigned, const int MSB, const int LSB);

		/**
		 * Returns the name of the signal
		 * @return the name of the signal
		 */
		const std::string& getName() const;

		/**
		 * Return a reference to the parent operator
		 */
		Operator* parentOp() const;

		/**
		 * Set the parent operator
		 * Warning: this method only changes the reference to the parent operator
		 * 			thus, this is a helper method, and should be used with caution
		 * 			Use Operator::setSignalParentOp() instead
		 */
		void setParentOp(Operator* newParentOp);


		/**
		 * Returns the width of the signal
		 */
		int width() const;


		/**
		 * Returns the extra declarations, if this is a table
		 */
		const std::string& tableAttributes() const;

		/**
		 * Set the extra declarations, if this is a table
		 */
		void setTableAttributes(std::string newTableAttributes);


		/**
		 * Returns the exponent width of the signal
		 */
		int wE() const;

		/**
		 * Returns the fraction width of the signal
		 */
		int wF() const;

		/**
		 * Returns the MSB weight of the (fixed-point) signal
		 */
		int MSB() const;

		/**
		 * Returns the LSB weight of the (fixed-point) signal
		 */
		int LSB() const;

		/** Returns the signess of the signal
		 */
		bool isSigned() const;

		/**
		 * Reports if the signal is a FloPoCo floating-point signal
		 */
		bool isFP() const;

		/**
		 * Set the value of isFP
		 */
		void setIsFP(bool newIsFP = true);

		/**
		 * Reports if the signal is a fixed-point signal
		 */
		bool isFix() const;

		/**
		 * Set the value of isFix
		 */
		void setIsFix(bool newIsFix = true);

		/**
		 * Reports if the signal is a signed fixed-point signal
		 */
		bool isFixSigned() const;

		/**
		 * Set the value of isFix and the signess
		 */
		void setFixSigned(bool newIsFix = true);

		/**
		 * Reports if the signal is an unsigned fixed-point signal
		 */
		bool isFixUnsigned() const;

		/**
		 * Set the value of isFix and the un-signess
		 */
		void setFixUnsigned(bool newIsFix = true);

		/**
		 * Reports if the signal is an IEEE floating-point signal
		 */
		bool isIEEE() const;

		/**
		 * Set the value of isIEEE
		 */
		void setIsIEEE(bool newIsIEEE = true);

		/**
		 * Reports if the signal has the bus flag active
		 */
		bool isBus() const;

		/**
		 * Returns the type of the signal
		 */
		SignalType type() const;

		/**
		 * Returns the list of predecessors
		 */
		vector<pair<Signal*, int>>* predecessors();

		/**
		 * Returns the predecessor at index count
		 */
		Signal* predecessor(int count);

		/**
		 * Returns the predecessor-delay pair at index count
		 */
		pair<Signal*, int>* predecessorPair(int count);

		/**
		 * Returns the list of successors
		 */
		vector<pair<Signal*, int>>* successors();

		/**
		 * Returns the successor at index count
		 */
		Signal* successor(int count);

		/**
		 * Returns the successor-delay pair at index count
		 */
		pair<Signal*, int>* successorPair(int count);

		/**
		 * Outputs the VHDL code for declaring this signal. TODO should be obsoleted?
		 */
		std::string toVHDL();

		/**
		 * Outputs the VHDL code for the type of this signal
		 */
		std::string toVHDLType();

		/**
		 * Obtain the name of a signal delayed by n cycles
		 * @param delay in cycles */
		std::string delayedName(int n);


		/**
		 * Outputs the VHDL code for declaring a signal with all its delayed versions
		 * This is the 2.0 equivalent of toVHDL()
		 * @return the VHDL for the declarations.
		 */
		std::string toVHDLDeclaration();


		/**
		 * Sets the cycle at which the signal is active
		 */
		void setCycle(int cycle) ;


		/**
		 * Obtain the declared cycle of this signal
		 * @return the cycle
		 */
		int getCycle();


		/**
		 * Updates the max delay associated to a signal
		 */
		void updateLifeSpan(int delay) ;


		/**
		 * Obtain max delay that has been applied to this signal
		 * @return the max delay
		 */
		int getLifeSpan() ;

		/**
		 * Obtain the delay of this signal
		 * @return the delay
		 */
		double getCriticalPath();

		/**
		 * Sets the delay of this signal
		 * @param[in] delay the delay
		 */
		void setCriticalPath(double delay);

		/**
		 * Obtain the contribution of this signal to the critical path
		 * @return the criticalPathContribution_
		 */
		double getCriticalPathContribution();

		/**
		 * Sets the contribution of this signal to the critical path
		 * @param[in] contribution the contribution
		 */
		void setCriticalPathContribution(double contribution);

		/**
		 * Return the value of hasBeenImplemented
		 */
		bool getHasBeenImplemented();

		/**
		 * Set the new value of hasBeen Implemented
		 */
		void setHasBeenImplemented(bool newVal);

		/**
		 * Return the value of hasBeenDrawn
		 */
		bool getHasBeenDrawn();

		/**
		 * Set the new value of hasBeenDrawn
		 */
		void setHasBeenDrawn(bool newVal);


		/**
		 * Set the number of possible output values.
		 */
		void  setNumberOfPossibleValues(int n);


		/**
		 * Get the number of possible output values.
		 */
		int getNumberOfPossibleValues();


		/**
		 * Converts the value of the signal into a nicely formated VHDL expression,
		 * including padding and putting quot or apostrophe.
		 * @param v value
		 * @param quot also put quotes around the value
		 * @return a string holding the value in binary
		 */
		std::string valueToVHDL(mpz_class v, bool quot = true);

		/**
		 * Converts the value of the signal into a nicely formated VHDL expression,
		 * including padding and putting quote or apostrophe. (Hex version.)
		 * @param v value
		 * @param quot also put quotes around the value
		 * @return a string holding the value in hexa
		 */
		std::string valueToVHDLHex(mpz_class v, bool quot = true);

		void setName(std::string name);
		void setType(SignalType t);

	private:
		Operator*     parentOp_;           /**< The operator which contains this signal */
		std::string   name_;               /**< The name of the signal */
		SignalType    type_;               /**< The type of the signal, see SignalType */
		int           width_;              /**< The width of the signal */

		double       constValue_;          /**< The value of the constant, for a constant signal */

		std::string   tableAttributes_;    /**< The values that will be used to fill the table, when implemented as a hard RAM block, or other attributes needed when declaring the table */

		int           numberOfPossibleValues_; /**< For signals of type out, indicates how many values will be acceptable. Typically 1 for correct rounding, and 2 for faithful rounding */

		int           lifeSpan_;           /**< The max delay that will be applied to this signal; */
		std::vector<pair<Signal*, int>> predecessors_; /**< the list of Signals that appear on the RHS of this signal.  */
		std::vector<pair<Signal*, int>> successors_;   /**< the list of Signals for which this signal appears on the RHS. The second value in the pair contains the (possible) delay on the edge */
		int           cycle_;              /**< the cycle at which this signal is active in a pipelined operator. 0 means synchronized with the inputs */
	    double        criticalPath_;       /**< the critical path within a cycle, or from the inputs if the operator is not pipelined */
	    double        criticalPathContribution_; /**< the delay that the signal adds to the critical path */

	    bool          hasBeenScheduled_;    /**< Has the signal already been scheduled? */
	    bool          hasBeenDrawn_;    /**< Has the signal already been drawn? */

		bool          isFP_;               /**< If the signal is of the FloPoCo floating-point type */
		bool          isFix_;              /**< If the signal is of the FloPoCo fixed-point type */
		bool          isIEEE_;             /**< If the signal is of the IEEE floating-point type */
		int           wE_;                 /**< The width of the exponent. Used for FP signals */
		int           wF_;                 /**< The width of the fraction. Used for FP signals */
		int           MSB_;                /**< MSB. Used for fixed-point signals */
		int           LSB_;                /**< LSB. Used for fixed-point signals */
		bool          isSigned_;           /**< true if this a signed fixed-point signals, false otherwise */
		bool          isBus_;              /**< True is the signal is a bus (std_logic_vector)*/


	};

}

#endif

