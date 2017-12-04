/*

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author:    Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,

  All rights reserved.

 */

#ifndef TABLE_HPP
#define TABLE_HPP
#include <gmpxx.h>

#include "Operator.hpp"

/**
 A basic hardware look-up table for FloPoCo.

	 If the input to your table are negative, etc, or if you want to
	 define errors, or... then derive a class from this one.

	 A Table is, so far, always combinatorial. It does increase the critical path,
	 taking into account inputDelays and reporting outputDelay.

	 On logic tables versus blockRam tables:
	 This has unfortunately to be managed twice,
	   firstly by passing the proper bool value to the logicTable argument of the constructor
	   and  secondly by calling useSoftRAM() or useHardRAM() on each instance to set the synthesis attributes.

	 You may want to force buffering the inputs to a table to be sure it will be synthesized as a BlockRAM.
*/

namespace flopoco{


	class Table : public Operator
	{
	public:

		/** Input width (in bits)*/
		int wIn;

		/** Output width (in bits)*/
		int wOut;

		/** minimal input value (default 0) */
		mpz_class minIn;

		/** maximal input value (default 2^wIn-1) */
		mpz_class maxIn;

		/**
		 * The obsolete Table constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the input in bits
		 * @param[in] wOut   the with of the output in bits
     	 * @param[in] logicTable   1 if the table is intended to be implemented as logic; -1 if the table is intended to be implemented as BRAM; 0: let the constructor decide
		 */
		Table(Target* target, int _wIn, int _wOut, int _minIn=0, int _maxIn=-1, int logicTable = 0,  map<string, double> inputDelays = emptyDelayMap );

		/**
		 * The Table constructor
		 * @param[in] target 		the target device
		 * @param[in] values 		the values used to fill the table
		 * @param[in] wIn    		the with of the input in bits
		 * @param[in] wOut   		the with of the output in bits
		 * @param[in] logicTable 	true if the table is intended to be implemented as logic;
		 * 							false (default value) if the table is intended to be implemented as BRAM
		 * @param[in] minIn			minimal input value (default: smallest value in values)
		 * @param[in] maxIn			maximal input value (default: largest value in values)
		 */
		Table(OperatorPtr parentOp, Target* target, vector<mpz_class> _values, int _wIn = -1, int _wOut = -1, string name="", int _logicTable = -1, int _minIn = -1, int _maxIn = -1);

		Table(OperatorPtr parentOp, Target* target);

		virtual ~Table() {};


		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);

		static void registerFactory();



		/**
		 * The function that will define the values contained in the table
		 * @param[in] x  input to the table, an integer value between minIn and maxIn
		 * @return    an mpz integer  between 0 and 2^wOut-1
		 */
		virtual mpz_class function(int x);//FOR TESTING = 0;

		/**
		 * This method is required as a trick mechanism:
		 * 	function() is pure virtual, and thus cannot be called from the constructor;
		 * 	thus, we need another function that does that call, so that everything
		 * 	compiles and links properly
		 */
		mpz_class call_function(int x);


		/** Overloading the method of Operator */
//		void outputVHDL(ostream& o, string name);

		/** A function that translates an real value into an integer input.
			 This function should be overridden by an implementation of Table.
			 It is optional.
		*/
		virtual int    double2input(double x);

		/** A function that translates an integer input value into its real semantics
			 This function should be overridden by an implementation of Table.
			 It is optional.
		*/
		virtual double input2double(int x);

		/** A function that translates an real value into an integer output
			 This function should be overridden by an implementation of Table
			 It is optional.
		*/
		virtual  mpz_class double2output(double x);

		/** A function that translates an integer output value into its real semantics
			 This function should be overridden by an implementation of Table
			 It is optional.
		*/
		virtual double output2double(mpz_class x);

#if 0 // TODO some day
		/** A function that translates an real value into an integer output
			 This function should be overridden by an implementation of Table
			 It is optional.
		*/
		virtual  mpz_class mpfr2output(double x);

		/** A function that translates an integer output value into its real semantics
			 This function should be overridden by an implementation of Table
			 It is optional.
		*/
		virtual double output2mpfr(mpz_class x);

#endif

		/** A function that returns an estimation of the size of the table in LUTs. Your mileage may vary thanks to boolean optimization */
		int size_in_LUTs();
	private:
		bool full; 					/**< true if there is no "don't care" inputs, i.e. minIn=0 and maxIn=2^wIn-1 */
		bool logicTable; 			/**< true: LUT-based table; false: BRAM-based */
		double cpDelay;  				/**< For a LUT-based table, its delay; */

		public:
		vector<mpz_class> values;	/**< the values used to fill the table */
	};

}
#endif
