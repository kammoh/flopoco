
#include "BitheapNew.hpp"

namespace flopoco {

	BitheapNew::BitheapNew(Operator* op_, unsigned width_, bool isSigned_, string name_, int compressionType_) :
		msb(width_-1), lsb(0),
		width(width_),
		height(0),
		name(name_),
		isSigned(isSigned_),
		op(op_),
		compressionType(compressionType_)
	{
		initialize();
	}


	BitheapNew::BitheapNew(Operator* op_, int msb_, int lsb_, bool isSigned_, string name_, int compressionType_) :
		msb(msb_), lsb(lsb_),
		width(msb_-lsb_+1),
		height(0),
		name(name_),
		isSigned(isSigned_),
		op(op_),
		compressionType(compressionType_)
	{
		initialize();
	}


	BitheapNew::~BitheapNew() {
		//erase the bits from the bit vector, as well as from the history
		for(unsigned i=0; i<bits.size(); i++)
		{
			bits[i].clear();
		}
		bits.clear();

		for(unsigned i=0; i<bits.size(); i++)
		{
			history[i].clear();
		}
		history.clear();
	}


	void BitheapNew::initialize()
	{
		stringstream s;

		srcFileName = op->getSrcFileName() + ":BitHeap"; // for REPORT to work
		guid = Operator::getNewUId();

		s << op->getName() << "_BitHeap_"<< name << "_" << guid;
		uniqueName_ = s.str();

		REPORT(DEBUG, "Creating BitHeap of width " << width);

		//set up the vector of lists of weighted bits, and the vector of bit uids
		for(unsigned i=0; i<width; i++)
		{
			bitUID.push_back(0);
			vector<Bit*> t, t2;
			bits.push_back(t);
			history.push_back(t2);
		}

		//initialize the constant bits
		constantBits = mpz_class(0);

		//initialize the VHDL code buffer
		vhdlCode.str("");

		//create a compression strategy
		compressionStrategy = nullptr;
		isCompressed = false;

		//create a Plotter object for the SVG output
		plotter = nullptr;
		drawCycleLine = true;
		drawCycleNumber = 1;
	}


	Bit* BitheapNew::addBit(int weight, string rhsAssignment)
	{
		REPORT(FULL, "adding a bit at weight " << weight << " with rhs=" << rhsAssignment);

		if((weight < lsb) || (weight>msb))
		{
			REPORT(INFO, "WARNING: in addBit, weight=" << weight
					<< " out of the bit heap range ("<< msb << ", " << lsb << ")... ignoring it");
			return nullptr;
		}

		//create a new bit
		//	the bit's constructor also declares the signal
		Bit* bit = new Bit(this, rhsAssignment, weight, BitType::free);

		//insert the new bit so that the vector is sorted by bit (cycle, delay)
		insertBitInColumn(bit, weight-lsb);

		REPORT(DEBUG, "added bit named "  << bit->getName() << " on column " << weight
				<< " at cycle=" << bit->signal->getCycle() << " cp=" << bit->signal->getCriticalPath());

		printColumnInfo(weight);

		isCompressed = false;

		return bit;
	}


#if 0 // don't see where it is used

	void BitheapNew::addBit(int weight, Signal *signal, int index)
	{
		// check this bit exists in the signal
		if((index < 0) || (index > signal->width()-1 ))
			THROWERROR("addBit: Index (=" << index << ") out of signal bit range");
		string rhs;
		if(signal->width()==1)
			rhs = signal->getName();
		else
			rhs = signal->getName() + of(index);

		addBit(weight, rhs);
	}

#endif



	
	void BitheapNew::insertBitInColumn(Bit* bit, unsigned columnNumber)
	{
		vector<Bit*>::iterator it = bits[columnNumber].begin();
		bool inserted = false;

		//if the column is empty, then just insert the bit
		if(bits[columnNumber].size() == 0)
		{
			bits[columnNumber].push_back(bit);
			return;
		}

		//insert the bit in the column
		//	in the lexicographic order of the timing
		while(it != bits[columnNumber].end())
		{
			if((bit->signal->getCycle() < (*it)->signal->getCycle())
					|| ((bit->signal->getCycle() == (*it)->signal->getCycle())
							&& (bit->signal->getCriticalPath() < (*it)->signal->getCriticalPath())))
			{
				bits[columnNumber].insert(it, bit);
				inserted = true;
				break;
			}else{
				it++;
			}
		}

		if(inserted == false)
		{
			bits[columnNumber].insert(bits[columnNumber].end(), bit);
		}

		isCompressed = false;
	}


