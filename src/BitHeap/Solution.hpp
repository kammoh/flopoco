#ifndef SOLUTION_HPP
#define SOLUTION_HPP

#include "BitHeap/Compressor.hpp"
#include <string>
namespace flopoco
{

	enum BitheapSolutionStatus {
		EMPTY,
		NO_COMPRESSION_NEEDED,
		HEURISTIC_PARTIAL,
		HEURISTIC_COMPLETE,
		OPTIMAL_PARTIAL,
		OPTIMAL_COMPLETE,
		MIXED_PARTIAL,
		MIXED_COMPLETE
	};

	class Solution
	{
	public:

		/**
		 * A basic constructor for a solution
		 */
		Solution();

		/**
		 *	@brief adds a compressor at a given stage and column
		 *	@param stage specifies the stage where the compressor is added
		 *	@param column specifies the column where the compressor is added
		 *	@param compressor pointer to the BasicCompressor
		 *	@param middleLength If BasicCompressor is a variable Compressor, its middleLength will
		 *  	be specified with this argument. Otherwise middleLength is zero.
		 */
		void addCompressor(unsigned int stage, unsigned int column, BasicCompressor* compressor, int middleLength = 0);

		/**
		 *	@brief resturns a vector of all compressors and their middleLength in a given position.
		 *		The position is specified by stage and column
		 *	@param stage specifies the stage
		 *	@param column specifies the column
		 */
		vector<pair<BasicCompressor*, unsigned int> > getCompressorsAtPosition(unsigned int stage, unsigned int column);

		/**
		 *	@brief returns number of stages
		 */
		int getNumberOfStages();

		/**
		 *	@brief returns number of columns for a given stage
		 *	@param stage specifies the stage
		 */
		int getNumberOfColumnsAtStage(unsigned int stage);

		/**
		 *	@brief sets the solutionStatus
		 *	@param stat the new status
		 */
		void setSolutionStatus(BitheapSolutionStatus stat);

		/**
		 *	@brief sets the solution as complete
		 */
		void markSolutionAsComplete();

		/**
		 *	@brief gets the solutionStatus
		 */
		BitheapSolutionStatus getSolutionStatus();

	private:

		/* Entry is the struct which specifies compressors in a solution*/
		struct Entry {
			unsigned int column;
			unsigned int stage;
			unsigned int amount;
			BasicCompressor* compressor;
			int middleLength;
		};

		/* three-dimensional vector of compressors in the solution.
			first dimension is the stage,
			second the column and
			third is a vector of all compressors*/
		vector<vector<vector<Entry> > > comps;


		BitheapSolutionStatus status;

		/**
		 * 	@brief clears empty stages if there are no later stages with compressors in them. clears also empty columns if there are no columns closer to MSB with compressors in them.
		 **/
		void cleanUpStagesAndColumns();

	};

}
#endif
