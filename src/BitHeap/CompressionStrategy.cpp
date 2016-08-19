
#include "CompressionStrategy.hpp"


using namespace std;

namespace flopoco{


	CompressionStrategy::CompressionStrategy(BitheapNew *bitheap_) :
		bitheap(bitheap_)
	{
		compressionDoneIndex = 0;

		compressionDelay = bitheap->op->getTarget()->lutDelay() + bitheap->op->getTarget()->localWireDelay();
		stagesPerCycle = (1.0/bitheap->op->getTarget()->frequency()) / compressionDelay;

		generatePossibleCompressors();
	}


	CompressionStrategy::~CompressionStrategy(){
	}


	void CompressionStrategy::startCompression()
	{

	}


	void CompressionStrategy::generatePossibleCompressors()
	{
		int col0, col1;

#if 0 // The generic alternative
		//Generate all "workable" compressors for 2 columns
		for(col0=maxCompressibleBits; col0>=3; col0--)
			for(col1=col0; col1>=0; col1--)
				if((col0 + col1<=maxCompressibleBits) && (intlog2(col0 + 2*col1)<=3))
					{
						vector<int> newVect;

						REPORT(DEBUG, "Generating compressor for col1=" << col1 <<", col0=" << col0);

						newVect.push_back(col0);
						newVect.push_back(col1);
						Compressor* bc = new Compressor(op->getTarget(), newVect);
						possibleCompressors.push_back(bc);
					}

#endif

		//generate the compressors, in decreasing order
		//	of their compression ratio
		{
			vector<int> newVect;

			col0=6;
			col1=0;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
		{
			vector<int> newVect;

			col0=4;
			col1=1;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
		{
			vector<int> newVect;

			col0=5;
			col1=0;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
		{
			vector<int> newVect;

			col0=3;
			col1=1;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
		{
			vector<int> newVect;

			col0=4;
			col1=0;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
		{
			vector<int> newVect;

			col0=3;
			col1=2;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
		{
			vector<int> newVect;

			col0=3;
			col1=0;

			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new Compressor(bitheap->op->getTarget(), newVect));
		}
	}
}




