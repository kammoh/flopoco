#ifndef FIRSTFITTINGCOMPRESSIONSTRATEGY_HPP
#define FIRSTFITTINGCOMPRESSIONSTRATEGY_HPP

#include "BitHeap/CompressionStrategy.hpp"
#include "BitHeap/BitheapNew.hpp"

namespace flopoco
{

class BitheapNew;

	class FirstFittingCompressionStrategy : public CompressionStrategy
	{
	public:

		/**
		 * A basic constructor for a compression strategy
		 */
		FirstFittingCompressionStrategy(BitheapNew *bitheap);

	private:
		void compressionAlgorithm();
	};

}
#endif
