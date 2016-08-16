
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


	void BitheapNew::addBit(int weight, string rhsAssignment, string comment)
	{
		if((!isFixedPoint) && (weight < 0))
			THROWERROR("Negative weight (" << weight << ") in addBit, for an integer bitheap");

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


	void BitheapNew::addBit(int weight, Signal *signal, int offset, string comment)
	{
		if((!isFixedPoint) && (weight < 0))
			THROWERROR("Negative weight (" << weight << ") in addBit, for an integer bitheap");
		if((signal->isFix()) && ((offset < signal->LSB()) || (offset > signal->MSB())))
			THROWERROR("Offset (" << offset << ") out of signal bit range");
		if((!signal->isFix()) && (offset >= signal->width()))
			THROWERROR("Negative weight (" << weight << ") in addBit, for an integer bitheap");

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

} /* namespace flopoco */
