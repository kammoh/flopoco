/*
The base Operator class, every operator should inherit it

Author : Florent de Dinechin, Bogdan Pasca

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
  All rights reserved.

*/


/*

TODOs for pipelining + bith heap compression

The algo should be:

0/ All the operators must have a pointer to their bit heap list (TODO: add the corresponding attributes and methods)

1/ if a bit heap bh is involved, the constructors perform all the bh->addBit() but do not call generateCompressorVHDL().
   They still use bh->getSumName.
   This leaves holes in the DAG corresponding to the vhdl stream 

2/ When schedule is called, it does all what it can (current code is OK for that).
   if some outputs are not scheduled
     (it means that a bit heap needs compression)
      a/ search in the (recursive) BH list one BH that is not yet scheduled and has all its bits already scheduled
           (if none is found: the DAG has another hole: error!)
      b/ generateCompressorTree for this BH.
         This adds code to the vhdl stream of this operator: it will need to be re-parsed
      c/ call schedule recursively.
   

For Martin: 
- Before generateCompressorVHDL is called, we will have the lexicographic timing 
  (i.e. cycle + delay within a cycle) for all the bits that are input to the bit heap.
  We really want Martin's algos to manage that.
  Keep in mind the typical case of a large multiplier: it adds 
      bits from its DSP blocks (arrive after 2 or 3 cycles) 
      to bits from the logic-based multipliers (arrive at cycle 0 after a small delay)
  Real-world bit heaps (e.g. sin/cos or exp or log) have even more complex, difficult to predict BH structures.
  
- Martin's algorithms compute cycles + delays. Two options to exploit this information:
    a/ ignore the cycles, just have each signal declared with a delay in the compressor trees, 
       and let Matei's scheduler re-compute cycles that will hopefully match those computed by Martin
    b/ let Martin directly hack the cycles and delays into the DAG -- probably much more code.
  I vote for a/ 

- The BitHeap will be simplified, all the timing information will be removed: 
   it is now in the Signals (once they have been scheduled).
  So the actual interface to provide to Martin is not yet fixed.





*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "Operator.hpp"  // Useful only for reporting. TODO split out the REPORT and THROWERROR #defines from Operator to another include.
#include "utils.hpp"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int.hpp>


namespace flopoco{


	// global variables used through most of FloPoCo,
	// to be encapsulated in something, someday?
	int Operator::uid = 0; 										//init of the uid static member of Operator
	int verbose=0;

	Operator::Operator(Target* target): Operator(nullptr, target){
		REPORT(INFO, "Operator  constructor without parentOp is deprecated");
	}
	
	Operator::Operator(Operator* parentOp, Target* target){
		vhdl.setOperator(this);
		stdLibType_                 = 0;						// unfortunately this is the historical default.
		target_                     = target;
		numberOfInputs_             = 0;
		numberOfOutputs_            = 0;
		hasRegistersWithoutReset_   = false;
		hasRegistersWithAsyncReset_ = false;
		hasRegistersWithSyncReset_  = false;
		hasClockEnable_             = false;
		pipelineDepth_              = 0;

		needRecirculationSignal_    = false;
		myuid                       = getNewUId();
		architectureName_			= "arch";
		indirectOperator_           = NULL;
		hasDelay1Feedbacks_         = false;

		isOperatorImplemented_      = false;
		isOperatorScheduled_        = false;
		isOperatorDrawn_            = false;

		isUnique_                   = true;
		uniquenessSet_              = false;

		signalsToSchedule.clear();
		unresolvedDependenceTable.clear();

 		parentOp_                   = nullptr; // will usually be overwritten soon after


		// Currently we set the pipeline and clock enable from the global target.
		// This is relatively safe from command line, in the sense that they can only be changed by the command-line,
		// so each sub-component of an operator will share the same target.
		// It also makes the subcomponent calls easier: pass clock and ce without thinking about it.
		// It is not very elegant because if the operator is eventually combinatorial, it will nevertheless have a clock and rst signal.
		if(target_->isPipelined())
			setSequential();
		else
			setCombinatorial();

		setClockEnable(target_->useClockEnable());

		//------- Resource estimation ----------------------------------
		resourceEstimate << "Starting Resource estimation report for entity: " << uniqueName_ << " --------------- " << endl;
		resourceEstimateReport << "";

		reHelper = new ResourceEstimationHelper(target_, this);
		reHelper->initResourceEstimation();

		reActive = false;
		//--------------------------------------------------------------

		//------- Floorplanning ----------------------------------------
		floorplan << "";

		flpHelper = new FloorplanningHelper(target_, this);
		flpHelper->initFloorplanning(0.75); 							// ratio (to what degree will the sub-components' bounding boxes
																		// be filled), see Tools/FloorplanningHelper
		//--------------------------------------------------------------
	}





	void Operator::addSubComponent(OperatorPtr op) {
		subComponentList_.push_back(op);

		REPORT(0, "WARNING: function addSubComponent() is deprecated! It seems it is still used for " << op->getName());
	}


	OperatorPtr Operator::getSubComponent(string name){
		for (auto op: subComponentList_) {
			if (op->getName()==name)
				return op;
		}
		return NULL;
	}




	void Operator::addInput(const std::string name, const int width, const bool isBus) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addInput, signal " << name << " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::in, width, isBus) ; // default TTL and cycle OK
		s->setCycle(0);
		s->setCriticalPath(0);
		s->setCriticalPathContribution(0);

		//add the signal to the input signal list and increase the number of inputs
		ioList_.push_back(s);
		numberOfInputs_ ++;
		//add the signal to the global signal list
		signalMap_[name] = s;

		//connect the input signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the sinal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		//start the scheduling, as some of the signals declared
		//	hereafter might depend on this input's timing
		schedule();
	}

	void Operator::addInput(const std::string name) {
		addInput(name, 1, false);
	}

	void Operator::addInput(const char* name) {
		addInput(name, 1, false);
	}

	void Operator::addOutput(const std::string name, const int width, const int numberOfPossibleOutputValues, const bool isBus) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addOutput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::out, width, isBus);
		s -> setNumberOfPossibleValues(numberOfPossibleOutputValues);
		//add the signal to the output signal list and increase the number of inputs
		ioList_.push_back(s);
		numberOfOutputs_ ++;
		//add the signal to the global signal list
		signalMap_[name] = s ;

		//connect the output signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the signal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		for(int i=0; i<numberOfPossibleOutputValues; i++)
			testCaseSignals_.push_back(s);
	}

	void Operator::addOutput(const std::string name) {
		addOutput (name, 1, 1, false);
	}

	void Operator::addOutput(const char* name) {
		addOutput (name, 1, 1, false);
	}


#if 1
	void Operator::addFixInput(const std::string name, const bool isSigned, const int msb, const int lsb) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addFixInput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::in, isSigned, msb, lsb);
		s->setCycle(0);
		s->setCriticalPath(0);
		s->setCriticalPathContribution(0);
		//add the signal to the input signal list and increase the number of inputs
		ioList_.push_back(s);
		numberOfInputs_ ++;

		//connect the input signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the signal to the global signal list
		signalMap_[name] = s ;

		//add the sinal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		//start the scheduling, as some of the signals declared
		//	hereafter might depend on this input's timing
		schedule();
	}

	void Operator::addFixOutput(const std::string name, const bool isSigned, const int msb, const int lsb, const int numberOfPossibleOutputValues) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addFixOutput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::out, isSigned, msb, lsb) ;
		s->setNumberOfPossibleValues(numberOfPossibleOutputValues);
		//add the signal to the output signal list and increase the number of outputs
		ioList_.push_back(s);
		numberOfOutputs_ ++;

		//connect the output signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the signal to the global signal list
		signalMap_[name] = s ;

		//add the signal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		for(int i=0; i<numberOfPossibleOutputValues; i++)
		  testCaseSignals_.push_back(s);
	}
#endif

	void Operator::addFPInput(const std::string name, const int wE, const int wF) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addFPInput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::in, wE, wF);
		s->setCycle(0);
		s->setCriticalPath(0);
		s->setCriticalPathContribution(0);
		//add the signal to the input signal list and increase the number of inputs
		ioList_.push_back(s);
		numberOfInputs_ ++;
		//add the signal to the global signal list
		signalMap_[name] = s ;

		//connect the input signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the sinal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		//start the scheduling, as some of the signals declared
		//	hereafter might depend on this input's timing
		schedule();
	}

	void Operator::addFPOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addFPOutput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::out, wE, wF);
		s -> setNumberOfPossibleValues(numberOfPossibleOutputValues);
		//add the signal to the output signal list and increase the number of outputs
		ioList_.push_back(s);
		numberOfOutputs_ ++;
		//add the signal to the global signal list
		signalMap_[name] = s ;

		//connect the output signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		for(int i=0; i<numberOfPossibleOutputValues; i++)
			testCaseSignals_.push_back(s);

		//add the sinal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);
	}


	void Operator::addIEEEInput(const std::string name, const int wE, const int wF) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addIEEEInput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::in, wE, wF, true);
		s->setCycle(0);
		s->setCriticalPath(0);
		s->setCriticalPathContribution(0);
		//add the signal to the input signal list and increase the number of inputs
		ioList_.push_back(s);
		numberOfInputs_ ++;
		//add the signal to the global signal list
		signalMap_[name] = s ;

		//connect the input signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the sinal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		//start the scheduling, as some of the signals declared
		//	hereafter might depend on this input's timing
		schedule();
	}

	void Operator::addIEEEOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addIEEEOutput, signal " << name<< " seems to already exist");
		}

		//create a new signal for the input
		// initialize its members
		Signal *s = new Signal(this, name, Signal::out, wE, wF, true) ;
		s -> setNumberOfPossibleValues(numberOfPossibleOutputValues);
		//add the signal to the output signal list and increase the number of outputs
		ioList_.push_back(s);
		numberOfOutputs_ ++;
		//add the signal to the global signal list
		signalMap_[name] = s ;

		//connect the output signal just created, if this is a subcomponent
		connectIOFromPortMap(s);

		//add the sinal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		for(int i=0; i<numberOfPossibleOutputValues; i++)
			testCaseSignals_.push_back(s);
	}


	void Operator::connectIOFromPortMap(Signal *portSignal)
	{
		Signal *connectionSignal = nullptr; // connectionSignal is the actual signal connected to portSignal
		map<std::string, Signal*>::iterator itStart, itEnd;

		//TODO: add more checks here
		//if this is a global operator, then there is nothing to be done
		if(parentOp_ == nullptr)
			return;

		//check that portSignal is really an I/O signal
		if((portSignal->type() != Signal::in) && (portSignal->type() != Signal::out))
			THROWERROR("Error: signal " << portSignal->getName() << " is not an input or output signal");

		//select the iterators according to the signal type
		if(portSignal->type() == Signal::in){
			REPORT(FULL, "connectIOFromPortMap(" << portSignal->getName() <<") : this is an input ");  
			itStart = parentOp_->tmpInPortMap_.begin();
			itEnd = parentOp_->tmpInPortMap_.end();
		}else{
			REPORT(FULL, "connectIOFromPortMap(" << portSignal->getName() <<") : this is an output ");  
			itStart = parentOp_->tmpOutPortMap_.begin();
			itEnd = parentOp_->tmpOutPortMap_.end();
		} 

		//check that portSignal exists on the parent operator's port map
		for(map<std::string, Signal*>::iterator it=itStart; it!=itEnd; it++)
		{
			if(it->first == portSignal->getName()){
				connectionSignal = it->second;
				break;
			}
		}
		//check if any match was found in the port mappings
		if(connectionSignal == nullptr)
			THROWERROR("Error: I/O port " << portSignal->getName() << " of operator " << getName()
					<< " is not connected to any signal of parent operator " << parentOp_->getName());
		//sanity check: verify that the signal that was found is actually part of the parent operator
		try{
			parentOp_->getSignalByName(connectionSignal->getName());
		}catch(string &e){
			THROWERROR("Error: signal " << connectionSignal->getName()
					<< " cannot be found in what is supposed to be its parent operator: " << parentOp_->getName());
		}

		REPORT(FULL, portSignal->getName() << "   connected to " << connectionSignal->getName() << " whose timing is cycle=" << connectionSignal->getCycle() << " CP=" << connectionSignal->getCriticalPath() );  
		//now we can connect the two signals
		if(portSignal->type() == Signal::in)
		{
			//this is an input signal
			portSignal->addPredecessor(connectionSignal, 0);
			connectionSignal->addSuccessor(portSignal, 0);
		}else{
			//this is an output signal
			portSignal->addSuccessor(connectionSignal, 0);
			connectionSignal->addPredecessor(portSignal, 0);
		}

		//if the port was connected to a signal automatically created,
		//	then copy the details of the port to the respective signal
		if(connectionSignal->getIncompleteDeclaration() == true)
		{
			connectionSignal->copySignalParameters(portSignal);
			connectionSignal->setIncompleteDeclaration(false);
			connectionSignal->setCriticalPathContribution(0.0);
		}
	}


	void Operator::reconnectIOPorts(bool restartSchedule)
	{
		//schedule the parent operator, if it hasn't already been done
		if(parentOp_ == nullptr){
			THROWERROR("Error: reconnectIOPorts: trying to connect a subcomponent to an empty parent operator");
		}else{
			if(!parentOp_->isOperatorScheduled())
				parentOp_->schedule(false);
		}
		//connect the inputs and outputs of the operator to the corresponding	signals in the parent operator
		for(vector<Signal*>::iterator it=ioList_.begin(); it!=ioList_.end(); it++)
			connectIOFromPortMap(*it);
		//	add the IO ports to the list of signals to be scheduled
		for(vector<Signal*>::iterator it=ioList_.begin(); it!=ioList_.end(); it++)
			signalsToSchedule.push_back(*it);
		//mark the signal as not scheduled
		markOperatorUnscheduled();
		//	now re-start the scheduling
		if(restartSchedule == true)
			schedule(false);
	}

	
	void Operator::markOperatorUnscheduled()
	{
		setIsOperatorScheduled(false);

		for(auto i: ioList_){
			if((i->type() == Signal::constant) || (i->type() == Signal::constantWithDeclaration)) {
				REPORT(FULL, "markOperatorUnscheduled: NOT de-scheduling signal " << i->getName());
				i->setHasBeenScheduled(true);
			}
			else {
				bool allPredecessorsConstant = i->unscheduleSignal();

				if(allPredecessorsConstant){
					REPORT(FULL, "markOperatorUnscheduled: NOT de-scheduling signal " << i->getName());
				}else{
					REPORT(FULL, "markOperatorUnscheduled: de-scheduling signal " << i->getName());
				}
			}
		}

		for(auto i: signalList_) {
			if((i->type() == Signal::constant) || (i->type() == Signal::constantWithDeclaration)) {
				REPORT(FULL, "markOperatorUnscheduled: NOT de-scheduling signal " << i->getName());
				i->setHasBeenScheduled(true);
			}
			else {
				bool allPredecessorsConstant = i->unscheduleSignal();

				if(allPredecessorsConstant){
					REPORT(FULL, "markOperatorUnscheduled: NOT de-scheduling signal " << i->getName());
				}else{
					REPORT(FULL, "markOperatorUnscheduled: de-scheduling signal " << i->getName());
				}
			}
		}

		for(auto i: subComponentList_) {
			i->markOperatorUnscheduled();
			for (auto j: i->ioList_) {
				signalsToSchedule.push_back(j);
			}
		}
	}


	void Operator::markOperatorScheduled()
	{
		setIsOperatorScheduled(true);

		for(auto i: ioList_){
			i->setHasBeenScheduled(true);
		}

		for(auto i: signalList_) {
			i->setHasBeenScheduled(true);
		}

		for(auto i: subComponentList_) {
			i->markOperatorScheduled();
		}
	}



	Signal* Operator::getSignalByName(string name) {
		//search for the signal in the list of signals
		if(signalMap_.find(name) == signalMap_.end()) {
			//signal not found, throw an error
			THROWERROR("ERROR in getSignalByName, signal " << name << " not declared");
		}
		//signal found, return the reference to it
		return signalMap_[name];
	}



	Signal* Operator::getDelayedSignalByName(string name) {
		//remove the '^xx' from the signal name, if it exists,
		//and then check if the signal is in the signal map
		string n;

		//check if this is the name of a delayed signal
		if(name.find('^') != string::npos){
			n = name.substr(0, name.find('^'));
		}else{
			n = name;
		}

		//search for the stripped signal name
		if(signalMap_.find(n) == signalMap_.end()){
			//signal not found, throw an error
			THROWERROR("ERROR in getDelayedSignalByName, signal " << name << " not declared");
		}

		return signalMap_[n];
	}


	vector<Signal*> Operator::getSignalList(){
		return signalList_;
	};

	bool Operator::isSignalDeclared(string name){
		if(signalMap_.find(name) ==  signalMap_.end()){
			return false;
		}
		return true;
	}


	void Operator::addHeaderComment(std::string comment){
		headerComment_ += comment;
	}

	void Operator::setName(std::string prefix, std::string postfix){
		ostringstream pr, po;

		if (prefix.length() > 0)
			pr << prefix << "_";
		else
			pr << "";
		if (postfix.length() > 0)
			po << "_" << postfix;
		else
			po << "";

		uniqueName_ = pr.str() + uniqueName_ + po.str();
	}

	void Operator::setName(std::string operatorName){
		uniqueName_ = operatorName;
	}

	// TODO Should be removed in favor of setNameWithFreqAndUID
	void Operator::setNameWithFreq(std::string operatorName){
		std::ostringstream o;

		o <<  operatorName <<  "_" ;
		if(target_->isPipelined())
			o << "F" << target_->frequencyMHz() ;
		else
			o << "comb";
		uniqueName_ = o.str();
	}

	void Operator::setNameWithFreqAndUID(std::string operatorName){
		std::ostringstream o;
		o <<  operatorName <<  "_" ;
		if(target_->isPipelined())
			o << "F"<<target_->frequencyMHz() ;
		else
			o << "comb";
		o << "_uid" << getNewUId();
		uniqueName_ = o.str();
	}

	void  Operator::changeName(std::string operatorName){
		commentedName_ = uniqueName_;
		uniqueName_ = operatorName;
	}

	string Operator::getName() const{
		return uniqueName_;
	}

	int Operator::getNewUId(){
		Operator::uid++;
		return Operator::uid;
	}

	OperatorPtr Operator::setParentOperator(OperatorPtr parentOp){
		if(parentOp_ != nullptr)
			THROWERROR("Error: parent operator already set for operator " << getName());
		parentOp_ = parentOp;

		return parentOp_;
	}

	
	int Operator::getIOListSize() const{
		return ioList_.size();
	}

	vector<Signal*>* Operator::getIOList(){
		return &ioList_;
	}

	Signal* Operator::getIOListSignal(int i){
		return ioList_[i];
	}



	void  Operator::outputVHDLSignalDeclarations(std::ostream& o) {
		for (unsigned int i=0; i < this->signalList_.size(); i++){
			Signal* s = this->signalList_[i];
			o << tab << s->toVHDL() << ";" << endl;
		}
	}



	void Operator::outputVHDLComponent(std::ostream& o, std::string name) {
		unsigned int i;

		o << tab << "component " << name << " is" << endl;
		if (ioList_.size() > 0)
		{
			o << tab << tab << "port ( ";
			if(isSequential()) {
				// add clk, rst, etc. signals which are not member of iolist
				if(hasClockEnable())
					o << "clk, rst, ce : in std_logic;" <<endl;
				else if(isRecirculatory())
					o << "clk, rst, stall_s: in std_logic;" <<endl;
				else
					o << "clk, rst : in std_logic;" <<endl;
			}

			for (i=0; i<this->ioList_.size(); i++){
				Signal* s = this->ioList_[i];

				if (i>0 || isSequential()) // align signal names
					o << tab << "          ";
				o <<  s->toVHDL();
				if(i < this->ioList_.size()-1)
					o << ";" << endl;
			}
			o << tab << ");"<<endl;
		}
		o << tab << "end component;" << endl;
	}

	void Operator::outputVHDLComponent(std::ostream& o) {
		this->outputVHDLComponent(o,  this->uniqueName_);
	}



	void Operator::outputVHDLEntity(std::ostream& o) {
		unsigned int i;
		o << "entity " << uniqueName_ << " is" << endl;
		if (ioList_.size() > 0)
		{
			o << tab << " port ( ";
			if(isSequential()) {
					// add clk, rst, etc. signals which are not member of iolist
				if(hasClockEnable())
					o << "clk, rst, ce : in std_logic;" <<endl;
				else if(isRecirculatory())
					o << "clk, rst, stall_s: in std_logic;" <<endl;
				else
					o << "clk, rst : in std_logic;" <<endl;
			}

			for (i=0; i<this->ioList_.size(); i++){
				Signal* s = this->ioList_[i];
				if (i>0 || isSequential()) // align signal names
					o << "          ";
				o << s->toVHDL();
				if(i < this->ioList_.size()-1)
					o<<";" << endl;
			}

			o << tab << ");"<<endl;
		}
		o << "end entity;" << endl << endl;
	}


	void Operator::setCopyrightString(std::string authorsYears){
		copyrightString_ = authorsYears;
	}

	void Operator::useStdLogicUnsigned() {
		stdLibType_ = 0;
	};

	/**
	 * Use the Synopsys de-facto standard ieee.std_logic_unsigned for this entity
	 */
	void Operator::useStdLogicSigned() {
		stdLibType_ = -1;
	};

	void Operator::useNumericStd() {
		stdLibType_ = 1;
	};

	void Operator::useNumericStd_Signed() {
		stdLibType_ = 2;
	};

	void Operator::useNumericStd_Unsigned() {
		stdLibType_ = 3;
	};

	int Operator::getStdLibType() {
		return stdLibType_;
	};

	void Operator::licence(std::ostream& o){
		licence(o, copyrightString_);
	}


	void Operator::licence(std::ostream& o, std::string authorsyears){
		o<<"--------------------------------------------------------------------------------"<<endl;
		// centering the unique name
		int s, i;

		if(uniqueName_.size()<76)
			s = (76-uniqueName_.size())/2;
		else
			s=0;
		o<<"--"; for(i=0; i<s; i++) o<<" "; o  << uniqueName_ << endl;

		// if this operator was renamed from the command line, show the original name
		if(commentedName_!="") {
			if(commentedName_.size()<74) s = (74-commentedName_.size())/2; else s=0;
			o<<"--"; for(i=0; i<s; i++) o<<" "; o << "(" << commentedName_ << ")" << endl;
		}
		o<< headerComment_;
		o << "-- VHDL generated for " << getTarget()->getID() << " @ " << getTarget()->frequencyMHz() <<"MHz"  <<endl;

		o<<"-- This operator is part of the Infinite Virtual Library FloPoCoLib"<<endl;
		o<<"-- All rights reserved "<<endl;
		o<<"-- Authors: " << authorsyears <<endl;
		o<<"--------------------------------------------------------------------------------"<<endl;
	}

	
	void Operator::pipelineInfo(std::ostream& o){
		if(isSequential())
			o<<"-- Pipeline depth: " << getPipelineDepth() << " cycles"  <<endl <<endl;
		else
			o << "-- combinatorial"  <<endl <<endl;
		
	}


	void Operator::stdLibs(std::ostream& o){
		o << "library ieee;"<<endl
		  << "use ieee.std_logic_1164.all;"<<endl;

		if(stdLibType_==0){
			o << "use ieee.std_logic_arith.all;"<<endl
			  << "use ieee.std_logic_unsigned.all;"<<endl;
		}
		if(stdLibType_==-1){
			o << "use ieee.std_logic_arith.all;"<<endl
			  << "use ieee.std_logic_signed.all;"<<endl;
		}
		if(stdLibType_==1){
			o << "use ieee.numeric_std.all;"<<endl;
		}
		// ???
		if(stdLibType_==2){
			o << "use ieee.numeric_std.all;"<<endl
			  << "use ieee.std_logic_signed.all;"<<endl;
		}
		if(stdLibType_==3){
			o << "use ieee.numeric_std.all;"<<endl
			  << "use ieee.std_logic_unsigned.all;"<<endl;
		}

		o << "library std;" << endl
		  << "use std.textio.all;"<< endl
		  << "library work;"<<endl<< endl;
	};


	void Operator::outputVHDL(std::ostream& o) {
		this->outputVHDL(o, this->uniqueName_);
	}

	bool Operator::isSequential() {
		return isSequential_;
	}

	bool Operator::isRecirculatory() {
		return needRecirculationSignal_;
	}

	void Operator::setSequential() {
		isSequential_=true;
		//TODO: for now, the vhdl code parsing is enabled for all operators
		//vhdl.disableParsing(false);
		vhdl.disableParsing(false);
	}

	void Operator::setCombinatorial() {
		isSequential_=false;
		//TODO: for now, the vhdl code parsing is enabled for all operators
		//vhdl.disableParsing(true);
		vhdl.disableParsing(false);
	}

	void Operator::setRecirculationSignal() {
		needRecirculationSignal_ = true;
	}


	int Operator::getPipelineDepth() {
		setPipelineDepth();
		return pipelineDepth_;
	}

	void Operator::setPipelineDepth(int d) {
		cerr << "WARNING: function setPipelineDepth(int depth) is deprecated" << endl;

		pipelineDepth_ = d;
	}

	void Operator::setPipelineDepth()
	{
		int maxInputCycle, maxOutputCycle;
		bool firstInput = true, firstOutput = true;

		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			if(firstInput && (ioList_[i]->type() == Signal::in))
			{
				maxInputCycle = ioList_[i]->getCycle();
				firstInput = false;
				continue;
			}
			if((ioList_[i]->type() == Signal::in) && (ioList_[i]->getCycle() > maxInputCycle))
			{
				maxInputCycle = ioList_[i]->getCycle();
				continue;
			}

			if(firstOutput && (ioList_[i]->type() == Signal::out))
			{
				maxOutputCycle = ioList_[i]->getCycle();
				firstOutput = false;
				continue;
			}
			if((ioList_[i]->type() == Signal::out) && (ioList_[i]->getCycle() > maxOutputCycle))
			{
				maxOutputCycle = ioList_[i]->getCycle();
				continue;
			}
		}

		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			if((ioList_[i]->type() == Signal::out) && (ioList_[i]->getCycle() != maxOutputCycle))
				REPORT(INFO, "WARNING: setPipelineDepth(): this operator's outputs are NOT SYNCHRONIZED!");
		}

		pipelineDepth_ = maxOutputCycle-maxInputCycle;
	}

	void Operator::outputFinalReport(ostream& s, int level) {

		if (getIndirectOperator()!=NULL){
			// interface operator
			if(getSubComponentList().size()!=1){
				ostringstream o;
				o << "!?! Operator " << getUniqueName() << " is an interface operator with " << getSubComponentList().size() << "children";
				throw o.str();
			}
			getSubComponentList()[0]->outputFinalReport(s, level);
		}else
		{
			// Hard operator
			if (! getSubComponentList().empty())
				for (auto i: getSubComponentList())
					i->outputFinalReport(s, level+1);

			ostringstream tabs, ctabs;
			for (int i=0;i<level-1;i++){
				tabs << "|" << tab;
				ctabs << "|" << tab;
			}

			if (level>0){
				tabs << "|" << "---";
				ctabs << "|" << tab;
			}

			s << tabs.str() << "Entity " << uniqueName_ << endl;
			if(this->getPipelineDepth()!=0)
				s << ctabs.str() << tab << "Pipeline depth = " << getPipelineDepth() << endl;
			else
				s << ctabs.str() << tab << "Not pipelined"<< endl;
		}
	}


	void Operator::setCycle(int cycle, bool report) {
		REPORT(0,"WARNING: function setCycle() is deprecated and no longer has any effect!");
	}

	int Operator::getCurrentCycle(){
		REPORT(0,"WARNING: function getCurrentCycle() is deprecated and no longer has any effect!");
		return -1;
	}

	void Operator::nextCycle(bool report) {
		REPORT(0, "WARNING: function nextCycle() is deprecated and no longer has any effect!");
	}

	void Operator::previousCycle(bool report) {
		REPORT(0, "WARNING: function previousCycle() is deprecated and no longer has any effect!");
	}


	void Operator::setCycleFromSignal(string name, bool report) {
		REPORT(0, "WARNING: function setCycleFromSignal() is deprecated and no longer has any effect!");
	}


	void Operator::setCycleFromSignal(string name, double criticalPath, bool report) {
		REPORT(0, "WARNING: function setCycleFromSignal() is deprecated and no longer has any effect!");
	}


	int Operator::getCycleFromSignal(string name, bool report) {

		if(isSequential()) {
			Signal* s;

			try {
				s = getSignalByName(name);
			}catch(string &e) {
				THROWERROR("): ERROR in getCycleFromSignal, " << endl << tab << e);
			}

			if(s->getCycle() < 0){
				THROWERROR("Signal " << name << " doesn't have (yet?) a valid cycle" << endl);
			}

			return s->getCycle();
		}else{
			//if combinatorial, everything happens at cycle 0
			return 0;
		}
	}

	double Operator::getCPFromSignal(string name, bool report)
	{
		Signal* s;

		try {
			s = getSignalByName(name);
		}
		catch(string &e) {
			THROWERROR("): ERROR in getCPFromSignal, " << endl << tab << e);
		}

		return s->getCriticalPath();
	}

	double Operator::getCPContributionFromSignal(string name, bool report)
	{
		Signal* s;

		try {
			s = getSignalByName(name);
		}
		catch(string &e) {
			THROWERROR("): ERROR in getCPContributionFromSignal, " << endl << tab << e);
		}

		return s->getCriticalPathContribution();
	}


	bool Operator::syncCycleFromSignal(string name, bool report) {

		REPORT(0, "WARNING: function syncCycleFromSignal() is deprecated and no longer has any effect!");

		return false;
	}



	bool Operator::syncCycleFromSignal(string name, double criticalPath, bool report) {

		REPORT(0, "WARNING: function syncCycleFromSignal() is deprecated and no longer has any effect!");
		return false;
	}

	// TODO get rid of this method
	void Operator::setSignalDelay(string name, double delay){
		REPORT(0, "WARNING: function setSignalDelay() is deprecated and no longer has any effect!");
	}

	// TODO get rid of this method
	double Operator::getSignalDelay(string name){
		REPORT(0, "WARNING: function getSignalDelay() is deprecated!");

		Signal* s;

		try {
			s=getSignalByName(name);
		}
		catch (string &e2) {
			cout << "WARNING: signal " << name << " was not found in file " << srcFileName << " when called using getSignalDelay" << endl;
			return 0.0;
		}

		return s->getCriticalPath();
	}

