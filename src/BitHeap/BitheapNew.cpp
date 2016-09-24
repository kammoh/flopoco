
#include "BitheapNew.hpp"

namespace flopoco {

	BitheapNew::BitheapNew(Operator* op_, unsigned size_, bool isSigned_, string name_, int compressionType_) :
		msb(size_-1), lsb(0),
		size(size_),
		height(0),
		name(name_),
		isSigned(isSigned_),
		isFixedPoint(false),
		op(op_),
		compressionType(compressionType_)
	{
		initialize();
	}


	BitheapNew::BitheapNew(Operator* op_, int msb_, int lsb_, bool isSigned_, string name_, int compressionType_) :
		msb(msb_), lsb(lsb_),
		size(msb_-lsb_+1),
		height(0),
		name(name_),
		isSigned(isSigned_),
		isFixedPoint(true),
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

		REPORT(DEBUG, "Creating BitHeap of size " << size);

		//set up the vector of lists of weighted bits, and the vector of bit uids
		for(unsigned i=0; i<size; i++)
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


	void BitheapNew::addBit(int weight, string rhsAssignment)
	{
		if((!isFixedPoint) && (weight < 0))
			THROWERROR("Negative weight (=" << weight << ") in addBit, for an integer bitheap");

		REPORT(FULL, "adding a bit at weight " << weight << " with rhs=" << rhsAssignment);

		//ignore bits beyond the declared msb
		if(weight > msb)
		{
			REPORT(INFO, "WARNING: in addBit, weight=" << weight
					<< " greater than msb=" << msb << "... ignoring it");
			return;
		}

		//create a new bit
		//	the bit's constructor also declares the signal
		Bit* bit = new Bit(this, rhsAssignment, weight, BitType::free);

		//insert the new bit so that the vector is sorted by bit (cycle, delay)
		insertBitInColumn(bit, weight-lsb);

		REPORT(DEBUG, "added bit named "  << bit->name << " on column " << weight
				<< " at cycle=" << bit->signal->getCycle() << " cp=" << bit->signal->getCriticalPath());

		printColumnInfo(weight);

		isCompressed = false;
	}


	void BitheapNew::addBit(int weight, Signal *signal, int offset)
	{
		if((!isFixedPoint) && (weight < 0))
			THROWERROR("Negative weight (=" << weight << ") in addBit, for an integer bitheap");
		if((signal->isFix()) && ((offset < signal->LSB()) || (offset > signal->MSB())))
			THROWERROR("Offset (=" << offset << ") out of signal bit range");
		if((!signal->isFix()) && (offset >= signal->width()))
			THROWERROR("Negative weight (=" << weight << ") in addBit, for an integer bitheap");

		REPORT(FULL, "adding a bit at weight " << weight << " with rhs=" << signal->getName() << of(offset));

		//ignore bits beyond the declared msb
		if(weight > msb)
		{
			REPORT(INFO, "WARNING: in addBit, weight=" << weight
					<< " greater than msb=" << msb << "... ignoring it");
			return;
		}

		//create a new bit
		//	the bit's constructor also declares the signal
		Bit* bit = new Bit(this, signal, offset, weight, BitType::free);

		//insert the new bit so that the vector is sorted by bit (cycle, delay)
		insertBitInColumn(bit, weight-lsb);

		REPORT(DEBUG, "added bit named "  << bit->name << " on column " << weight
			<< " at cycle=" << bit->signal->getCycle() << " cp=" << bit->signal->getCriticalPath());
		printColumnInfo(weight);

		isCompressed = false;
	}


	void BitheapNew::addBit(Signal *signal, int offset, int weight)
	{
		addBit(weight, signal, offset);
	}


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
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		vector<Bit*> bitsColumn = bits[weight];

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
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		bool bitFound = false;
		vector<Bit*> bitsColumn = bits[weight];
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
			THROWERROR("Bit " << bit->name << " with uid=" << bit->getUid()
					<< " not found in column with weight=" << weight);

		REPORT(DEBUG,"removed bit " << bit->name << " from column " << weight);

