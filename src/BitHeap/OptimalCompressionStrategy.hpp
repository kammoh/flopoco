#ifndef OPTIMALCOMPRESSIONSTRATEGY_HPP
#define OPTIMALCOMPRESSIONSTRATEGY_HPP

#include "BitHeap/CompressionStrategy.hpp"
#include "BitHeap/BitheapNew.hpp"

namespace flopoco
{

class BitheapNew;

	class OptimalCompressionStrategy : public CompressionStrategy
	{
	public:

		/**
		 * A basic constructor for a compression strategy
		 */
		OptimalCompressionStrategy(BitheapNew *bitheap);

	private:
		/**
		 *	@brief starts the compression algorithm. It will call parandehAfshar()
		 */
		void compressionAlgorithm();





	};

}
#endif
