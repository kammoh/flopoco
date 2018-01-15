
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

		REPORT(DEBUG, "compressionDelay is " << compressionDelay << " and stagesPerCycle is " << stagesPerCycle);


		//generate all the compressors that can be used
		generatePossibleCompressors();

		//create the plotter
		bitheapPlotter = new BitheapPlotter(bitheap);
	}

	CompressionStrategy::~CompressionStrategy(){
	}

	void CompressionStrategy::orderBitsByColumnAndStage()
	{
		REPORT(DEBUG, "in orderBitsByColumnAndStage")
		double objectiveDelay = 1.0/bitheap->op->getTarget()->frequency();

		//first get the minimal cycle and maximum cycle and delay
		unsigned int minCycle = 0 - 1;
		unsigned int maxCycle = 0;
		double minDelay = 1.0;
		double maxDelay = -1.0;
		Bit* minBit;
		Bit* maxBit;
		for(unsigned int i = 0; i < bitheap->bits.size(); i++){
			for(unsigned int j = 0; j < bitheap->bits[i].size(); j++){
				if(bitheap->bits[i][j]->signal->getCycle() > maxCycle){
					maxCycle = bitheap->bits[i][j]->signal->getCycle();
					maxDelay = -1.0;
				}
				if(bitheap->bits[i][j]->signal->getCycle() < minCycle){
					minCycle = bitheap->bits[i][j]->signal->getCycle();
					minDelay = 1.0;
				}

				if(bitheap->bits[i][j]->signal->getCycle() == maxCycle){
					if(bitheap->bits[i][j]->signal->getCriticalPath() > maxDelay){
						maxDelay = bitheap->bits[i][j]->signal->getCriticalPath();
						maxBit = bitheap->bits[i][j];
					}
				}
				if(bitheap->bits[i][j]->signal->getCycle() == minCycle){
					if(bitheap->bits[i][j]->signal->getCriticalPath() < minDelay){
						minDelay = bitheap->bits[i][j]->signal->getCriticalPath();
						minBit = bitheap->bits[i][j];
					}
				}
			}
		}
		REPORT(DEBUG, "max is cycle " << maxCycle << " with delay " << maxDelay);
		REPORT(DEBUG, "min is cycle " << minCycle << " with delay " << minDelay);
		REPORT(DEBUG, "minBit is " << minBit << " and maxBit is " << maxBit);

		REPORT(DEBUG, "objectiveDelay = " << objectiveDelay);
		REPORT(DEBUG, "compressionDelay = " << compressionDelay);
		vector<double> boundaries;
		double remainingTime = objectiveDelay;
		while(remainingTime >= compressionDelay){
			remainingTime -= compressionDelay;
			boundaries.insert(boundaries.begin(), remainingTime);
		}
		for(unsigned int i = 0; i < boundaries.size(); i++){
			REPORT(DEBUG, "boundary number " << i << " is at " << boundaries[i]);
		}

		//get the first and last stage, where bits arrive.

		unsigned int maxStage;
		unsigned int minStage;
		maxStage = getStageOfArrivalForBit(maxBit);
		minStage = getStageOfArrivalForBit(minBit);

		maxStage -= minStage;
		//in [0 ... maxStage] can bits arrive
		REPORT(DEBUG, "maxStage is " << maxStage << " with a offset(minStage) of " << minStage);

		orderedBits.resize(maxStage + 1);
		for(unsigned s = 0; s < orderedBits.size(); s++){
			orderedBits[s].resize(bitheap->width);
		}

		//add the bits to orderedBits
		for(unsigned int c = 0; c < bitheap->bits.size(); c++){
			for(unsigned int i = 0; i < bitheap->bits[c].size(); i++){
				unsigned int tempStage = getStageOfArrivalForBit(bitheap->bits[c][i]);
				tempStage -= minStage;	//subtract the offset
				orderedBits[tempStage][c].push_back(bitheap->bits[c][i]); //TODO: check if we set the new criticalPath accordingly
				REPORT(DEBUG, "added Bit with cycle " << bitheap->bits[c][i]->signal->getCycle() << " and criticalPath " << bitheap->bits[c][i]->signal->getCriticalPath() << " to stage " << tempStage << " and column " << c);
			}
		}
	}

	void CompressionStrategy::fillBitAmounts(){
		bitAmount.resize(orderedBits.size());
		for(unsigned int s = 0; s < orderedBits.size(); s++){
			bitAmount[s].resize(orderedBits[s].size() + 4, 0);	//add four because we allow compressors to have outputbits in higher columns than the MSB of the bitheap. Those outputbits will be removed.
		}
		for(unsigned int s = 0; s < orderedBits.size(); s++){
			for(unsigned int c = 0; c < orderedBits[s].size(); c++){
				bitAmount[s][c] = orderedBits[s][c].size();
			}
		}
	}

	double CompressionStrategy::getCompressionEfficiency(unsigned int stage, unsigned int column, BasicCompressor* compressor, unsigned int middleLength){
		int inputBits = 0;
		int outputBits = 0;
		for(unsigned int c = 0; c < compressor->getHeights(middleLength); c++){
			if(bitAmount.size() > stage && bitAmount[stage].size() > column + c){
				//position exists, now check how many bits there are to compress
				if(bitAmount[stage][column + c] >= (int)compressor->getHeightsAtColumn(c, middleLength)){
					inputBits += compressor->getHeightsAtColumn(c, middleLength);
				}
				else{
					if(bitAmount[stage][column + c] > 0){ //preventing negative values (holes)
						inputBits += bitAmount[stage][column + c];
					}
				}
			}
		}
		for(unsigned int c = 0; c < compressor->getOutHeights(middleLength); c++){
			if(compressor->getOutHeightsAtColumn(c, middleLength) > 0){
				outputBits += compressor->getOutHeightsAtColumn(c, middleLength);
			}
		}
		double ratio = 0.0;
		if(compressor->getArea(middleLength) != 0.0){
			ratio = ((double)(inputBits - outputBits)) / compressor->getArea(middleLength);
		}	//else return 0.0
		return ratio;
	}

	void CompressionStrategy::placeCompressor(unsigned int stage, unsigned int column, BasicCompressor* compressor, unsigned int middleLength){
		for(unsigned int c = 0; c < compressor->getHeights(middleLength); c++){
			if(bitAmount.size() > stage && bitAmount[stage].size() > column + c){
				bitAmount[stage][column + c] -= compressor->getHeightsAtColumn(c, middleLength);
			}
		}
		for(unsigned int c = 0; c < compressor->getOutHeights(middleLength); c++){
			if(bitAmount.size() <= stage + 1){
				THROWERROR("tried to add bits into a stage, which does not exist");
			}
			else{
				if(bitAmount[stage + 1].size() > column + c){	//drop outputbits which go beyond the border
					bitAmount[stage + 1][column + c] += compressor->getOutHeightsAtColumn(c, middleLength);
				}
			}
		}
		solution.addCompressor(stage, column, compressor, middleLength);
	}

	void CompressionStrategy::printBitAmounts(){
		//first find max value length and maxcolumn
		unsigned int maxValueDigits = 1;
		unsigned int maxColumn = 1;
		for(unsigned int s = 0; s < bitAmount.size(); s++){
			if(bitAmount[s].size() > maxColumn){
				maxColumn = bitAmount[s].size();
			}
			for(unsigned int c = 0; c < bitAmount[s].size(); c++){
				unsigned int currentMaxValueDigits = 0;
				int value = bitAmount[s][c];
				if(value < 0){
					value *= (-1);
				}
				while(value > 0){
					currentMaxValueDigits++;
					value /= 10;
				}

				if(bitAmount[s][c] < 0){
					currentMaxValueDigits++;
				}

				if(currentMaxValueDigits > maxValueDigits){
					maxValueDigits = currentMaxValueDigits;
				}
			}

		}

		for(unsigned int s = 0; s < bitAmount.size(); s++){
			ostringstream line;
			line.str("");
			unsigned int tempColumn = maxColumn;
			while(tempColumn > 0){
				tempColumn--;
				unsigned int addZeros = maxValueDigits + 1;
				if(bitAmount[s].size() > tempColumn){
					int value = bitAmount[s][tempColumn];
					while(value != 0){
						addZeros--;
						value /= 10;
					}
					if(bitAmount[s][tempColumn] <= 0){
						addZeros--;	//- sign or zero
					}
					for(unsigned int k = 0; k < addZeros; k++){
						line << " ";
					}
					line << bitAmount[s][tempColumn] << " ";
				}
			}
			cout << line.str() << endl;
		}
	}

	void CompressionStrategy::applyAllCompressorsFromSolution(){
		REPORT(DEBUG, "applying all compressors");

		for(unsigned int cb = 0; cb < bitheap->bits.size(); cb++){
			for(unsigned int k = 0; k < bitheap->bits[cb].size(); k++){
				Bit* bit = bitheap->bits[cb][k];
			}
		}
		for(unsigned int s = 0; s < bitAmount.size(); s++){
			for(unsigned int c = 0; c < bitheap->width; c++){	//drop all compressors which start > MSB
				vector<pair<BasicCompressor*, unsigned int> > tempVector;
				tempVector = solution.getCompressorsAtPosition(s, c);
				for(unsigned int j = 0; j < tempVector.size(); j++){
					//applyCompressor
					Compressor* realCompressor = tempVector[j].first->getCompressor(); 	//TODO: consider middleLength
					unsigned int middleLength = tempVector[j].second;
					vector<vector<Bit*> > tempBitVector;
					tempBitVector.resize(realCompressor->heights.size());
					//fill Bitvectors
					for(unsigned int cTemp = 0; cTemp < realCompressor->heights.size(); cTemp++){
						int maxBitsToCompress = realCompressor->heights[cTemp];
						unsigned int bitsFound = 0;
						for(unsigned int k = 0; k < bitheap->bits[c + cTemp].size(); k++){
							if(bitsFound < maxBitsToCompress){
								Bit* tempBit = bitheap->bits[c + cTemp][k];
								if(tempBit->type == BitType::free && getStageOfArrivalForBit(tempBit) == s){
									bitsFound++;
									tempBitVector[cTemp].push_back(tempBit);
								}
							}
						}
					}
					applyCompressor(tempBitVector, realCompressor, c);
				}
			}
			putRemainingBitsIntoNextStage(s);
			bitheap->removeCompressedBits();
		}
		concatenateLSBColumns();
	}

	void CompressionStrategy::putRemainingBitsIntoNextStage(unsigned int s){
		for(unsigned int c = 0; c < bitheap->bits.size(); c++){
			for(unsigned int j = 0; j < bitheap->bits[c].size(); j++){
				Bit* bit = bitheap->bits[c][j];
				if(bit->type == BitType::free){
					if(getStageOfArrivalForBit(bit) == s){
						double cp = bit->signal->getCriticalPath();
						cp += compressionDelay;
						bit->signal->setCriticalPath(cp);
					}
				}
			}
		}
		setCycleRight();
	}

	bool CompressionStrategy::checkAlgorithmReachedAdder(unsigned int adderHeight, unsigned int stage){
		if(stage >= bitAmount.size()){
			THROWERROR("Doing the check if the algorithm for generating the compressortree is finished. Tried to access stage " << stage << " but there aren't that many stages");
		}
		for(unsigned int c = 0; c < bitAmount[stage].size(); c++){
			if(bitAmount[stage][c] > (int) adderHeight){
				return false;	//column exists where there are more bits left
			}
		}

		//now check if in all other stages there are no bits left
		for(unsigned int s = 0; s < bitAmount.size(); s++){
			if(s != stage){
				for(unsigned int c = 0; c < bitAmount[s].size(); c++){
					if(bitAmount[s][c] > 0){
						return false;
					}
				}
			}
		}

		//both tests went succesfull
		return true;
	}


	unsigned CompressionStrategy::getStageOfArrivalForBit(Bit* bit){
		//for a bit to be in stage i, its criticalPath delay has to be smaller or equal to boundaries[i].
		//(for the same cycle)
		double objectiveDelay = 1.0/bitheap->op->getTarget()->frequency();
		vector<double> boundaries;
		double remainingTime = objectiveDelay;
		while(remainingTime >= compressionDelay){
			remainingTime -= compressionDelay;
			boundaries.insert(boundaries.begin(), remainingTime);
		}
		unsigned int stage = bit->signal->getCycle() * stagesPerCycle;	//maxStage = number of stages - 1
		double tempCriticalPath = bit->signal->getCriticalPath();
		for(unsigned i = 0; i < boundaries.size(); i++){
			if(tempCriticalPath > boundaries[i]){
				stage++;
			}
			if(tempCriticalPath > boundaries[stagesPerCycle - 1]){
				THROWERROR("criticalPath of signal is bigger than the biggest boundary. Bit should be in next cycle");
			}
		}
		return stage;
	}

	void CompressionStrategy::setCycleRight(){
		//if the critical path of a signal exceeds the boundary of the last stage in this cycle, we have to
		//put this bit into the next cycle. e.g. if the boundaries are 2*10^-10 and 5*10^-10 and a signal has the critical path of 6*10^-10, put it into the next cycle

		double objectiveDelay = 1.0/bitheap->op->getTarget()->frequency();
		vector<double> boundaries;
		double maxDelay = objectiveDelay - compressionDelay;
		for(unsigned int c = 0; c < bitheap->bits.size(); c++){
			for(unsigned int j = 0; j < bitheap->bits[c].size(); j++){
				Bit* bit = bitheap->bits[c][j];
				if(bit->type == BitType::free){
					double tempCriticalPath = bit->signal->getCriticalPath();
					if(tempCriticalPath > maxDelay){
						bit->signal->updateLifeSpan(bit->signal->getLifeSpan() + 1);
						//bit->signal->setCycle(bit->signal->getCycle() + 1);
						bit->signal->setCriticalPath(0.0);
					}
				}
			}
		}
	}

	void CompressionStrategy::orderCompressorsByCompressionEfficiency(){
		vector<double> compressionRatio;
		vector<bool> alreadyChosen;

		for(unsigned int i = 0; i < possibleCompressors.size(); i++){
			double ratio = possibleCompressors[i]->getEfficiency();
			compressionRatio.push_back(ratio);
			alreadyChosen.push_back(false);
		}

		vector<BasicCompressor*> rightOrder;
		for(unsigned int z = 0; z < possibleCompressors.size(); z++){
			unsigned int pos = 0;
			double currentMaxRatio = -1.0;
			for(unsigned int i = 0; i < possibleCompressors.size(); i++){
				if((alreadyChosen[i] == false) && (compressionRatio[i] > currentMaxRatio)){
					currentMaxRatio = compressionRatio[i];
					pos = i;
				}
			}
			rightOrder.push_back(possibleCompressors[pos]);
			alreadyChosen[pos] = true;
			REPORT(DEBUG, "found compressor " << possibleCompressors[pos]->getStringOfIO() << " with compressionRatio of " << currentMaxRatio << ". Original compressor position is " << pos);
		}
		possibleCompressors = rightOrder;
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
			REPORT(DEBUG, "starting compressionAlgorithm");
			compressionAlgorithm();
			REPORT(DEBUG, "compressionAlgorithm ended");

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
					REPORT(DEBUG, "before applying compressor");
					Compressor* realCompressor = possibleCompressors[i]->getCompressor();
					REPORT(DEBUG, "after getCompressor");
					applyCompressor(compressorBitVector, realCompressor, bitheap->lsb+j);
					REPORT(DEBUG, "after applying compressor");
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
		// Following line commented by F2D: outPortMap does the declaration
		// bitheap->op->declare(compressorIONames.str(), compressor->wOut, (compressor->wOut > 1));
		bitheap->op->outPortMap(compressor, "R", compressorIONames.str());

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

	void CompressionStrategy::applyCompressor(vector<vector<Bit*> > inputBits, Compressor* compressor, int weight)
	{
		vector<string> compressorInputs;
		ostringstream inputName, vectorName;
		int instanceUID = Operator::getNewUId();
		unsigned int count = 0;

		bool bitVectorIsEmpty = true;
		for(unsigned int c = 0; c < inputBits.size(); c++){
			if(inputBits[c].size() > 0){
				bitVectorIsEmpty = false;
			}
		}
		if(bitVectorIsEmpty){
			THROWERROR("Bit vector to compress is empty in applyCompressor");
		}
		if(compressor == nullptr){
			THROWERROR("Compressor empty in applyCompressor");
		}

		//inputs for compressor
		for(unsigned int c = 0; c < compressor->heights.size(); c++){
			inputName.str("");
			for(unsigned int i = 0; i < compressor->heights[c]; i++){
				if(i == 0){
					if(inputBits[c].size() > i){
						inputName << inputBits[c][i]->getName();
					}
					else{
						inputName << "\"0\"";
					}
				}
				else{
					if(inputBits[c].size() > i){
						inputName << " & " << inputBits[c][i]->getName();
					}
					else{
						inputName << " & \"0\"";
					}
				}
			}
			compressorInputs.push_back(string(inputName.str()));
		}

		//inputsignals
		bitheap->op->vhdl << endl;
		for(unsigned i=0; i<compressor->heights.size(); i++)
		{
			vectorName.str("");
			vectorName << compressor->getName() << "_bh"
					<< bitheap->guid << "_uid" << instanceUID << "_In" << i;
			bitheap->op->vhdl << tab << bitheap->op->declare(vectorName.str(), compressor->heights[i])
					<< " <= \"\" & " << compressorInputs[i] << ";" << endl;
			bitheap->op->inPortMap(compressor, join("X",i), vectorName.str());
		}

		//create the signals for the comrpessor outputs. Can handle compresssors who have
		//more than one outputbit per column. If that is the case, its asumed that the outputvector
		//for e.g. the second bits spans from lsb of the compressor to the closest occurence of the in this
		//example second bit. Only the bits which are not constantly zero will be added to the bitheap.
		unsigned int tempHeight = 0;
		while(true){
			tempHeight++;
			//does this height occures in the compressor
			int lastOccurence = -1;
			for(unsigned int c = 0; c < compressor->outHeights.size(); c++){
				if(compressor->outHeights[c] >= tempHeight){
					lastOccurence = c;
				}
			}
			if(lastOccurence == -1){
				break;	//not found
			}


			vectorName.str("");
			vectorName << compressor->getName() << "_bh" << bitheap->guid << "_uid"
					<< instanceUID << "_Out" << (tempHeight - 1);
			bitheap->op->declare(vectorName.str(), lastOccurence + 1, (lastOccurence + 1 > 1));

			//check if there are more than one outputVectors in total
			bool singleOutputVector = true;
			for(unsigned int c = 0; c < compressor->outHeights.size(); c++){
				if(compressor->outHeights[c] > 1){
					singleOutputVector = false;
					break;
				}
			}
			if(singleOutputVector == true){
				bitheap->op->outPortMap(compressor, "R", vectorName.str(), false);
			}
			else{
				ostringstream tempR;
				tempR.str("");
				tempR << "R" << (tempHeight - 1);
				bitheap->op->outPortMap(compressor, tempR.str(), vectorName.str(), false);
			}

			//create the compressor
			compressor->setShared();
			bitheap->op->vhdl << bitheap->op->instance(compressor, join(compressor->getName(), "_uid", instanceUID)) << endl;

			//add the outputBits to the bitheap
			for(unsigned int c = 0; c <= lastOccurence; c++){
				if(c + weight < bitheap->width && compressor->outHeights[c] >= tempHeight){
					bitheap->addBit(c + weight, vectorName.str() + of(c));
				}
			}

		}

		//set the inputbits to compressed
		for(unsigned int c = 0; c < inputBits.size(); c++){
			for(unsigned int i = 0; i < inputBits[c].size(); i++){
				bitheap->markBit(inputBits[c][i], BitType::compressed);
			}
		}

		setCycleRight();
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
				cout << "bitheap->bits.size() is " << bitheap->bits.size() << " and bitheap->bits[bitheap->width-1].size() is " << bitheap->bits[bitheap->width-1].size() << endl;
				if(bitheap->bits[bitheap->width-1].size() > 1)
				{
					cout << " case >1" << endl;
					adderIn0 << bitheap->bits[bitheap->width-1][0]->getName();
					adderIn1 << bitheap->bits[bitheap->width-1][1]->getName();
				}else if(bitheap->bits[bitheap->width-1].size() > 0)
				{
					cout << "case > 0 (==1)" << endl;
					adderIn0 << bitheap->bits[bitheap->width-1][0]->getName();
					adderIn1 << "\"0\"";
				}else{
					cout << "case == 0" << endl;
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

			//declare the adder
			IntAdder* adder;

			//create the adder
#if 0
			bitheap->op->declare(adderOutName.str(), bitheap->msb-adderStartIndex+1+1);
			adder = new IntAdder(bitheap->op, bitheap->op->getTarget(), bitheap->msb-adderStartIndex+1+1);

			//create the port maps for the adder
			bitheap->op->inPortMap(adder, "X", adderIn0Name.str());
			bitheap->op->inPortMap(adder, "Y", adderIn1Name.str());
			bitheap->op->inPortMap(adder, "Cin", adderCinName.str());
			bitheap->op->outPortMap(adder, "R", adderOutName.str());


			//create the instance of the adder
			bitheap->op->vhdl << bitheap->op->instance(adder, join("bitheapFinalAdd_bh", bitheap->guid)) << endl;
#else
			bitheap->op->newInstance("IntAdder",
															 "bitheapFinalAdd_bh"+to_string(bitheap->guid),
															 "wIn=" + to_string(bitheap->msb-adderStartIndex+1+1),
															 "X=>"+ adderIn0Name.str()
															 + ",Y=>"+adderIn1Name.str()
															 + ",Cin=>" + adderCinName.str(),
															 "R=>"+ adderOutName.str()   );

#endif
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
		BasicCompressor *compressor;
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
			BasicCompressor *newCompressor;

			col0=6;

			newVect.push_back(col0);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 3.0, true);
			//newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			BasicCompressor *newCompressor;

			col0=4;
			col1=1;

			newVect.push_back(col0);
			newVect.push_back(col1);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 2.0, true);
			//newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			BasicCompressor *newCompressor;

			col0=5;

			newVect.push_back(col0);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 2.0, true);
			//newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			BasicCompressor *newCompressor;

			col0=3;
			col1=1;

			newVect.push_back(col0);
			newVect.push_back(col1);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 2.0, true);
			//newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			BasicCompressor *newCompressor;

			col0=4;

			newVect.push_back(col0);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 2.0, true);
			//newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			BasicCompressor *newCompressor;

			col0=3;
			col1=2;

			newVect.push_back(col0);
			newVect.push_back(col1);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 2.0, true);
			//newCompressor->setShared();
			possibleCompressors.push_back(newCompressor);
		}
		{
			vector<int> newVect;
			BasicCompressor *newCompressor;

			col0=3;

			newVect.push_back(col0);
			newCompressor = new BasicCompressor(bitheap->op->getTarget(), newVect, 1.0, true);
			//newCompressor->setShared();
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
