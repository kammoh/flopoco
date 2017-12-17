
#include "CompressionStrategy.hpp"


using namespace std;

namespace flopoco{


	CompressionStrategy::CompressionStrategy(BitheapNew *bitheap_) :
		bitheap(bitheap_)
	{
		stringstream s;

		 //for REPORT to work
		srcFileName = bitheap->op->getSrcFileName() + ":BitHeap:CompressionStrategy";
		guid = Operator::getNewUId();
		s << bitheap->name << "_CompressionStrategy_" << guid;
		uniqueName_ = s.str();

		//no chunks already compressed
		compressionDoneIndex = 0;

		//the initial delay between the soonest bit and the rest of the bits
		//	to be compressed is equal to the delay of a basic compressor
		compressionDelay = bitheap->op->getTarget()->logicDelay(bitheap->op->getTarget()->lutInputs());
		stagesPerCycle = (1.0/bitheap->op->getTarget()->frequency()) / compressionDelay;

		//generate all the compressors that can be used
		generatePossibleCompressors();

		//create the plotter
		bitheapPlotter = new BitheapPlotter(bitheap);
	}


	CompressionStrategy::~CompressionStrategy(){
	}


	void CompressionStrategy::startCompression()
	{
		bool bitheapCompressed = false;
		double delay;
		unsigned int colorCount = 0;

		// Add the constant bits
		REPORT(DEBUG, "Adding the constant bits");
		bitheap-> op->vhdl << endl << tab << "-- Adding the constant bits" << endl;
		bool isConstantNonzero = false;
		for (unsigned w=0; w<bitheap-> width; w++){
			if (1 == ((bitheap->constantBits>>w) & 1) ){
				Bit* bit = bitheap->addBit(w-bitheap->lsb, "'1'");

				//set the signal to constant type, with declaration
				bit->signal->setType(Signal::constantWithDeclaration);
				//initialize the signals predecessors and successors
				bit->signal->resetPredecessors();
				bit->signal->resetSuccessors();
				//set the timing for the constant signal, at cycle 0, criticalPath 0, criticalPathContribution 0
				bit->signal->setCycle(0);
				bit->signal->setCriticalPath(0.0);
				bit->signal->setCriticalPathContribution(0.0);
				bit->signal->setHasBeenScheduled(true);

				isConstantNonzero = true;
			}
		}
		//when the constant bits are all zero, report it
		if(!isConstantNonzero){
			REPORT(DEBUG, "All the constant bits are zero, nothing to add");
			bitheap-> op->vhdl << tab << tab << "-- All the constant bits are zero, nothing to add" << endl;
		}
		bitheap-> op->vhdl << endl;

		
		//take a snapshot of the bitheap, before the start of the compression
		bitheapPlotter->takeSnapshot(getSoonestBit(0, bitheap->width-1), getSoonestBit(0, bitheap->width-1));

		//first, set the delay to the delay of a compressor
		delay = compressionDelay;

		REPORT(DEBUG, "current delay is: delay=" << delay);

		//compress the bitheap to height 2
		//	(the last column can be of height 3)
		if(!bitheap->isCompressed || bitheap->compressionRequired())
		{
			while(bitheap->compressionRequired())
			{
				Bit *soonestBit, *soonestCompressibleBit;

				//get the soonest bit in the bitheap
				soonestBit = getSoonestBit(0, bitheap->width-1);
				//get the soonest compressible bit in the bitheap
				soonestCompressibleBit = getSoonestCompressibleBit(0, bitheap->width-1, delay);

				//apply as many compressors as possible, with the current delay
				bitheapCompressed = compress(delay, soonestCompressibleBit);

				//take a snapshot of the bitheap, if a compression was performed
				//	including the bits that are to be removed
				if(bitheapCompressed == true)
				{
					//color the newly added bits
					colorCount++;
					bitheap->colorBits(BitType::justAdded, colorCount);
					//take a snapshot of the bitheap
					bitheapPlotter->takeSnapshot(soonestBit, soonestCompressibleBit);
				}

				//remove the bits that have just been compressed
				bitheap->removeCompressedBits();

				//mark the bits that have just been added as free to be compressed
				bitheap->markBitsForCompression();

				//take a snapshot of the bitheap, if a compression was performed
				//	without the bits that have been removed
				//	with the newly added bits as available for compression
				if(bitheapCompressed == true)
				{
					//take a snapshot of the bitheap
					bitheapPlotter->takeSnapshot(soonestBit, soonestCompressibleBit);
				}
				//send the parts of the bitheap that has already been compressed to the final result
				concatenateLSBColumns();

				//print the status of the bitheap
				bitheap->printBitHeapStatus();

				//if no compression was performed, then the delay between
				//	the soonest bit and the rest of the bits that need to be compressed needs to be increased
				if(bitheapCompressed == false)
				{
					//passing from the delay of a compressor to a delay equal to a period, with the target FPGA
					//	then increase the delay progressively with a delay equal to a period
					if(delay == compressionDelay)
						delay = 1.0 / bitheap->op->getTarget()->frequency();
					else
						delay += 1.0 / bitheap->op->getTarget()->frequency();
				}
			}
		}else{
			REPORT(DEBUG, "Bitheap already compressed, so startCompression has nothing else to do.");
		}
		//generate the final addition
		generateFinalAddVHDL(bitheap->op->getTarget()->getVendor() == "Xilinx");
		//concatenate all the chunks and create the final result
		concatenateChunks();
		//mark the bitheap as compressed
		bitheap->isCompressed = true;
		//plot the bitheap
		bitheapPlotter->plotBitHeap();
	}