#if 1
	double Operator::getCriticalPath()
	{
		REPORT(0, "WARNING: function getCriticalPath() is deprecated!");

		return -1;
	}

	
	void Operator::setCriticalPath(double delay)
	{
		REPORT(0, "WARNING: function setCriticalPath() is deprecated  and no longer has any effect!");
	}

	void Operator::addToCriticalPath(double delay)
	{
		REPORT(0, "WARNING: function addToCriticalPath() is deprecated  and no longer has any effect!");
	}


#endif


	bool Operator::manageCriticalPath(double delay, bool report){
		REPORT(0, "WARNING: function manageCriticalPath() is deprecated  and no longer has any effect!");

		return false;
	}

	double Operator::getOutputDelay(string s)
	{
		Signal *signal = NULL;

		for(unsigned i=0; i<ioList_.size(); i++)
			if((ioList_[i]->getName() == s) && (ioList_[i]->type() == Signal::out))
			{
				signal = ioList_[i];
			}
		if(signal == NULL)
			THROWERROR("Error: getOutputDelay(): signal " << s << " not found" << endl);

		return signal->getCriticalPath();
	}

	string Operator::declare(string name, const int width, bool isbus, Signal::SignalType regType, bool incompleteDeclaration) {
		return declare(0.0, name, width, isbus, regType, incompleteDeclaration);
	}

	string Operator::declare(string name, Signal::SignalType regType, bool incompleteDeclaration) {
		return declare(0.0, name, 1, false, regType, incompleteDeclaration);
	}

	string Operator::declare(double criticalPathContribution, string name, Signal::SignalType regType, bool incompleteDeclaration) {
		return declare(criticalPathContribution, name, 1, false, regType, incompleteDeclaration);
	}

	string Operator::declare(double criticalPathContribution, string name, const int width, bool isbus, Signal::SignalType regType, bool incompleteDeclaration) {
		Signal* s;

		// check the signal doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declare(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, width, isbus);
		// initialize the rest of its attributes
		initNewSignal(s, criticalPathContribution, regType, incompleteDeclaration);

		// add the signal on the list of signals from which to schedule
		signalsToSchedule.push_back(s);

		return name;
	}


	string Operator::declareFixPoint(string name, const bool isSigned, const int MSB, const int LSB, Signal::SignalType regType, bool incompleteDeclaration){
		return declareFixPoint(0.0, name, isSigned, MSB, LSB, regType, incompleteDeclaration);
	}


	string Operator::declareFixPoint(double criticalPathContribution, string name, const bool isSigned, const int MSB, const int LSB, Signal::SignalType regType, bool incompleteDeclaration){
		Signal* s;

		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declareFixPoint(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, isSigned, MSB, LSB);

		initNewSignal(s, criticalPathContribution, regType, incompleteDeclaration);

		//set the flag for a fixed-point signal
		s->setIsFix(true);
		s->setIsFP(false);
		s->setIsIEEE(false);

		// add the signal on the list of signals from which to schedule
		signalsToSchedule.push_back(s);

		return name;
	}


	string Operator::declareFloatingPoint(string name, const int wE, const int wF, Signal::SignalType regType, const bool ieeeFormat, bool incompleteDeclaration){
		return declareFloatingPoint(0.0, name, wE, wF, regType, ieeeFormat, incompleteDeclaration);
	}


	string Operator::declareFloatingPoint(double criticalPathContribution, string name, const int wE, const int wF, Signal::SignalType regType, const bool ieeeFormat, bool incompleteDeclaration){
		Signal* s;

		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declareFixPoint(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, wE, wF, ieeeFormat);

		initNewSignal(s, criticalPathContribution, regType, incompleteDeclaration);

		s->setIsFix(false);
		//set the flag for a floating-point signal
		if(ieeeFormat){
			s->setIsFP(false);
			s->setIsIEEE(true);
		}else{
			s->setIsFP(true);
			s->setIsIEEE(false);
		}

		// add the signal on the list of signals from which to schedule
		signalsToSchedule.push_back(s);

		return name;
	}


	std::string Operator::declareTable(string name, int width, std::string tableAttributes, bool incompleteDeclaration)
	{
		return declareTable(0.0, name, width, tableAttributes, incompleteDeclaration);
	}

	std::string Operator::declareTable(double criticalPathContribution, string name, int width, std::string tableAttributes, bool incompleteDeclaration)
	{
		Signal* s;

		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declareFixPoint(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, Signal::table, width, tableAttributes);

		initNewSignal(s, criticalPathContribution, Signal::table, incompleteDeclaration);

		// set all flag types to false
		s->setIsFix(false);
		s->setIsFP(false);
		s->setIsIEEE(false);

		// add the signal on the list of signals from which to schedule
		signalsToSchedule.push_back(s);

		return name;
	}


	void Operator::initNewSignal(Signal* s, double criticalPathContribution, Signal::SignalType regType, bool incompleteDeclaration)
	{
		//set, if needed, the global parameters used for generating the registers
		if((regType==Signal::registeredWithoutReset) || (regType==Signal::registeredWithZeroInitialiser))
			hasRegistersWithoutReset_ = true;
		if(regType==Signal::registeredWithSyncReset)
			hasRegistersWithSyncReset_ = true;
		if(regType==Signal::registeredWithAsyncReset)
			hasRegistersWithAsyncReset_ = true;

		//define its cycle, critical path and contribution to the critical path
		s->setCycle(0);
		s->setCriticalPath(0.0);
		s->setCriticalPathContribution(criticalPathContribution);

		//whether this is an incomplete signal declaration
		s->setIncompleteDeclaration(incompleteDeclaration);

		//initialize the signals predecessors and successors
		s->resetPredecessors();
		s->resetSuccessors();

		//add the signal to signalMap and signalList
		signalList_.push_back(s);
		signalMap_[s->getName()] = s;
	}

	void  Operator::resizeFixPoint(string lhsName, string rhsName, const int MSB, const int LSB, const int indentLevel){
		Signal* rhsSignal = getSignalByName(rhsName);
		bool isSigned = rhsSignal->isFixSigned();
		int oldMSB = rhsSignal->MSB();
		int oldLSB = rhsSignal->LSB();

		REPORT(DEBUG, "Resizing signal " << rhsName << " from (" << oldMSB << ", " << oldLSB << ") to (" << MSB << ", " << LSB << ")");

		for (int i=0; i<indentLevel; i++)
			vhdl << tab;

		vhdl << declareFixPoint(rhsSignal->getCriticalPathContribution(), lhsName, isSigned, MSB, LSB, rhsSignal->type()) << " <= ";

		// Cases (old is input, new is output)
		//            1            2W             3W        4         5E         6 E
		// Old:      ooooooo   oooooooo      oooooooooo    oooo     ooo               ooo
		// New:  nnnnnnnn        nnnnnnnn     nnnnnn      nnnnnnn       nnnn      nnn

		bool paddLeft, paddRight;
		int m,l, paddLeftSize, paddRightSize, oldSize; 	// eventually we take the slice m downto l of the input bit vector

		paddLeft      = MSB>oldMSB;
		paddLeftSize  = MSB-oldMSB;			// in case paddLeft is true
		paddRight     = LSB<oldLSB;
		paddRightSize = oldLSB-LSB;			// in case paddRight is true
		oldSize       = oldMSB-oldLSB+1;

		// Take input vector downto what ?
		if (LSB>=oldLSB) { // case 1 or 3
			l = LSB-oldLSB;
		}
		else {             // case 2 or 4
			l=0;
		}

		// and from what bit?
		if (MSB>oldMSB) { // cases 1 or 4
			m = oldSize-1;
		}
		else { // oldMSB>=MSB, cases 2 or 3
			if(MSB<oldMSB)
				REPORT(DETAILED, "Warning: cutting off some MSBs when resizing signal " << rhsName << " from ("
						<< oldMSB << ", " << oldLSB << ") to (" << MSB << ", " << LSB << ")");
			m = oldSize-(oldMSB-MSB)-1;
		}

		// Now do the work.
		// Possible left padding/sign extension
		if(paddLeft) {
			if(isSigned) 	{
				if(paddLeftSize == 1)
					vhdl << "(" << rhsName << of(oldSize-1) << ") & "; // sign extension
				else
					vhdl << "(" << paddLeftSize -1 << " downto 0 => " << rhsName << of(oldSize-1) << ") & "; // sign extension
			}
			else {
				vhdl << zg(paddLeftSize) << " & ";
			}
		}

		// copy the relevant bits
		vhdl << rhsName << range(m, l);

		// right padding
		if(paddRight) {
			vhdl << " & " << zg(paddRightSize);
		}

		vhdl << "; -- fix resize from (" << oldMSB << ", " << oldLSB << ") to (" << MSB << ", " << LSB << ")" << endl;
	}


	string Operator::delay(string signalName, int nbDelayCycles, SignalDelayType delayType)
	{
		ostringstream result;

		//check that the number of delay cycles is a positive integer
		if(nbDelayCycles < 0)
			THROWERROR("Error in delay(): trying to add a negative number of delay cycles:" << nbDelayCycles);
		//check that the signal exists
		if(!isSignalDeclared(signalName))
			THROWERROR("Error in delay(): trying to delay a signal that does not yet exist:" << signalName);

		if(delayType == SignalDelayType::pipeline)
			result << signalName << "^" << nbDelayCycles;
		else
			result << signalName << "^^" << nbDelayCycles;
		return result.str();
	}



	#if 1
	string Operator::use(string name) {
		cerr << "WARNING: function use() is deprecated" << endl;

		if(isSequential()) {
			Signal *s;

			try {
				s=getSignalByName(name);
			}
			catch (string &e2) {
				THROWERROR("ERROR in use(), " << endl << tab << e2 << endl);
			}

			// update the lifeSpan of s
			s->updateLifeSpan( this->getCurrentCycle() - s->getCycle() );
			//return s->delayedName( currentCycle_ - s->getCycle() );
			return s->delayedName( 0 );
		}
		else
			return name;
	}

	string Operator::use(string name, int delay) {
		cerr << "WARNING: function use() is deprecated" << endl;

		if(isSequential()) {
			Signal *s;
			try {
				s = getSignalByName(name);
			}
			catch (string &e2) {
				THROWERROR("ERROR in use(), " << endl << tab << e2 << endl);
			}

			// update the lifeSpan of s
			s->updateLifeSpan( delay );

			//return s->delayedName( currentCycle_ - s->getCycle() );
			return s->delayedName( delay );
		}else
			return name;
	}

	#endif

	void Operator::outPortMap(Operator* op, string componentPortName, string actualSignalName, bool newSignal){
		Signal *s;

		// check if the signal already exists, when we're supposed to create a new signal
		if(newSignal && (signalMap_.find(actualSignalName) !=  signalMap_.end())) {
			THROWERROR("ERROR in outPortMap(): signal " << actualSignalName << " already exists");
		}

		//check if the signal connected to the port exists, and return it if so
		//	or create it if it doesn't exist
		s = nullptr;
		if(newSignal){
			//create the new signal
			//	this will be an incomplete signal, as we cannot know the exact details of the signal yet
			//	the rest of the information will be completed by addOutput, which has the rest of the required information
			//	and add it to the list of signals to be scheduled
			declare(actualSignalName, -1, false);
			getSignalByName(actualSignalName)->setIncompleteDeclaration(true);
			s = getSignalByName(actualSignalName);
		}else
		{
			try {
				s = getSignalByName(actualSignalName);
			}
			catch(string &e2) {
				THROWERROR("ERROR in outPortMap(): " << e2);
			}
		}

		// add the mapping to the output mapping list of Op
		tmpOutPortMap_[componentPortName] = s;

		//add the signal to the list of signals to be scheduled
		//	if the signal already exists
		if(s != nullptr)
			signalsToSchedule.push_back(s);
	}


	void Operator::inPortMap(Operator* op, string componentPortName, string actualSignalName){
		Signal *s;
		std::string name;

		//check if the signal already exists
		try{
			s = getSignalByName(actualSignalName);
			//check that port can be scheduled
			if(s->getHasBeenScheduled() == false){
				THROWERROR("ERROR in inPortMap() while trying to connect an input: "
					<< componentPortName << " is to be connected to a signal not yet scheduled:"
					<< s->getName() << ". Cannot continue as the architecture might depend on its timing." << endl);
			}
		}
		catch(string &e2) {
			THROWERROR("ERROR in inPortMap(): " << e2);
		}

		// add the mapping to the input mapping list of Op
		tmpInPortMap_[componentPortName] = s;
	}



	void Operator::inPortMapCst(Operator* op, string componentPortName, string actualSignal){
		Signal *s;
		string name;
		double constValue;
		sollya_obj_t node;

		// TODO: do we need to add the input port mapping to the mapping list of Op?
		// 		as this is a constant signal

		//try to parse the constant
		node = sollya_lib_parse_string(actualSignal.c_str());
		// If conversion did not succeed (i.e. parse error)
		if(node == 0)
			THROWERROR("Unable to parse string " << actualSignal << " as a numeric constant" << endl);
		sollya_lib_get_constant_as_double(&constValue, node);
		sollya_lib_clear_obj(node);

		//create a new signal for constant input
		s = new Signal(this, join(actualSignal, "_cst_", vhdlize(getNewUId())), Signal::constant, constValue);

		//initialize the signals predecessors and successors
		s->resetPredecessors();
		s->resetSuccessors();

		//set the timing for the constant signal, at cycle 0, criticalPath 0, criticalPathContribution 0
		s->setCycle(0);
		s->setCriticalPath(0.0);
		s->setCriticalPathContribution(0.0);
		s->setHasBeenScheduled(true);

		// add the newly created signal to signalMap and signalList
		signalList_.push_back(s);
		signalMap_[s->getName()] = s;

		// add the signal to the list of signals to be scheduled
		signalsToSchedule.push_back(s);

		tmpInPortMap_[componentPortName] = s;
	}


	string Operator::instance(Operator* op, string instanceName){
		ostringstream o;

		//block the state of this operator's implementation to
		//	the currently chose state: either fully flattened, or shared
		op->uniquenessSet_ = true;

		//disable the drawing for this subcomponent
		//	if needed, the drawing procedures will re-enable it
		op->setIsOperatorDrawn(true);

		//check that the operator being instantiated has the parent operator set to this operator
		//	if not, then set it and restart the scheduling for the subcomponent
		if((op->isUnique()) & (op->parentOp_ == nullptr))
		{
			op->parentOp_ = this;
			op->reconnectIOPorts(true);
		}

		// TODO add more checks here

		//checking that all the signals are covered
		for(unsigned int i=0; i<op->getIOList()->size(); i++)
		{
			Signal *signal = op->getIOListSignal(i);
			bool isSignalMapped = false;
			bool isPredScheduled = false;
			map<string, Signal*>::iterator iterStart, iterStop;

			//set the start and stop values for the iterators, for either the input,
			//	or the output temporary port map, depending on the signal type
			if(signal->type() == Signal::in)
			{
				iterStart = tmpInPortMap_.begin();
				iterStop = tmpInPortMap_.end();
			}else{
				iterStart = tmpOutPortMap_.begin();
				iterStop = tmpOutPortMap_.end();
			}

			for(map<string, Signal*>::iterator it=iterStart; it!=iterStop; it++)
			{
				if(it->first == signal->getName())
				{
					//mark the signal as connected
					isSignalMapped = true;
					//check if the signal connected to the port has been scheduled
					isPredScheduled = it->second->getHasBeenScheduled();
					break;
				}
			}

			//if the port is not connected to a signal in the parent operator,
			//	then we cannot continue
			if(!isSignalMapped)
				THROWERROR("ERROR in instance() while trying to create a new instance of "
						<< op->getName() << " called " << instanceName << ": input/output "
						<< signal->getName() << " is not mapped to anything" << endl);
			//if the signal to which the port is connected is not yet scheduled,
			//	then we cannot continue
			if(!isPredScheduled && (signal->type() == Signal::in))
				THROWERROR("ERROR in instance() while trying to create a new instance of "
						<< op->getName() << " called " << instanceName << ": input/output "
						<< signal->getName() << " is connected to a signal not yet scheduled."
						<< " Cannot continue as the architecture might depend on its timing." << endl);
		}

		o << tab << instanceName << ": " << op->getName();
		o << endl;
		o << tab << tab << "port map ( ";

		// build vhdl and erase portMap_
		if(op->isSequential())
		{
			o << "clk  => clk" << "," << endl
			  << tab << tab << "           rst  => rst";
			if (op->isRecirculatory()) {
				o << "," << endl << tab << tab << "           stall_s => stall_s";
			};
			if (op->hasClockEnable()) {
				o << "," << endl << tab << tab << "           ce => ce";
			};
		}

		//build the code for the inputs
		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++)
		{
			string rhsString;

			if((it != tmpInPortMap_.begin()) || op->isSequential())
				o << "," << endl <<  tab << tab << "           ";

			// The following code assumes that the IO is declared as standard_logic_vector
			// If the actual parameter is a signed or unsigned, we want to automatically convert it
			if(it->second->type() == Signal::constant){
				rhsString = it->second->getName().substr(0, it->second->getName().find("_cst"));
			}else if(it->second->isFix()){
				rhsString = std_logic_vector(it->second->getName());
			}else{
				rhsString = it->second->getName();
			}

			o << it->first << " => " << rhsString;
		}

		//build the code for the outputs
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
		{
			//the signal connected to the output might be an incompletely declared signal,
			//	so its information must be completed and it must be properly connected now
			if(it->second->getIncompleteDeclaration() == true)
			{
				//copy the details from the output port
				it->second->copySignalParameters(op->getSignalByName(it->first));
				//mark the signal as completely declared
				it->second->setIncompleteDeclaration(false);
				//connect the port to the corresponding signal
				//	for unique instances
				if(op->isUnique())
				{
					it->second->addPredecessor(op->getSignalByName(it->first));
					op->getSignalByName(it->first)->addSuccessor(it->second);
				}
				//the new signal doesn't add anything to the critical path
				it->second->setCriticalPathContribution(0.0);
				//add the newly created signal to the list of signals to schedule
				it->second->setHasBeenScheduled(false);
				if(op->isUnique())
				{
					signalsToSchedule.push_back(it->second);
				}
			}

			if(  (it != tmpOutPortMap_.begin())  ||   (tmpInPortMap_.size() != 0)   ||   op->isSequential()  )
				o << "," << endl <<  tab << tab << "           ";

			o << it->first << " => " << it->second->getName();
		}

		o << ");" << endl;

		//if this is a global operator, then
		//	check if this is the first time adding this global operator
		//	to the operator list. If it isn't, then insert the copy, instead
		bool newOpFirstAdd = true;

		if(op->isShared())
		{
			for(unsigned int i=0; i<UserInterface::globalOpList.size(); i++)
				if(UserInterface::globalOpList[i]->getName() == op->getName())
				{
					newOpFirstAdd = false;
					break;
				}
		}

		//create a reference to the operator that will be added to the operator list
		//	either the operator, or a copy of the operator
		Operator *opCpy;

		if(op->isShared())
		{
			opCpy = newSharedInstance(op);

			//if this is the first instance of a global operator, then add
			//	the original to the global operator list, as well
			if(newOpFirstAdd == true)
			{
				op->setIsOperatorImplemented(false);
				UserInterface::addToGlobalOpList(op);
			}
		}else{
			//save a reference
			opCpy = op;
		}

		//mark the subcomponent as having to be scheduled
		opCpy->markOperatorUnscheduled();

		//schedule the subcomponent
		for(auto i : opCpy->ioList_)
			if(i->type() == Signal::in)
				opCpy->signalsToSchedule.push_back(i);
		opCpy->schedule(!opCpy->isOperatorScheduled());

		//add the operator to the subcomponent list/map
		subComponentList_.push_back(opCpy);

		//clear the port mappings
		tmpInPortMap_.clear();
		tmpOutPortMap_.clear();


		//Floorplanning ------------------------------------------------
		floorplan << manageFloorplan();
		flpHelper->addToFlpComponentList(opCpy->getName());
		flpHelper->addToInstanceNames(opCpy->getName(), instanceName);
		//--------------------------------------------------------------


		return o.str();
	}


	OperatorPtr Operator::newSharedInstance(Operator *originalOperator)
	{
		//create a new operator
		Operator *newOp = new Operator(originalOperator->getTarget());

		//deep copy the original operator
		//	this should also connect the inputs/outputs of the new operator to the right signals
		newOp->deepCloneOperator(originalOperator);

		//reconnect the inputs/outputs to the corresponding external signals
		newOp->setParentOperator(this);
		newOp->reconnectIOPorts(false);

		//recreate the connection of the outputs and the corresponding dependences
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
		{
			//the signal connected to the output might be an incompletely declared signal,
			//	so its information must be completed and it must be properly connected now
			if(it->second->getIncompleteDeclaration() == true)
			{
				it->second->addPredecessor(newOp->getSignalByName(it->first));
				newOp->getSignalByName(it->first)->addSuccessor(it->second);

				it->second->setCriticalPathContribution(0.0);

				//add the newly created signal to the list of signals to schedule
				it->second->setHasBeenScheduled(false);
				signalsToSchedule.push_back(it->second);
			}
		}

		//reconnect the inputs/outputs to the corresponding internal signals
		for(unsigned int i=0; i<newOp->ioList_.size(); i++)
		{
			vector<pair<Signal*, int>> newPredecessors, newSuccessors;
			Signal *originalSignal = originalOperator->getSignalByName(newOp->ioList_[i]->getName());

			//create the new list of predecessors for the signal currently processed
			//	only connect to internal signals
			for(unsigned int j=0; j<originalSignal->predecessors()->size(); j++)
			{
				pair<Signal*, int> tmpPair = *(originalSignal->predecessorPair(j));

				//only connect to internal signals
				if(tmpPair.first->parentOp()->getName() != originalSignal->parentOp()->getName())
					continue;

				//signals connected only to constants are already scheduled
				if(((tmpPair.first->type() == Signal::constant) || (tmpPair.first->type() == Signal::constantWithDeclaration))
						&& (originalSignal->predecessors()->size() == 1))
				{
					signalList_[i]->setCycle(0);
					signalList_[i]->setCriticalPath(0.0);
					signalList_[i]->setCriticalPathContribution(0.0);
					signalList_[i]->setHasBeenScheduled(true);
				}

				newOp->ioList_[i]->addPredecessor(newOp->getSignalByName(tmpPair.first->getName()), tmpPair.second);
				newOp->getSignalByName(tmpPair.first->getName())->addSuccessor(newOp->ioList_[i], tmpPair.second);
			}

			//create the new list of successors for the signal currently processed
			//	only connect to internal signals
			for(unsigned int j=0; j<originalSignal->successors()->size(); j++)
			{
				pair<Signal*, int> tmpPair = *(originalSignal->successorPair(j));

				//only connect to internal signals
				if(tmpPair.first->parentOp()->getName() != originalSignal->parentOp()->getName())
					continue;

				newOp->ioList_[i]->addSuccessor(newOp->getSignalByName(tmpPair.first->getName()), tmpPair.second);
				newOp->getSignalByName(tmpPair.first->getName())->addPredecessor(newOp->ioList_[i], tmpPair.second);
			}
		}

		//set a new name for the copy of the operator
		newOp->setName(newOp->getName() + "_copy_" + vhdlize(getNewUId()));

		//mark the subcomponent as drawn
		newOp->setIsOperatorDrawn(true);

		return newOp;
	}


	void Operator::scheduleSharedInstance(Operator *op, Operator *originalOperator, bool forceReschedule)
	{
		//TODO: for debug purposes
		string opName = op->getName();

		int maxCycle = 0;

		//first, do an initial schedule of the inputs, if they haven't been scheduled
		for(auto i : op->ioList_)
			if((i->type() == Signal::in) && (i->getHasBeenScheduled() == false))
				setSignalTiming(i, true);

		//determine the maximum input cycle
		for(unsigned int i=0; i<op->getIOList()->size(); i++)
			if((op->getIOListSignal(i)->type() == Signal::in)
					&& (op->getIOListSignal(i)->getCycle() > maxCycle))
				maxCycle = op->getIOListSignal(i)->getCycle();

		//set all the inputs to the maximum cycle
		for(unsigned int i=0; i<op->getIOList()->size(); i++)
			if((op->getIOListSignal(i)->type() == Signal::in) &&
					(op->getIOListSignal(i)->getCycle() != maxCycle))
			{
				//if we have to delay the input, then we need to reset the critical path, as well
				Signal *inputSignal = op->getIOListSignal(i);

				inputSignal->setCycle(maxCycle);
				//inputSignal->setCriticalPath(0.0);
				inputSignal->setCriticalPath(op->getTarget()->ffDelay());
				inputSignal->setHasBeenScheduled(true);

				//update the lifespan of inputSignal's predecessors
				for(unsigned int i=0; i<inputSignal->predecessors()->size(); i++)
				{
					//predecessor signals that belong to a subcomponent do not need to have their lifespan affected
					if((inputSignal->parentOp()->getName() != inputSignal->predecessor(i)->parentOp()->getName()) &&
							(inputSignal->predecessor(i)->type() == Signal::out))
						continue;
					inputSignal->predecessor(i)->updateLifeSpan(
							inputSignal->getCycle() - inputSignal->predecessor(i)->getCycle());
				}
			}

		//	schedule the operator
		//		in the new schedule signals must be distributed into cycles
		//		in the same way as in the original operator
		bool rescheduleNeeded = true;
		bool firstSchedulePass = true;

		while(rescheduleNeeded == true)
		{
			//TODO: rescheduling the inputs is not needed, as they have just been synchronized to the same cycle
			//first, do an initial schedule of the circuit
			//	schedule the inputs, if they haven't been scheduled
			for(auto i : op->ioList_)
				if((i->type() == Signal::in) && (i->getHasBeenScheduled() == false))
					setSignalTiming(i, true);
			//	schedule the successors of the inputs
			for(auto i : op->ioList_)
				if(i->type() == Signal::in)
					for(auto j : *i->successors())
						scheduleSignal(j.first, forceReschedule);

			//if this is the second pass, then there is nothing else to be done
			if(!firstSchedulePass)
				break;

			//check if the schedule of the new instance matches with
			//	the schedule of the original operator
			rescheduleNeeded = false;
			for(auto i : op->signalList_)
			{
				if(i->getCycle()-maxCycle
						!= originalOperator->getSignalByName(i->getName())->getCycle())
				{
					rescheduleNeeded = true;
					firstSchedulePass = false;
					break;
				}
			}
			for(auto i : op->ioList_)
			{
				if((i->type() == Signal::out) &&
						(i->getCycle()-maxCycle
								!= originalOperator->getSignalByName(i->getName())->getCycle()))
				{
					rescheduleNeeded = true;
					firstSchedulePass = false;
					break;
				}
			}

			//if the schedule was not successful, then advance the inputs of the new instance
			if(rescheduleNeeded == true)
			{
				for(auto i : op->ioList_)
					if(i->type() == Signal::in)
					{
						//if we have to delay the input, the we need to reset the critical path, as well
						i->setCycle(i->getCycle() + 1);
						//i->setCriticalPath(0.0);
						i->setCriticalPath(op->getTarget()->ffDelay());

						//update the lifespan of inputSignal's predecessors
						for(auto j : *i->predecessors())
						{
							//predecessor signals that belong to a subcomponent do not need to have their lifespan affected
							if((i->parentOp()->getName() != j.first->parentOp()->getName()) &&
									(i->type() == Signal::out))
								continue;
							j.first->updateLifeSpan(i->getCycle() - j.first->getCycle());
						}
					}
			}
		}

		//op->setIsOperatorImplemented(true);
	}


	OperatorPtr Operator::newInstance(string opName, string instanceName, string parameters, string inPortMaps, string outPortMaps, string inPortMapsCst)
	{
		OperatorFactoryPtr instanceOpFactory = UserInterface::getFactoryByName(opName);
		OperatorPtr instance = nullptr;
		vector<string> parametersVector;
		vector<Signal*>::iterator itSignal;
		string portName, signalName, mapping;

		REPORT(DEBUG, "entering newInstance("<< opName << ", " << instanceName <<")" );
		//parse the parameters
		parametersVector.push_back(opName);
		while(!parameters.empty())
		{
			if(parameters.find(" ") != string::npos){
				parametersVector.push_back(parameters.substr(0,parameters.find(" ")));
				parameters.erase(0,parameters.find(" ")+1);
			}else{
				parametersVector.push_back(parameters);
				parameters.erase();
			}
		}

		//parse the input port mappings
		parsePortMappings(instance, inPortMaps, 0);
		//parse the constant input port mappings, if there are any
		parsePortMappings(instance, inPortMapsCst, 1);
		//parse the input port mappings
		parsePortMappings(instance, outPortMaps, 2);
		REPORT(DEBUG, "   newInstance("<< opName << ", " << instanceName <<"): after parsePortMapping" );
		for (auto i: parametersVector){
			REPORT(DEBUG, i);
		}
			
		//create the operator
		instance = instanceOpFactory->parseArguments(this, target_, parametersVector);

		REPORT(DEBUG, "   newInstance("<< opName << ", " << instanceName <<"): after factory call" );

		//create the instance
		vhdl << this->instance(instance, instanceName);
		REPORT(DEBUG, "   newInstance("<< opName << ", " << instanceName <<"): after instance()" );

		return instance;
	}


	void Operator::parsePortMappings(OperatorPtr instance, string portMappings0, int portTypes)
	{
		string portMappings="";
		// First remove any space
		for (unsigned int i=0; i<portMappings0.size(); i++) {
			if((portMappings0[i] != ' ') && (portMappings0[i] != '\t')) {
				portMappings += portMappings0[i];
			}
		}
		if(portMappings0!="") {
			// First tokenize using stack overflow code
			std::vector<std::string> tokens;
			std::size_t start = 0, end = 0;
			while ((end = portMappings.find(',', start)) != std::string::npos) {
			tokens.push_back(portMappings.substr(start, end - start));
			start = end + 1;
			}
			tokens.push_back(portMappings.substr(start));
			// now iterate over the found token, each of which should be a port map of syntax "X=>Y"
			for(auto& mapping: tokens) {
				size_t sepPos = mapping.find("=>");
				if(sepPos == string::npos)
					THROWERROR("Error: in newInstance: these port maps are not specified correctly: <" << portMappings<<">");
				string portName = mapping.substr(0, sepPos);
				string signalName = mapping.substr(sepPos+2, mapping.size()-sepPos-2);
				REPORT(4, "port map " << portName << "=>" << signalName << " of type " << portTypes);
				if(portTypes == 0)
					inPortMap(instance, portName, signalName);
				else if(portTypes == 1)
					inPortMapCst(instance, portName, signalName);
				else
					outPortMap(instance, portName, signalName);
			}
		}
	}



	string Operator::buildVHDLSignalDeclarations() {
		ostringstream o;

		for(unsigned int i=0; i<signalList_.size(); i++) {
			//constant signals don't need a declaration
			//inputs/outputs treated separately
			if((signalList_[i]->type() == Signal::constant) ||
				(signalList_[i]->type() == Signal::in) || (signalList_[i]->type() == Signal::out))
				continue;

			o << signalList_[i]->toVHDLDeclaration() << endl;
		}
		//now the signals from the I/O List which have the cycle>0
		for(unsigned int i=0; i<ioList_.size(); i++) {
			if(ioList_[i]->getLifeSpan()>0){
				o << ioList_[i]->toVHDLDeclaration() << endl;
			}

		}

		return o.str();
	}


	void Operator::useHardRAM(Operator* t) {
		if (target_->getVendor() == "Xilinx")
		{
			addAttribute("rom_extract", "string", t->getName()+": component", "yes");
			addAttribute("rom_style", "string", t->getName()+": component", "block");
		}
		if (target_->getVendor() == "Altera")
			addAttribute("altera_attribute", "string", t->getName()+": component", "-name ALLOW_ANY_ROM_SIZE_FOR_RECOGNITION ON");
	}

	void Operator::useSoftRAM(Operator* t) {
		if (target_->getVendor() == "Xilinx")
		{
			addAttribute("rom_extract", "string", t->getName()+": component", "yes");
			addAttribute("rom_style", "string", t->getName()+": component", "distributed");
		}
		if (target_->getVendor() == "Altera")
			addAttribute("altera_attribute", "string", t->getName()+": component", "-name ALLOW_ANY_ROM_SIZE_FOR_RECOGNITION OFF");
	}


	void Operator::setArchitectureName(string architectureName) {
		architectureName_ = architectureName;
	};


	void Operator::newArchitecture(std::ostream& o, std::string name){
		o << "architecture " << architectureName_ << " of " << name  << " is" << endl;
	}


	void Operator::beginArchitecture(std::ostream& o){
		o << "begin" << endl;
	}


	void Operator::endArchitecture(std::ostream& o){
		o << "end architecture;" << endl << endl;
	}


	string Operator::buildVHDLComponentDeclarations() {
		ostringstream o;

		for(unsigned int i=0; i<subComponentList_.size(); i++)
		{
			//if this is a global operator, then it should be output only once,
			//	and with the name of the global copy, not that of the local copy
			string componentName = subComponentList_[i]->getName();

			if(componentName.find("_copy_") != string::npos)
			{
				//a global component; only the first copy should be output
				bool isComponentOutput = false;

				componentName = componentName.substr(0, componentName.find("_copy_"));

				for(unsigned int j=0; j<i; j++)
					if(subComponentList_[j]->getName().find(componentName) != string::npos)
					{
						isComponentOutput = true;
						break;
					}
				if(isComponentOutput)
					continue;
				else
				{
					subComponentList_[i]->outputVHDLComponent(o, componentName);
					o << endl;
				}
			}else
			{
				//just a regular subcomponent
				subComponentList_[i]->outputVHDLComponent(o, componentName);
				o << endl;
			}
		}

		return o.str();
	}


	void Operator::addConstant(std::string name, std::string t, mpz_class v) {
		ostringstream tmp;
		tmp << v;
		constants_[name] =  make_pair(t, tmp.str());
	}

	void Operator::addConstant(std::string name, std::string t, int v) {
		ostringstream tmp;
		tmp << v;
		constants_[name] =  make_pair(t, tmp.str());
	}

	void Operator::addConstant(std::string name, std::string t, string v) {
		constants_[name] =  make_pair(t, v);
	}

	void Operator::addType(std::string name, std::string value) {
		types_ [name] =  value;
	}


	void Operator::addAttribute(std::string attributeName,  std::string attributeType,  std::string object, std::string value, bool addSignal ) {
		// TODO add some checks ?
		attributes_[attributeName] = attributeType;
		pair<string,string> p = make_pair(attributeName,object);
		attributesValues_[p] = value;
		attributesAddSignal_[attributeName] = addSignal; 
	}


	string Operator::buildVHDLTypeDeclarations() {
		ostringstream o;
		string name, value;
		for(map<string, string >::iterator it = types_.begin(); it !=types_.end(); it++) {
			name  = it->first;
			value = it->second;
			o <<  "type " << name << " is "  << value << ";" << endl;
		}
		return o.str();
	}


	string Operator::buildVHDLConstantDeclarations() {
		ostringstream o;
		string name, type, value;
		for(map<string, pair<string, string> >::iterator it = constants_.begin(); it !=constants_.end(); it++) {
			name  = it->first;
			type = it->second.first;
			value = it->second.second;
			o <<  "constant " << name << ": " << type << " := " << value << ";" << endl;
		}
		return o.str();
	}



	string Operator::buildVHDLAttributes() {
		ostringstream o;
		// First add the declarations of attribute names
		for(map<string, string>::iterator it = attributes_.begin(); it !=attributes_.end(); it++) {
			string name  = it->first;
			string type = it->second;
			o <<  "attribute " << name << ": " << type << ";" << endl;
		}
		// Then add the declarations of attribute values
		for(map<pair<string, string>, string>::iterator it = attributesValues_.begin(); it !=attributesValues_.end(); it++) {
			string name  = it->first.first;
			string object = it->first.second;
			string value = it->second;
			if(attributes_[name]=="string")
				value = '"' + value + '"';
			o <<  "attribute " << name << " of " << object << (attributesAddSignal_[name]?" : signal " : "") << " is " << value << ";" << endl;
		}
		return o.str();
	}




	string  Operator::buildVHDLRegisters() {
		ostringstream o;

		// execute only if the operator is sequential, otherwise output nothing
		string recTab = "";
		if ( isRecirculatory() || hasClockEnable() )
			recTab = tab;
		if (isSequential()){
			// First registers without reset
			o << tab << "process(clk)" << endl;
			o << tab << tab << "begin" << endl;
			o << tab << tab << tab << "if clk'event and clk = '1' then" << endl;
			if (isRecirculatory())
				o << tab << tab << tab << tab << "if stall_s = '0' then" << endl;
			else if (hasClockEnable())
				o << tab << tab << tab << tab << "if ce = '1' then" << endl;
			for(unsigned int i=0; i<signalList_.size(); i++) {
				Signal *s = signalList_[i];
				if ((s->type() == Signal::registeredWithoutReset) || (s->type()==Signal::registeredWithZeroInitialiser)
						|| (s->type() == Signal::wire) || (s->type() == Signal::table) || (s->type() == Signal::constantWithDeclaration))
					if(s->getLifeSpan() > 0) {
						for(int j=1; j <= s->getLifeSpan(); j++)
							o << recTab << tab << tab <<tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
					}
			}
			for(unsigned int i=0; i<ioList_.size(); i++) {
				Signal *s = ioList_[i];
				if(s->getLifeSpan() >0) {
					for(int j=1; j <= s->getLifeSpan(); j++)
						o << recTab << tab << tab <<tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
				}
			}
			if (isRecirculatory() || hasClockEnable())
				o << tab << tab << tab << tab << "end if;" << endl;
			o << tab << tab << tab << "end if;\n";
			o << tab << tab << "end process;\n";

			// then registers with asynchronous reset
			if (hasRegistersWithAsyncReset_) {
				o << tab << "process(clk, rst)" << endl;
				o << tab << tab << "begin" << endl;
				o << tab << tab << tab << "if rst = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithAsyncReset)
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++){
								if ( (s->width()>1) || (s->isBus()))
									o << tab << tab <<tab << tab << s->delayedName(j) << " <=  (others => '0');" << endl;
								else
									o << tab <<tab << tab << tab << s->delayedName(j) << " <=  '0';" << endl;
							}
						}
				}
				o << tab << tab << tab << "elsif clk'event and clk = '1' then" << endl;
				if (isRecirculatory()) o << tab << tab << tab << tab << "if stall_s = '0' then" << endl;
				else if (hasClockEnable()) o << tab << tab << tab << tab << "if ce = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithAsyncReset)
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++)
								o << recTab << tab <<tab << tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
						}
				}
			if (isRecirculatory() || hasClockEnable())
				o << tab << tab << tab << tab << "end if;" << endl;
				o << tab << tab << tab << "end if;" << endl;
				o << tab << tab <<"end process;" << endl;
			}

			// then registers with synchronous reset
			if (hasRegistersWithSyncReset_) {
				o << tab << "process(clk, rst)" << endl;
				o << tab << tab << "begin" << endl;
				o << tab << tab << tab << "if clk'event and clk = '1' then" << endl;
				o << tab << tab << tab << tab << "if rst = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithSyncReset)
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++){
								if ( (s->width()>1) || (s->isBus()))
									o << tab <<tab << tab <<tab << tab << s->delayedName(j) << " <=  (others => '0');" << endl;
								else
									o << tab <<tab << tab <<tab << tab << s->delayedName(j) << " <=  '0';" << endl;
							}
						}
				}
				o << tab << tab << tab << tab << "else" << endl;
				if (isRecirculatory()) o << tab << tab << tab << tab << "if stall_s = '0' then" << endl;
				else if (hasClockEnable()) o << tab << tab << tab << tab << "if ce = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithSyncReset)
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++)
								o << tab <<tab << tab <<tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
						}
				}
				if (isRecirculatory() || hasClockEnable())
					o << tab << tab << tab << tab << "end if;" << endl;
				o << tab << tab << tab << tab << "end if;" << endl;
				o << tab << tab << tab << "end if;" << endl;
				o << tab << tab << "end process;" << endl;
			}
		}
		return o.str();
	}


	void Operator::outputClock_xdc(){
		ofstream file; 

		// For Vivado
		file.open("/tmp/clock.xdc", ios::out);
		file << "# This file was created by FloPoCo to be used by the vivado_runsyn utility. Sorry to clutter your tmp." << endl;
		file << "create_clock -name clk -period "  << (1.0e9/target_->frequency())
			// << "  [get_ports clk]"
				 << endl;
		for(auto i: ioList_) {
			if(i->type()==Signal::in)
				file << "set_input_delay ";
			else // should be output
				file << "set_output_delay ";
			file <<	"-clock clk 0 [get_ports " << i->getName() << "]" << endl;
		}
		file.close();

#if 0
		// For quartus prime	-- no longer needed as quartus_runsyn reads the frequency from the comments in the VHDL and create this file.
		file.open("/tmp/"+getName()+".sdc", ios::out);
		file << "# This file was created by FloPoCo to be used by the quartus_runsyn utility. Sorry to clutter your tmp." << endl;
		file << "create_clock -name clk -period "  << (1.0e9/target_->frequency()) << "  [get_ports clk]"
				 << endl;
		file.close();
#endif
	}

	
	void Operator::buildStandardTestCases(TestCaseList* tcl) {
		// Each operator should overload this method. If not, it is mostly harmless but deserves a warning.
		cerr << "WARNING: No standard test cases implemented for this operator" << endl;
	}




	void Operator::buildRandomTestCaseList(TestCaseList* tcl, int n){
		TestCase *tc;
		// Generate test cases using random input numbers
		for (int i = 0; i < n; i++) {
			// TODO free all this memory when exiting TestBench
			tc = buildRandomTestCase(i);
			tcl->add(tc);
		}
	}

	TestCase* Operator::buildRandomTestCase(int i){
		TestCase *tc = new TestCase(this);
		// Generate test cases using random input numbers */
		// TODO free all this memory when exiting TestBench
		// Fill inputs
		for (unsigned int j = 0; j < ioList_.size(); j++) {
			Signal* s = ioList_[j];
			if(s->type() == Signal::in){
				mpz_class a = getLargeRandom(s->width());
				tc->addInput(s->getName(), a);
			}
		}
		// Get correct outputs
		emulate(tc);

		// add to the test case list
		return tc;
	}

	map<string, double> Operator::getInputDelayMap(){
		map<string, double> inputDelayMap;

		 REPORT(INFO, "WARNING: getInputDelayMap() no longer has the same meaning, due to the overhaul of the pipeline framework;" << endl
						<< tab << "the delay map for the instance being built will be returned");
		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++)
			inputDelayMap[it->first] = it->second->getCriticalPath();

		return inputDelayMap;
	}

	map<string, double> Operator::getOutDelayMap(){
		map<string, double> outputDelayMap;

		REPORT(INFO, "WARNING: getOutDelayMap() no longer has the same meaning, due to the overhaul of the pipeline framework;" << endl
					 << tab << "the delay map for the instance being built will be returned");
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
			outputDelayMap[it->first] = it->second->getCriticalPath();

		return outputDelayMap;
	}

	map<string, int> Operator::getDeclareTable(){
		REPORT(INFO, "WARNING: function getDeclareTable() is deprecated and no longer has any effect!" << endl
					 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
					 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!");

		map<string, int> emptyMap;

		return emptyMap;
	}

	Target* Operator::getTarget(){
		return target_;
	}

	OperatorPtr Operator::getParentOp(){
		return parentOp_;
	}

	string Operator::getUniqueName(){
		return uniqueName_;
	}

	string Operator::getArchitectureName(){
		return architectureName_;
	}

	vector<Signal*> Operator::getTestCaseSignals(){
		return testCaseSignals_;
	}

	map<string, string> Operator::getPortMap(){
		cerr << "WARNING: function getDeclareTable() is deprecated!" << endl
				<< tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
				<< tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
		map<string, string> tmpMap;

		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++)
			tmpMap[it->first] = it->second->getName();
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
			tmpMap[it->first] = it->second->getName();

		return tmpMap;
	}


	string Operator::getSrcFileName(){
		return srcFileName;
	}

	int Operator::getOperatorCost(){
		return cost;
	}

	int Operator::getNumberOfInputs(){
		return numberOfInputs_;
	}

	int Operator::getNumberOfOutputs(){
		return numberOfOutputs_;
	}

	map<string, Signal*> Operator::getSignalMap(){
		return signalMap_;
	}

	map<string, pair<string, string> > Operator::getConstants(){
		return constants_;
	}

	map<string, string> Operator::getAttributes(){
		return attributes_;
	}

	map<string, string> Operator::getTypes(){
		return types_;
	}

	map<pair<string,string>, string> Operator::getAttributesValues(){
		return attributesValues_;
	}

	bool Operator::getHasRegistersWithoutReset(){
		return hasRegistersWithoutReset_;
	}

	bool Operator::getHasRegistersWithAsyncReset(){
		return hasRegistersWithAsyncReset_;
	}

	bool Operator::getHasRegistersWithSyncReset(){
		return hasRegistersWithSyncReset_;
	}

	bool Operator::hasReset() {
		return hasRegistersWithSyncReset_ || hasRegistersWithAsyncReset_;
	}

	bool Operator::hasClockEnable(){
		return hasClockEnable_;
	}

	void Operator::setClockEnable(bool val){
		hasClockEnable_=val;
	}

	string Operator::getCopyrightString(){
		return copyrightString_;
	}

	bool Operator::getNeedRecirculationSignal(){
		return needRecirculationSignal_;
	}

	Operator* Operator::getIndirectOperator(){
		return indirectOperator_;
	}

	vector<Operator*> Operator::getSubComponentList(){
		return subComponentList_;
	}

	vector<Operator*>& Operator::getSubComponentListR(){
		return subComponentList_;
	}

	FlopocoStream* Operator::getFlopocoVHDLStream(){
		return &vhdl;
	}

	void Operator::outputVHDL(std::ostream& o, std::string name) {

		//safety checks
		//	if this is a copy of a global operator, then its code doesn't need to
		//	be added to the file, as the global copy is already there
		if(isShared()
				&& (getName().find("_copy_") != string::npos))
		{
			return;
		}

		if (! vhdl.isEmpty() ){
			licence(o);
			pipelineInfo(o);
			stdLibs(o);
			outputVHDLEntity(o);
			newArchitecture(o,name);
			o << buildVHDLComponentDeclarations();
			o << buildVHDLAttributes();
			o << buildVHDLSignalDeclarations();			//TODO: this cannot be called before scheduling the signals (it requires the lifespan of the signals, which is not yet computed)
			o << buildVHDLTypeDeclarations();
			o << buildVHDLConstantDeclarations();
			beginArchitecture(o);
			o << buildVHDLRegisters();					//TODO: this cannot be called before scheduling the signals (it requires the lifespan of the signals, which is not yet computed)
			if(getIndirectOperator())
				o << getIndirectOperator()->vhdl.str();
			else
				o << vhdl.str();
			endArchitecture(o);
		}
	}

	//TODO: this function should not be called before the operator's signals are scheduled
	void Operator::parse2()
	{
		ostringstream newStr;
		string oldStr, workStr;
		size_t currentPos, nextPos, tmpCurrentPos, tmpNextPos;
		int count, lhsNameLength, rhsNameLength;
		bool unknownLHSName = false, unknownRHSName = false;

		REPORT(DEBUG, "Starting second-level parsing for operator " << srcFileName);
		REPORT(FULL, "vhdl stream before parse2(): " << endl << vhdl.str());
		//reset the new vhdl code buffer
		newStr.str("");

		//set the old code to the vhdl code stored in the FlopocoStream
		//	this also triggers the code's parsing, if necessary
		oldStr = vhdl.str();

		//iterate through the old code, one statement at the time
		// code that doesn't need to be modified: goes directly to the new vhdl code buffer
		// code that needs to be modified: ?? should be removed from lhs_name, $$ should be removed from rhs_name,
		//		delays of the type rhs_name_dxxx should be added for the right-hand side signals
		bool isSelectedAssignment = (oldStr.find('?') > oldStr.find('$'));
		currentPos = 0;
		nextPos = (isSelectedAssignment ? oldStr.find('$') : oldStr.find('?'));
		while(nextPos !=  string::npos)
		{
			string lhsName, rhsName;
			size_t auxPosition, auxPosition2;
			Signal *lhsSignal, *rhsSignal;

			unknownLHSName = false;
			unknownRHSName = false;

			//copy the code from the beginning to this position directly to the new vhdl buffer
			newStr << oldStr.substr(currentPos, nextPos-currentPos);

			//now get a new line to parse
			workStr = oldStr.substr(nextPos+2, oldStr.find(';', nextPos)+1-nextPos-2);

			//REPORT(FULL, "processing " << workStr);
			//extract the lhs_name
			if(isSelectedAssignment == true)
			{
				int lhsNameStart, lhsNameStop;

				lhsNameStart = workStr.find('?');
				lhsNameStop  = workStr.find('?', lhsNameStart+2);
				lhsName = workStr.substr(lhsNameStart+2, lhsNameStop-lhsNameStart-2);

				auxPosition = lhsNameStop+2;
			}else
			{
				lhsName = workStr.substr(0, workStr.find('?'));

				//copy lhsName to the new vhdl buffer
				newStr << lhsName;

				auxPosition = lhsName.size()+2;
			}
			lhsNameLength = lhsName.size();

			//check for component instances
			//	the first parse marks the name of the component as a lhsName
			//	must remove the markings
			//the rest of the code contains pairs of ??lhsName?? => $$rhsName$$ pairs
			//	for which the helper signals must be removed and delays _dxxx must be added
			auxPosition2 = workStr.find("port map"); // This is OK because this string can only be created by flopoco
			if(auxPosition2 != string::npos)	{
				//try to parse the names of the signals in the port mapping
				if(workStr.find("?", auxPosition2) == string::npos) {
					//empty port mapping
					newStr << workStr.substr(auxPosition, workStr.size());
				}
				else 	{
					//parse a list of ??lhsName?? => $$rhsName$$
					//	or ??lhsName?? => 'x' or ??lhsName?? => "xxxx"
					size_t tmpCurrentPos, tmpNextPos;

					//copy the code up to the port mappings
					newStr << workStr.substr(auxPosition, workStr.find("?", auxPosition2)-auxPosition);

					tmpCurrentPos = workStr.find("?", auxPosition2);
					while(tmpCurrentPos != string::npos)
					{
						bool singleQuoteSep = false, doubleQuoteSep = false;

						unknownLHSName = false;
						unknownRHSName = false;

						//extract a lhsName
						tmpNextPos = workStr.find("?", tmpCurrentPos+2);
						lhsName = workStr.substr(tmpCurrentPos+2, tmpNextPos-tmpCurrentPos-2);

						//copy lhsName to the new vhdl buffer
						newStr << lhsName;

						//copy the code up to the next rhsName to the new vhdl buffer
						tmpCurrentPos = tmpNextPos+2;
						tmpNextPos = workStr.find("$", tmpCurrentPos);

						//check for constant as rhs name
						if(workStr.find("\'", tmpCurrentPos) < tmpNextPos)
						{
							tmpNextPos = workStr.find("\'", tmpCurrentPos);
							singleQuoteSep = true;
						}else if(workStr.find("\"", tmpCurrentPos) < tmpNextPos)
						{
							tmpNextPos = workStr.find("\"", tmpCurrentPos);
							doubleQuoteSep = true;
						}

						newStr << workStr.substr(tmpCurrentPos, tmpNextPos-tmpCurrentPos);

						//extract a rhsName
						//	this might be a constant
						if(singleQuoteSep)
						{
							//a 1-bit constant
							tmpCurrentPos = tmpNextPos+1;
							tmpNextPos = workStr.find("\'", tmpCurrentPos);
						}else if(doubleQuoteSep)
						{
							//a multiple bit constant
							tmpCurrentPos = tmpNextPos+1;
							tmpNextPos = workStr.find("\"", tmpCurrentPos);
						}else
						{
							//a regular signal name
							tmpCurrentPos = tmpNextPos+2;
							tmpNextPos = workStr.find("$", tmpCurrentPos);
						}
						rhsName = workStr.substr(tmpCurrentPos, tmpNextPos-tmpCurrentPos);

						rhsSignal = NULL;
						lhsSignal = NULL;

						//copy rhsName to the new vhdl buffer
						//	annotate it if necessary
						if((lhsName != "clk") && (lhsName != "rst") && !(singleQuoteSep || doubleQuoteSep))
						{
							//obtain the rhs signal
							//	this might be an undeclared name (e.g. library function, constant etc.)
							try
							{
							    rhsSignal = getSignalByName(rhsName);

							    //obtain the lhs signal, from the list of successors of the rhs signal
							    //	if lhs is an input
							    //if lhs is an output, look on the predecessors list
							    for(size_t i=0; i<rhsSignal->successors()->size(); i++)
							    	if((rhsSignal->successor(i)->getName() == lhsName)
							    			//the lhs signal must be the input of a subcomponent
							    			&& (rhsSignal->parentOp()->getName() != rhsSignal->successor(i)->parentOp()->getName()))
							    	{
							    		lhsSignal = rhsSignal->successor(i);
							    		break;
							    	}
							    //if the first lookup did not succeed, then lhs is an output,
							    //	so look for it in the predecessor list of rhs
							    if(lhsSignal == NULL)
							    {
							    	for(size_t i=0; i<rhsSignal->predecessors()->size(); i++)
							    		if((rhsSignal->predecessor(i)->getName() == lhsName)
							    				//the lhs signal must be the input of a subcomponent
							    				&& (rhsSignal->parentOp()->getName() != rhsSignal->predecessor(i)->parentOp()->getName()))
							    		{
							    			lhsSignal = rhsSignal->predecessor(i);
							    			break;
							    		}
							    }
							}catch(string &e)
							{
							    //this might be an undeclared rhs name
							    rhsSignal = NULL;
							    unknownRHSName = true;

							    //rhs name not found, so no tests done on lhs either
							    lhsSignal = NULL;
							    unknownLHSName = true;
							}

							newStr << rhsName;
							//output signals do not need to be delayed
							if(lhsSignal->type() != Signal::out)
							{
								if(getTarget()->isPipelined() && !unknownLHSName
										&& (lhsSignal->getCycle()-rhsSignal->getCycle()+getFunctionalDelay(rhsSignal, lhsSignal) > 0))
								{
								  newStr << "_d" << vhdlize(lhsSignal->getCycle()-rhsSignal->getCycle()+getFunctionalDelay(rhsSignal, lhsSignal));
								}
							}
						} else
						{
							//this signal is clk, rst or a constant
							newStr << (singleQuoteSep ? "\'" : doubleQuoteSep ? "\"" : "")
							    << rhsName << (singleQuoteSep ? "\'" : doubleQuoteSep ? "\"" : "");
						}

						//prepare to parse a new pair
						tmpCurrentPos = workStr.find("?", tmpNextPos+2);

						//copy the rest of the code to the new vhdl buffer
						if(tmpCurrentPos != string::npos)
							newStr << workStr.substr(tmpNextPos+(singleQuoteSep||doubleQuoteSep ? 1 : 2),
									tmpCurrentPos-tmpNextPos-(singleQuoteSep||doubleQuoteSep ? 1 : 2));
						else
							newStr << workStr.substr(tmpNextPos+(singleQuoteSep||doubleQuoteSep ? 1 : 2), workStr.size());
					}
				}

				//prepare for a new instruction to be parsed
				currentPos = nextPos + workStr.size() + 2;
				nextPos = oldStr.find('?', currentPos);
				//special case for the selected assignment statements
				isSelectedAssignment = false;
				if(oldStr.find('$', currentPos) < nextPos)
				{
					nextPos = oldStr.find('$', currentPos);
					isSelectedAssignment = true;
				}

				continue;
			}

			//the lhsName could be of the form (lhsName1, lhsName2, ...)
			//	extract the first name in the list
			if(lhsName[0] == '(')
			{
				count = 1;
				while((lhsName[count] != ' ') && (lhsName[count] != '\t')
						&& (lhsName[count] != ',') && (lhsName[count] != ')'))
					count++;
				lhsName = lhsName.substr(1, count-1);
			}

			//this could be a user-defined name
			try
			{
			    lhsSignal = getSignalByName(lhsName);
			}catch(string &e)
			{
			    lhsSignal = NULL;
			    unknownLHSName = true;
			}

			//check for selected signal assignments
			//	with signal_name select... etc
			//	the problem is there is a signal belonging to the right hand side
			//	on the left-hand side, before the left hand side signal, which breaks the regular flow
			if(isSelectedAssignment == true)
			{
				//extract the first rhs signal name
				tmpCurrentPos = 0;
				tmpNextPos = workStr.find('$');

				rhsName = workStr.substr(tmpCurrentPos, tmpNextPos);

				//remove the possible parentheses around the rhsName
				string newRhsName = rhsName;
				if(rhsName.find("(") != string::npos)
				{
					newRhsName = newRhsName.substr(rhsName.find("(")+1, rhsName.find(")")-rhsName.find("(")-1);
				}
				//this could also be a delayed signal name
				try{
					rhsSignal = getSignalByName(newRhsName);
				}catch(string &e){
					try{
						rhsSignal = getDelayedSignalByName(newRhsName);
						//extract the name
						rhsName = rhsName.substr(0, newRhsName.find('^'));
					}catch(string e2){
					    //THROWERROR("Error in parse2(): signal " << newRhsName << " not found:" << e2);
					    //this is a user-defined name
					    rhsSignal = NULL;
					    unknownRHSName = true;
					}
				}catch(...){
					//THROWERROR("Error in parse2(): signal " << newRhsName << " not found:");
					//this is a user-defined name
					rhsSignal = NULL;
					unknownRHSName = true;
				}

				//output the rhs signal name
				newStr << rhsName;
				if(getTarget()->isPipelined() && !unknownLHSName  && !unknownRHSName
						&& (lhsSignal->getCycle()-rhsSignal->getCycle()+getFunctionalDelay(rhsSignal, lhsSignal) > 0))
					newStr << "_d" << vhdlize(lhsSignal->getCycle()-rhsSignal->getCycle()+getFunctionalDelay(rhsSignal, lhsSignal));

				//copy the code up until the lhs signal name
				tmpCurrentPos = tmpNextPos+2;
				tmpNextPos = workStr.find('?', tmpCurrentPos);

				newStr << workStr.substr(tmpCurrentPos, tmpNextPos-tmpCurrentPos);

				//copy the lhs signal name
				newStr << lhsName;

				//prepare to parse a new rhs name
				tmpCurrentPos = tmpNextPos+4+lhsNameLength;
				tmpNextPos = workStr.find('$', tmpCurrentPos);

				//if there are is more code that needs preparing, then just pass
				//to the next instruction
				if(tmpNextPos == string::npos)
				{
					//copy the rest of the code
					newStr << workStr.substr(tmpCurrentPos, workStr.size()-tmpCurrentPos);

					//prepare for a new instruction to be parsed
					currentPos = nextPos + workStr.size() + 2;
					nextPos = oldStr.find('?', currentPos);
					//special case for the selected assignment statements
					isSelectedAssignment = false;
					if(oldStr.find('$', currentPos) < nextPos)
					{
						nextPos = oldStr.find('$', currentPos);
						isSelectedAssignment = true;
					}

					continue;
				}
			}

			//extract the rhsNames and annotate them find the position of the rhsName, and copy
			// the vhdl code up to the rhsName in the new vhdl code buffer
			// There was a bug here  if one variable is had select in its name
			// Took me 2hours to figure out
			// Bug fixed by having the lexer add spaces around select (so no need to test for all the space/tab/enter possibilibits.
			// I wonder how many such bugs remain
			if(workStr.find(" select ") == string::npos	 )			{ 
				tmpCurrentPos = lhsNameLength+2;
				tmpNextPos = workStr.find('$', lhsNameLength+2);
			}
			while(tmpNextPos != string::npos)
			{
				int cycleDelay;

				unknownRHSName = false;

				//copy into the new vhdl stream the code that doesn't need to be modified
				newStr << workStr.substr(tmpCurrentPos, tmpNextPos-tmpCurrentPos);

				//skip the '$$'
				tmpNextPos += 2;

				//extract a new rhsName
				rhsName = workStr.substr(tmpNextPos, workStr.find('$', tmpNextPos)-tmpNextPos);
				rhsNameLength = rhsName.size();
				//remove the possible parentheses around the rhsName
				string newRhsName = rhsName;
				if(rhsName.find("(") != string::npos)
				{
					newRhsName = newRhsName.substr(rhsName.find("(")+1, rhsName.find(")")-rhsName.find("(")-1);
				}

				//this could also be a delayed signal name
				try{
					rhsSignal = getSignalByName(newRhsName);
				}catch(string &e){
					try{
						rhsSignal = getDelayedSignalByName(newRhsName);
						//extract the name
						rhsName = rhsName.substr(0, newRhsName.find('^'));
					}catch(string e2){
						//THROWERROR("Error in parse2(): signal " << newRhsName << " not found:" << e2);
						//this is a user-defined name
						rhsSignal = NULL;
						unknownRHSName = true;
					}
				}catch(...){
				    //THROWERROR("Error in parse2(): signal " << newRhsName << " not found:");
				    //this is a user-defined name
				    rhsSignal = NULL;
				    unknownRHSName = true;
				}

				//copy the rhsName with the delay information into the new vhdl buffer
				//	rhsName becomes rhsName_dxxx, if the rhsName signal is declared at a previous cycle
				if(!unknownLHSName && !unknownRHSName)
				  cycleDelay = lhsSignal->getCycle()-rhsSignal->getCycle()+getFunctionalDelay(rhsSignal, lhsSignal);
				newStr << rhsName;
				if(getTarget()->isPipelined() && !unknownLHSName && !unknownRHSName && (cycleDelay>0))
					newStr << "_d" << vhdlize(cycleDelay);

				//find the next rhsName, if there is one
				tmpCurrentPos = tmpNextPos + rhsNameLength + 2;
				tmpNextPos = workStr.find('$', tmpCurrentPos);
			}

			//copy the code that is left up until the end of the line in
			//	the new vhdl code buffer, without changing it
			newStr << workStr.substr(tmpCurrentPos, workStr.size()-tmpCurrentPos);

			//get a new line to parse
			currentPos = nextPos + workStr.size() + 2;
			nextPos = oldStr.find('?', currentPos);
			//special case for the selected assignment statements
			isSelectedAssignment = false;
			if(oldStr.find('$', currentPos) < nextPos)
			{
				nextPos = oldStr.find('$', currentPos);
				isSelectedAssignment = true;
			}
		}

		//the remaining code might contain identifiers that were marked
		//	with ??; these must also be removed
		//if(nextPos == string::npos) && (oldStr.find )

		//copy the remaining code to the vhdl code buffer
		newStr << oldStr.substr(currentPos, oldStr.size()-currentPos);

		vhdl.setSecondLevelCode(newStr.str());

		REPORT(DEBUG, "Finished second-level parsing of VHDL code for operator " << srcFileName);
	}


	int Operator::getFunctionalDelay(Signal *rhsSignal, Signal *lhsSignal)
	{
		bool isLhsPredecessor = false;

		for(size_t i=0; i<lhsSignal->predecessors()->size(); i++)
		{
			pair<Signal*, int> newPair = *lhsSignal->predecessorPair(i);

			if(newPair.first->getName() == rhsSignal->getName())
			{
				isLhsPredecessor = true;

				if(newPair.second < 0)
					return (-1)*newPair.second;
				else
					return 0;
			}
		}

		if(isLhsPredecessor == false)
			THROWERROR("Error in getFunctionalDelay: trying to obtain the functional delay between signal "
					<< rhsSignal->getName() << " and signal " << lhsSignal->getName() << " which are not directly connected");
		return 0;
	}


	int Operator::getPipelineDelay(Signal *rhsSignal, Signal *lhsSignal)
	{
		bool isLhsPredecessor = false;

		for(size_t i=0; i<lhsSignal->predecessors()->size(); i++)
		{
			pair<Signal*, int> newPair = *lhsSignal->predecessorPair(i);

			if(newPair.first->getName() == rhsSignal->getName())
			{
				isLhsPredecessor = true;

				if(newPair.second > 0)
					return newPair.second;
				else
					return 0;
			}
		}

		if(isLhsPredecessor == false)
			THROWERROR("Error in getPipelineDelay: trying to obtain the pipeline delay between signal "
					<< rhsSignal->getName() << " and signal " << lhsSignal->getName() << " which are not directly connected");
		return 0;
	}


	int Operator::getDelay(Signal *rhsSignal, Signal *lhsSignal)
	{
		bool isLhsPredecessor = false;

		for(size_t i=0; i<lhsSignal->predecessors()->size(); i++)
		{
			pair<Signal*, int> newPair = *lhsSignal->predecessorPair(i);

			if(newPair.first->getName() == rhsSignal->getName())
			{
				isLhsPredecessor = true;

				if(newPair.second < 0)
					return (-1)*newPair.second;
				else
					return newPair.second;
			}
		}

		if(isLhsPredecessor == false)
			THROWERROR("Error in getDelay: trying to obtain the delay between signal "
					<< rhsSignal->getName() << " and signal " << lhsSignal->getName() << " which are not directly connected");
		return 0;
	}


	// this is called by schedule() to transform the (string, string, int) dependencies produced by the lexer at each ;
	// into signal dependencies in the graph.
	void Operator::extractSignalDependences()
	{
		//try to parse the unknown dependences first (we have identified a dependency A->B but A or B has not yet been declared)
		// unresolvedDependenceTable is a global variable that holds this information
		for(vector<triplet<string, string, int>>::iterator it=unresolvedDependenceTable.begin(); it!=unresolvedDependenceTable.end(); it++)
		{
			Signal *lhs, *rhs;
			int delay;
			bool unknownLHSName = false, unknownRHSName = false;

			try{
				lhs = getSignalByName(it->first); // Was this signal declared since last time? 
			}catch(string &e){
				//REPORT(DEBUG, "Warning: detected unknown signal name on the left-hand side of an assignment: " << it->first);
				unknownLHSName = true;
			}

			try{
				rhs = getSignalByName(it->second); // or this one
			}catch(string &e){
				//REPORT(DEBUG, "Warning: detected unknown signal name on the right-hand side of an assignment: " << it->second);
				unknownRHSName = true;
			}

			delay = it->third;

			//add the dependences to the corresponding signals
			//	if they are both known:
			//		erase the entry from the unresolvedDependenceTable
			//		add the signals to the list of signals to be scheduled
			if(!unknownLHSName && !unknownRHSName)
			{
				//add the dependences
				lhs->addPredecessor(rhs, delay);
				rhs->addSuccessor(lhs, delay);
				//mark the signals as needing to be scheduled (possibly again)
				lhs->setHasBeenScheduled(false);
				rhs->setHasBeenScheduled(false);
				//remove the current entry from the unresolved dependence table
				unresolvedDependenceTable.erase(it);
			}
		}

		//start parsing the dependence table, modifying the signals of each triplet
		// dependenceTable is produced by the lexer between two VHDL statements / semicolons
		for(vector<triplet<string, string, int>>::iterator it=vhdl.dependenceTable.begin(); it!=vhdl.dependenceTable.end(); it++)
		{
			Signal *lhs, *rhs;
			int delay;
			bool unknownLHSName = false, unknownRHSName = false;

			try{
			    lhs = getSignalByName(it->first);
			}catch(string &e){
				//REPORT(DEBUG, "Warning: detected unknown signal name on the left-hand side of an assignment: " << it->first);
			    unknownLHSName = true;
			}

			try{
			    rhs = getSignalByName(it->second);
			}catch(string &e){
				//REPORT(DEBUG, "Warning: detected unknown signal name on the right-hand side of an assignment: " << it->second);
			    unknownRHSName = true;
			}

			delay = it->third;

			//add the dependences to the corresponding signals, if they are both known
			//	add a new entry to the unknownDependenceTable, if not add them to the list of unknown dependences
			if(!unknownLHSName && !unknownRHSName)
			{
				lhs->addPredecessor(rhs, delay);
				rhs->addSuccessor(lhs, delay);
				//add the signals to the list of signals to be scheduled
				//	if they don't already exist on the list
				bool lhsPresent = false;

				for(size_t i=0; i<signalsToSchedule.size(); i++)
				{
					if(signalsToSchedule[i]->getName() == lhs->getName())
						lhsPresent = true;
					// Commented out 
					// if(signalsToSchedule[i]->getName() == rhs->getName())
					// 	rhsPresent = true;
				}

				if(!lhsPresent)
					signalsToSchedule.push_back(lhs);
				// if(!rhsPresent)
				// 	signalsToSchedule.push_back(rhs);
			}else{
				triplet<string, string, int> newDep = make_triplet(it->first, it->second, it->third);
				unresolvedDependenceTable.push_back(newDep);
			}
		}

		//start the parsing of the dependence table for the subcomponents
		for(unsigned int i=0; i<subComponentList_.size(); i++)
		{
			subComponentList_[i]->extractSignalDependences();
		}

		//clear the current partial dependence table
		vhdl.dependenceTable.clear();
	}


	void Operator::schedule(bool forceReschedule)
	{
		//TODO: add more checks here
		//if the operator is already scheduled, then there is nothing else to do
		if(isOperatorScheduled() && !forceReschedule)
			return;

		//extract the dependences between the operator's internal signals
		extractSignalDependences();

		//check if this is a global operator
		//	if this is a global operator, and this is the global copy, then schedule it
		//	if this is a global operator, but a local copy, then use the
		//	scheduling of the global copy

		//test if this operator is a global operator
		bool isGlobalOperator = false;
		string globalOperatorName = getName();

		//this might be a copy of the global shared operator, so try to remove the suffix
		if(globalOperatorName.find("_copy_") != string::npos)
			globalOperatorName = globalOperatorName.substr(0, globalOperatorName.find("_copy_"));

		//look in the global operator list for the operator
		for(unsigned int i=0; i<UserInterface::globalOpList.size(); i++)
			if(UserInterface::globalOpList[i]->getName() == globalOperatorName)
			{
				isGlobalOperator = true;
				break;
			}

		//if this is a shared operator, and this is a copy, then schedule the operator
		//	according to the original operator
		if(isGlobalOperator && (getName().find("_copy_") != string::npos))
		{
			Operator* originalOperator;

			//search for the global copy of the operator, which should already be scheduled
			for(unsigned int i=0; i<UserInterface::globalOpList.size(); i++)
				if(UserInterface::globalOpList[i]->getName() == globalOperatorName)
				{
					originalOperator = UserInterface::globalOpList[i];
					break;
				}

			scheduleSharedInstance(this, originalOperator, forceReschedule);

			return;
		}
		
		//schedule each of the signals on the list of signals to be scheduled
		for(auto i: signalsToSchedule)
		{
			//schedule the current signal
			setSignalTiming(i, forceReschedule);
		}

		//start the schedule on the children of the signals on the list of signals to be scheduled
		for(auto currentSignal: signalsToSchedule)
		{
			for(auto successor : *currentSignal->successors())
				scheduleSignal(successor.first, forceReschedule);
		}

		//clear the list of signals to schedule
		signalsToSchedule.clear();
	}


	
	void Operator::scheduleSignal(Signal *targetSignal, bool forceReschedule)
	{
		//TODO: add more checks here
		//check if the signal has already been scheduled
		if((targetSignal->getHasBeenScheduled() == true) && (forceReschedule == false))
			//there is nothing else to be done
			return;

		//check if all the signal's predecessors have been scheduled
		for(unsigned int i=0; i<targetSignal->predecessors()->size(); i++)
			if(targetSignal->predecessor(i)->getHasBeenScheduled() == false)
				//not all predecessors are scheduled, so the signal cannot be scheduled
				return;

		//if the preconditions are satisfied, schedule the signal
		setSignalTiming(targetSignal, forceReschedule);

		//update the lifespan of targetSignal's predecessors
		for(unsigned int i=0; i<targetSignal->predecessors()->size(); i++)
		{
			//predecessor signals that belong to a subcomponent do not need to have their lifespan affected
			if((targetSignal->parentOp()->getName() != targetSignal->predecessor(i)->parentOp()->getName()) &&
					(targetSignal->predecessor(i)->type() == Signal::out))
				continue;
			if(targetSignal->predecessorPair(i)->second >= 0)
				targetSignal->predecessor(i)->updateLifeSpan(targetSignal->getCycle()
						- targetSignal->predecessor(i)->getCycle());
			else
				targetSignal->predecessor(i)->updateLifeSpan(targetSignal->getCycle()+abs(targetSignal->predecessorPair(i)->second)
						- targetSignal->predecessor(i)->getCycle());
		}

		//check if this is an input signal for a sub-component
		if(targetSignal->type() == Signal::in)
		{
			//this is an input for a sub-component
			bool allInputsScheduled = true;

			//check if all the inputs of the operator have been scheduled
			for(unsigned int i=0; i<targetSignal->parentOp()->getIOList()->size(); i++)
				if((targetSignal->parentOp()->getIOListSignal(i)->type() == Signal::in)
						&& (targetSignal->parentOp()->getIOListSignal(i)->getHasBeenScheduled() == false))
				{
					allInputsScheduled = false;
					break;
				}

			//if all the inputs of the operator have been scheduled, then just stop
			if(allInputsScheduled == false)
				return;

			//schedule the signal's parent operator
			targetSignal->parentOp()->schedule(forceReschedule);
		}else
		{
			//this is a regular signal inside of the operator

			//try to schedule the successors of the signal
			for(unsigned int i=0; i<targetSignal->successors()->size(); i++)
				scheduleSignal(targetSignal->successor(i), forceReschedule);
		}

	}

	void Operator::setSignalTiming(Signal* targetSignal, bool forceReschedule)
	{
		int maxCycle = 0;
		double maxCriticalPath = 0.0, maxTargetCriticalPath;

		//check if the signal has already been scheduled
		if((targetSignal->getHasBeenScheduled() == true) && (forceReschedule == false))
			//there is nothing else to be done
			return;

		//initialize the maximum cycle and critical path of the predecessors
		if(targetSignal->predecessors()->size() != 0)
		{
			//if the delay is negative, it means this is a functional delay,
			//	so we can ignore it for the pipeline computations
			maxCycle = targetSignal->predecessor(0)->getCycle() + max(0, targetSignal->predecessorPair(0)->second);
			maxCriticalPath = targetSignal->predecessor(0)->getCriticalPath();
		}

		//determine the lexicographic maximum cycle and critical path of the signal's parents
		for(unsigned int i=1; i<targetSignal->predecessors()->size(); i++)
		{
			Signal* currentPred = targetSignal->predecessor(i);
			//if the delay is negative, it means this is a functional delay,
			//	so we can ignore it for the pipeline computations
			int currentPredCycleDelay = max(0, targetSignal->predecessorPair(i)->second);

			//constant signals are not taken into account
			if((currentPred->type() == Signal::constant) || (currentPred->type() == Signal::constantWithDeclaration))
				continue;

			//check if the predecessor is at a later cycle
			if(currentPred->getCycle()+currentPredCycleDelay >= maxCycle)
			{
				//differentiate between delayed and non-delayed signals
				if((currentPred->getCycle()+currentPredCycleDelay > maxCycle) && (currentPredCycleDelay > 0))
				{
					//if the maximum cycle is at a delayed signal, then
					//	the critical path must also be reset
					maxCycle = currentPred->getCycle()+currentPredCycleDelay;
					maxCriticalPath = 0;
				}else
				{
					if(currentPred->getCycle() > maxCycle)
					{
						maxCycle = currentPred->getCycle();
						maxCriticalPath = currentPred->getCriticalPath();
					}else if((currentPred->getCycle() == maxCycle) && (currentPred->getCriticalPath() > maxCriticalPath))
					{
						//the maximum cycle and critical path come from a
						//	predecessor, on a link without delay
						maxCycle = currentPred->getCycle();
						maxCriticalPath = currentPred->getCriticalPath();
					}
				}
			}
		}

		//compute the cycle and the critical path for the node itself from
		//	the maximum cycle and critical path of the predecessors
		maxTargetCriticalPath = 1.0 / getTarget()->frequency() - getTarget()->ffDelay();
		//check if the signal needs to pass to the next cycle,
		//	due to its critical path contribution
		if(maxCriticalPath + targetSignal->getCriticalPathContribution() > maxTargetCriticalPath)
		{
			double totalDelay = maxCriticalPath + targetSignal->getCriticalPathContribution();

			while(totalDelay > maxTargetCriticalPath)
			{
				// if maxCriticalPath+criticalPathContribution > 1/frequency, it may insert several pipeline levels.
				// This is what we want to pipeline block-RAMs and DSPs up to the nominal frequency by just passing their overall delay.
				maxCycle++;
				totalDelay -= maxTargetCriticalPath;
			}

			if(totalDelay < 0)
				totalDelay = 0.0;
			targetSignal->setCycle(maxCycle);

#define ASSUME_RETIMING 1 // we leave a bit of the critical path to this signal
#if ASSUME_RETIMING
			targetSignal->setCriticalPath(totalDelay);
#else //ASSUME_NO_RETIMING
			targetSignal->setCriticalPath(targetSignal->getCriticalPathContribution());
#endif
			
		}else
		{
			targetSignal->setCycle(maxCycle);
			targetSignal->setCriticalPath( maxCriticalPath + targetSignal->getCriticalPathContribution());
		}

		//update the lifespan of inputSignal's predecessors
		for(unsigned int i=0; i<targetSignal->predecessors()->size(); i++)
		{
			//predecessor signals that belong to a subcomponent do not need to have their lifespan affected
			if((targetSignal->parentOp()->getName() != targetSignal->predecessor(i)->parentOp()->getName()) &&
					(targetSignal->predecessor(i)->type() == Signal::out))
				continue;
			targetSignal->predecessor(i)->updateLifeSpan(
					targetSignal->getCycle() - targetSignal->predecessor(i)->getCycle());
		}

		targetSignal->setHasBeenScheduled(true);
	}


	void Operator::drawDotDiagram(ofstream& file, int mode, std::string dotDrawingMode, std::string tabs)
	{
		bool mustDrawCompact = true;

		//check if this operator has already been drawn
		if(mode == 1)
		{
			//for global operators, which are not subcomponents of other operators
			if(isOperatorDrawn_ == true)
				//nothing else to do
				return;
		}else
		{
			//for subcomponents, the logic is reversed
			if(isOperatorDrawn_ == false)
				//nothing else to do
				return;
		}

		file << "\n";

		//draw a component (a graph) or a subcomponent (a subgraph)
		if(mode == 1)
		{
			//main component in the globalOpList
			file << tabs << "digraph " << getName() << "\n";
		}else if(mode == 2)
		{
			//a subcomponent
			file << tabs << "subgraph cluster_" << getName() << "\n";
		}else
		{
			THROWERROR("drawDotDiagram: error: unhandled mode=" << mode);
		}

		file << tabs << "{\n";

		//increase the tabulation
		tabs = join(tabs, "\t");

		//if this is a

		file << tabs << "//graph drawing options\n";
		file << tabs << "label=" << getName() << ";\n"
				<< tabs << "labelloc=bottom;\n" << tabs << "labeljust=right;\n";

		if(mode == 2)
			file << tabs << "style=\"bold, dotted\";\n";

		if(dotDrawingMode == "compact")
			file << tabs << "ratio=compress;\n" << tabs << "fontsize=8;\n";
		else
			file << tabs << "ratio=auto;\n";

		//check if it's worth drawing the subcomponent in the compact style
		int maxInputCycle = -1, maxOutputCycle = -1;
		for(int i=0; (unsigned int)i<ioList_.size(); i++)
			if((ioList_[i]->type() == Signal::in) && (ioList_[i]->getCycle() > maxInputCycle))
				maxInputCycle = ioList_[i]->getCycle();
			else if((ioList_[i]->type() == Signal::out) && (ioList_[i]->getCycle() > maxOutputCycle))
				maxOutputCycle = ioList_[i]->getCycle();
		//if the inputs and outputs of a subcomponent are at the same cycle,
		//	then it's probably not worth drawing the operator in the compact style
		if((maxOutputCycle-maxInputCycle < 1) && (dotDrawingMode == "compact") && (mode == 2))
			mustDrawCompact = false;

		//if the dot drawing option is compact
		//	then, if this is a subcomponent, only draw the input-output connections
		if((mode == 2) && (dotDrawingMode == "compact") && mustDrawCompact)
		{
			file << tabs << "nodesep=0.15;\n" << tabs << "ranksep=0.15;\n" << tabs << "concentrate=yes;\n\n";

			//draw the input/output signals
			file << tabs << "//input/output signals of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
				file << drawDotNode(ioList_[i], tabs);

			//draw the subcomponents of this operator
			file << "\n" << tabs << "//subcomponents of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<subComponentList_.size(); i++)
				subComponentList_[i]->drawDotDiagram(file, 2, dotDrawingMode, tabs);

			file << "\n";

			//draw the invisible node, which replaces the content of the subcomponent
			file << tabs << "//signal connections of operator " << this->getName() << "\n";
			file << drawDotNode(NULL, tabs);
			//draw edges between the inputs and the intermediary node
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
				if(ioList_[i]->type() == Signal::in)
					file << drawDotEdge(ioList_[i], NULL, tabs);
			//draw edges between the intermediary node and the outputs
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
				if(ioList_[i]->type() == Signal::out)
					file << drawDotEdge(NULL, ioList_[i], tabs);
		}else
		{
			file << tabs << "nodesep=0.25;\n" << tabs << "ranksep=0.5;\n\n";

			//draw the input/output signals
			int inputCount = 0, outputCount = 0;
			file << tabs << "//input/output signals of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
			{
				file << drawDotNode(ioList_[i], tabs);
				inputCount += ((ioList_[i]->type() == Signal::in) ? 1 : 0);
				outputCount += ((ioList_[i]->type() == Signal::out) ? 1 : 0);
			}
			//force all the inputs of the operator to have the same rank (same for the outputs)
			string inputRankString = "", outputRankString = "";
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
			{
				if(ioList_[i]->type() == Signal::in)
					inputRankString += " " + ioList_[i]->getName() + "__" + this->getName() + ",";
				if(ioList_[i]->type() == Signal::out)
					outputRankString += " " + ioList_[i]->getName() + "__" + this->getName() + ",";
			}
			if(inputRankString != "")
			{
				//remove the last comma
				inputRankString = inputRankString.substr(0, inputRankString.size()-1);
				file << tabs << "{rank=same" << inputRankString << "};" << "\n";
			}
			if(outputRankString != "")
			{
				//remove the last comma
				outputRankString = outputRankString.substr(0, outputRankString.size()-1);
				file << tabs << "{rank=same" << outputRankString << "};" << "\n";
			}
			//draw the signals of this operator as nodes
			file << tabs << "//internal signals of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<signalList_.size(); i++)
				file << drawDotNode(signalList_[i], tabs);

			//draw the subcomponents of this operator
			file << "\n" << tabs << "//subcomponents of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<subComponentList_.size(); i++)
				subComponentList_[i]->drawDotDiagram(file, 2, dotDrawingMode, tabs);

			file << "\n";

			//draw the out connections of each input of this operator
			file << tabs << "//input and internal signal connections of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
				if(ioList_[i]->type() == Signal::in)
					file << drawDotNodeEdges(ioList_[i], tabs);
			//draw the out connections of each signal of this operator
			for(int i=0; (unsigned int)i<signalList_.size(); i++)
				file << drawDotNodeEdges(signalList_[i], tabs);
		}

		//decrease tabulation
		tabs = tabs.substr(0, tabs.length()-1);
		//end of component/subcomponent
		file << tabs << "}\n\n";
		//increase tabulation
		tabs = join(tabs, "\t");

		//for subcomponents, draw the connections of the output ports
		//draw the out connections of each output of this operator
		if(mode == 2)
		{
			file << tabs << "//output signal connections of operator " << this->getName() << "\n";
			for(int i=0; (unsigned int)i<ioList_.size(); i++)
				if(ioList_[i]->type() == Signal::out)
					file << drawDotNodeEdges(ioList_[i], tabs);
		}

		if(mode == 1)
			setIsOperatorDrawn(true);
		else
			setIsOperatorDrawn(false);
	}

	std::string Operator::drawDotNode(Signal *node, std::string tabs)
	{
	  ostringstream stream;
	  std::string nodeName = (node!=NULL ? node->getName() : "invisibleNode");

	  //different flow for the invisible node, that replaces the content of a subcomponent
	  if(node == NULL)
	  {
		  //output the node name
		  //	for uniqueness purposes the name is signal_name::parent_operator_name
		  stream << tabs << nodeName << "__" << this->getName() << " ";

		  //output the node's properties
		  stream << "[ label=\"...\", shape=plaintext, color=black, style=\"bold\", fontsize=32, fillcolor=white];\n";

		  return stream.str();
	  }

	  //process the node's name for correct dot format
	  if(node->type() == Signal::constant)
		  nodeName = node->getName().substr(node->getName().find("_cst")+1);

	  //output the node name
	  //	for uniqueness purposes the name is signal_name::parent_operator_name
	  stream << tabs << nodeName << "__" << node->parentOp()->getName() << " ";

	  //output the node's properties
	  stream << "[ label=\"" << nodeName << "\\n" << node->getCriticalPathContribution() << "\\n(" << node->getCycle() << ", "
	      << node->getCriticalPath() << ")\"";
	  stream << ", shape=box, color=black";
	  stream << ", style" << ((node->type() == Signal::in || node->type() == Signal::out) ? "=\"bold, filled\"" : "=filled");
	  stream << ", fillcolor=" << Signal::getDotNodeColor(node->getCycle());
	  stream << ", peripheries=" << (node->type() == Signal::in ? "2" : node->type() == Signal::out ? "3" : "1");
	  stream << " ];\n";

	  return stream.str();
	}

	std::string Operator::drawDotNodeEdges(Signal *node, std::string tabs)
	{
	  ostringstream stream;
	  std::string nodeName = node->getName();
	  std::string nodeParentName;

	  //process the node's name for correct dot format
	  if(node->type() == Signal::constant)
		  nodeName = node->getName().substr(node->getName().find("_cst")+1);

	  for(int i=0; (unsigned int)i<node->successors()->size(); i++)
	  {
		  if(node->successor(i)->type() == Signal::constant)
			  nodeParentName = node->successor(i)->getName().substr(node->successor(i)->getName().find("_cst")+1);
		  else
			  nodeParentName = node->successor(i)->getName();

	      stream << tabs << nodeName << "__" << node->parentOp()->getName() << " -> "
		   << nodeParentName << "__" << node->successor(i)->parentOp()->getName() << " [";
	      stream << " arrowhead=normal, arrowsize=1.0, arrowtail=normal, color=black, dir=forward, ";
	      stream << " label=" << max(0, node->successorPair(i)->second);
	      stream << " ];\n";
	  }

	  return stream.str();
	}

	std::string Operator::drawDotEdge(Signal *source, Signal *sink, std::string tabs)
	{
		ostringstream stream;
		std::string sourceNodeName = (source == NULL ? "invisibleNode" : source->getName());
		std::string sinkNodeName = (sink == NULL ? "invisibleNode" : sink->getName());

		//process the source node's name for correct dot format
		if((source != NULL) && (source->type() == Signal::constant))
			sourceNodeName = source->getName().substr(source->getName().find("_cst")+1);
		//process the sink node's name for correct dot format
		if((sink != NULL) && (sink->type() == Signal::constant))
			sinkNodeName = sink->getName().substr(sink->getName().find("_cst")+1);

		stream << tabs << sourceNodeName << "__" << (source!=NULL ? source->parentOp()->getName() : this->getName()) << " -> "
				<< sinkNodeName << "__" << (sink!=NULL ? sink->parentOp()->getName() : this->getName()) << " [";
		stream << " arrowhead=normal, arrowsize=1.0, arrowtail=normal, color=black, dir=forward, ";
		if((source != NULL) && (sink != NULL))
			stream << " label=" << max(0, sink->getCycle()-source->getCycle());
		stream << " ];\n";

		return stream.str();
	}


	void Operator::setuid(int mm){
		myuid = mm;
	}

	int Operator::getuid(){
		return myuid;
	}



	void  Operator::setIndirectOperator(Operator* op){
		indirectOperator_ = op;

		if(op != NULL){
			op->setuid(getuid()); //the selected implementation becomes this operator

			op->setName(getName());//accordingly set the name of the implementation

			signalList_ = op->signalList_;
			subComponentList_ = op->subComponentList_;
			ioList_ = op->ioList_;
		 }
	}

	void Operator::cleanup(vector<Operator*> *ol, Operator* op){
		//iterate through all the components of op
		for(unsigned int it=0; it<op->subComponentList_.size(); it++)
			cleanup(ol, subComponentList_[it]);

		for(unsigned j=0; j<(*ol).size(); j++){
			if((*ol)[j]->myuid == op->myuid){
				(*ol).erase((*ol).begin()+j);
			}
		}
	}

	string Operator::signExtend(string name, int w){
		Signal* s;

		try{
			s = getSignalByName(name);
		}
		catch(string &e){
			THROWERROR("ERROR in signExtend, " << e);
		}

		//get the signals's width
		if(w == s->width()){
			//nothing to do
			return name;
		}
		else if(w < s->width()){
			REPORT(INFO, "WARNING: signExtend() called for a sign extension to " << w
						 << " bits of signal " << name << " whose width is " << s->width());
			return name;
		}
		else{
			ostringstream n;
			n << "(";
			for(int i=0; i<(w - s->width()); i++){
				n << name << of(s->width() -1) << " & ";
			}
			n << name << ")";
			return n.str();
		}
	}

	string Operator::zeroExtend(string name, int w){
		Signal* s;

		try{
			s = getSignalByName(name);
		}
		catch(string &e){
			THROWERROR("ERROR in zeroExtend, " << e);
		}

		//get the signals's width
		if (w == s->width()){
			//nothing to do
			return name;
		}
		else if(w < s->width()){
			REPORT(INFO,  "WARNING: zeroExtend() to " << w << " bits of signal " << name << " whose width is " << s->width());
			return name;
		}
		else{
			ostringstream n;
			n << "(" << zg(w-s->width()) << " &" << name << ")";
			return n.str();
		}
	}

	void Operator::emulate(TestCase * tc) {
		throw std::string("emulate() not implemented for ") + uniqueName_;
	}

	bool Operator::hasComponent(string s){
		return (getSubComponent(s) != NULL);
	}

	void Operator::addComment(string comment, string align){
		vhdl << align << "-- " << comment << endl;
	}

	void Operator::addFullComment(string comment, int lineLength) {
		string align = "--";
		// - 2 for the two spaces
		for (unsigned i = 2; i < (lineLength-2-comment.size()) / 2; i++)
			align += "-";
		vhdl << align << " " << comment << " " << align << endl;
	}


	//Completely replace "this" with a copy of another operator.
	void  Operator::cloneOperator(Operator *op){
		subComponentList_           = op->getSubComponentList();
		signalList_                 = op->getSignalList();
		ioList_                     = op->getIOListV();

		parentOp_                   = op->parentOp_;

		target_                     = op->getTarget();
		uniqueName_                 = op->getUniqueName();
		architectureName_           = op->getArchitectureName();
		testCaseSignals_            = op->getTestCaseSignals();
		tmpInPortMap_               = op->tmpInPortMap_;
		tmpOutPortMap_              = op->tmpOutPortMap_;
		vhdl.vhdlCode.str(op->vhdl.vhdlCode.str());
		vhdl.vhdlCodeBuffer.str(op->vhdl.vhdlCodeBuffer.str());

		vhdl.dependenceTable        = op->vhdl.dependenceTable;

		srcFileName                 = op->getSrcFileName();
		cost                        = op->getOperatorCost();
		subComponentList_           = op->getSubComponentList();
		stdLibType_                 = op->getStdLibType();
		numberOfInputs_             = op->getNumberOfInputs();
		numberOfOutputs_            = op->getNumberOfOutputs();
		isSequential_               = op->isSequential();
		pipelineDepth_              = op->getPipelineDepth();
		signalMap_                  = op->getSignalMap();
		constants_                  = op->getConstants();
		attributes_                 = op->getAttributes();
		types_                      = op->getTypes();
		attributesValues_           = op->getAttributesValues();

		hasRegistersWithoutReset_   = op->getHasRegistersWithoutReset();
		hasRegistersWithAsyncReset_ = op->getHasRegistersWithAsyncReset();
		hasRegistersWithSyncReset_  = op->getHasRegistersWithSyncReset();

		commentedName_              = op->commentedName_;
		headerComment_              = op->headerComment_;
		copyrightString_            = op->getCopyrightString();
		needRecirculationSignal_    = op->getNeedRecirculationSignal();
		hasClockEnable_             = op->hasClockEnable();
		indirectOperator_           = op->getIndirectOperator();
		hasDelay1Feedbacks_         = op->hasDelay1Feedbacks();

		isOperatorImplemented_      = op->isOperatorImplemented();
		isOperatorDrawn_            = op->isOperatorDrawn();

		isUnique_                   = op->isUnique();
		uniquenessSet_              = op->uniquenessSet_;

		resourceEstimate.str(op->resourceEstimate.str());
		resourceEstimateReport.str(op->resourceEstimateReport.str());
		reHelper                    = op->reHelper;
		reActive                    = op->reActive;

		floorplan.str(op->floorplan.str());
		flpHelper                   = op->flpHelper;
	}


	void  Operator::deepCloneOperator(Operator *op){
		//start by cloning the operator
		cloneOperator(op);

		//create deep copies of the signals, sub-components, instances etc.

		//create deep copies of the signals
		vector<Signal*> newSignalList;
		for(unsigned int i=0; i<signalList_.size(); i++)
		{
			if((signalList_[i]->type() == Signal::in) || (signalList_[i]->type() == Signal::out))
				continue;

			Signal* tmpSignal = new Signal(this, signalList_[i]);

			//if this a constant signal, then it doesn't need to be scheduled
			if((tmpSignal->type() == Signal::constant) || (tmpSignal->type() == Signal::constantWithDeclaration))
			{
				tmpSignal->setCycle(0);
				tmpSignal->setCriticalPath(0.0);
				tmpSignal->setCriticalPathContribution(0.0);
				tmpSignal->setHasBeenScheduled(true);
			}

			newSignalList.push_back(tmpSignal);
		}
		signalList_.clear();
		signalList_.insert(signalList_.begin(), newSignalList.begin(), newSignalList.end());

		//create deep copies of the inputs/outputs
		vector<Signal*> newIOList;
		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			Signal* tmpSignal = new Signal(this, ioList_[i]);

			//if this a constant signal, then it doesn't need to be scheduled
			if((tmpSignal->type() == Signal::constant) || (tmpSignal->type() == Signal::constantWithDeclaration))
			{
				tmpSignal->setCycle(0);
				tmpSignal->setCriticalPath(0.0);
				tmpSignal->setCriticalPathContribution(0.0);
				tmpSignal->setHasBeenScheduled(true);
			}

			newIOList.push_back(tmpSignal);
		}
		ioList_.clear();
		ioList_.insert(ioList_.begin(), newIOList.begin(), newIOList.end());
		//signalList_.insert(signalList_.end(), newIOList.begin(), newIOList.end());

		//update the signal map
		signalMap_.clear();
		//insert the inputs/outputs
		for(unsigned int i=0; i<ioList_.size(); i++)
		  signalMap_[ioList_[i]->getName()] = ioList_[i];
		//insert the internal signals
		for(unsigned int i=0; i<signalList_.size(); i++)
		  signalMap_[signalList_[i]->getName()] = signalList_[i];

		//create deep copies of the subcomponents
		vector<Operator*> newOpList;
		for(unsigned int i=0; i<op->getSubComponentList().size(); i++)
		{
			Operator* tmpOp = new Operator(op->getSubComponentList()[i]->getTarget());

			tmpOp->deepCloneOperator(op->getSubComponentList()[i]);
			//add the new subcomponent to the subcomponent list
			newOpList.push_back(tmpOp);
		}
		subComponentList_.clear();
		subComponentList_ = newOpList;

		//recreate the signal dependences, for each of the signals
		for(unsigned int i=0; i<signalList_.size(); i++)
		{
			vector<pair<Signal*, int>> newPredecessors, newSuccessors;
			Signal *originalSignal = op->getSignalByName(signalList_[i]->getName());

			//create the new list of predecessors for the signal currently treated
			for(unsigned int j=0; j<originalSignal->predecessors()->size(); j++)
			{
				pair<Signal*, int> tmpPair = *(originalSignal->predecessorPair(j));

				//for the input signals, there is no need to do the predecessor list here
				if(signalList_[i]->type() == Signal::in)
				{
					break;
				}

				//if the signal is connected to the output of a subcomponent,
				//	then just skip this predecessor, as it will be added later on
				if((tmpPair.first->type() == Signal::out)
						&& (tmpPair.first->parentOp()->getName() != originalSignal->parentOp()->getName()))
					continue;

				//signals connected only to constants are already scheduled
				if(((tmpPair.first->type() == Signal::constant) || (tmpPair.first->type() == Signal::constantWithDeclaration))
						&& (originalSignal->predecessors()->size() == 1))
				{
					signalList_[i]->setCycle(0);
					signalList_[i]->setCriticalPath(0.0);
					signalList_[i]->setCriticalPathContribution(0.0);
					signalList_[i]->setHasBeenScheduled(true);
				}

				newPredecessors.push_back(make_pair(getSignalByName(tmpPair.first->getName()), tmpPair.second));
			}
			//replace the old list of predecessors with the new one
			signalList_[i]->resetPredecessors();
			signalList_[i]->addPredecessors(newPredecessors);

			//create the new list of successors for the signal currently treated
			for(unsigned int j=0; j<originalSignal->successors()->size(); j++)
			{
				pair<Signal*, int> tmpPair = *(originalSignal->successorPair(j));

				//for the output signals, there is no need to do the successor list here
				if(signalList_[i]->type() == Signal::out)
				{
					break;
				}

				//if the signal is connected to the input of a subcomponent,
				//	then just skip this successor, as it will be added later on
				if((tmpPair.first->type() == Signal::in)
						&& (tmpPair.first->parentOp()->getName() != originalSignal->parentOp()->getName()))
					continue;

				newSuccessors.push_back(make_pair(getSignalByName(tmpPair.first->getName()), tmpPair.second));
			}
			//replace the old list of predecessors with the new one
			signalList_[i]->resetSuccessors();
			signalList_[i]->addSuccessors(newSuccessors);
		}

		//update the signal map
		signalMap_.clear();
		for(unsigned int i=0; i<signalList_.size(); i++)
		  signalMap_[signalList_[i]->getName()] = signalList_[i];
		for(unsigned int i=0; i<ioList_.size(); i++)
		  signalMap_[ioList_[i]->getName()] = ioList_[i];

		//no need to recreate the signal dependences for each of the input/output signals,
		//	as this is either done by instance, or it is done by the parent operator of this operator
		//	or, they may not need to be set at all (e.g. when you just copy an operator)

		//connect the inputs and outputs of the subcomponents to the corresponding signals
		for(unsigned int i=0; i<subComponentList_.size(); i++)
		{
			Operator *currentOp = subComponentList_[i], *originalOp;

			//look for the original operator
			for(unsigned int j=0; j<op->getSubComponentList().size(); j++)
				if(currentOp->getName() == op->getSubComponentList()[j]->getName())
				{
					originalOp = op->getSubComponentList()[j];
					break;
				}

			//connect the inputs/outputs of the subcomponent
			for(unsigned int j=0; j<currentOp->getIOList()->size(); j++)
			{
				Signal *currentIO = currentOp->getIOListSignal(j), *originalIO;

				//recreate the predecessor and successor list, as it is in
				//	the original operator, which we are cloning
				originalIO = originalOp->getSignalByName(currentIO->getName());
				//recreate the predecessor/successor (for the input/output) list
				for(unsigned int k=0; k<originalIO->predecessors()->size(); k++)
				{
					pair<Signal*, int>* tmpPair = originalIO->predecessorPair(k);

					//if the signal is only connected to a constant,
					//	then the signal doesn't need to be scheduled
					if(((tmpPair->first->type() == Signal::constant) || (tmpPair->first->type() == Signal::constantWithDeclaration))
							&& (originalIO->predecessors()->size() == 1))
					{
						currentIO->setCycle(0);
						currentIO->setCriticalPath(0.0);
						currentIO->setCriticalPathContribution(0.0);
						currentIO->setHasBeenScheduled(true);
					}

					if(currentIO->type() == Signal::in)
					{
						currentIO->addPredecessor(getSignalByName(tmpPair->first->getName()), tmpPair->second);
						getSignalByName(tmpPair->first->getName())->addSuccessor(currentIO, tmpPair->second);
					}else if(currentIO->type() == Signal::out)
					{
						currentIO->addPredecessor(currentOp->getSignalByName(tmpPair->first->getName()), tmpPair->second);
						currentOp->getSignalByName(tmpPair->first->getName())->addSuccessor(currentIO, tmpPair->second);
					}
				}
				//recreate the successor/predecessor (for the input/output) list
				for(unsigned int k=0; k<originalIO->successors()->size(); k++)
				{
					pair<Signal*, int>* tmpPair = originalIO->successorPair(k);

					if(currentIO->type() == Signal::in)
					{
						currentIO->addSuccessor(currentOp->getSignalByName(tmpPair->first->getName()), tmpPair->second);
						currentOp->getSignalByName(tmpPair->first->getName())->addPredecessor(currentIO, tmpPair->second);
					}else if(currentIO->type() == Signal::out)
					{
						currentIO->addSuccessor(getSignalByName(tmpPair->first->getName()), tmpPair->second);
						getSignalByName(tmpPair->first->getName())->addPredecessor(currentIO, tmpPair->second);
					}
				}
			}
		}

		//create deep copies of the instances
		//	replace the references in the instances to references to
		//	the newly created deep copies
		//the port maps for the instances do not need to be modified
		subComponentList_.clear();
		for(unsigned int i=0; i<subComponentList_.size(); i++)
			subComponentList_.push_back(subComponentList_[i]);

	}



	bool Operator::isOperatorImplemented(){
		return isOperatorImplemented_;
	}

	void Operator::setIsOperatorImplemented(bool newValue){
		isOperatorImplemented_ = newValue;
	}

	bool Operator::isOperatorScheduled(){
	  return isOperatorScheduled_;
	}

	void Operator::setIsOperatorScheduled(bool newValue){
	  isOperatorScheduled_ = newValue;
	}

	bool Operator::isOperatorDrawn(){
		return isOperatorDrawn_;
	}

	void Operator::setIsOperatorDrawn(bool newValue){
		isOperatorDrawn_ = newValue;
	}


	bool Operator::setShared(){
		if((uniquenessSet_ == true) && (isUnique_ == true))
		{
			THROWERROR("error in setShared(): the function has already been called; for integrity reasons only one call is allowed");
		}else
		{
			isUnique_ = false;
			uniquenessSet_ = true;

			return isUnique_;
		}
	}

	bool Operator::isUnique(){
		return isUnique_;
	}

	bool Operator::isShared(){
		return !isUnique_;
	}


	void Operator::parseVHDL()
	{
		//trigger the second parse
		//	for sequential and combinatorial operators as well
		REPORT(FULL, "  2nd VHDL CODE PARSING PASS");
		parse2();

		//trigger the first code parse for the operator's subcomponents
		for(auto it: subComponentList_)
		{
			it->parseVHDL();
		}
	}


	void Operator::outputVHDLToFile(vector<Operator*> &oplist, ofstream& file)
	{
		string srcFileName = "Operator.cpp"; // for REPORT

		for(auto it: oplist)
		{
			try {
				// check for subcomponents
				if(! it->getSubComponentListR().empty() ){
					//recursively call to print subcomponents
					outputVHDLToFile(it->getSubComponentListR(), file);
				}

				//output the vhdl code to file
				//	for global operators, this is done only once
				if(!it->isOperatorImplemented())
				{
					it->outputVHDL(file);
					it->setIsOperatorImplemented(true);
				}
			} catch (std::string &s) {
					cerr << "Exception while generating '" << it->getName() << "': " << s << endl;
			}
		}
	}




