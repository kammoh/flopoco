#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP


#include <iostream>
#include <sstream>
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <string>

#include "gmp.h"
#include "mpfr.h"

#include "../Operator.hpp"
#include "../utils.hpp"

namespace flopoco
{

	/**
	 * The Compressor class generates basic compressors, that convert
	 * one or several columns of bits with the same weight into a row
	 * of bits of increasing weights.
	 */
	class Compressor : public Operator
	{
	public:

		/**
		 * A basic constructor
		 */
		Compressor(Target * target, vector<int> heights, bool compactView = false);

		/**
		 * Destructor
		 */
		~Compressor();


		/**
		 * Return the height of column @column
		 */
		unsigned getColumnSize(int column);

		/**
		 * Return the size of the compressed result
		 */
		int getOutputSize();


		void emulate(TestCase * tc);

		// User-interface stuff
		/** Factory method */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);
		/** Register the factory */
		static void registerFactory();

	public:
		vector<int> heights;                /*< the heights of the columns */
		int wIn;                            /*< the number of bits at the input of the compressor */
		int wOut;                           /*< size of the output */
		int maxVal;                         /*< the maximum value that the compressor can count up to */
		bool compactView;                   /*< print the VHDL in a more compact way, or not */
	};
}

#endif