	bool CompressionStrategy::compress(double delay, Bit *soonestCompressibleBit)
	{
		bool compressionPerformed = false;

		//if there is no soonest bit, the there's nothing else to do
		//	the bit wit the smallest (cycle, critical path), which is also compressible
		if(soonestCompressibleBit == nullptr)
			return false;

		//try to apply the compressors in the decreasing
		//	order of their compression ratio
		for(unsigned i=0; i<possibleCompressors.size(); i++)
		{
			//go through all the lines and try to apply the compressor
			//	to bits that are within the given delay
			for(unsigned j=compressionDoneIndex; j<bitheap->bits.size(); j++)
			{
				vector<Bit*> compressorBitVector = canApplyCompressor(j, i, soonestCompressibleBit, delay);

				while(compressorBitVector.size() > 0)
				{
					applyCompressor(compressorBitVector, possibleCompressors[i], bitheap->lsb+j);
					compressorBitVector.clear();
					compressorBitVector = canApplyCompressor(j, i, soonestCompressibleBit, delay);
					compressionPerformed = true;
				}
			}
		}

		return compressionPerformed;
	}


	void CompressionStrategy::applyCompressor(vector<Bit*> bitVector, Compressor* compressor, int weight)
	{
		vector<string> compressorInputs;
		ostringstream inputName, compressorIONames;
		int instanceUID = Operator::getNewUId();
		unsigned count = 0;

		if(bitVector.size() == 0)
			THROWERROR("Bit vector to compress is empty in applyCompressor");
		if(compressor == nullptr)
			THROWERROR("Compressor empty in applyCompressor");

		//build the inputs to the compressor
		for(unsigned i=0; i<compressor->heights.size(); i++)
		{
			inputName.str("");

			if(compressor->heights[i] > 0)
			{
				if(count >= bitVector.size())
					THROWERROR("Bit vector does not containing sufficient bits, "
							<< "as requested by the compressor is applyCompressor.");

				inputName << bitVector[count]->getName();
				count++;
			}
			for(int j=1; j<compressor->heights[i]; j++)
			{
				if(count >= bitVector.size())
					THROWERROR("Bit vector does not containing sufficient bits, "
							<< "as requested by the compressor is applyCompressor.");

				inputName << " & " << bitVector[count]->getName();
				count++;
			}

			compressorInputs.push_back(string(inputName.str()));
		}

		//create the signals for the compressor inputs
		//	and create the port mappings
		bitheap->op->vhdl << endl;
		for(unsigned i=0; i<compressor->heights.size(); i++)
		{
			compressorIONames.str("");
			compressorIONames << compressor->getName() << "_bh"
					<< bitheap->guid << "_uid" << instanceUID << "_In" << i;
			bitheap->op->vhdl << tab << bitheap->op->declare(compressorIONames.str(), compressor->heights[i])
					<< " <= \"\" & " << compressorInputs[i] << ";" << endl;
			bitheap->op->inPortMap(compressor, join("X",i), compressorIONames.str());
		}

		//create the signals for the compressor output
		//	and create the port mapping
		compressorIONames.str("");
		compressorIONames << compressor->getName() << "_bh"
				<< bitheap->guid << "_uid" << instanceUID << "_Out";
		bitheap->op->declare(compressorIONames.str(), compressor->wOut, (compressor->wOut > 1));
		bitheap->op->outPortMap(compressor, "R", compressorIONames.str());

		//create the compressor
		//	this will be a global component, so set is as shared
		compressor->setShared();
		//create the compressor instance
		bitheap->op->vhdl << bitheap->op->instance(compressor, join(compressor->getName(), "_uid", instanceUID)) << endl;

		//mark the bits that were at the input of the compressor as having been compressed
		//	so that they can be eliminated from the bitheap
		for(unsigned i=0; i<bitVector.size(); i++)
			bitheap->markBit(bitVector[i], BitType::compressed);

		//mark the compressor as having been used
		compressor->markUsed();

		//add the result of the compression to the bitheap
		bitheap->addSignal(compressorIONames.str(), weight);

		//mark the bits of the result of the compression as just added
		bitheap->markBits(bitheap->op->getSignalByName(compressorIONames.str()), BitType::justAdded, weight);
	}


