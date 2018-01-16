#ifndef PARANDEHAFSHARCOMPRESSIONSTRATEGY_HPP
#define PARANDEHAFSHARCOMPRESSIONSTRATEGY_HPP

#include "BitHeap/CompressionStrategy.hpp"
#include "BitHeap/BitheapNew.hpp"

namespace flopoco
{

class BitheapNew;

	class ParandehAfsharCompressionStrategy : public CompressionStrategy
	{
	public:

		/**
		 * A basic constructor for a compression strategy
		 */
		ParandehAfsharCompressionStrategy(BitheapNew *bitheap);

	private:
		/**
		 *	@brief starts the compression algorithm. It will call parandehAfshar()
		 */
		void compressionAlgorithm();

		/**
		 *	@brief returns the compressor to be used in a given stage and column. Returns nullptr,
		 * 		if there is no suitable compressor. Second argueent of the pair is the column, where
		 *		the compressor is used (a forawrd and backward search is being done. therefore the
		 *		column can differ).
		 *	@param stage specifies the stage
		 *	@param column specifies the column
		 */
		pair<BasicCompressor*, int> ParandehAfsharSearch(unsigned int stage, unsigned int column);


		/**
		 *	@brief starts parandehAfshar
		 */
		void parandehAfshar();
	};

}
#endif
