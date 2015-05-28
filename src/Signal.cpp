#include <iostream>
#include <sstream>
#include "Signal.hpp"

using namespace std;

namespace flopoco{


	// plain logic vector, or wire
	Signal::Signal(Operator* parentOp, const string name, const Signal::SignalType type, const int width, const bool isBus) :
		parentOp_(parentOp), name_(name), type_(type), width_(width), numberOfPossibleValues_(1),
		lifeSpan_(0),  cycle_(0), criticalPath_(0.0), criticalPathContribution_(0.0),
		isFP_(false), isFix_(false), isIEEE_(false),
		wE_(0), wF_(0), MSB_(0), LSB_(0),
		isSigned_(false), isBus_(isBus) {
	}

	// fixed point constructor
	Signal::Signal(Operator* parentOp, const string name, const Signal::SignalType type, const bool isSigned, const int MSB, const int LSB) :
		parentOp_(parentOp), name_(name), type_(type), width_(MSB-LSB+1), numberOfPossibleValues_(1),
		lifeSpan_(0), cycle_(0), criticalPath_(0.0), criticalPathContribution_(0.0),
		isFP_(false), isFix_(true),  isIEEE_(false),
		wE_(0), wF_(0), MSB_(MSB), LSB_(LSB),
		isSigned_(isSigned), isBus_(true)
	{
	}

	Signal::Signal(Operator* parentOp, const string name, const Signal::SignalType type, const int wE, const int wF, const bool ieeeFormat) :
		parentOp_(parentOp), name_(name), type_(type), width_(wE+wF+3), numberOfPossibleValues_(1),
		lifeSpan_(0), cycle_(0), criticalPath_(0.0), criticalPathContribution_(0.0),
		isFP_(true), isFix_(false), isIEEE_(false),
		wE_(wE), wF_(wF), MSB_(0), LSB_(0),
		isSigned_(false), isBus_(false)
	{
		if(ieeeFormat) { // correct some of the initializations above
			width_  = wE+wF+1;
			isFP_   = false;
			isIEEE_ = true;
		}
	}

	Signal::Signal(Signal* originalSignal)
	{
		if(originalSignal->isFix())
			this->Signal(originalSignal->parentOp(), originalSignal->getName(), originalSignal->type(), originalSignal->isSigned(), originalSignal->MSB(), originalSignal->LSB());
		else if(originalSignal->isFP())
			this->Signal(originalSignal->parentOp(), originalSignal->getName(), originalSignal->type(), originalSignal->wE(), originalSignal->wF(), originalSignal->isIEEE());
		else
			this->Signal(originalSignal->parentOp(), originalSignal->getName(), originalSignal->type(), originalSignal->width(), originalSignal->isBus());
	}

	Signal::~Signal(){}

	void	Signal::promoteToFix(const bool isSigned, const int MSB, const int LSB){
		if(MSB - LSB +1 != width_){
			std::ostringstream o;
			o << "Error in Signal::promoteToFix(" <<  getName() << "): width doesn't match";
			throw o.str();
		}	
		isFix_ = true;
		MSB_   = MSB;
		LSB_   = LSB;
		isSigned_ = isSigned;
	}


	const string& Signal::getName() const { 
		return name_; 
	}

	Operator* Signal::parentOp() const {return parentOp_;}

	void Signal::setParentOp(Operator* newParentOp)
	{
		//erase the signal from the operator's signal list and map
		for(unsigned int i=0; i<parentOp_->getSignalList(); i++)
			if(parentOp_->getSignalList()[i]->getName() == name_)
				parentOp_->getSignalList().erase(parentOp_->getSignalList().begin()+i);
		parentOp_->getSignalMap().erase(name_);

		//change the signal's parent operator
		parentOp_ = newParentOp;

		//add the signal to the new parent's signal list
		parentOp_->signalList_.push_back(this);
		parentOp_->getSignalMap()[this->getName()] = this;
	}

	int Signal::width() const {return width_;}
	
	int Signal::wE() const {return(wE_);}

	int Signal::wF() const {return(wF_);}
	
	int Signal::MSB() const {return(MSB_);}
	
	int Signal::LSB() const {return(LSB_);}
	
	bool Signal::isSigned() const {return isSigned_;};

	bool Signal::isFP() const {return isFP_;}

	void Signal::setIsFP(bool newIsFP) {isFP_ = newIsFP;}

	bool Signal::isFix() const {return isFix_;}