#if 1
	void Operator::outputVHDLToFile(ofstream& file){
		vector<Operator*> oplist;

		REPORT(DEBUG, "Entering outputVHDLToFile");

		//build a copy of the global oplist hidden in UserInterface (if it exists):
		for (unsigned i=0; i<UserInterface::globalOpList.size(); i++)
			oplist.push_back(UserInterface::globalOpList[i]);
		// add self (and all its subcomponents) to this list
		oplist.push_back(this);
		// generate the code
		Operator::outputVHDLToFile(oplist, file);
	}
#endif


	void Operator::setHasDelay1Feedbacks(){
		hasDelay1Feedbacks_=true;
	}


	bool Operator::hasDelay1Feedbacks(){
		return hasDelay1Feedbacks_;
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for resource estimations


	//--Logging functions

	std::string Operator::addFF(int count){
		reActive = true;
		return reHelper->addFF(count);
	}

	std::string Operator::addLUT(int nrInputs, int count){
		reActive = true;
		return reHelper->addLUT(nrInputs, count);
	}

	std::string Operator::addReg(int width, int count){
		reActive = true;
		return reHelper->addReg(width, count);
	}

	std::string Operator::addMultiplier(int count){
		reActive = true;
		return reHelper->addMultiplier(count);
	}

	std::string Operator::addMultiplier(int widthX, int widthY, double ratio, int count){
		reActive = true;
		return reHelper->addMultiplier(widthX, widthY, ratio, count);
	}

	std::string Operator::addAdderSubtracter(int widthX, int widthY, double ratio, int count){
		reActive = true;
		return reHelper->addAdderSubtracter(widthX, widthY, ratio, count);
	}

	std::string Operator::addMemory(int size, int width, int type, int count){
		reActive = true;
		return reHelper->addMemory(size, width, type, count);
	}

	//---More particular resource logging
	std::string Operator::addDSP(int count){
		reActive = true;
		return reHelper->addDSP(count);
	}

	std::string Operator::addRAM(int count){
		reActive = true;
		return reHelper->addRAM(count);
	}

	std::string Operator::addROM(int count){
		reActive = true;
		return reHelper->addROM(count);
	}

	std::string Operator::addSRL(int width, int depth, int count){
		reActive = true;
		return reHelper->addSRL(width, depth, count);
	}

	std::string Operator::addWire(int count, std::string signalName){
		reActive = true;
		return reHelper->addWire(count, signalName);
	}

	std::string Operator::addIOB(int count, std::string portName){
		reActive = true;
		return reHelper->addIOB(count, portName);
	}

	//---Even more particular resource logging-------------------------

	std::string Operator::addMux(int width, int nrInputs, int count){
		reActive = true;
		return reHelper->addMux(width, nrInputs, count);
	}

	std::string Operator::addCounter(int width, int count){
		reActive = true;
		return reHelper->addCounter(width, count);
	}

	std::string Operator::addAccumulator(int width, bool useDSP, int count){
		reActive = true;
		return reHelper->addAccumulator(width, useDSP, count);
	}

	std::string Operator::addDecoder(int wIn, int wOut, int count){
		reActive = true;
		return reHelper->addDecoder(wIn, wOut, count);
	}

	std::string Operator::addArithOp(int width, int nrInputs, int count){
		reActive = true;
		return reHelper->addArithOp(width, nrInputs, count);
	}

	std::string Operator::addFSM(int nrStates, int nrTransitions, int count){
		reActive = true;
		return reHelper->addFSM(nrStates, nrTransitions, count);
	}

	//--Resource usage statistics---------------------------------------
	std::string Operator::generateStatistics(int detailLevel){
		reActive = true;
		return reHelper->generateStatistics(detailLevel);
	}

	//--Utility functions related to the generation of resource usage statistics

	std::string Operator::addPipelineFF(){
		reActive = true;
		return reHelper->addPipelineFF();
	}

	std::string Operator::addWireCount(){
		reActive = true;
		return reHelper->addWireCount();
	}

	std::string Operator::addPortCount(){
		reActive = true;
		return reHelper->addPortCount();
	}

	std::string Operator::addComponentResourceCount(){
		reActive = true;
		return reHelper->addComponentResourceCount();
	}

	void Operator::addAutomaticResourceEstimations(){
		reActive = true;
		resourceEstimate << reHelper->addAutomaticResourceEstimations();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for floorplanning

	std::string Operator::manageFloorplan(){
		return flpHelper->manageFloorplan();
	}

	std::string Operator::addPlacementConstraint(std::string source, std::string sink, int type){
		return flpHelper->addPlacementConstraint(source, sink, type);
	}

	std::string Operator::addConnectivityConstraint(std::string source, std::string sink, int nrWires){
		return flpHelper->addConnectivityConstraint(source, sink, nrWires);
	}

	std::string Operator::addAspectConstraint(std::string source, double ratio){
		return flpHelper->addAspectConstraint(source, ratio);
	}

	std::string Operator::addContentConstraint(std::string source, int value, int length){
		return flpHelper->addContentConstraint(source, value, length);
	}

	std::string Operator::processConstraints(){
		return flpHelper->processConstraints();
	}

	std::string Operator::createVirtualGrid(){
		return flpHelper->createVirtualGrid();
	}

	std::string Operator::createPlacementGrid(){
		return flpHelper->createPlacementGrid();
	}

	std::string Operator::createConstraintsFile(){
		return flpHelper->createConstraintsFile();
	}

	std::string Operator::createPlacementForComponent(std::string moduleName){
		return flpHelper->createPlacementForComponent(moduleName);
	}

	std::string Operator::createFloorplan(){
		return flpHelper->createFloorplan();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////

}


