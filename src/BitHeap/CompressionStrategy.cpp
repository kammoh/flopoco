
#include "CompressionStrategy.hpp"


using namespace std;

namespace flopoco{


	CompressionStrategy::CompressionStrategy(BitheapNew *bitheap_) :
			bitheap(bitheap_)
	{
		compressionDelay = bitheap->op->getTarget()->lutDelay() + bitheap->op->getTarget()->localWireDelay();
		stagesPerCycle = (1.0/bitheap->op->getTarget()->frequency()) / compressionDelay;
	}


	CompressionStrategy::~CompressionStrategy(){
	}


	void CompressionStrategy::startCompression()
	{

	}
}




