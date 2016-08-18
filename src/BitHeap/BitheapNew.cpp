
#include "BitheapNew.hpp"

namespace flopoco {

	BitheapNew::BitheapNew(Operator* op_, unsigned size_, bool isSigned_, string name_, int compressionType_) :
		op(op_),
		msb(size_-1), lsb(0),
		size(size_),
		height(0),
		name(name_),
		isSigned(isSigned_),
		isFixedPoint(false),
		compressionType(compressionType_)
	{
		initialize();
	}


	BitheapNew::BitheapNew(Operator* op_, int msb_, int lsb_, bool isSigned_, string name_, int compressionType_) :
		op(op_),
		msb(msb_), lsb(lsb_),
		size(msb_-lsb_+1),
		height(0),
		name(name_),
		isSigned(isSigned_),
		isFixedPoint(true),
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
		for(int i=0; i<size; i++)
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
		compressionStrategy = new CompressionStrategy(this);
		isCompressed = false;

		//create a Plotter object for the SVG output
		plotter = new Plotter(this);
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
		Bit* bit = new Bit(this, rhsAssignment, weight, Bit::BitType::free);

		//insert the new bit so that the vector is sorted by bit (cycle, delay)
		insertBitInColumn(bit, bits[weight]);

		REPORT(DEBUG, "added bit named "  << bit->name << " on column " << weight
				<< " at cycle=" << bit->signal->getCycle() << " cp=" << bit->signal->getCriticalPath());

		printColumnInfo(weight);
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
		Bit* bit = new Bit(this, signal, offset, weight, Bit::BitType::free);

		//insert the new bit so that the vector is sorted by bit (cycle, delay)
		insertBitInColumn(bit, bits[weight]);

