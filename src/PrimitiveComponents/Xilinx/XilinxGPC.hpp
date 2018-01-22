#pragma once

#include "Operator.hpp"
#include "utils.hpp"
#include "BitHeap/Compressor.hpp"

namespace flopoco
{
    class XilinxGPC : public Compressor
	{
	public:

		/**
		 * A basic constructor
		 */
        XilinxGPC(Operator *parentOp, Target * target, vector<int> heights);

		/** Factory method */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
		/** Register the factory */
		static void registerFactory();

	public:

	private:
	};

    class BasicXilinxGPC : public BasicCompressor
	{
	public:
        BasicXilinxGPC(Target * target);

        virtual double getEfficiency(unsigned int middleLength);
	};
}