	void CompressionStrategy::generateFinalAddVHDL(bool isXilinx)
	{
		//check if the last two lines are already compressed
		if(bitheap->getMaxHeight() < 2)
		{
			//an adder isn't necessary; concatenateLSBColumns should do the rest
			concatenateLSBColumns();
		}else{
			ostringstream adderIn0, adderIn0Name, adderIn1, adderIn1Name, adderOutName, adderCin, adderCinName;
			int adderStartIndex = compressionDoneIndex + 1;

			//create the names for the inputs/output of the adder
			adderIn0Name << "bitheapFinalAdd_bh" << bitheap->guid << "_In0";
			adderIn1Name << "bitheapFinalAdd_bh" << bitheap->guid << "_In1";
			adderCinName << "bitheapFinalAdd_bh" << bitheap->guid << "_Cin";
			adderOutName << "bitheapFinalAdd_bh" << bitheap->guid << "_Out";

			//add the first bits to the adder inputs
			if(compressionDoneIndex <= bitheap->width)
			{
				if(bitheap->bits[bitheap->width-1].size() > 1)
				{
					adderIn0 << bitheap->bits[bitheap->width-1][0]->getName();
					adderIn1 << bitheap->bits[bitheap->width-1][1]->getName();
				}else if(bitheap->bits[bitheap->width-1].size() > 0)
				{
					adderIn0 << bitheap->bits[bitheap->width-1][0]->getName();
					adderIn1 << "\"0\"";
				}else{
					adderIn0 << "\"0\"";
					adderIn1 << "\"0\"";
				}
			}
			//add the rest of the terms to the adder inputs
			for(int i=bitheap->width-2; i>(int)compressionDoneIndex; i--)
			{
				if(bitheap->bits[i].size() > 1)
				{
					adderIn0 << " & " << bitheap->bits[i][0]->getName();
					adderIn1 << " & " << bitheap->bits[i][1]->getName();
				}else if(bitheap->bits[i].size() > 0)
				{
					adderIn0 << " & " << bitheap->bits[i][0]->getName();
					adderIn1 << " & " << "\"0\"";
				}else{
					adderIn0 << " & " << "\"0\"";
					adderIn1 << " & " << "\"0\"";
				}
			}
			//add the last column to the adder, if compression is still required on that column
			if((compressionDoneIndex == 0) && (bitheap->bits[compressionDoneIndex].size() > 1))
			{
				adderIn0 << " & " << bitheap->bits[compressionDoneIndex][0]->getName();
				adderIn1 << " & " << bitheap->bits[compressionDoneIndex][1]->getName();

				if(bitheap->bits[compressionDoneIndex].size() > 2)
					adderCin << bitheap->bits[compressionDoneIndex][2]->getName();
				else
					adderCin << "\'0\'";

				adderStartIndex--;
			}else{
				if(bitheap->bits[compressionDoneIndex+1].size() > 2)
					adderCin << bitheap->bits[compressionDoneIndex+1][2]->getName();
				else
					adderCin << "\'0\'";
			}

			//create the signals for the inputs/output of the adder
			bitheap->op->vhdl << endl;
			bitheap->op->vhdl << tab << bitheap->op->declare(adderIn0Name.str(), bitheap->msb-adderStartIndex+1+1)
					<< " <= \"0\" & " << adderIn0.str() << ";" << endl;
			bitheap->op->vhdl << tab << bitheap->op->declare(adderIn1Name.str(), bitheap->msb-adderStartIndex+1+1)
					<< " <= \"0\" & " << adderIn1.str() << ";" << endl;
			bitheap->op->vhdl << tab << bitheap->op->declare(adderCinName.str())
					<< " <= " << adderCin.str() << ";" << endl;
			bitheap->op->vhdl << endl;
			bitheap->op->declare(adderOutName.str(), bitheap->msb-adderStartIndex+1+1);

			//declare the adder
			IntAdder* adder;

			//create the adder
			adder = new IntAdder(bitheap->op, bitheap->op->getTarget(), bitheap->msb-adderStartIndex+1+1);

			//create the port maps for the adder
			bitheap->op->inPortMap(adder, "X", adderIn0Name.str());
			bitheap->op->inPortMap(adder, "Y", adderIn1Name.str());
			bitheap->op->inPortMap(adder, "Cin", adderCinName.str());
			bitheap->op->outPortMap(adder, "R", adderOutName.str());


			//create the instance of the adder
			bitheap->op->vhdl << bitheap->op->instance(adder, join("bitheapFinalAdd_bh", bitheap->guid)) << endl;

			//add the result of the final add as the last chunk
			chunksDone.push_back(join(adderOutName.str(), range(bitheap->msb-adderStartIndex, 0)));
		}
	}