	void BitheapNew::removeBit(int weight, int direction)
	{
		if((weight < lsb) || (weight > msb))
			THROWERROR("Weight " << weight << " out of the bit heap range ("<< msb << ", " << lsb << ") in removeBit");

		vector<Bit*> bitsColumn = bits[weight-lsb];

		//if dir=0 the bit will be removed from the beginning of the list,
		//	else from the end of the list of weighted bits
		if(direction == 0)
			bitsColumn.erase(bitsColumn.begin());
		else if(direction == 1)
			bitsColumn.pop_back();
		else
			THROWERROR("Invalid direction for removeBit: direction=" << direction);

		REPORT(DEBUG,"removed a bit from column " << weight);

		isCompressed = false;
	}


	void BitheapNew::removeBit(int weight, Bit* bit)
	{
		if((weight < lsb) || (weight > msb))
			THROWERROR("Weight " << weight << " out of the bit heap range ("<< msb << ", " << lsb << ") in removeBit");

		bool bitFound = false;
		vector<Bit*> bitsColumn = bits[weight-lsb];
		vector<Bit*>::iterator it = bitsColumn.begin();

		//search for the bit and erase it,
		//	if it is present in the column
		while(it != bitsColumn.end())
		{
			if((*it)->getUid() == bit->getUid())
			{
				bitsColumn.erase(it);
				bitFound = true;
			}else{
				it++;
			}
		}
		if(bitFound == false)
			THROWERROR("Bit " << bit->getName() << " with uid=" << bit->getUid()
					<< " not found in column with weight=" << weight);

		REPORT(DEBUG,"removed bit " << bit->getName() << " from column " << weight);

		isCompressed = false;
	}


	void BitheapNew::removeBits(int weight, unsigned count, int direction)
	{
		if((weight < lsb) || (weight > msb))
			THROWERROR("Weight " << weight << " out of the bit heap range ("<< msb << ", " << lsb << ") in removeBit");

		vector<Bit*> bitsColumn = bits[weight-lsb];
		unsigned currentCount = 0;

		if(count > bitsColumn.size())
		{
			REPORT(DEBUG, "WARNING: column with weight=" << weight << " only contains "
					<< bitsColumn.size() << " bits, but " << count << " are to be removed");
			count = bitsColumn.size();
		}

		while(currentCount < count)
		{
			removeBit(weight, direction);
		}

		isCompressed = false;
	}


	void BitheapNew::removeBits(int msb, int lsb, unsigned count, int direction)
	{
		if(lsb < this->lsb)
			THROWERROR("LSB (=" << lsb << ") out of bitheap bit range  ("<< this->msb << ", " << this->lsb << ") in removeBit");
		if(msb > this->lsb)
			THROWERROR("MSB (=" << msb << ") out of bitheap bit range  ("<< this->msb << ", " << this->lsb << ") in removeBit");

		for(int i=lsb; i<=msb; i++)
		{
			unsigned currentWeight = lsb - this->lsb;

			removeBits(currentWeight, count, direction);
		}

		isCompressed = false;
	}


	void BitheapNew::removeCompressedBits()
	{
		for(unsigned i=0; i<width; i++)
		{
			vector<Bit*>::iterator it = bits[i].begin(), lastIt = it;

			//search for the bits marked as compressed and erase them
			while(it != bits[i].end())
			{
				if((*it)->type == BitType::compressed)
				{
					bits[i].erase(it);
					it = lastIt;
				}else{
					lastIt = it;
					it++;
				}
			}
		}
	}