		REPORT(DEBUG, "added bit named "  << bit->name << " on column " << weight
			<< " at cycle=" << bit->signal->getCycle() << " cp=" << bit->signal->getCriticalPath());
		printColumnInfo(weight);
	}


	void BitheapNew::addBit(Signal *signal, int offset, int weight)
	{
		addBit(weight, signal, offset);
	}


	void BitheapNew::insertBitInColumn(Bit* bit, vector<Bit*> bitsColumn)
	{
		vector<Bit*>::iterator it = bitsColumn.begin();
		bool inserted = false;

		while(it != bitsColumn.end())
		{
			if((bit->signal->getCycle() < (*it)->signal->getCycle())
					|| ((bit->signal->getCycle() == (*it)->signal->getCycle())
							&& (bit->signal->getCriticalPath() < (*it)->signal->getCriticalPath())))
			{
				bitsColumn.insert(it, bit);
				inserted = true;
			}else{
				it++;
			}
		}
		if(inserted == false)
		{
			bitsColumn.insert(bitsColumn.end(), bit);
		}
	}


	void BitheapNew::removeBit(int weight, int direction)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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
	}


	void BitheapNew::removeBit(int weight, Bit* bit)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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
	}


	void BitheapNew::removeBits(int weight, unsigned count, int direction)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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
	}


	void BitheapNew::removeCompressedBits()
	{
		for(unsigned i=0; i<=size; i++)
		{
			vector<Bit*> bitsColumn = bits[i];
			vector<Bit*>::iterator it = bitsColumn.begin(), lastIt = it;

			//search for the bits marked as compressed and erase them
			while(it != bitsColumn.end())
			{
				if((*it)->type == Bit::BitType::compressed)
				{
					bitsColumn.erase(it);
					it = lastIt;
				}else{
					lastIt = it;
					it++;
				}
			}
		}
	}


	void BitheapNew::markBit(int weight, unsigned number, Bit::BitType type)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		vector<Bit*> bitsColumn = bits[weight];

		if(number >= bitsColumn.size())
			THROWERROR("Column with weight=" << weight << " only contains "
					<< bitsColumn.size() << " bits, but bit number=" << number << " is to be marked");

		bits[weight][number]->type = type;
	}


	void BitheapNew::markBit(Bit* bit, Bit::BitType type)
	{
		bool bitFound = false;

		for(unsigned i=0; i<=size; i++)
		{
			vector<Bit*> bitsColumn = bits[i];
			vector<Bit*>::iterator it = bitsColumn.begin(), lastIt = it;

			//search for the bit
			while(it != bitsColumn.end())
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


	void BitheapNew::markBits(int msb, int lsb, Bit::BitType type, unsigned number)
	{
		if(lsb < this->lsb)
			THROWERROR("LSB (=" << lsb << ") out of bitheap bit range in removeBit");
		if(msb > this->lsb)
			THROWERROR("MSB (=" << msb << ") out of bitheap bit range in removeBit");

		for(int i=lsb; i<=msb; i++)
		{
			vector<Bit*> bitsColumn = bits[i];
			unsigned currentWeight = i - this->lsb;
			unsigned currentNumber = number;

			if(number >= bitsColumn.size())
			{
				REPORT(DEBUG, "Column with weight=" << currentWeight << " only contains "
						<< bitsColumn.size() << " bits, but number=" << number << " bits are to be marked");
				currentNumber = bitsColumn.size();
			}

			for(unsigned j=0; j<currentNumber; j++)
				markBit(currentWeight, j, type);
		}
	}


	void BitheapNew::markBits(vector<Bit*> bits, Bit::BitType type)
	{
		for(unsigned i=0; i<bits.size(); i++)
			markBit(bits[i], type);
	}


	void BitheapNew::addConstantOneBit(int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits += (mpz_class(1) << weight);
	}


	void BitheapNew::subConstantOneBit(int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits -= (mpz_class(1) << weight);
	}


	void BitheapNew::addConstant(mpz_class constant, int weight = 0)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits += (constant << weight);
	}


	void BitheapNew::addConstant(mpz_class constant, int msb, int lsb, int weight = 0)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		mpz_class constantCpy = mpz_class(constant);

		if(lsb < this->lsb)
			constantCpy = constantCpy >> (this->lsb - lsb);
		else
			constantCpy = constantCpy << (lsb - this->lsb);

		constantBits += (constantCpy << weight);
	}


	void BitheapNew::subConstant(mpz_class constant, int weight = 0)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		constantBits -= (constant << weight);
	}


	void BitheapNew::subConstant(mpz_class constant, int msb, int lsb, int weight = 0)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in removeBit");

		mpz_class constantCpy = mpz_class(constant);

		if(lsb < this->lsb)
			constantCpy = constantCpy >> (this->lsb - lsb);
		else
			constantCpy = constantCpy << (lsb - this->lsb);

		constantBits -= (constantCpy << weight);
	}


	void BitheapNew::addUnsignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");

		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if(size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		for(int i=startIndex; i<=endIndex; i++)
			addBit(i, join(signalName, of(i-startIndex+startIndex-weight)));
	}


	void BitheapNew::addUnsignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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

		for(int i=startIndex; i<=endIndex; i++)
			addBit(i, join(signalName, of(i-startIndex+startIndex-weight-lsb)));
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
			addUnsignedBitVector(signal->getName(), signal->width(), weight);
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

			addUnsignedBitVector(signal->getName(), msb-lsb+1, weight);
		}
	}


	void BitheapNew::subtractUnsignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");

		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if(size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		for(int i=startIndex; i<=endIndex; i++)
		{
			ostringstream s;

			s << "not(" << signalName << of(i-startIndex+startIndex-weight) << ")";
			addBit(i, s.str());
		}
		addConstantOneBit(startIndex);
		for(int i=endIndex+1; i<=this->msb; i++)
			addConstantOneBit(i);
	}


	void BitheapNew::subtractUnsignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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

			s << "not(" << signalName << of(i-startIndex+startIndex-weight) << ")";
			addBit(i, s.str());
		}
		addConstantOneBit(startIndex);
		for(int i=endIndex+1; i<=this->msb; i++)
			addConstantOneBit(i);
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
			subtractUnsignedBitVector(signal->getName(), signal->width(), weight);
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

			subtractUnsignedBitVector(signal->getName(), msb-lsb+1, weight);
		}
	}


	void BitheapNew::addSignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: adding a signed signal " << signalName << " to an unsigned bitheap");

		ostringstream s;
		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if(size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		for(int i=startIndex; i<endIndex; i++)
			addBit(i, join(signalName, of(i-startIndex+startIndex-weight)));

		s << "not(" << signalName << of(endIndex-startIndex+startIndex-weight) << ")";
		addBit(endIndex, s.str());

		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);
	}


	void BitheapNew::addSignedBitVector(string signalName, int msb, int lsb, int weight = 0)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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

		for(int i=startIndex; i<endIndex; i++)
			addBit(i, join(signalName, of(i-startIndex+startIndex-weight-lsb)));

		s << "not(" << signalName << of(endIndex-startIndex+startIndex-weight-lsb) << ")";
		addBit(endIndex, s.str());

		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);
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
			addSignedBitVector(signal->getName(), signal->width(), weight);
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

			addSignedBitVector(signal->getName(), msb-lsb+1, weight);
		}
	}


	void BitheapNew::subtractSignedBitVector(string signalName, unsigned size, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in subtractUnsignedBitVector");
		if(!isSigned)
			REPORT(DEBUG, "WARNING: subtracting a signed signal "
					<< signalName << " from an unsigned bitheap");

		int startIndex, endIndex;

		if(weight < lsb)
			startIndex = lsb;
		else
			startIndex = weight;
		if(size+weight > msb)
			endIndex = msb;
		else
			endIndex = size+weight-1;

		for(int i=startIndex; i<endIndex; i++)
		{
			ostringstream s;

			s << "not(" << signalName << of(i-startIndex+startIndex-weight) << ")";
			addBit(i, s.str());
		}
		addBit(endIndex, join(signalName, of(endIndex-startIndex+startIndex-weight)));
		addConstantOneBit(startIndex);
		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);
	}


	void BitheapNew::subtractSignedBitVector(string signalName, int msb, int lsb, int weight)
	{
		if(isFixedPoint && ((weight < lsb) || (weight > msb)))
			THROWERROR("Weight (=" << weight << ") out of bitheap bit range in addSignedBitVector");
		if(!isFixedPoint && ((weight < 0) || (weight >= size)))
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

			s << "not(" << signalName << of(i-startIndex+startIndex-weight-lsb) << ")";
			addBit(i, s.str());
		}
		addBit(endIndex, join(signalName, of(endIndex-startIndex+startIndex-weight-lsb)));
		addConstantOneBit(startIndex);
		for(int i=endIndex; i<=this->msb; i++)
			addConstantOneBit(i);
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
			subtractSignedBitVector(signal->getName(), signal->width(), weight);
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

			subtractSignedBitVector(signal->getName(), msb-lsb+1, weight);
		}
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
	}

} /* namespace flopoco */

