	Bit* CompressionStrategy::getSoonestBit(unsigned lsbColumn, unsigned msbColumn)
	{
		Bit* soonestBit = nullptr;
		unsigned count = lsbColumn;

		if(((int)lsbColumn < bitheap->lsb) || ((int)msbColumn > bitheap->msb))
			THROWERROR("Invalid arguments for getSoonest bit: lsbColumn="
					<< lsbColumn << " msbColumn=" << msbColumn);
		if(bitheap->getMaxHeight() == 0)
			REPORT(DEBUG, "Warning: trying to obtain the soonest bit from an empty bitheap!");

		//determine the first non-empty bit column
		while((count < msbColumn) && (bitheap->bits[count].size() == 0))
			count++;
		//select the first bit from the column
		soonestBit = bitheap->bits[count][0];

		//determine the soonest bit
		for(unsigned i=count; i<=msbColumn; i++)
			for(unsigned j=0; j<bitheap->bits[i].size(); j++)
			{
				Bit* currentBit = bitheap->bits[i][j];

				//only consider bits that are available for compression
				if(currentBit->type != BitType::free)
					continue;

				if((soonestBit->signal->getCycle() > currentBit->signal->getCycle()) ||
						((soonestBit->signal->getCycle() == currentBit->signal->getCycle())
								&& (soonestBit->signal->getCriticalPath() > currentBit->signal->getCriticalPath())))
				{
					soonestBit = currentBit;
				}
			}

		return soonestBit;
	}