	void BitheapNew::markBit(int weight, unsigned number, BitType type)
	{
		if((weight < lsb) || (weight > msb))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range ("<< msb << ", " << lsb << ")  in markBit");
	
		if(number >= bits[weight-lsb].size())
			THROWERROR("Column with weight=" << weight << " only contains "
					<< bits[weight].size() << " bits, but bit number=" << number << " is to be marked");

		bits[weight-lsb][number]->type = type;
	}


	void BitheapNew::markBit(Bit* bit, BitType type)
	{
		bool bitFound = false;

		for(unsigned i=0; i<=width; i++)
		{
			vector<Bit*>::iterator it = bits[i].begin(), lastIt = it;

			//search for the bit
			while(it != bits[i].end())
			{
				if(((*it)->getName() == bit->getName()) && ((*it)->getUid() == bit->getUid()))
				{
					(*it)->type = type;
					bitFound = true;
					return;
				}else{
					it++;
				}
			}
		}
		if(bitFound == false)
			THROWERROR("Bit=" << bit->getName() << " with uid="
					<< bit->getUid() << " not found in bitheap");
	}


	void BitheapNew::markBits(int msb, int lsb, BitType type, unsigned number)
	{
		if(lsb < this->lsb)
			THROWERROR("markBits: LSB (=" << lsb << ") out of bitheap bit range ("<< this->msb << ", " << this->lsb << ")");
		if(msb > this->lsb)
			THROWERROR("markBits: MSB (=" << msb << ") out of bitheap bit range ("<< this->msb << ", " << this->lsb << ")");

		for(int i=lsb; i<=msb; i++)
		{
			unsigned currentWeight = i - this->lsb;
			unsigned currentNumber = number;

			if(number >= bits[i].size())
			{
				REPORT(DEBUG, "Column with weight=" << currentWeight << " only contains "
						<< bits[i].size() << " bits, but number=" << number << " bits are to be marked");
				currentNumber = bits[i].size();
			}

			for(unsigned j=0; j<currentNumber; j++)
				markBit(currentWeight, j, type);
		}
	}


	void BitheapNew::markBits(vector<Bit*> bits, BitType type)
	{
		for(unsigned i=0; i<bits.size(); i++)
			markBit(bits[i], type);
	}


	void BitheapNew::markBits(Signal *signal, BitType type, int weight)
	{
		int startIndex;

		if(weight > msb)
		{
			THROWERROR("Error in markBits: weight cannot be more than the msb: weight=" << weight);
		}

		if(weight < lsb)
			startIndex = 0;
		else
			startIndex = weight - lsb;

		for(int i=startIndex; (unsigned)i<bits.size(); i++)
		{
			for(unsigned int j=0; j<bits[i].size(); j++)
			{
				if(bits[i][j]->getRhsAssignment().find(signal->getName()) != string::npos)
				{
					bits[i][j]->type = type;
				}
			}
		}
	}


	void BitheapNew::markBitsForCompression()
	{
		for(unsigned i=0; i<width; i++)
		{
			for(unsigned j=0; j<bits[i].size(); j++)
			{
				if(bits[i][j]->type == BitType::justAdded)
				{
					bits[i][j]->type = BitType::free;
				}
			}
		}
	}


	void BitheapNew::colorBits(BitType type, unsigned int newColor)
	{
		for(unsigned i=0; i<width; i++)
		{
			for(unsigned j=0; j<bits[i].size(); j++)
			{
				if(bits[i][j]->type == type)
				{
					bits[i][j]->colorCount = newColor;
				}
			}
		}
	}



	void BitheapNew::addConstantOneBit(int weight)
	{
		if( (weight < lsb) || (weight > msb) )
			THROWERROR("addConstantOneBit(): weight (=" << weight << ") out of bitheap bit range  ("<< this->msb << ", " << this->lsb << ")");

		constantBits += (mpz_class(1) << (weight-lsb));

		isCompressed = false;
	}