	void Signal::setIsFix(bool newIsFix) {isFix_ = newIsFix;}

	bool Signal::isFixSigned() const {return isFix_ && isSigned_;}

	void Signal::setFixSigned(bool newIsFix) {isFix_ = newIsFix; isSigned_ = true;}

	bool Signal::isFixUnsigned() const {return isFix_ && !isSigned_;}

	void Signal::setFixUnsigned(bool newIsFix) {isFix_ = newIsFix; isSigned_ = false;}

	bool Signal::isIEEE() const {return isIEEE_;}

	void Signal::setIsIEEE(bool newIsIEEE) {isIEEE_ = newIsIEEE;}

	bool Signal::isBus() const {return isBus_;}

	Signal::SignalType Signal::type() const {return type_;}
	
	string Signal::toVHDLType() {
		ostringstream o; 
		if ((1==width())&&(!isBus_)) 
			o << " std_logic" ;
		else 
			if(isFP_) 
				o << " std_logic_vector(" << wE() <<"+"<<wF() << "+2 downto 0)";
			else if(isFix_){
				o << (isSigned_?" signed":" unsigned") << "(" << MSB_;
				if(LSB_<0)
					o  << "+" << -LSB_;
				else
					o << "-" << LSB_;
				o << " downto 0)";
			} 
			else
				o << " std_logic_vector(" << width()-1 << " downto 0)";
		return o.str();
	}


	
	string Signal::toVHDL() {
		ostringstream o; 
		if(type()==Signal::wire || type()==Signal::registeredWithoutReset
				|| type()==Signal::registeredWithAsyncReset || type()==Signal::registeredWithSyncReset
				|| type()==Signal::registeredWithZeroInitialiser)
			o << "signal ";
		o << getName();
		o << " : ";
		if (type()==Signal::in)
			o << "in ";
		if(type()==Signal::out)
			o << "out ";
	
		o << toVHDLType();
		return o.str();
	}



	string Signal::delayedName(int delay){
		ostringstream o;
#if 0
		o << getName();
		if(delay>0) {
			for (int i=0; i<delay; i++){
				o  << "_d";
			}
		}
#else // someday we need to civilize pipe signal names
		o << getName();
		if(delay>0) 
			o << "_d" << delay;
#endif
		return o.str();
	}


	string Signal::toVHDLDeclaration() {
		ostringstream o; 
		o << "signal ";
		if (type_!=Signal::in)
			o << getName() << (lifeSpan_ > 0 ? ", ": "");
		if (lifeSpan_ > 0)
			o << getName() << "_d" << 1;
		for (int i=2; i<=lifeSpan_; i++) {
			o << ", " << getName() << "_d" << i;
		}
		o << " : ";
		
		o << toVHDLType();

		if (type()==Signal::registeredWithZeroInitialiser) {
			if( (1==width()) && (!isBus_) )
				o << " := '0'";
			else
				o << ":= (others=>'0')";
		}
		o << ";";
		return o.str();
	}

	void Signal::setCycle(int cycle) {
		cycle_ = cycle;
	}

	int Signal::getCycle() {
		return cycle_;
	}

	void Signal::updateLifeSpan(int delay) {
		if(delay>lifeSpan_)
			lifeSpan_=delay;
	}

	int Signal::getLifeSpan() {
		return lifeSpan_;
	}

	double Signal::getCriticalPath(){
		return criticalPath_;
	}
	
	void Signal::setCriticalPath(double cp){
		criticalPath_=cp;
	}

	double Signal::getCriticalPathContribution(){
		return criticalPathContribution_;
	}

	void Signal::setCriticalPathContribution(double contribution){
		criticalPathContribution_ = contribution;
	}

	void Signal::resetPredecessors()
	{
		predecessors_.clear();
	}

	void Signal::addPredecessor(Signal* predecessor, int delayCycles)
	{
		//check if the signal already exists, within the same instance
		for(int i=0; (unsigned)i<predecessors_.size(); i++)
		{
			pair<Signal*, int> predecessorPair = predecessors_[i];
			if((predecessorPair.first->parentOp()->getName() == predecessor->parentOp()->getName())
					&& (predecessorPair.first->getName() == predecessor->getName())
					&& (predecessorPair.first->type() == predecessor->type())
					&& (predecessorPair.second == delayCycles))
				throw("ERROR in addPredecessor(): trying to add an already existing signal "
						<< predecessor->getName() << " to the predecessor list");
		}

		//safe to insert a new signal in the predecessor list
		pair<Signal*, int> newPredecessorPair = make_pair(predecessor, delayCycles);
		predecessors_.push_back(newPredecessorPair);
	}