	Bit* CompressionStrategy::getSoonestCompressibleBit(unsigned lsbColumn, unsigned msbColumn, double delay)
	{
		Bit *soonestBit = nullptr, *soonestCompressibleBit = nullptr;
		unsigned count = lsbColumn;
		vector<Bit*> appliedCompressor;

		if(((int)lsbColumn < bitheap->lsb) || ((int)msbColumn > bitheap->msb))
			THROWERROR("Invalid arguments for getSoonest bit: lsbColumn="
					<< lsbColumn << " msbColumn=" << msbColumn);
		if(bitheap->getMaxHeight() == 0)
			REPORT(DEBUG, "Warning: trying to obtain the soonest bit from an empty bitheap!");

		//determine the first non-empty bit column
		while((count < msbColumn) && (bitheap->bits[count].size() == 0))
			count++;

		//search in each column for the soonest bit
		for(unsigned i=count; i<=msbColumn; i++)
		{
			//clear the possible content of the compressor
			appliedCompressor.clear();

			//initialize the soonest bit
			if(bitheap->bits[i].size())
			{
				soonestBit = bitheap->bits[i][0];
			}else
			{
				//empty column, so no compressor can be applied
				continue;
			}

			//try to apply a compressor to the current column
			for(unsigned j=0; j<possibleCompressors.size(); j++)
			{
				appliedCompressor = canApplyCompressor(i, j, soonestBit, delay);
				if(appliedCompressor.size() > 0)
					break;
			}

			//if a compressor can be applied on this column, then check
			//	if the current soonest bit is earlier
			if(soonestCompressibleBit == nullptr)
			{
				soonestCompressibleBit = soonestBit;
				continue;
			}
			if(appliedCompressor.size() > 0)
				if((soonestBit->signal->getCycle() > soonestCompressibleBit->signal->getCycle()) ||
						((soonestBit->signal->getCycle() == soonestCompressibleBit->signal->getCycle())
								&& (soonestBit->signal->getCriticalPath() > soonestCompressibleBit->signal->getCriticalPath())))
				{
					soonestCompressibleBit = soonestBit;
				}
		}

		return soonestCompressibleBit;
	}


	vector<Bit*> CompressionStrategy::canApplyCompressor(unsigned columnNumber, unsigned compressorNumber, Bit* soonestBit, double delay)
	{
		Compressor *compressor;
		vector<Bit*> returnValue;
		double soonestBitTotalTime = 0.0;

		if(compressorNumber > possibleCompressors.size())
		{
			THROWERROR("Compressor index out of range: compressorNumber="
					<< compressorNumber << " maximum index=" << possibleCompressors.size()-1);
		}else{
			compressor = possibleCompressors[compressorNumber];
			soonestBitTotalTime = soonestBit->signal->getCycle() * (1.0/bitheap->op->getTarget()->frequency()) +
									soonestBit->signal->getCriticalPath();
		}

		//check there are sufficient columns of bits
		returnValue.clear();
		if(compressor->heights.size()+columnNumber-1 >= bitheap->bits.size())
			return returnValue;

		for(unsigned i=0; i<compressor->heights.size(); i++)
		{
			int bitCount = 0;
			unsigned columnIndex = 0;

			while((bitCount < compressor->heights[i])
					&& (columnIndex < bitheap->bits[columnNumber+i].size()))
			{
				Bit *currentBit = bitheap->bits[columnNumber+i][columnIndex];
				double currentBitTotalTime = 0.0;

				//only consider bits that are free
				if(currentBit->type != BitType::free)
				{
					//pass to the next bit
					columnIndex++;
					continue;
				}

				//the total time of the current bit
				currentBitTotalTime = currentBit->signal->getCycle() * 1.0/bitheap->op->getTarget()->frequency() +
										currentBit->signal->getCriticalPath();
				//check if the bit can be compressed
				if(currentBitTotalTime-soonestBitTotalTime <= delay)
				{
					returnValue.push_back(currentBit);
					bitCount++;
				}
				//pass to the next bit in the column
				columnIndex++;
			}
			//if the current column does not contain sufficient bits,
			//	then the compressor cannot be applied
			if(bitCount < compressor->heights[i])
			{
				returnValue.clear();
				return returnValue;
			}
		}

		return returnValue;
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
			Compressor *newCompressor;

			col0=6;

			newVect.push_back(col0);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			Compressor *newCompressor;

			col0=4;
			col1=1;

			newVect.push_back(col0);
			newVect.push_back(col1);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			Compressor *newCompressor;

			col0=5;

			newVect.push_back(col0);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			Compressor *newCompressor;

			col0=3;
			col1=1;

			newVect.push_back(col0);
			newVect.push_back(col1);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			Compressor *newCompressor;

			col0=4;

			newVect.push_back(col0);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			Compressor *newCompressor;

			col0=3;
			col1=2;

			newVect.push_back(col0);
			newVect.push_back(col1);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			Compressor *newCompressor;

			col0=3;

			newVect.push_back(col0);
			newCompressor = new Compressor(bitheap->op->getTarget(), newVect, true);
			newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
	}