	void BitheapNew::subtractConstantOneBit(int weight)
	{
		if( (weight < lsb) || (weight > msb) )
			THROWERROR("subtractConstantOneBit(): weight (=" << weight << ") out of bitheap bit range  ("<< this->msb << ", " << this->lsb << ")");

		constantBits -= (mpz_class(1) << (weight-lsb));

		isCompressed = false;
	}


	void BitheapNew::addConstant(mpz_class constant, int shift)
	{
		if( (shift < lsb) || (shift+intlog2(constant) > msb) )
			THROWERROR("addConstant: Constant " << constant << " shifted by " << shift << " has bits out of the bitheap range ("<< this->msb << ", " << this->lsb << ")");

		constantBits += (constant << (shift-lsb));

		isCompressed = false;
	}



	void BitheapNew::subtractConstant(mpz_class constant, int shift)
	{
		if( (shift < lsb) || (shift+intlog2(constant) > msb) )
			THROWERROR("subtractConstant: Constant " << constant << " shifted by " << shift << " has bits out of the bitheap range ("<< this->msb << ", " << this->lsb << ")");

		constantBits -= (constant << (shift-lsb));

		isCompressed = false;
	}







	void BitheapNew::addSignal(string signalName, int shift)
	{

		REPORT(0, "addSignal	" << signalName << " shift=" << shift);
		Signal* s = op->getSignalByName(signalName);
		int sMSB = s->MSB();
		int sLSB = s->LSB();
	
		// No error reporting here: the current situation is that addBit will ignore bits thrown out of the bit heap (with a warning)
	
		if( (sLSB+shift < lsb) || (sMSB+shift > msb) )
			REPORT(0,"WARNING: addSignal(): " << signalName << " shifted by " << shift << " has bits out of the bitheap range ("<< this->msb << ", " << this->lsb << ")");

		if(s->isSigned()) { // We must sign-extend it, using the constant trick

			if(!op->getSignalByName(signalName)->isBus()) {
				// getting here doesn't make much sense
				THROWERROR("addSignal(): signal " << signalName << " is not a bus, but is signed : this looks like a bug");
			}

			// Now we have a bit vector but it might be of width 1, in which case the following loop is empty.
			for(int w=sLSB; w<=sMSB-1; w++) {
				addBit(w+shift,	 signalName + of(w-sLSB));
			}
			if(sMSB==this->msb) { // No point in complementing it and adding a constant bit
				addBit(sMSB+shift,	 signalName + of(sMSB-sLSB));
			}
			else{
				// complement the leading bit
				addBit(sMSB+shift,	"not("+signalName + of(sMSB-sLSB)+")");
				// add the string of ones from this bit to the MSB of the bit heap
				for(int w=sMSB; w<=this->msb-shift; w++) {
					addConstantOneBit(w+shift);
				}
			}
		}
			
		else{ // unsigned
			if(!op->getSignalByName(signalName)->isBus())
				addBit(shift,signalName);
			else {// isBus: this is a bit vector
				for(int w=sLSB; w<=sMSB; w++) {
					addBit(w+shift, signalName + of(w-sLSB));
				}
			}
		}
		
		isCompressed = false;
	}


