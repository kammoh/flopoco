
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

} /* namespace flopoco */
