
#include "Bit.hpp"

using namespace std;

namespace flopoco
{

	Bit::Bit(BitheapNew *bitheap_, string rhsAssignment_, int weight_, BitType type_) :
		weight(weight_), type(type_), bitheap(bitheap_), rhsAssignment(rhsAssignment_)
	{
		std::ostringstream p;

		uid = bitheap->newBitUid(weight);
		p  << "heap_bh" << bitheap->getGUid() << "_w" << weight << "_" << uid;
		name = p.str();

		compressor = nullptr;

		bitheap->getOp()->declare(name);
		signal = bitheap->getOp()->getSignalByName(name);
		bitheap->getOp()->vhdl << tab << name << " <= " << rhsAssignment << ";" << endl;
	}


	Bit::Bit(BitheapNew *bitheap_, Signal *signal_, int offset_, int weight_, BitType type_) :
		weight(weight_), type(type_), bitheap(bitheap_)
	{
		std::ostringstream p;

		uid = bitheap->newBitUid(weight);
		p  << "heap_bh" << bitheap->getGUid() << "_w" << weight << "_" << uid;
		name = p.str();

		p.str("");
		p << signal->getName() << (signal->width()==1 ? "" : of(offset_));
		rhsAssignment = p.str();

		compressor = nullptr;

		bitheap->getOp()->declare(name);
		signal = bitheap->getOp()->getSignalByName(name);
		bitheap->getOp()->vhdl << tab << name << " <= " << rhsAssignment << ";" << endl;
	}


	Bit::Bit(Bit* bit) {
		std::ostringstream p;

		weight = bit-> weight;
		type = bit->type;
		rhsAssignment = bit->rhsAssignment;

		bitheap = bit->bitheap;
		compressor = bit->compressor;

		uid = bitheap->newBitUid(weight);

		p  << "heap_bh" << bitheap->getGUid() << "_w" << weight << "_" << uid;
		name = p.str();

		bitheap->getOp()->declare(name);
		signal = bitheap->getOp()->getSignalByName(name);
		bitheap->getOp()->vhdl << tab << name << " <= " << rhsAssignment << ";" << endl;
	}


	int Bit::getUid()
	{
		return uid;
	}

	void Bit::setCompressor(Compressor *compressor_)
	{
		compressor = compressor_;
	}

	Compressor* Bit::getCompressor()
	{
		return compressor;
	}



	bool operator< (Bit& b1, Bit& b2){
		return ((b1.signal->getCycle() < b2.signal->getCycle())
				|| (b1.signal->getCycle()==b2.signal->getCycle() && b1.signal->getCriticalPath()<b2.signal->getCriticalPath()));
	}

	bool operator<= (Bit& b1, Bit& b2){
		return ((b1.signal->getCycle() < b2.signal->getCycle())
				|| (b1.signal->getCycle()==b2.signal->getCycle() && b1.signal->getCriticalPath()<=b2.signal->getCriticalPath()));
	}

	bool operator> (Bit& b1, Bit& b2){
		return ((b1.signal->getCycle() > b2.signal->getCycle())
				|| (b1.signal->getCycle()==b2.signal->getCycle() && b1.signal->getCriticalPath()>b2.signal->getCriticalPath()));
	}

	bool operator>= (Bit& b1, Bit& b2){
		return ((b1.signal->getCycle() > b2.signal->getCycle())
				|| (b1.signal->getCycle()==b2.signal->getCycle() && b1.signal->getCriticalPath()>=b2.signal->getCriticalPath()));
	}

	bool operator== (Bit& b1, Bit& b2){
		return ((b1.signal->getCycle() == b2.signal->getCycle())
						&& (b1.signal->getCriticalPath() == b2.signal->getCriticalPath()));
	}

	bool operator!= (Bit& b1, Bit& b2){
		return  ((b1.signal->getCycle() != b2.signal->getCycle()) || (b1.signal->getCriticalPath() != b2.signal->getCriticalPath()));
	}
}