	void Signal::removePredecessor(Signal* predecessor, int delayCycles)
	{
		//only try to remove the predecessor if the signal
		//	already exists, within the same instance and with the same delay
		for(int i=0; (unsigned)i<predecessors_.size(); i++)
		{
			pair<Signal*, int> predecessorPair = predecessors_[i];
			if((predecessorPair.first->parentOp_->getName() == predecessor->parentOp_->getName())
					&& (predecessorPair.first->getName() == predecessor->getName())
					&& (predecessorPair.first->type() == predecessor->type())
					&& (predecessorPair.second == delayCycles))
			{
				//delete the element from the list
				predecessors_.erase(predecessors_.begin()+i);
				return;
			}
		}

		throw("ERROR in removePredecessor(): trying to remove a non-existing signal "
				<< predecessor->getName() << " from the predecessor list");
	}

	void Signal::resetSuccessors()
	{
		successors_.clear();
	}

	void Signal::addSuccessor(Signal* successor, int delayCycles)
	{
		//check if the signal already exists, within the same instance
		for(int i=0; (unsigned)i<successors_.size(); i++)
		{
			pair<Signal*, int> successorPair = successors_[i];
			if((successorPair.first->parentOp()->getName() == successor->parentOp()->getName())
					&& (successorPair.first->getName() == successor->getName())
					&& (successorPair.first->type() == successor->type())
					&& (successorPair.second == delayCycles))
				throw("ERROR in addSuccessor(): trying to add an already existing signal "
						<< successor->getName() << " to the predecessor list");
		}

		//safe to insert a new signal in the predecessor list
		pair<Signal*, int> newSuccessorPair = make_pair(successor, delayCycles);
		successors_.push_back(newSuccessorPair);
	}

	void Signal::removeSuccessor(Signal* successor, int delayCycles)
	{
		//only try to remove the successor if the signal
		//	already exists, within the same instance and with the same delay
		for(int i=0; (unsigned)i<successors_.size(); i++)
		{
			pair<Signal*, int> successorPair = successors_[i];
			if((successorPair.first->parentOp()->getName() == successor->parentOp()->getName())
					&& (successorPair.first->getName() == successor->getName())
					&& (successorPair.first->type() == successor->type())
					&& (successorPair.second == delayCycles))
			{
				//delete the element from the list
				successors_.erase(successors_.begin()+i);
				return;
			}
		}

		throw("ERROR in removeSuccessor(): trying to remove a non-existing signal "
				<< successor->getName() << " to the predecessor list");
	}

	void  Signal::setNumberOfPossibleValues(int n){
		numberOfPossibleValues_ = n;
	}


	int  Signal::getNumberOfPossibleValues(){
		return numberOfPossibleValues_;
	}


	std::string Signal::valueToVHDL(mpz_class v, bool quot){
		std::string r;

		/* Get base 2 representation */
		r = v.get_str(2);

		/* Some checks */
		if ((int) r.size() > width())	{
			std::ostringstream o;
			o << "Error in " <<  __FILE__ << "@" << __LINE__ << ": value (" << r << ") is larger than signal " << getName();
			throw o.str();
		}

		/* Do padding */
		while ((int)r.size() < width())
			r = "0" + r;

		/* Put apostrophe / quot */
		if (!quot) return r;
		if ((width() > 1) || ( isBus() ))
			return "\"" + r + "\"";
		else
			return "'" + r + "'";
	}


	std::string Signal::valueToVHDLHex(mpz_class v, bool quot){
		std::string o;

		/* Get base 16 representation */
		o = v.get_str(16);

		/* Some check */
		/* XXX: Too permissive */
		if ((int)o.size() * 4 > width() + 4)	{
			std::ostringstream o;
			o << "Error in " <<  __FILE__ << "@" << __LINE__ << ": value is larger than signal " << getName();
			throw o.str();
		}

		/* Do padding */
		while ((int)o.size() * 4 < width())
			o = "0" + o;

		/* Put apostrophe / quot */
		if (!quot) return o;
		if (width() > 1)
			return "x\"" + o + "\"";
		else
			return "'" + o + "'";
	}


	void Signal::setName(std::string name) {
		name_=name;
	}

	void Signal::setType(SignalType t) {
		type_ = t;
	}

}