	void CompressionStrategy::concatenateLSBColumns()
	{
		unsigned count = compressionDoneIndex;

		//check up until which index has the bitheap already been compressed
		while((count < bitheap->bits.size()) && (bitheap->bits[count].size() < 2))
			count++;
		//all the columns might have been already compressed
		if(count == bitheap->bits.size())
			count--;
		//if the current column needs compression, then do not consider it
		if((bitheap->bits[count].size() >= 2) && (count > 0))
			count--;

		if(count == compressionDoneIndex)
		{
			if((!bitheap->compressionRequired()) && (count == 0) && (bitheap->bits[count].size() < 2))
			{
				//the bitheap is compressed, all that is left
				//	is to add the lsb bit to the result
				bitheap->op->vhdl << tab
						<< bitheap->op->declare(join("tmp_bitheapResult_bh", bitheap->guid, "_", count));
				if(bitheap->bits[count].size() > 0)
					bitheap->op->vhdl << " <= " << bitheap->bits[count][0]->getName() << ";" << endl;
				else
					bitheap->op->vhdl << " <= \'0\';" << endl;

				chunksDone.insert(chunksDone.begin(), join("tmp_bitheapResult_bh", bitheap->guid, "_", count));
			}else{
				//no new columns that have been compressed
				return;
			}
		}else if(count > compressionDoneIndex){
			//new columns have been compressed

			//create the signals for a new chunk
			if(count-compressionDoneIndex+(compressionDoneIndex == 0 ? 1 : 0) > 1)
				bitheap->op->vhdl << tab
					<< bitheap->op->declare(join("tmp_bitheapResult_bh", bitheap->guid, "_", count),
							count-compressionDoneIndex+(compressionDoneIndex == 0 ? 1 : 0), true) << " <= " ;
			else
				bitheap->op->vhdl << tab
					<< bitheap->op->declare(join("tmp_bitheapResult_bh", bitheap->guid, "_", count)) << " <= " ;

			//add the bits to the chunk
			//	add the first bit
			if(bitheap->bits[count].size() > 0)
				bitheap->op->vhdl << bitheap->bits[count][0]->getName();
			else
				bitheap->op->vhdl << "\"0\"";
			//	add the rest of the bits
			for(unsigned i=count-1; i>compressionDoneIndex; i--)
			{
				if(bitheap->bits[i].size() > 0)
					bitheap->op->vhdl << " & " << bitheap->bits[i][0]->getName();
				else
					bitheap->op->vhdl << " & \"0\"";
			}
			//add one last bit, if necessary
			if(compressionDoneIndex == 0)
			{
				if(bitheap->bits[0].size() > 0)
					bitheap->op->vhdl << " & " << bitheap->bits[0][0]->getName();
				else
					bitheap->op->vhdl << " & \"0\"";
			}
			bitheap->op->vhdl << ";" << endl;

			//add the new chunk to the vector of compressed chunks
			chunksDone.push_back(join("tmp_bitheapResult_bh", bitheap->guid, "_", count));

			//advance the compression done index
			compressionDoneIndex = count;
		}
	}


	void CompressionStrategy::concatenateChunks()
	{
		if(chunksDone.size() == 0)
			THROWERROR("No chunks to concatenate in concatenateChunksDone");

		//create the signal containing the result of the bitheap compression
		bitheap->op->vhdl << tab
				<< bitheap->op->declare(join("bitheapResult_bh", bitheap->guid), bitheap->width) << " <= ";
		// the chunks in reverse order,	from msb to lsb
		bitheap->op->vhdl << chunksDone[chunksDone.size()-1];
		for(int i=(int)chunksDone.size()-2; i>=0; i--)
			bitheap->op->vhdl << " & " << chunksDone[i];
		bitheap->op->vhdl << ";" << endl;
	}
}