	void BitheapNew::subtractSignal(string signalName, int shift)
	{

		REPORT(0, "subtractSignal  " << signalName << " shift=" << shift);
		Signal* s = op->getSignalByName(signalName);
		int sMSB = s->MSB();
		int sLSB = s->LSB();
		
		// No error reporting here: the current situation is that addBit will ignore bits thrown out of the bit heap (with a warning)
		
		if( (sLSB+shift < lsb) || (sMSB+shift > msb) )
			REPORT(0,"WARNING: subtractSignal(): " << signalName << " shifted by " << shift << " has bits out of the bitheap range ("<< this->msb << ", " << this->lsb << ")");
		
		// If the bit vector is of width 1, the following loop is empty.
		for(int w=sLSB; w<=sMSB; w++) {
			addBit(w+shift,	 "not(" + signalName + of(w-sLSB) + ")"  );
		}
		// add the string of ones from this bit to the MSB of the bit heap
		for(int w=sMSB+1; w<=this->msb-shift; w++) {
			 addConstantOneBit(w+shift);
		}
		addConstantOneBit(sLSB+shift);

		isCompressed = false;
	}



	
	void BitheapNew::resizeBitheap(unsigned newWidth, int direction)
	{
		if((direction != 0) && (direction != 1))
			THROWERROR("Invalid direction in resizeBitheap: direction=" << direction);
		if(newWidth < width)
		{
			REPORT(DEBUG, "WARNING: resizing the bitheap from width="
					<< width << " to width=" << newWidth);
			if(direction == 0){
				REPORT(DEBUG, "\t will loose the information in the lsb columns");
			}else{
				REPORT(DEBUG, "\t will loose the information in the msb columns");
			}
		}

		//add/remove the columns
		if(newWidth < width)
		{
			//remove columns
			if(direction == 0)
			{
				//remove lsb columns
				bits.erase(bits.begin(), bits.begin()+(width-newWidth));
				history.erase(history.begin(), history.begin()+(width-newWidth));
				bitUID.erase(bitUID.begin(), bitUID.begin()+(width-newWidth));
				lsb += width-newWidth;
			}else{
				//remove msb columns
				bits.resize(newWidth);
				history.resize(newWidth);
				bitUID.resize(newWidth);
				msb -= width-newWidth;
			}
		}else{
			//add columns
			if(direction == 0)
			{
				//add lsb columns
				vector<Bit*> newVal;
				bits.insert(bits.begin(), newWidth-width, newVal);
				history.insert(history.begin(), newWidth-width, newVal);
				bitUID.insert(bitUID.begin(), newWidth-width, 0);
				lsb -= width-newWidth;
			}else{
				//add msb columns
				bits.resize(newWidth-width);
				history.resize(newWidth-width);
				bitUID.resize(newWidth-width);
				msb += width-newWidth;
			}
		}

		//update the information inside the bitheap
		width = newWidth;
		height = getMaxHeight();
		for(int i=lsb; i<=msb; i++)
			for(unsigned j=0; j<bits[i-lsb].size(); j++)
				bits[i-lsb][j]->weight = i;

		isCompressed = false;
	}


	void BitheapNew::resizeBitheap(int newMsb, int newLsb)
	{
		if((newMsb < newLsb))
			THROWERROR("Invalid arguments in resizeBitheap: newMsb=" << newMsb << " newLsb=" << newLsb);
		if(newMsb<msb || newLsb>lsb)
		{
			REPORT(DEBUG, "WARNING: resizing the bitheap from msb="
					<< msb << " lsb=" << lsb << " to newMsb=" << newMsb << " newLsb=" << newLsb);
			if(newMsb < msb){
				REPORT(DEBUG, "\t will loose the information in the msb columns");
			}
			if(newLsb>lsb){
				REPORT(DEBUG, "\t will loose the information in the lsb columns");
			}
		}

		//resize on the lsb, if necessary
		if(newLsb > lsb)
			resizeBitheap(msb-newLsb+1, 0);
		//resize on the msb, if necessary
		if(newMsb < msb)
			resizeBitheap(newMsb-newLsb+1, 1);

		isCompressed = false;
	}


	void BitheapNew::mergeBitheap(BitheapNew* bitheap)
	{
		if(op->getName() != bitheap->op->getName())
			THROWERROR("Cannot merge bitheaps belonging to different operators!");
		if(isSigned != bitheap->isSigned)
			REPORT(DEBUG, "WARNING: merging bitheaps with different signedness");

		//resize if necessary
		if(width < bitheap->width)
		{
			if(lsb > bitheap->lsb)
				resizeBitheap(msb, bitheap->lsb);
			if(msb < bitheap->msb)
				resizeBitheap(bitheap->msb, lsb);
		}
		//add the bits
		for(int i=bitheap->lsb; i<bitheap->msb; i++) {
			for(unsigned j=0; j<bitheap->bits[i-bitheap->lsb].size(); j++)
				insertBitInColumn(bitheap->bits[i-bitheap->lsb][j], i-bitheap->lsb + (lsb-bitheap->lsb));
		}
		//make the bit all point to this bitheap
		for(unsigned i=0; i<width; i++)
			for(unsigned j=0; j<bits[i].size(); j++)
				bits[i][j]->bitheap = this;

		isCompressed = false;
	}