		isCompressed = false;
	}


	void BitheapNew::removeBits(int weight, unsigned count, int direction)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		vector<Bit*> bitsColumn = bits[weight];
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
			THROWERROR("LSB (=" << lsb << ") out of bitheap bit range in removeBit");
		if(msb > this->lsb)
			THROWERROR("MSB (=" << msb << ") out of bitheap bit range in removeBit");

		for(int i=lsb; i<=msb; i++)
		{
			unsigned currentWeight = lsb - this->lsb;

			removeBits(currentWeight, count, direction);
		}

		isCompressed = false;
	}


	void BitheapNew::removeCompressedBits()
	{
		for(unsigned i=0; i<size; i++)
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
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		if(number >= bits[weight].size())
			THROWERROR("Column with weight=" << weight << " only contains "
					<< bits[weight].size() << " bits, but bit number=" << number << " is to be marked");

		bits[weight][number]->type = type;
	}


	void BitheapNew::markBit(Bit* bit, BitType type)
	{
		bool bitFound = false;

		for(unsigned i=0; i<=size; i++)
		{
			vector<Bit*>::iterator it = bits[i].begin(), lastIt = it;

			//search for the bit
			while(it != bits[i].end())
			{
				if(((*it)->name == bit->name) && ((*it)->getUid() == bit->getUid()))
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
			THROWERROR("Bit=" << bit->name << " with uid="
					<< bit->getUid() << " not found in bitheap");
	}


	void BitheapNew::markBits(int msb, int lsb, BitType type, unsigned number)
	{
		if(lsb < this->lsb)
			THROWERROR("LSB (=" << lsb << ") out of bitheap bit range in removeBit");
		if(msb > this->lsb)
			THROWERROR("MSB (=" << msb << ") out of bitheap bit range in removeBit");

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


	void BitheapNew::addConstantOneBit(int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits += (mpz_class(1) << weight);

		isCompressed = false;
	}


	void BitheapNew::subConstantOneBit(int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits -= (mpz_class(1) << weight);

		isCompressed = false;
	}


	void BitheapNew::addConstant(mpz_class constant, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits += (constant << weight);

		isCompressed = false;
	}


	void BitheapNew::addConstant(mpz_class constant, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		mpz_class constantCpy = mpz_class(constant);

		if(lsb < this->lsb)
			constantCpy = constantCpy >> (this->lsb - lsb);
		else
			constantCpy = constantCpy << (lsb - this->lsb);

		constantBits += (constantCpy << weight);

		isCompressed = false;
	}


	void BitheapNew::subConstant(mpz_class constant, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits -= (constant << weight);

		isCompressed = false;
	}


	void BitheapNew::subConstant(mpz_class constant, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		mpz_class constantCpy = mpz_class(constant);

		if(lsb < this->lsb)
			constantCpy = constantCpy >> (this->lsb - lsb);
		else
			constantCpy = constantCpy << (lsb - this->lsb);

		constantBits -= (constantCpy << weight);

		isCompressed = false;
	}


	void BitheapNew::addUnsignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");

		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if((int)size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		if(op->getSignalByName(signalName)->width() == 1)
		{
			addBit(startIndex, signalName);
		}else{
			for(int i=startIndex; i<=endIndex; i++)
				addBit(i, join(signalName, of(i-startIndex+startIndex-weight)));
		}

		isCompressed = false;
	}


	void BitheapNew::addUnsignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");

		int startIndex, endIndex;

		if(weight+lsb < this->lsb)
			startIndex = this->lsb;
		else
			startIndex = weight+lsb;
		if(msb+weight > this->msb)
			endIndex = this->msb;
		else
			endIndex = msb+weight;

		if(op->getSignalByName(signalName)->width() == 1)
		{
			addBit(startIndex, signalName);
		}else{
			for(int i=startIndex; i<=endIndex; i++)
				addBit(i, join(signalName, of(i-startIndex+startIndex-weight-lsb)));
		}

		isCompressed = false;
	}


	void BitheapNew::addUnsignedBitVector(Signal* signal, int weight)
	{
		if(signal->isSigned())
			REPORT(DEBUG, "WARNING: adding signed signal "
					<< signal->getName() << " as an unsigned bit vector");
		if(signal->isSigned() && !isSigned)
			REPORT(DEBUG, "WARNING: adding signed signal "
					<< signal->getName() << " to an unsigned bitheap");

		if(signal->isFix())
			addUnsignedBitVector(signal->getName(), signal->MSB(), signal->LSB(), weight);
		else
			addUnsignedBitVector(signal->getName(), (unsigned)signal->width(), weight);

		isCompressed = false;
	}


	void BitheapNew::addUnsignedBitVector(Signal* signal, int msb, int lsb, int weight)
	{
		if(signal->isSigned())
			REPORT(DEBUG, "WARNING: adding signed signal "
					<< signal->getName() << " as an unsigned bit vector");
		if(signal->isSigned() && !isSigned)
			REPORT(DEBUG, "WARNING: adding signed signal "
					<< signal->getName() << " to an unsigned bitheap");

		if(signal->isFix()){
			if((signal->LSB() > msb) || (signal->MSB() < lsb))
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (msb=" << signal->MSB() << ", lsb=" << signal->LSB()
						<< ") to bitheap with msb=" << this->msb << ", lsb=" << this->lsb);

			addUnsignedBitVector(signal->getName(), msb, lsb, weight);
		}else{
			if(msb-lsb+1 > signal->width())
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (size=" << signal->width() << ") to bitheap with msb="
						<< this->msb << ", lsb=" << this->lsb);

			addUnsignedBitVector(signal->getName(), (unsigned)(msb-lsb+1), weight);
		}

		isCompressed = false;
	}


	void BitheapNew::subtractUnsignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");

		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if((int)size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		for(int i=startIndex; i<=endIndex; i++)
		{
			ostringstream s;

			if(op->getSignalByName(signalName)->width() == 1)
			{
				s << "not(" << signalName << ")";
			}else{
				s << "not(" << signalName << of(i-startIndex+startIndex-weight) << ")";
			}
			addBit(i, s.str());
		}
		addConstantOneBit(startIndex);
		for(int i=endIndex+1; i<=this->msb; i++)
			addConstantOneBit(i);

		isCompressed = false;
	}


	void BitheapNew::subtractUnsignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");

		int startIndex, endIndex;

		if(weight+lsb < this->lsb)
			startIndex = this->lsb;
		else
			startIndex = weight+lsb;
		if(msb+weight > this->msb)
			endIndex = this->msb;
		else
			endIndex = msb+weight;

		for(int i=startIndex; i<=endIndex; i++)
		{
			ostringstream s;

			if(op->getSignalByName(signalName)->width() == 1)
			{
				s << "not(" << signalName << ")";
			}else{
				s << "not(" << signalName << of(i-startIndex+startIndex-weight) << ")";
			}
			addBit(i, s.str());
		}
		addConstantOneBit(startIndex);
		for(int i=endIndex+1; i<=this->msb; i++)
			addConstantOneBit(i);

		isCompressed = false;
	}


	void BitheapNew::subtractUnsignedBitVector(Signal* signal, int weight)
	{
		if(signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting signed signal "
					<< signal->getName() << " as an unsigned bit vector");
		if(signal->isSigned() && !isSigned)
			REPORT(DEBUG, "WARNING: subtracting signed signal "
					<< signal->getName() << " from an unsigned bitheap");

		if(signal->isFix())
			subtractUnsignedBitVector(signal->getName(), signal->MSB(), signal->LSB(), weight);
		else
			subtractUnsignedBitVector(signal->getName(), (unsigned)signal->width(), weight);

		isCompressed = false;
	}


	void BitheapNew::subtractUnsignedBitVector(Signal* signal, int msb, int lsb, int weight)
	{
		if(signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting signed signal "
					<< signal->getName() << " as an unsigned bit vector");
		if(signal->isSigned() && !isSigned)
			REPORT(DEBUG, "WARNING: subtracting signed signal "
					<< signal->getName() << " from an unsigned bitheap");

		if(signal->isFix()){
			if((signal->LSB() > msb) || (signal->MSB() < lsb))
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (msb=" << signal->MSB() << ", lsb=" << signal->LSB()
						<< ") to bitheap with msb=" << this->msb << ", lsb=" << this->lsb);

			subtractUnsignedBitVector(signal->getName(), msb, lsb, weight);
		}else{
			if(msb-lsb+1 > signal->width())
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (size=" << signal->width() << ") to bitheap with msb="
						<< this->msb << ", lsb=" << this->lsb);

			subtractUnsignedBitVector(signal->getName(), (unsigned)(msb-lsb+1), weight);
		}

		isCompressed = false;
	}


	void BitheapNew::addSignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: adding a signed signal " << signalName << " to an unsigned bitheap");

		ostringstream s;
		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if((int)size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		if(op->getSignalByName(signalName)->width() == 1)
		{
			addBit(startIndex, signalName);
		}else{
			for(int i=startIndex; i<endIndex; i++)
				addBit(i, join(signalName, of(i-startIndex+startIndex-weight)));
		}

		s << "not(" << signalName << of(endIndex-startIndex+startIndex-weight) << ")";
		addBit(endIndex, s.str());

		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);

		isCompressed = false;
	}


	void BitheapNew::addSignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: adding a signed signal " << signalName << " to an unsigned bitheap");

		ostringstream s;
		int startIndex, endIndex;

		if(weight+lsb < this->lsb)
			startIndex = this->lsb;
		else
			startIndex = weight+lsb;
		if(msb+weight > this->msb)
			endIndex = this->msb;
		else
			endIndex = msb+weight;

		if(op->getSignalByName(signalName)->width() == 1)
		{
			addBit(startIndex, signalName);
		}else{
			for(int i=startIndex; i<endIndex; i++)
				addBit(i, join(signalName, of(i-startIndex+startIndex-weight-lsb)));
		}

		s << "not(" << signalName << of(endIndex-startIndex+startIndex-weight-lsb) << ")";
		addBit(endIndex, s.str());

		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);

		isCompressed = false;
	}


	void BitheapNew::addSignedBitVector(Signal* signal, int weight)
	{
		if(!signal->isSigned())
			REPORT(DEBUG, "WARNING: adding unsigned signal "
					<< signal->getName() << " as an signed bit vector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: adding signed signal "
					<< signal->getName() << " to an unsigned bitheap");

		if(signal->isFix())
			addSignedBitVector(signal->getName(), signal->MSB(), signal->LSB(), weight);
		else
			addSignedBitVector(signal->getName(), (unsigned)signal->width(), weight);

		isCompressed = false;
	}


	void BitheapNew::addSignedBitVector(Signal* signal, int msb, int lsb, int weight)
	{
		if(!signal->isSigned())
			REPORT(DEBUG, "WARNING: adding unsigned signal "
					<< signal->getName() << " as an signed bit vector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: adding signed signal "
					<< signal->getName() << " to an unsigned bitheap");

		if(signal->isFix()){
			if((signal->LSB() > msb) || (signal->MSB() < lsb))
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (msb=" << signal->MSB() << ", lsb=" << signal->LSB()
						<< ") to bitheap with msb=" << this->msb << ", lsb=" << this->lsb);

			addSignedBitVector(signal->getName(), msb, lsb, weight);
		}else{
			if(msb-lsb+1 > signal->width())
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (size=" << signal->width() << ") to bitheap with msb="
						<< this->msb << ", lsb=" << this->lsb);

			addSignedBitVector(signal->getName(), (unsigned)(msb-lsb+1), weight);
		}

		isCompressed = false;
	}


	void BitheapNew::subtractSignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: subtracting a signed signal "
					<< signalName << " from an unsigned bitheap");

		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if((int)size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		for(int i=startIndex; i<endIndex; i++)
		{
			ostringstream s;

			if(op->getSignalByName(signalName)->width() == 1)
			{
				s << "not(" << signalName << ")";
			}else{
				s << "not(" << signalName << of(i-startIndex+startIndex-weight) << ")";
			}
			addBit(i, s.str());
		}
		addBit(endIndex, join(signalName, of(endIndex-startIndex+startIndex-weight)));
		addConstantOneBit(startIndex);
		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);

		isCompressed = false;
	}


	void BitheapNew::subtractSignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isFixedPoint && ((weight < 0) || ((unsigned)weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: subtracting a signed signal "
					<< signalName << " from an unsigned bitheap");

		ostringstream s;
		int startIndex, endIndex;

		if(weight+lsb < this->lsb)
			startIndex = this->lsb;
		else
			startIndex = weight+lsb;
		if(msb+weight > this->msb)
			endIndex = this->msb;
		else
			endIndex = msb+weight;

		for(int i=startIndex; i<endIndex; i++)
		{
			ostringstream s;

			if(op->getSignalByName(signalName)->width() == 1)
			{
				s << "not(" << signalName << ")";
			}else{
				s << "not(" << signalName << of(i-startIndex+startIndex-weight-lsb) << ")";
			}
			addBit(i, s.str());
		}
		addBit(endIndex, join(signalName, of(endIndex-startIndex+startIndex-weight-lsb)));
		addConstantOneBit(startIndex);
		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);

		isCompressed = false;
	}


	void BitheapNew::subtractSignedBitVector(Signal* signal, int weight)
	{
		if(!signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting unsigned signal "
					<< signal->getName() << " as an signed bit vector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: subtracting signed signal "
					<< signal->getName() << " from an unsigned bitheap");

		if(signal->isFix())
			subtractSignedBitVector(signal->getName(), signal->MSB(), signal->LSB(), weight);
		else
			subtractSignedBitVector(signal->getName(), (unsigned)signal->width(), weight);

		isCompressed = false;
	}


	void BitheapNew::subtractSignedBitVector(Signal* signal, int msb, int lsb, int weight)
	{
		if(!signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting unsigned signal "
					<< signal->getName() << " as an signed bit vector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: subtracting signed signal "
					<< signal->getName() << " from an unsigned bitheap");

		if(signal->isFix()){
			if((signal->LSB() > msb) || (signal->MSB() < lsb))
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (msb=" << signal->MSB() << ", lsb=" << signal->LSB()
						<< ") to bitheap with msb=" << this->msb << ", lsb=" << this->lsb);

			subtractSignedBitVector(signal->getName(), msb, lsb, weight);
		}else{
			if(msb-lsb+1 > signal->width())
				THROWERROR("Incorrect bounds while adding unsigned signal " << signal->getName()
						<< " (size=" << signal->width() << ") to bitheap with msb="
						<< this->msb << ", lsb=" << this->lsb);

			subtractSignedBitVector(signal->getName(), (unsigned)(msb-lsb+1), weight);
		}

		isCompressed = false;
	}


	void BitheapNew::addSignal(Signal* signal, int weight)
	{
		if(!isSigned && signal->isSigned())
			REPORT(DEBUG, "WARNING: adding a signed signal "
					<< signal->getName() << " to an unsigned bitheap");
		if(isSigned && !signal->isSigned())
			REPORT(DEBUG, "WARNING: adding an unsigned signal "
					<< signal->getName() << " to a signed bitheap");

		if(signal->isSigned())
			addSignedBitVector(signal, weight);
		else
			addUnsignedBitVector(signal, weight);

		isCompressed = false;
	}


	void BitheapNew::addSignal(Signal* signal, int msb, int lsb, int weight)
	{
		if(!isSigned && signal->isSigned())
			REPORT(DEBUG, "WARNING: adding a signed signal "
					<< signal->getName() << " to an unsigned bitheap");
		if(isSigned && !signal->isSigned())
			REPORT(DEBUG, "WARNING: adding an unsigned signal "
					<< signal->getName() << " to a signed bitheap");

		if(signal->isSigned())
			addSignedBitVector(signal, msb, lsb, weight);
		else
			addUnsignedBitVector(signal, msb, lsb, weight);

		isCompressed = false;
	}


	void BitheapNew::subtractSignal(Signal* signal, int weight)
	{
		if(!isSigned && signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting a signed signal "
					<< signal->getName() << " from an unsigned bitheap");
		if(isSigned && !signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting an unsigned signal "
					<< signal->getName() << " from a signed bitheap");

		if(signal->isSigned())
			subtractSignedBitVector(signal, weight);
		else
			subtractUnsignedBitVector(signal, weight);

		isCompressed = false;
	}


	void BitheapNew::subtractSignal(Signal* signal, int msb, int lsb, int weight)
	{
		if(!isSigned && signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting a signed signal "
					<< signal->getName() << " from an unsigned bitheap");
		if(isSigned && !signal->isSigned())
			REPORT(DEBUG, "WARNING: subtracting an unsigned signal "
					<< signal->getName() << " from a signed bitheap");

		if(signal->isSigned())
			subtractSignedBitVector(signal, msb, lsb, weight);
		else
			subtractUnsignedBitVector(signal, msb, lsb, weight);

		isCompressed = false;
	}


	void BitheapNew::resizeBitheap(unsigned newSize, int direction)
	{
		if((direction != 0) && (direction != 1))
			THROWERROR("Invalid direction in resizeBitheap: direction=" << direction);
		if(isFixedPoint)
			REPORT(DEBUG, "WARNING: using generic resize method to resize a fixed-point bitheap");
		if(newSize < size)
		{
			REPORT(DEBUG, "WARNING: resizing the bitheap from size="
					<< size << " to size=" << newSize);
			if(direction == 0){
				REPORT(DEBUG, "\t will loose the information in the lsb columns");
			}else{
				REPORT(DEBUG, "\t will loose the information in the msb columns");
			}
		}

		//add/remove the columns
		if(newSize < size)
		{
			//remove columns
			if(direction == 0)
			{
				//remove lsb columns
				bits.erase(bits.begin(), bits.begin()+(size-newSize));
				history.erase(history.begin(), history.begin()+(size-newSize));
				bitUID.erase(bitUID.begin(), bitUID.begin()+(size-newSize));
				if(isFixedPoint)
					lsb += size-newSize;
			}else{
				//remove msb columns
				bits.resize(newSize);
				history.resize(newSize);
				bitUID.resize(newSize);
				if(isFixedPoint)
					msb -= size-newSize;
			}
		}else{
			//add columns
			if(direction == 0)
			{
				//add lsb columns
				vector<Bit*> newVal;
				bits.insert(bits.begin(), newSize-size, newVal);
				history.insert(history.begin(), newSize-size, newVal);
				bitUID.insert(bitUID.begin(), newSize-size, 0);
				if(isFixedPoint)
					lsb -= size-newSize;
			}else{
				//add msb columns
				bits.resize(newSize-size);
				history.resize(newSize-size);
				bitUID.resize(newSize-size);
				if(isFixedPoint)
					msb += size-newSize;
			}
		}

		//update the information inside the bitheap
		size = newSize;
		if(!isFixedPoint)
		{
			lsb = 0;
			msb = newSize-1;
		}
		height = getMaxHeight();
		for(int i=lsb; i<=msb; i++)
			for(unsigned j=0; j<bits[i-lsb].size(); j++)
				bits[i-lsb][j]->weight = i;

		isCompressed = false;
	}


	void BitheapNew::resizeBitheap(int newMsb, int newLsb)
	{
		if((newMsb < newLsb) || (newLsb<0 && !isFixedPoint) || (newMsb<0 && !isFixedPoint))
			THROWERROR("Invalid arguments in resizeBitheap (isFixedPoint="
					<< isFixedPoint << "): newMsb=" << newMsb << " newLsb=" << newLsb);
		if(!isFixedPoint)
			REPORT(DEBUG, "WARNING: using fixed-point resize method to resize an integer bitheap");
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
		if(size < bitheap->size)
		{
			if(lsb > bitheap->lsb)
				resizeBitheap(msb, bitheap->lsb);
			if(msb < bitheap->msb)
				resizeBitheap(bitheap->msb, lsb);
		}
		//add the bits
		for(int i=bitheap->lsb; i<bitheap->msb; i++)
			for(unsigned j=0; j<bitheap->bits[i-bitheap->lsb].size(); j++)
				insertBitInColumn(bitheap->bits[i-bitheap->lsb][j], i-bitheap->lsb+(lsb-bitheap->lsb));
		//make the bit all point to this bitheap
		for(unsigned i=0; i<size; i++)
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
			REPORT(FULL, "\t column weight=" << weight << " name=" << bits[weight-lsb][i]->name
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

















