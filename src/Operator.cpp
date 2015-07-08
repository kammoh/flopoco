/*
The base Operator class, every operator should inherit it

Author : Florent de Dinechin, Bogdan Pasca

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
  All rights reserved.

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
	multimap < string, TestState > Operator::testMemory;		//init the multimap
	int verbose=0;

	Operator::Operator(Target* target, map<string, double> inputDelays){
		stdLibType_                 = 0;						// unfortunately this is the historical default.
		target_                     = target;
		numberOfInputs_             = 0;
		numberOfOutputs_            = 0;
		hasRegistersWithoutReset_   = false;
		hasRegistersWithAsyncReset_ = false;
		hasRegistersWithSyncReset_  = false;
		hasClockEnable_             = false;
		pipelineDepth_              = 0;

		//disabled during the overhaul
		/*
		currentCycle_               = 0;
		criticalPath_               = 0;
		*/

		needRecirculationSignal_    = false;
		//disabled during the overhaul
		//inputDelayMap               = inputDelays;
		myuid                       = getNewUId();
		architectureName_			= "arch";
		indirectOperator_           = NULL;
		hasDelay1Feedbacks_         = false;


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





	void Operator::addSubComponent(Operator* op) {
		oplist.push_back(op);

		cerr << "WARNING: function addSubComponent() should only be used inside Operator!" << endl
				<< tab << tab << "Subcomponents should add the instance, with its port mappings, not just the sub-operator" << endl;
	}




	void Operator::addToGlobalOpList() {
		// We assume all the operators added to GlobalOpList are un-pipelined.
		addToGlobalOpList(this);
	}

	void Operator::addToGlobalOpList(Operator *op) {
		// We assume all the operators added to GlobalOpList are un-pipelined.
		bool alreadyPresent = false;
		vector<Operator*> *globalOpListRef = target_->getGlobalOpListRef();

		for(unsigned i=0; i<globalOpListRef->size(); i++){
			if( op->getName() == (*globalOpListRef)[i]->getName() ){
				alreadyPresent = true;
				REPORT(DEBUG, "Operator::addToGlobalOpListRef(): " << op->getName() << " already present in globalOpList");
				break;
			}
		}

		if(!alreadyPresent)
			globalOpListRef->push_back(op);

		cerr << "WARNING: function addToGlobalOpList() should only be used inside Operator!" << endl
				<< tab << tab << "Subcomponents should add the instance, with its port mappings, not just the sub-operator" << endl;
	}




	void Operator::addInput(const std::string name, const int width, const bool isBus) {
		//search if the signal has already been declared
		if (signalMap_.find(name) != signalMap_.end()) {
			//if yes, signal the error
			THROWERROR("ERROR in addInput, signal " << name<< " seems to already exist");
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

		//disabled during the overhaul
		//declareTable[name] = s->getCycle();

		//search the input delays and try to set the critical path
		// of the input to the one specified in the inputDelays
		//disabled during the overhaul
		/*
		map<string, double>::iterator foundSignal = inputDelayMap.find(name);
		if(foundSignal != inputDelayMap.end())
		{
			s->setCriticalPath(foundSignal->second);
		}
		*/
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

		for(int i=0; i<numberOfPossibleOutputValues; i++)
			testCaseSignals_.push_back(s);

		//disabled in the old version
		//	it intervenes with the pipelining mechanism
		//declareTable[name] = s->getCycle();
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
		//add the signal to the global signal list
		signalMap_[name] = s ;

		//disabled during the overhaul
		//declareTable[name] = s->getCycle();

		//search the input delays and try to set the critical path
		// of the input to the one specified in the inputDelays
		//disabled during the overhaul
		/*
		map<string, double>::iterator foundSignal = inputDelayMap.find(name);
		if(foundSignal != inputDelayMap.end())
		{
			s->setCriticalPath(foundSignal->second);
		}
		*/
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
		//add the signal to the global signal list
		signalMap_[name] = s ;

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

		//disabled during the overhaul
		//declareTable[name] = s->getCycle();

		//search the input delays and try to set the critical path
		// of the input to the one specified in the inputDelays
		//disabled during the overhaul
		/*
		map<string, double>::iterator foundSignal = inputDelayMap.find(name);
		if(foundSignal != inputDelayMap.end())
		{
			s->setCriticalPath(foundSignal->second);
		}
		*/
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

		for(int i=0; i<numberOfPossibleOutputValues; i++)
			testCaseSignals_.push_back(s);

		//disabled during the overhaul
		//declareTable[name] = s->getCycle();
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

		//disabled during the overhaul
		//declareTable[name] = s->getCycle();

		//search the input delays and try to set the critical path
		// of the input to the one specified in the inputDelays
		//disabled during the overhaul
		/*
		map<string, double>::iterator foundSignal = inputDelayMap.find(name);
		if(foundSignal != inputDelayMap.end())
		{
			s->setCriticalPath(foundSignal->second);
		}
		*/
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

		for(int i=0; i<numberOfPossibleOutputValues; i++)
			testCaseSignals_.push_back(s);

		//disabled during the overhaul
		//declareTable[name] = s->getCycle();
	}



	Signal * Operator::getSignalByName(string name) {
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
		if(name.find('^') != string::npos)
		{
			n = name.substr(0, name.find('^'));
		}else
		{
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

	vector<Instance*> Operator::getInstances(){
		return instances_;
	};


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

	void Operator::setNameWithFreq(std::string operatorName){
		std::ostringstream o;

		o <<  operatorName <<  "_" ;
		if(target_->isPipelined())
			o << target_->frequencyMHz() ;
		else
			o << "comb";
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
			o << tab << "port ( ";
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
		o<<"-- This operator is part of the Infinite Virtual Library FloPoCoLib"<<endl;
		o<<"-- All rights reserved "<<endl;
		o<<"-- Authors: " << authorsyears <<endl;
		o<<"--------------------------------------------------------------------------------"<<endl;
	}



	void Operator::pipelineInfo(std::ostream& o){
		if(isSequential())
			o<<"-- Pipeline depth: " << getPipelineDepth() << " cycles"  <<endl <<endl;
		else
			o<<"-- combinatorial"  <<endl <<endl;
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
		vhdl.disableParsing(false);
	}

	void Operator::setCombinatorial() {
		isSequential_=false;
		vhdl.disableParsing(true);
	}

	void Operator::setRecirculationSignal() {
		needRecirculationSignal_ = true;
	}


	int Operator::getPipelineDepth() {
		return pipelineDepth_;
	}

	void Operator::setPipelineDepth(int d) {
		cerr << "WARNING: function setPipelineDepth() is deprecated" << endl;

		pipelineDepth_ = d;
	}

	void Operator::setPipelineDepth() {
		int maxCycle = 0;

		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			if((ioList_[i]->type() == Signal::out) && (ioList_[i]->getCycle() > maxCycle))
				maxCycle = ioList_[i]->getCycle();
		}

		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			if((ioList_[i]->type() == Signal::out) && (ioList_[i]->getCycle() != maxCycle))
				cerr << "WARNING: setPipelineDepth(): this operator's outputs are NOT SYNCHRONIZED!" << endl;
		}

		pipelineDepth_ = maxCycle;
	}

	void Operator::outputFinalReport(int level) {

		if (getIndirectOperator() != NULL){
			// interface operator
			if(getOpList().size() != 1){
				ostringstream o;
				o << "!?! Operator " << getUniqueName() << " is an interface operator with " << getOpList().size() << " children";
				throw o.str();
			}
			getOpListR()[0]->outputFinalReport(level);
		}
		else{
			// Hard operator
			for (unsigned i=0; i< getOpList().size(); i++)
				if (!getOpListR().empty())
					getOpListR()[i]->outputFinalReport(level+1);

			ostringstream tabs, ctabs;
			for (int i=0;i<level-1;i++){
				tabs << "|" << tab;
				ctabs << "|" << tab;
			}

			if (level>0){
				tabs << "|" << "---";
				ctabs << "|" << tab;
			}

			cerr << tabs.str() << "Entity " << uniqueName_ << endl;
			if(this->getPipelineDepth() != 0)
				cerr << ctabs.str() << tab << "Pipeline depth = " << getPipelineDepth() << endl;
			else
				cerr << ctabs.str() << tab << "Not pipelined"<< endl;
		}
	}


	void Operator::setCycle(int cycle, bool report) {
		//disabled during the overhaul
		/*
		criticalPath_ = 0;
		// lexing part
		vhdl.flush(currentCycle_);
		if(isSequential()) {
			currentCycle_=cycle;
			vhdl.setCycle(currentCycle_);
			if(report){
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl ;
			}
			// automatically update pipeline depth of the operator
			if (currentCycle_ > pipelineDepth_)
				pipelineDepth_ = currentCycle_;
		}
		*/

		cerr << "WARNING: function setCycle() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}

	int Operator::getCurrentCycle(){
		//disabled during the overhaul
		/*
		return currentCycle_;
		*/

		cerr << "WARNING: function getCurrentCycle() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

		return -1;
	}

	void Operator::nextCycle(bool report) {
		/*
		// lexing part
		vhdl.flush(currentCycle_);

		if(isSequential()) {

			currentCycle_ ++;
			vhdl.setCycle(currentCycle_);
			if(report)
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl;

			criticalPath_ = 0;
			// automatically update pipeline depth of the operator
			if (currentCycle_ > pipelineDepth_)
				pipelineDepth_ = currentCycle_;
		}
		*/

		cerr << "WARNING: function nextCycle() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}

	void Operator::previousCycle(bool report) {
		//disabled during the overhaul
		/*
		// lexing part
		vhdl.flush(currentCycle_);

		if(isSequential()) {

			currentCycle_ --;
			vhdl.setCycle(currentCycle_);
			if(report)
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl;

		}
		*/

		cerr << "WARNING: function previousCycle() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}


	void Operator::setCycleFromSignal(string name, bool report) {
		//disabled during the overhaul
		/*
		setCycleFromSignal(name, 0.0, report);
		*/

		cerr << "WARNING: function setCycleFromSignal() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}


	void Operator::setCycleFromSignal(string name, double criticalPath, bool report) {
		//disabled during the overhaul
		/*
		// lexing part
		vhdl.flush(currentCycle_);


		if(isSequential()) {
			Signal* s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				ostringstream e;
				e << srcFileName << " (" << uniqueName_ << "): ERROR in setCycleFromSignal, "; // just in case

				e << endl << tab << e2;
				throw e.str();
			}

			if( s->getCycle() < 0 ) {
				ostringstream o;
				o << "signal " << name<< " doesn't have (yet?) a valid cycle";
				throw o.str();
			}

			currentCycle_ = s->getCycle();
			criticalPath_ = criticalPath;
			vhdl.setCycle(currentCycle_);

			if(report)
				vhdl << tab << "---------------- cycle " << currentCycle_ << "----------------" << endl ;
			// automatically update pipeline depth of the operator
			if (currentCycle_ > pipelineDepth_)
				pipelineDepth_ = currentCycle_;
		}
		*/

		cerr << "WARNING: function setCycleFromSignal() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}


	int Operator::getCycleFromSignal(string name, bool report) {
		//disabled during the overhaul
		/*
		// lexing part
		vhdl.flush(currentCycle_);
		*/

		if(isSequential()) {
			Signal* s;

			try {
				s = getSignalByName(name);
			}
			catch(string &e) {
				THROWERROR("): ERROR in getCycleFromSignal, " << endl << tab << e);
			}

			if(s->getCycle() < 0)
			{
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
		//disabled during the overhaul
		/*
		return(syncCycleFromSignal(name, 0.0, report));
		*/

		cerr << "WARNING: function syncCycleFromSignal() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

		return false;
	}



	bool Operator::syncCycleFromSignal(string name, double criticalPath, bool report) {
		//disabled during the overhaul
		/*
		bool advanceCycle = false;

		// lexing part
		vhdl.flush(currentCycle_);
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in syncCycleFromSignal, "; // just in case

		if(isSequential()) {
			Signal* s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}

			if( s->getCycle() < 0 ) {
				ostringstream o;
				o << "signal " << name << " doesn't have (yet?) a valid cycle";
				throw o.str();
			}

			if (s->getCycle() == currentCycle_){
				advanceCycle = false;
				if (criticalPath>criticalPath_)
					criticalPath_=criticalPath ;
			}

			if (s->getCycle() > currentCycle_){
				advanceCycle = true;
				currentCycle_ = s->getCycle();
				criticalPath_= criticalPath;
				vhdl.setCycle(currentCycle_);
			}

			if(report && advanceCycle)
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl ;

			// automatically update pipeline depth of the operator
			if (currentCycle_ > pipelineDepth_)
				pipelineDepth_ = currentCycle_;
		}

		return advanceCycle;
		*/

		cerr << "WARNING: function syncCycleFromSignal() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

		return false;
	}

	// TODO get rid of this method
	void Operator::setSignalDelay(string name, double delay){
		cout << "WARNING: setSignalDelay obsolete (used in " << srcFileName << ")" << endl;
	}

	// TODO get rid of this method
	double Operator::getSignalDelay(string name){
		cout << "WARNING: getSignalDelay is obsolete (used in " << srcFileName << ")" << endl;

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

	double Operator::getCriticalPath()
	{
		//disabled during the overhaul
		/*
		return criticalPath_;
		*/

		cerr << "WARNING: function getCriticalPath() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

		return -1;
	}

	void Operator::setCriticalPath(double delay)
	{
		//disabled during the overhaul
		/*
		criticalPath_=delay;
		*/

		cerr << "WARNING: function setCriticalPath() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}

	void Operator::addToCriticalPath(double delay)
	{
		//disabled during the overhaul
		/*
		criticalPath_ += delay;
		*/

		cerr << "WARNING: function addToCriticalPath() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
	}





	bool Operator::manageCriticalPath(double delay, bool report){
		//disabled during the overhaul
		/*
		if(isSequential()) {
#if 0 // code up to version 3.0
			if ( target_->ffDelay() + (totalDelay) > (1.0/target_->frequency())){
				nextCycle(report);
				criticalPath_ = min(delay, 1.0/target_->frequency());
				return true;
			}
			else{
				criticalPath_ += delay;
				return false;
			}
#else // May insert several register levels, experimental in 3.0
			double totalDelay = criticalPath_ + delay;
			criticalPath_ = totalDelay;  // will possibly be reset in the loop below
			// cout << "total delay =" << totalDelay << endl;
			while ( target_->ffDelay() + (totalDelay) > (1.0/target_->frequency())){
				// This code behaves as the previous as long as delay < 1/frequency
				// if delay > 1/frequency, it may insert several pipeline levels.
				// This is what we want to pipeline blockrams and DSPs up to the nominal frequency by just passing their overall delay.
				nextCycle(); // this resets criticalPath. Therefore, if we entered the loop, CP=0 when we exit
				totalDelay -= (1.0/target_->frequency()) + target_->ffDelay();
				// cout << " after one nextCycle total delay =" << totalDelay << endl;
			}
#endif
		}
		else {
			criticalPath_ += delay;
			return false;
		}
		*/

		cerr << "WARNING: function manageCriticalPath() is deprecated and no longer has any effect!" << endl
			 << tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
			 << tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

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


	string Operator::declare(string name, const int width, bool isbus, Signal::SignalType regType) {
		return declare(0.0, name, width, isbus, regType);
	}

	string Operator::declare(string name, Signal::SignalType regType) {
		return declare(0.0, name, 1, false, regType);
	}

	string Operator::declare(double criticalPathContribution, string name, Signal::SignalType regType) {
		return declare(criticalPathContribution, name, 1, false, regType);
	}

	string Operator::declare(double criticalPathContribution, string name, const int width, bool isbus, Signal::SignalType regType) {
		Signal* s;

		// check the signal doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declare(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, width, isbus);

		initNewSignal(s, criticalPathContribution, regType);

		return name;
	}


	string Operator::declareFixPoint(string name, const bool isSigned, const int MSB, const int LSB, Signal::SignalType regType){
		return declareFixPoint(0.0, name, isSigned, MSB, LSB, regType);
	}


	string Operator::declareFixPoint(double criticalPathContribution, string name, const bool isSigned, const int MSB, const int LSB, Signal::SignalType regType){
		Signal* s;

		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declareFixPoint(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, isSigned, MSB, LSB);

		initNewSignal(s, criticalPathContribution, regType);

		//set the flag for a fixed-point signal
		s->setIsFix(true);
		s->setIsFP(false);
		s->setIsIEEE(false);

		return name;
	}


	string Operator::declareFloatingPoint(string name, const int wE, const int wF, Signal::SignalType regType, const bool ieeeFormat){
		return declareFloatingPoint(0.0, name, wE, wF, regType, ieeeFormat);
	}


	string Operator::declareFloatingPoint(double criticalPathContribution, string name, const int wE, const int wF, Signal::SignalType regType, const bool ieeeFormat){
		Signal* s;

		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declareFixPoint(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, wE, wF, ieeeFormat);

		initNewSignal(s, criticalPathContribution, regType);

		s->setIsFix(false);
		//set the flag for a floating-point signal
		if(ieeeFormat)
		{
			s->setIsFP(false);
			s->setIsIEEE(true);
		}
		else
		{
			s->setIsFP(true);
			s->setIsIEEE(false);
		}

		return name;
	}


	void Operator::initNewSignal(Signal* s, double criticalPathContribution, Signal::SignalType regType)
	{
		//set, if needed, the global parameters used for generating the registers
		if((regType==Signal::registeredWithoutReset) || (regType==Signal::registeredWithZeroInitialiser))
			hasRegistersWithoutReset_ = true;
		if(regType==Signal::registeredWithSyncReset)
			hasRegistersWithSyncReset_ = true;
		if(regType==Signal::registeredWithAsyncReset)
			hasRegistersWithAsyncReset_ = true;

		// define its cycle, critical path and contribution to the critical path
		s->setCycle(0);
		s->setCriticalPath(0.0);
		s->setCriticalPathContribution(criticalPathContribution);

		//initialize the signals predecessors and successors
		s->resetPredecessors();
		s->resetSuccessors();

		// add the signal to signalMap and signalList
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

			if(s->getCycle() < 0) {
				THROWERROR("signal " << name<< " doesn't have (yet?) a valid cycle" << endl);
			}
			if(s->getCycle() > this->getCurrentCycle()) {
				THROWERROR("active cycle of signal " << name<< " is later than current cycle, cannot delay it" << endl);
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
		Signal* formal;
		Signal* s;

		// check if the signal already exists, when we're supposed to create a new signal
		if(signalMap_.find(actualSignalName) !=  signalMap_.end() && newSignal) {
			THROWERROR("ERROR in outPortMap() for entity " << op->getName()  << ", "
					<< "signal " << actualSignalName << " already exists");
		}

		//check if the port in the target operator exists, and return it if so
		try {
			formal = op->getSignalByName(componentPortName);
		}
		catch(string &e2) {
			THROWERROR("ERROR in outPortMap() for entity " << op->getName()  << ", " << e2);
		}

		//check if the output port of the instantiated operator is indeed of output port type
		if(formal->type() != Signal::out){
			THROWERROR("signal " << componentPortName << " of component " << op->getName()
					<< " doesn't seem to be an output port");
		}

		//check if the signal connected to the port exists, and return it if so, or create it if necessary
		if(newSignal){
			s = new Signal(this, formal); 	// create a copy using the default copy constructor
			s->setName(actualSignalName); 	// except for the name
			s->setType(Signal::wire); 		// ... and the fact that we declare a wire

			//initialize the signals predecessors and successors
			s->resetPredecessors();
			s->resetSuccessors();

			// add the newly created signal to signalMap and signalList
			signalList_.push_back(s);
			signalMap_[s->getName()] = s;
		}else
		{
			try {
				s = getSignalByName(actualSignalName);
			}
			catch(string &e2) {
				THROWERROR("ERROR in outPortMap() for entity " << getName()  << ", " << e2);
			}
		}

		// add the mapping to the output mapping list of Op
		op->tmpOutPortMap_[componentPortName] = s;

		//add componentPortName as a predecessor of actualSignalName,
		// and actualSignalName as a successor of componentPortName
		formal->addSuccessor(s, 0);
		s->addPredecessor(formal, 0);
	}


	void Operator::inPortMap(Operator* op, string componentPortName, string actualSignalName){
		Signal *formal, *s;
		std::string name;

		//check if the signal already exists
		try{
			s = getSignalByName(actualSignalName);
		}
		catch(string &e2) {
			THROWERROR("ERROR in inPortMap() for entity " << op->getName() << ": " << e2);
		}

		//check if the signal already exists
		try{
			formal = op->getSignalByName(componentPortName);
		}
		catch(string &e2) {
			THROWERROR("ERROR in inPortMap() for entity " << op->getName() << ": " << e2);
		}

		//check if the input port of the instantiated operator is indeed of input port type
		if(formal->type() != Signal::in){
			THROWERROR("ERROR in inPortMap() for entity " << op->getName() << ": signal " << componentPortName
					<< " of component " << op->getName() << " doesn't seem to be an input port");
		}

		// add the mapping to the input mapping list of Op
		op->tmpInPortMap_[componentPortName] = s;

		//add componentPortName as a successor of actualSignalName,
		// and actualSignalName as a predecessor of componentPortName
		formal->addPredecessor(s, 0);
		s->addSuccessor(formal, 0);
	}



	void Operator::inPortMapCst(Operator* op, string componentPortName, string actualSignal){
		Signal* formal;
		Signal *s;
		string name;
		double constValue;
		sollya_obj_t node;

		//check if the signal already exists
		try {
			formal = op->getSignalByName(componentPortName);
		}
		catch (string &e2) {
			THROWERROR("ERROR in inPortMapCst() for entity " << op->getName() << ": " << e2);
		}

		//check if the input port of the instantiated operator is indeed of input port type
		if(formal->type() != Signal::in){
			THROWERROR("ERROR in inPortMapCst() for entity " << op->getName() << ": signal " << componentPortName
					<< " of component " << op->getName() << " doesn't seem to be an input port");
		}

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
		s = new Signal(this, join(actualSignal, "_cst"), Signal::constant, constValue);

		//initialize the signals predecessors and successors
		s->resetPredecessors();
		s->resetSuccessors();

		// add the newly created signal to signalMap and signalList
		signalList_.push_back(s);
		signalMap_[s->getName()] = s;

		op->tmpInPortMap_[componentPortName] = s;
	}


	string Operator::instance(Operator* op, string instanceName, bool isGlobalOperator){
		ostringstream o;

		// TODO add more checks here

		//checking that all the signals are covered
		for(unsigned int i=0; i<op->getIOList()->size(); i++)
		{
			Signal *signal = op->getIOListSignal(i);
			bool isSignalMapped = false;

			if(signal->type() == Signal::in)
			{
				//this is an input signal, so look for it in the input port mappings
				for(map<string, Signal*>::iterator it=op->tmpInPortMap_.begin(); it!=op->tmpInPortMap_.end(); it++)
				{
					if(it->first == signal->getName())
					{
						isSignalMapped = true;
						break;
					}
				}
			}else
			{
				//this is an output signal, so look for it in the output port mappings
				for(map<string, Signal*>::iterator it=op->tmpOutPortMap_.begin(); it!=op->tmpOutPortMap_.end(); it++)
				{
					if(it->first == signal->getName())
					{
						isSignalMapped = true;
						break;
					}
				}
			}

			if(!isSignalMapped)
				THROWERROR("ERROR in instance() while trying to create a new instance of "
						<< op->getName() << " called " << instanceName << ": input/output "
						<< signal->getName() << " is not mapped to anything" << endl);
		}

		o << tab << instanceName << ": " << op->getName();

		if(op->isSequential())
			o << "  -- maxInputDelay=" << getMaxInputDelays(op->ioList_);
		o << endl;
		o << tab << tab << "port map ( ";

		// build vhdl and erase portMap_
		if(op->isSequential()) {
			o << "clk  => clk" << "," << endl
			  << tab << tab << "           rst  => rst";
			if (op->isRecirculatory()) {
				o << "," << endl << tab << tab << "           stall_s => stall_s";
			};
			if (op->hasClockEnable()) {
				o << "," << endl << tab << tab << "           ce => ce";
			};
		}


		for(map<string, Signal*>::iterator it=op->tmpInPortMap_.begin(); it!=op->tmpInPortMap_.end(); it++) {
			string rhsString;

			if((it != op->tmpInPortMap_.begin()) || op->isSequential())
				o << "," << endl <<  tab << tab << "           ";

			// The following code assumes that the IO is declared as standard_logic_vector
			// If the actual parameter is a signed or unsigned, we want to automatically convert it
			if(it->second->isFix()){
				rhsString = std_logic_vector(it->second->getName());
			}
			else {
				rhsString = it->second->getName();
			}

			o << it->first << " => " << rhsString;
		}

		for(map<string, Signal*>::iterator it=op->tmpOutPortMap_.begin(); it!=op->tmpOutPortMap_.end(); it++) {
			string rhsString;

			if(!((it == op->tmpOutPortMap_.begin()) && (op->tmpInPortMap_.size() != 0)) || op->isSequential())
				o << "," << endl <<  tab << tab << "           ";

			o << it->first << " => " << it->second->getName();
		}

		o << ");" << endl;

		//create a new instance
		Instance* newInstance = new Instance(instanceName, op, tmpInPortMap_, tmpOutPortMap_);
		if(isGlobalOperator)
			newInstance->setHasBeenImplemented(true);
		instances_.push_back(newInstance);

		//clear the port mappings
		tmpInPortMap_.clear();
		tmpOutPortMap_.clear();

		// add the operator to the subcomponent list
		oplist.push_back(op);
		if(isGlobalOperator)
			addToGlobalOpList(op);


		//Floorplanning ------------------------------------------------
		floorplan << manageFloorplan();
		flpHelper->addToFlpComponentList(op->getName());
		flpHelper->addToInstanceNames(op->getName(), instanceName);
		//--------------------------------------------------------------


		return o.str();
	}



	string Operator::buildVHDLSignalDeclarations() {
		ostringstream o;

		for(unsigned int i=0; i<signalList_.size(); i++) {
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

		for(unsigned int i=0; i<oplist.size(); i++) {
			oplist[i]->outputVHDLComponent(o, oplist[i]->uniqueName_);
			o << endl;
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


	void Operator::addAttribute(std::string attributeName,  std::string attributeType,  std::string object, std::string value ) {
		// TODO add some checks ?
		attributes_[attributeName] = attributeType;
		pair<string,string> p = make_pair(attributeName,object);
		attributesValues_[p] = value;
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
			o <<  "attribute " << name << " of " << object << " is " << value << ";" << endl;
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
				if ((s->type() == Signal::registeredWithoutReset) || (s->type()==Signal::registeredWithZeroInitialiser) || (s->type() == Signal::wire))
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

		//		cout << tc->getInputVHDL();
		//    cout << tc->getExpectedOutputVHDL();

		// add to the test case list
		return tc;
	}

	map<string, double> Operator::getInputDelayMap(){
		map<string, double> inputDelayMap;

		cerr << "WARNING: this function no longer has the same meaning, due to the overhaul of the pipeline framework;" << endl
				<< tab << "the delay map for the instance being built will be returned" << endl;
		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it++; it!=tmpInPortMap_.end())
			inputDelayMap[it->first] = it->second->getCriticalPath();

		return inputDelayMap;
	}

	map<string, double> Operator::getOutDelayMap(){
		map<string, double> outputDelayMap;

		cerr << "WARNING: this function no longer has the same meaning, due to the overhaul of the pipeline framework;" << endl
				<< tab << "the delay map for the instance being built will be returned" << endl;
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it++; it!=tmpOutPortMap_.end())
			outputDelayMap[it->first] = it->second->getCriticalPath();

		return outputDelayMap;
	}

	map<string, int> Operator::getDeclareTable(){
		cerr << "WARNING: function getDeclareTable() is deprecated and no longer has any effect!" << endl
				<< tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
				<< tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

		return NULL;
	}

	Target* Operator::getTarget(){
		return target_;
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

		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it++; it!=tmpInPortMap_.end())
			tmpMap[it->first] = it->second->getName();
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it++; it!=tmpOutPortMap_.end())
			tmpMap[it->first] = it->second->getName();

		return tmpMap;
	}

	map<string, Operator*> Operator::getSubComponents(){
		cerr << "WARNING: function getSubComponents() is deprecated!" << endl
				<< tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
				<< tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;
		map<string, Operator*> subComponents;

		for(unsigned int i=0; i<instances_.size(); i++)
			subComponents[instances_[i]->getName()] = instances_[i]->getOperator();

		return subComponents;
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

	vector<Operator*> Operator::getOpList(){
		return oplist;
	}

	vector<Operator*>& Operator::getOpListR(){
		return oplist;
	}

	FlopocoStream* Operator::getFlopocoVHDLStream(){
		return &vhdl;
	}

	void Operator::outputVHDL(std::ostream& o, std::string name) {
		if (! vhdl.isEmpty() ){
			licence(o);
			pipelineInfo(o);
			stdLibs(o);
			outputVHDLEntity(o);
			newArchitecture(o,name);
			o << buildVHDLComponentDeclarations();
			o << buildVHDLSignalDeclarations();			//TODO: this cannot be called before scheduling the signals (it requires the lifespan of the signals, which is not yet computed)
			o << buildVHDLTypeDeclarations();
			o << buildVHDLConstantDeclarations();
			o << buildVHDLAttributes();
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
		REPORT(DEBUG, "Starting second-level parsing for operator " << srcFileName);

		vector<pair<string,int> >:: iterator iterUse;
		map<string, int>::iterator iterDeclare;

		string name;
		int declareCycle, useCycle;

		string str (vhdl.str());

		//parse the useTable and check that the declarations are OK
		for (iterUse = (vhdl.useTable).begin(); iterUse!=(vhdl.useTable).end();++iterUse){
			name     = (*iterUse).first;
			useCycle = (*iterUse).second;

			ostringstream tSearch;
			ostringstream tReplace;
			string replaceString;

			tSearch << "__"<<name<<"__"<<useCycle<<"__";
			string searchString (tSearch.str());

			iterDeclare = declareTable.find(name);
			declareCycle = iterDeclare->second;

			if (iterDeclare != declareTable.end()){
				tReplace << use(name, useCycle - declareCycle);
				replaceString = tReplace.str();
				if (useCycle<declareCycle){
					if(!hasDelay1Feedbacks_){
						cerr << srcFileName << " (" << uniqueName_ << "): WARNING: Signal " << name
								<< " defined @ cycle "<< declareCycle<< " and used @ cycle " << useCycle << endl;
						cerr << srcFileName << " (" << uniqueName_ << "): If this is a feedback signal you may ignore this warning"<<endl;
					}else{
						if(declareCycle - useCycle != 1){
							cerr << srcFileName << " (" << uniqueName_ << "): ERROR: Signal " << name
									<<" defined @ cycle "<<declareCycle<<" and used @ cycle " << useCycle <<endl;
							exit(1);
						}
					}
				}
			}else{
				/* parse the declare by hand and check lower/upper case */
				bool found = false;
				string tmp;
				for (iterDeclare = declareTable.begin(); iterDeclare!=declareTable.end();++iterDeclare){
					tmp = iterDeclare->first;
					if ( (to_lowercase(tmp)).compare(to_lowercase(name))==0){
						found = true;
						break;
					}
				}

				if (found == true){
					cerr  << srcFileName << " (" << uniqueName_ << "): ERROR: Clash on signal:"<<name<<". Definition used signal name "<<tmp<<". Check signal case!"<<endl;
					exit(-1);
				}

				tReplace << name;
				replaceString = tReplace.str();
			}

			if ( searchString != replaceString ){
				string::size_type pos = 0;
				while ( (pos = str.find(searchString, pos)) != string::npos ) {
					str.replace( pos, searchString.size(), replaceString );
					pos++;
				}
			}
		}

		for (iterDeclare = declareTable.begin(); iterDeclare!=declareTable.end();++iterDeclare){
			name = iterDeclare->first;
			useCycle = iterDeclare->second;

			ostringstream tSearch;
			tSearch << "__"<<name<<"__"<<useCycle<<"__";
			//			cout << "searching for: " << tSearch.str() << endl;
			string searchString (tSearch.str());

			ostringstream tReplace;
			tReplace << name;
			string replaceString(tReplace.str());

			if ( searchString != replaceString ){

				string::size_type pos = 0;
				while ( (pos = str.find(searchString, pos)) != string::npos ) {
					str.replace( pos, searchString.size(), replaceString );
					pos++;
				}
			}
		}

		vhdl.setSecondLevelCode(str);
		REPORT(DEBUG, "   ... done second-level parsing for operator "<<srcFileName);
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

			//disabled during the overhaul
			//setCycle(op->getPipelineDepth());

			op->setName(getName());//accordingly set the name of the implementation

			signalList_ = op->signalList_;
			//disabled during the overhaul
			//subComponents_ = op->subComponents_;
			oplist = op->oplist;
			ioList_ = op->ioList_;
		 }
	}

	void Operator::cleanup(vector<Operator*> *ol, Operator* op){
		//iterate through all the components of op
		for(unsigned int it=0; it<op->oplist.size(); it++)
			cleanup(ol, oplist[it]);

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
		}else if(w < s->width()){
			cout << "WARNING: you required a sign extension to " << w
					<< " bits of signal " << name << " whose width is " << s->width() << endl;
			return name;
		}else{
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
		}else if(w < s->width()){
			cout << "WARNING: you required a zero extension to " << w
					<< " bits of signal " << name << " whose width is " << s->width() << endl;
			return name;
		}else{
			ostringstream n;
			n << "(" << zg(w-s->width()) << " &" << name << ")";
			return n.str();
		}
	}

	void Operator::emulate(TestCase * tc) {
		throw std::string("emulate() not implemented for ") + uniqueName_;
	}

	bool Operator::hasComponent(string s){
		for(unsigned int i=0; i<instances_.size(); i++)
			if(instances_[i]->getName() == s)
				return true;

		return false;
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
		//disabled during the overhaul
		//subComponents_ = op->getSubComponents();
		instances_                  = op->getInstances();
		signalList_                 = op->getSignalList();
		ioList_                     = op->getIOListV();

		target_                     = op->getTarget();
		uniqueName_                 = op->getUniqueName();
		architectureName_           = op->getArchitectureName();
		testCaseSignals_            = op->getTestCaseSignals();
		//disabled during the overhaul
		//portMap_ = op->getPortMap();
		tmpInPortMap_               = op->tmpInPortMap_;
		tmpOutPortMap_              = op->tmpOutPortMap_;
		//disabled during the overhaul
		//outDelayMap = map<string,double>(op->getOutDelayMap());
		//inputDelayMap = op->getInputDelayMap();
		//disabled during the overhaul
		//vhdl.vhdlCodeBuffer << op->vhdl.vhdlCodeBuffer.str();
		vhdl.vhdlCode       << op->vhdl.vhdlCode.str();
		//vhdl.currentCycle_   = op->vhdl.currentCycle_;
		//vhdl.useTable        = op->vhdl.useTable;
		vhdl.dependenceTable        = op->vhdl.dependenceTable;
		srcFileName                 = op->getSrcFileName();
		//disabled during the overhaul
		//declareTable = op->getDeclareTable();
		cost                        = op->getOperatorCost();
		oplist                      = op->getOpList();
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
		//disabled during the overhaul
		//currentCycle_               = op->getCurrentCycle();
		//criticalPath_               = op->getCriticalPath();
		needRecirculationSignal_    = op->getNeedRecirculationSignal();
		hasClockEnable_             = op->hasClockEnable();
		indirectOperator_           = op->getIndirectOperator();
		hasDelay1Feedbacks_         = op->hasDelay1Feedbacks();

		resourceEstimate            = op->resourceEstimate;
		resourceEstimateReport      = op->resourceEstimateReport;
		reHelper                    = op->reHelper;
		reActive                    = op->reActive;

		floorplan                   = op->floorplan;
		flpHelper                   = op->flpHelper;
	}




	void Operator::outputVHDLToFile(vector<Operator*> &oplist, ofstream& file){
		string srcFileName = "Operator.cpp"; // for REPORT
		for(unsigned i=0; i<oplist.size(); i++) {
			try {
				REPORT(FULL, "---------------OPERATOR: " << oplist[i]->getName() << "-------------");
				//disabled during the overhaul
				//REPORT(FULL, "  DECLARE LIST" << printMapContent(oplist[i]->getDeclareTable()));
				//REPORT(FULL, "  USE LIST" << printVectorContent(  (oplist[i]->getFlopocoVHDLStream())->getUseTable()) );

				// check for subcomponents
				if(! oplist[i]->getOpListR().empty() ){
					//recursively call to print subcomponent
					outputVHDLToFile(oplist[i]->getOpListR(), file);
				}
				oplist[i]->getFlopocoVHDLStream()->flush();

				// second parse is only for sequential operators
				if (oplist[i]->isSequential()){
					REPORT(FULL, "  2nd PASS");
					oplist[i]->parse2();
				}
				oplist[i]->outputVHDL(file);

			} catch (std::string &s) {
					cerr << "Exception while generating '" << oplist[i]->getName() << "': " << s << endl;
			}
		}
	}




#if 1
	void Operator::outputVHDLToFile(ofstream& file){
		vector<Operator*> oplist;

		REPORT(DEBUG, "outputVHDLToFile");

		//build a copy of the global oplist hidden in Target (if it exists):
		vector<Operator*> *goplist = target_->getGlobalOpListRef();
		for (unsigned i=0; i<goplist->size(); i++)
			oplist.push_back((*goplist)[i]);
		// add self (and all its subcomponents) to this list
		oplist.push_back(this);
		// generate the code
		Operator::outputVHDLToFile(oplist, file);
	}
#endif



	float Operator::pickRandomNum ( float limit, int fp, int sp )
	{
		static boost::mt19937 rng;
		float element;
		const float  limitMax = 112.0;
		string distribution = "gauss";

		// the rng need to be "re-seed" in order to provide different number otherwise it will always give the same
		static unsigned int seed = 0;
		rng.seed ( ( ++seed + time ( NULL ) ) );

		if ( distribution == "gauss" ){
			if ( limit == 0 ){
				// fp represent the mean and sp the standard deviation
				boost::normal_distribution<> nd ( fp, sp );
				boost::variate_generator < boost::mt19937&, boost::normal_distribution<> > var_nor ( rng, nd );
				do{
					element = fabs ( var_nor () );
				}while( element > limitMax );
			}
			else{
				boost::normal_distribution<> nd ( fp, 3 * sp);
				boost::variate_generator < boost::mt19937&, boost::normal_distribution<> > var_nor ( rng, nd );
				do{
					element = fabs ( var_nor () );
				}while ( element >= limit || element > limitMax );
			}
		}
		if ( distribution == "uniform" ){
			if ( fp > sp){
				int temp = fp;
				fp = sp;
				sp = temp;
			}
			boost::uniform_int<> ud ( fp, sp );
			boost::variate_generator < boost::mt19937&, boost::uniform_int<> > var_uni ( rng, ud );
			element = var_uni ();
		}
		return element;
	}


	bool Operator::checkExistence ( TestState parameters, string opName ){
		if ( !testMemory.empty () ){
			multimap < string, TestState >::key_compare memoryComp = testMemory.key_comp ();
			multimap < string, TestState >::iterator it = testMemory.begin ();
			while ( it != testMemory.end () && memoryComp ( ( *it ).first, opName ) ){
				it++;
			}

			// boolean indicating that we are still analysing TestState on the same operator
			bool firstEqual = true;
			for(it; it != testMemory.end () && firstEqual ; it++ ){
				bool exist = false;
				TestState  memoryVect = ( *it ).second;
				if ( opName.compare ( ( * ( it ) ).first ) != 0 ){
					firstEqual = false;
				}
				else{
					exist = parameters.equality ( &memoryVect );
					if ( exist ){
						return exist;
					}
				}
			}
		}
		return false;
	}

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