	void BitheapNew::startCompression()
	{
		//create a new compression strategy, if one isn't present already
		if(compressionStrategy == nullptr)
			compressionStrategy = new CompressionStrategy(this);
		//start the compression
		compressionStrategy->startCompression();
		//mark the bitheap compressed
		isCompressed = true;
	}


	string BitheapNew::getSumName()
	{
		return join("bitheapResult_bh", guid);
	}


	string BitheapNew::getSumName(int msb, int lsb)
	{
		return join(join("bitheapResult_bh", guid), range(msb, lsb));
	}


	unsigned BitheapNew::getMaxHeight()
	{
		unsigned maxHeight = 0;

		for(unsigned i=0; i<bits.size(); i++)
			if(bits[i].size() > maxHeight)
				maxHeight = bits[i].size();
		height = maxHeight;

		return height;
	}


	unsigned BitheapNew::getColumnHeight(int weight)
	{
		if((weight < lsb) || (weight > msb))
			THROWERROR("Invalid argument for getColumnHeight: weight=" << weight);

		return bits[weight-lsb].size();
	}


	bool BitheapNew::compressionRequired()
	{
		if(getMaxHeight() < 3){
			return false;
		}else if(height > 3){
			return true;
		}else{
			for(unsigned i=1; i<bits.size(); i++)
				if(bits[i].size() > 2)
					return true;
			return false;
		}
	}


	Plotter* BitheapNew::getPlotter()
	{
		return plotter;
	}


	Operator* BitheapNew::getOp()
	{
		return op;
	}


	int BitheapNew::getGUid()
	{
		return guid;
	}


	string BitheapNew::getName()
	{
		return name;
	}


	void BitheapNew::setIsSigned(bool newIsSigned)
	{
		isSigned = newIsSigned;
	}


	bool BitheapNew::getIsSigned()
	{
		return isSigned;
	}


	int BitheapNew::newBitUid(unsigned weight)
	{
		if(((int)weight < lsb) || ((int)weight > msb))
			THROWERROR("Invalid argument for newBitUid: weight=" << weight);

		int returnVal = bitUID[weight-lsb];

		bitUID[weight-lsb]++;
		return returnVal;
	}


	void BitheapNew::printColumnInfo(int weight)
	{
		if((weight < lsb) || (weight > msb))
			THROWERROR("Invalid argument for printColumnInfo: weight=" << weight);

		for(unsigned i=0; i<bits[weight-lsb].size(); i++)
		{
			REPORT(FULL, "\t column weight=" << weight << " name=" << bits[weight-lsb][i]->getName()
					<< " cycle=" << bits[weight-lsb][i]->signal->getCycle()
					<< " criticaPath=" << bits[weight-lsb][i]->signal->getCriticalPath());
		}
	}


	void BitheapNew::printBitHeapStatus()
	{
		REPORT(DEBUG, "Bitheap status:");
		for(unsigned w=0; w<bits.size(); w++)
		{
			REPORT(DEBUG, "Column weight=" << w << ":\t height=" << bits[w].size());
			printColumnInfo(w);
		}
	}


	vector<vector<Bit*>> BitheapNew::getBits()
	{
		return bits;
	}


	CompressionStrategy* BitheapNew::getCompressionStrategy()
	{
		return compressionStrategy;
	}


	void BitheapNew::initializeDrawing()
	{

	}


	void BitheapNew::closeDrawing(int offsetY)
	{

	}


	void BitheapNew::drawConfiguration(int offsetY)
	{

	}

	void BitheapNew::drawBit(int cnt, int w, int turnaroundX, int offsetY, int c)
	{

	}



} /* namespace flopoco */

















