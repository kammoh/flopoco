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

		isOperatorImplemented_       = false;
		isOperatorScheduled_         = false;


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
		oplist.push_back(op);

		cerr << "WARNING: function addSubComponent() should only be used inside Operator!" << endl;
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

//	vector<Instance*> Operator::getInstances(){
//		return instances_;
//	};


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
		setPipelineDepth();
		return pipelineDepth_;
	}

	void Operator::setPipelineDepth(int d) {
		cerr << "WARNING: function setPipelineDepth() is deprecated" << endl;

		pipelineDepth_ = d;
	}

	void Operator::setPipelineDepth() {
		int minInputCycle, maxOutputCycle;
		bool firstInput = true, firstOutput = true;

		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			if(firstInput && (ioList_[i]->type() == Signal::in))
			{
				minInputCycle = ioList_[i]->getCycle();
				firstInput = false;
				continue;
			}
			if((ioList_[i]->type() == Signal::in) && (ioList_[i]->getCycle() < minInputCycle))
			{
				minInputCycle = ioList_[i]->getCycle();
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
				cerr << "WARNING: setPipelineDepth(): this operator's outputs are NOT SYNCHRONIZED!" << endl;
		}

		pipelineDepth_ = maxOutputCycle-minInputCycle;
	}

	void Operator::outputFinalReport(ostream& s, int level) {

		if (getIndirectOperator()!=NULL){
			// interface operator
			if(getOpList().size()!=1){
				ostringstream o;
				o << "!?! Operator " << getUniqueName() << " is an interface operator with " << getOpList().size() << "children";
				throw o.str();
			}
			getOpList()[0]->outputFinalReport(s, level);
		}else
		{
			// Hard operator
			if (! getOpList().empty())
				for (auto i: getOpList())
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

	void Operator::resetPredecessors(Signal* targetSignal)
	{
		targetSignal->predecessors()->clear();
	}

	void Operator::addPredecessor(Signal* targetSignal, Signal* predecessor, int delayCycles)
	{
		//check if the signal already exists, within the same instance
		for(int i=0; (unsigned)i<targetSignal->predecessors()->size(); i++)
		{
			pair<Signal*, int> predecessorPair = *(targetSignal->predecessorPair(i));
			if((predecessorPair.first->parentOp()->getName() == predecessor->parentOp()->getName())
					&& (predecessorPair.first->getName() == predecessor->getName())
					&& (predecessorPair.first->type() == predecessor->type())
					&& (predecessorPair.second == delayCycles))
			{
				REPORT(FULL, "ERROR in addPredecessor(): trying to add an already existing signal "
						<< predecessor->getName() << " to the predecessor list");
				//nothing else to do
				return;
			}
		}

		//safe to insert a new signal in the predecessor list
		pair<Signal*, int> newPredecessorPair = make_pair(predecessor, delayCycles);
		targetSignal->predecessors()->push_back(newPredecessorPair);
	}

	void Operator::addPredecessors(Signal* targetSignal, vector<pair<Signal*, int>> predecessorList)
	{
		for(unsigned int i=0; i<predecessorList.size(); i++)
			addPredecessor(targetSignal, predecessorList[i].first, predecessorList[i].second);
	}

	void Operator::removePredecessor(Signal* targetSignal, Signal* predecessor, int delayCycles)
	{
		//only try to remove the predecessor if the signal
		//	already exists, within the same instance and with the same delay
		for(int i=0; (unsigned)i<targetSignal->predecessors()->size(); i++)
		{
			pair<Signal*, int> predecessorPair = *(targetSignal->predecessorPair(i));
			if((predecessorPair.first->parentOp()->getName() == predecessor->parentOp()->getName())
					&& (predecessorPair.first->getName() == predecessor->getName())
					&& (predecessorPair.first->type() == predecessor->type())
					&& (predecessorPair.second == delayCycles))
			{
				//delete the element from the list
				targetSignal->predecessors()->erase(targetSignal->predecessors()->begin()+i);
				return;
			}
		}

		throw("ERROR in removePredecessor(): trying to remove a non-existing signal "
				+ predecessor->getName() + " from the predecessor list");
	}

	void Operator::resetSuccessors(Signal* targetSignal)
	{
		targetSignal->successors()->clear();
	}

	void Operator::addSuccessor(Signal* targetSignal, Signal* successor, int delayCycles)
	{
		//check if the signal already exists, within the same instance
		for(int i=0; (unsigned)i<targetSignal->successors()->size(); i++)
		{
			pair<Signal*, int> successorPair = *(targetSignal->successorPair(i));
			if((successorPair.first->parentOp()->getName() == successor->parentOp()->getName())
					&& (successorPair.first->getName() == successor->getName())
					&& (successorPair.first->type() == successor->type())
					&& (successorPair.second == delayCycles))
			{
				REPORT(FULL, "ERROR in addSuccessor(): trying to add an already existing signal "
						<< successor->getName() << " to the predecessor list");
				//nothing else to do
				return;
			}
		}

		//safe to insert a new signal in the predecessor list
		pair<Signal*, int> newSuccessorPair = make_pair(successor, delayCycles);
		targetSignal->successors()->push_back(newSuccessorPair);
	}

	void Operator::addSuccessors(Signal* targetSignal, vector<pair<Signal*, int>> successorList)
	{
		for(unsigned int i=0; i<successorList.size(); i++)
			addSuccessor(targetSignal, successorList[i].first, successorList[i].second);
	}

	void Operator::removeSuccessor(Signal* targetSignal, Signal* successor, int delayCycles)
	{
		//only try to remove the successor if the signal
		//	already exists, within the same instance and with the same delay
		for(int i=0; (unsigned)i<targetSignal->successors()->size(); i++)
		{
			pair<Signal*, int> successorPair = *(targetSignal->successorPair(i));
			if((successorPair.first->parentOp()->getName() == successor->parentOp()->getName())
					&& (successorPair.first->getName() == successor->getName())
					&& (successorPair.first->type() == successor->type())
					&& (successorPair.second == delayCycles))
			{
				//delete the element from the list
				targetSignal->successors()->erase(targetSignal->successors()->begin()+i);
				return;
			}
		}

		throw("ERROR in removeSuccessor(): trying to remove a non-existing signal "
				+ successor->getName() + " to the predecessor list");
	}

	void Operator::setSignalParentOp(Signal* signal, Operator* newParentOp)
	{
		//erase the signal from the operator's signal list and map
		for(unsigned int i=0; i<(signal->parentOp()->getSignalList()).size(); i++)
			if(signal->parentOp()->getSignalList()[i]->getName() == signal->getName())
				signal->parentOp()->getSignalList().erase(signal->parentOp()->getSignalList().begin()+i);
		signal->parentOp()->getSignalMap().erase(signal->getName());

		//change the signal's parent operator
		signal->setParentOp(newParentOp);

		//add the signal to the new parent's signal list
		signal->parentOp()->signalList_.push_back(signal);
		signal->parentOp()->getSignalMap()[signal->getName()] = signal;
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
					return true;
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

	/*
	double Operator::getMaxInputDelays(map<string, double> inputDelays)
	{
		double maxInputDelay = 0;
		map<string, double>::iterator iter;
		for (iter = inputDelays.begin(); iter!=inputDelays.end();++iter)
			if (iter->second > maxInputDelay)
				maxInputDelay = iter->second;

		return maxInputDelay;
	}
	*/

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


	std::string Operator::declareTable(string name, Signal::SignalType regType, std::string tableValues)
	{
		return declareTable(0.0, name, regType, tableValues);
	}

	std::string Operator::declareTable(double criticalPathContribution, string name, Signal::SignalType regType, std::string tableValues)
	{
		Signal* s;

		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			THROWERROR("ERROR in declareFixPoint(), signal " << name << " already exists" << endl);
		}

		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(this, name, regType, tableValues);

		initNewSignal(s, criticalPathContribution, regType);

		s->setIsFix(false);
		s->setIsFP(false);
		s->setIsIEEE(false);

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

		//define its cycle, critical path and contribution to the critical path
		s->setCycle(0);
		s->setCriticalPath(0.0);
		s->setCriticalPathContribution(criticalPathContribution);

		//initialize the signals predecessors and successors
		resetPredecessors(s);
		resetSuccessors(s);

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


	string Operator::delay(string signalName, int nbDelayCycles)
	{
		ostringstream result;

		//check that the number of delay cycles is a positive integer
		if(nbDelayCycles < 0)
			THROWERROR("Error in delay(): trying to add a negative number of delay cycles:" << nbDelayCycles);

		result << signalName << "^" << nbDelayCycles;
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

			//disabled during the overhaul
			/*
			if(s->getCycle() < 0) {
				THROWERROR("signal " << name<< " doesn't have (yet?) a valid cycle" << endl);
			}
			if(s->getCycle() > this->getCurrentCycle()) {
				THROWERROR("active cycle of signal " << name<< " is later than current cycle, cannot delay it" << endl);
			}
			*/

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
		Signal *formal, *s;

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

		//check if the signal connected to the port exists, and return it if so
		//	or create it if it doesn't exist
		if(newSignal){
			s = new Signal(this, formal); 	// create a copy using the default copy constructor
			s->setName(actualSignalName); 	// except for the name
			s->setType(Signal::wire); 		// ... and the fact that we declare a wire

			//initialize the signals predecessors and successors
			resetPredecessors(s);
			resetSuccessors(s);

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
		tmpOutPortMap_[componentPortName] = s;

		//create the connections between the signals in instance()
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
		tmpInPortMap_[componentPortName] = s;

		//create the connections between the signals in instance()
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
		s = new Signal(this, join(actualSignal, "_cst_", vhdlize(getNewUId())), Signal::constant, constValue);

		//initialize the signals predecessors and successors
		resetPredecessors(s);
		resetSuccessors(s);

		//set the timing for the constant signal, at cycle 0, criticalPath 0, criticalPathContribution 0
		s->setCycle(0);
		s->setCriticalPath(0.0);
		s->setCriticalPathContribution(0.0);
		s->setHasBeenImplemented(true);

		// add the newly created signal to signalMap and signalList
		signalList_.push_back(s);
		signalMap_[s->getName()] = s;

		tmpInPortMap_[componentPortName] = s;
	}


	string Operator::instance(Operator* op, string instanceName, bool isGlobalOperator){
		ostringstream o;

		// TODO add more checks here

		//checking that all the signals are covered
		for(unsigned int i=0; i<op->getIOList()->size(); i++)
		{
			Signal *signal = op->getIOListSignal(i);
			bool isSignalMapped = false;
			map<string, Signal*>::iterator iterStart, iterStop;

			//set the start and stop values for the iterators, for either the input,
			//	or the output temporary port map, depending on the signal type
			if(signal->type() == Signal::in)
			{
				iterStart = tmpInPortMap_.begin();
				iterStop = tmpInPortMap_.end();
			}else
			{
				iterStart = tmpOutPortMap_.begin();
				iterStop = tmpOutPortMap_.end();
			}

			for(map<string, Signal*>::iterator it=iterStart; it!=iterStop; it++)
			{
				if(it->first == signal->getName())
				{
					isSignalMapped = true;
					break;
				}
			}

			if(!isSignalMapped)
				THROWERROR("ERROR in instance() while trying to create a new instance of "
						<< op->getName() << " called " << instanceName << ": input/output "
						<< signal->getName() << " is not mapped to anything" << endl);
		}

		o << tab << instanceName << ": " << op->getName();

		//disabled during the overhaul
		//TODO: is this information still relevant?
		/*
		if(op->isSequential())
			o << "  -- maxInputDelay=" << getMaxInputDelays(op->ioList_);
		*/
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

		//build the code for the inputs
		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++){
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
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++){
			if(!((it == tmpOutPortMap_.begin()) && (tmpInPortMap_.size() != 0)) || op->isSequential())
				o << "," << endl <<  tab << tab << "           ";

			o << it->first << " => " << it->second->getName();
		}

		o << ");" << endl;

		//if this is a global operator, then
		//	check if this is the first time adding this global operator
		//	to the operator list. If it isn't, then insert the copy, instead
		bool newOpFirstAdd = true;

		if(isGlobalOperator)
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

		if(isGlobalOperator)
		{
			//create a new operator
			Operator *newOp = new Operator(op->getTarget());

			//deep copy the original operator
			//	this should also connect the inputs/outputs of the new operator to the right signals
			newOp->deepCloneOperator(op);

			//set a new name for the copy of the operator
			newOp->setName(newOp->getName() + "_cpy_" + vhdlize(getNewUId()));

			//mark the subcomponent as not requiring to be implemented
			newOp->setIsOperatorImplemented(true);

			//save a reference
			opCpy = newOp;

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

		//update the signals to take into account the input and output port maps
		//update the inputs
		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++)
		{
			//add componentPortName as a successor of actualSignalName,
			// and actualSignalName as a predecessor of componentPortName
			addPredecessor(opCpy->getSignalByName(it->first), it->second, 0);
			addSuccessor(it->second, opCpy->getSignalByName(it->first), 0);

			//input ports connected to constant signals do not need scheduling
			if(it->second->type() == Signal::constant)
			{
				opCpy->getSignalByName(it->first)->setCycle(0);
				opCpy->getSignalByName(it->first)->setCriticalPath(0.0);
				opCpy->getSignalByName(it->first)->setCriticalPathContribution(0.0);
				opCpy->getSignalByName(it->first)->setHasBeenImplemented(true);
			}
		}

		//update the outputs
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
		{
			//add componentPortName as a predecessor of actualSignalName,
			// and actualSignalName as a successor of componentPortName
			addSuccessor(opCpy->getSignalByName(it->first), it->second, 0);
			addPredecessor(it->second, opCpy->getSignalByName(it->first), 0);
		}

		//add the operator to the subcomponent list/map
		oplist.push_back(opCpy);
		subComponents_[instanceName] = opCpy;

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

		for(unsigned int i=0; i<oplist.size(); i++)
		{
			//if this is a global operator, then it should be output only once,
			//	and with the name of the global copy, not that of the local copy
			string componentName = oplist[i]->getName();

			if(componentName.find("_cpy_") != string::npos)
			{
				//a global component; only the first copy should be output
				bool isComponentOutput = false;

				componentName = componentName.substr(0, componentName.find("_cpy_"));

				for(unsigned int j=0; j<i; j++)
					if(oplist[j]->getName().find(componentName) != string::npos)
					{
						isComponentOutput = true;
						break;
					}
				if(isComponentOutput)
					continue;
				else
				{
					oplist[i]->outputVHDLComponent(o, componentName);
					o << endl;
				}
			}else
			{
				//just a regular subcomponent
				oplist[i]->outputVHDLComponent(o, componentName);
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
		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++)
			inputDelayMap[it->first] = it->second->getCriticalPath();

		return inputDelayMap;
	}

	map<string, double> Operator::getOutDelayMap(){
		map<string, double> outputDelayMap;

		cerr << "WARNING: this function no longer has the same meaning, due to the overhaul of the pipeline framework;" << endl
				<< tab << "the delay map for the instance being built will be returned" << endl;
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
			outputDelayMap[it->first] = it->second->getCriticalPath();

		return outputDelayMap;
	}

	map<string, int> Operator::getDeclareTable(){
		cerr << "WARNING: function getDeclareTable() is deprecated and no longer has any effect!" << endl
				<< tab << tab << "if you are using this function to build your circuit's pipeline, " << endl
				<< tab << tab << "please NOTE that SYNCHRONIZATION IS NOW IMPLICIT!" << endl;

		map<string, int> emptyMap;

		return emptyMap;
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

		for(map<string, Signal*>::iterator it=tmpInPortMap_.begin(); it!=tmpInPortMap_.end(); it++)
			tmpMap[it->first] = it->second->getName();
		for(map<string, Signal*>::iterator it=tmpOutPortMap_.begin(); it!=tmpOutPortMap_.end(); it++)
			tmpMap[it->first] = it->second->getName();

		return tmpMap;
	}

	map<string, Operator*> Operator::getSubComponents(){
		return subComponents_;
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
		ostringstream newStr;
		string oldStr, workStr;
		int currentPos, nextPos, count;
		int tmpCurrentPos, tmpNextPos, lhsNameLength, rhsNameLength;

		REPORT(DEBUG, "Starting second-level parsing for operator " << srcFileName);

		//reset the new vhdl code buffer
		newStr.str("");

		//set the old code to the vhdl code stored in the FlopocoStream
		//	this also triggers the code's parsing, if necessary
		oldStr = vhdl.str();

		//iterate through the old code, one statement at the time
		// code that doesn't need to be modified: it goes directly to the new vhdl code buffer
		// code that needs to be modified: ?? should be removed from lhs_name, $$ should be removed from rhs_name,
		//		delays of the type rhs_name_xxx should be added for the right-hand side signals
		bool isSelectedAssignment = (oldStr.find('?') > oldStr.find('$'));
		currentPos = 0;
		nextPos = oldStr.find('?');
		while(nextPos != string::npos)
		{
			string lhsName, rhsName;
			Signal *lhsSignal, *rhsSignal;

			//copy the code from the beginning to this position directly to the new vhdl buffer
			newStr << oldStr.substr(currentPos, nextPos-currentPos);

			//now get a new line to parse
			workStr = oldStr.substr(nextPos+2, oldStr.find(';', nextPos)+1-nextPos-2);

			//extract the lhs_name
			if(isSelectedAssignment == true)
			{
				int lhsNameStart, lhsNameStop;

				lhsNameStart = workStr.find('?');
				lhsNameStop  = workStr.find('?', lhsNameStart+2);
				lhsName = workStr.substr(lhsNameStart+2, lhsNameStop-lhsNameStart-2);
			}else
			{
				lhsName = workStr.substr(0, workStr.find('?'));

				//copy lhsName to the new vhdl buffer
				newStr << lhsName;
			}
			lhsNameLength = lhsName.size();

			//check for component instances
			//	the first parse marks the name of the component as a lhsName
			//	must remove the markings and output the code normally
			if(workStr.find("port map") != string::npos)
			{
				newStr << workStr.substr(lhsNameLength+2, workStr.size());

				//get a new line to parse
				currentPos = nextPos + workStr.size() + 2;
				nextPos = oldStr.find('?', currentPos);

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
			lhsSignal = getSignalByName(lhsName);

			//check for selected signal assignments
			//	with signal_name select... etc
			//	the problem is there is a signal belonging to the right hand side
			//	on the left-hand side, before the left hand side signal, which breaks the regular flow
			if(workStr.find("select") != string::npos)
			{
				//extract the first rhs signal name
				tmpCurrentPos = 0;
				tmpNextPos = workStr.find('$');

				rhsName = workStr.substr(tmpCurrentPos, tmpNextPos);

				//this could also be a delayed signal name
				try{
					rhsSignal = getSignalByName(rhsName);
				}catch(string e){
					try{
						rhsSignal = getDelayedSignalByName(rhsName);
						//extract the name
						rhsName = rhsName.substr(0, rhsName.find('^'));
					}catch(string e2){
						THROWERROR("Error in parse2(): signal " << rhsName << " not found:" << e2);
					}
				}catch(...){
					THROWERROR("Error in parse2(): signal " << rhsName << " not found:");
				}

				//output the rhs signal name
				newStr << rhsName;
				if(getTarget()->isPipelined() && (lhsSignal->getCycle()-rhsSignal->getCycle() > 0))
					newStr << "_d" << vhdlize(lhsSignal->getCycle()-rhsSignal->getCycle());

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
			if(workStr.find("select") == string::npos)
			{
				tmpCurrentPos = lhsNameLength+2;
				tmpNextPos = workStr.find('$', lhsNameLength+2);
			}
			while(tmpNextPos != string::npos)
			{
				int cycleDelay;
				//copy into the new vhdl stream the code that doesn't need to be modified
				newStr << workStr.substr(tmpCurrentPos, tmpNextPos-tmpCurrentPos);

				//skip the '$$'
				tmpNextPos += 2;

				//extract a new rhsName
				rhsName = workStr.substr(tmpNextPos, workStr.find('$', tmpNextPos)-tmpNextPos);
				rhsNameLength = rhsName.size();

				//this could also be a delayed signal name
				try{
					rhsSignal = getSignalByName(rhsName);
				}catch(string e){
					try{
						rhsSignal = getDelayedSignalByName(rhsName);
						//extract the name
						rhsName = rhsName.substr(0, rhsName.find('^'));
					}catch(string e2){
						THROWERROR("Error in parse2(): signal " << rhsName << " not found:" << e2);
					}
				}catch(...){
					THROWERROR("Error in parse2(): signal " << rhsName << " not found:");
				}

				//copy the rhsName with the delay information into the new vhdl buffer
				//	rhsName becomes rhsName_dxxx, if the rhsName signal is declared at a previous cycle
				cycleDelay = lhsSignal->getCycle()-rhsSignal->getCycle();
				newStr << rhsName;
				if(getTarget()->isPipelined() && (cycleDelay>0))
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

		//copy the remaining code to the vhdl code buffer
		newStr << oldStr.substr(currentPos, oldStr.size()-currentPos);

		vhdl.setSecondLevelCode(newStr.str());

		REPORT(DEBUG, "Finished second-level parsing for operator " << srcFileName);
	}


	void Operator::extractSignalDependences()
	{
		//start parsing the dependence table, modifying the signals of each triplet
		for(vector<triplet<string, string, int>>::iterator it=vhdl.dependenceTable.begin(); it!=vhdl.dependenceTable.end(); it++)
		{
			Signal *lhs, *rhs;
			int delay;

			lhs = getSignalByName(it->first);
			rhs = getSignalByName(it->second);
			delay = it->third;

			addPredecessor(lhs, rhs, delay);
			addSuccessor(rhs, lhs, delay);
		}
	}


	void Operator::startScheduling()
	{
		//TODO: add more checks here
		//check that all the inputs are at the same cycle
		for(unsigned int i=0; i<ioList_.size(); i++)
			for(unsigned int j=i+1; j<ioList_.size(); j++)
				if((ioList_[i]->type() == Signal::in) && (ioList_[j]->type() == Signal::in) && (ioList_[i]->getCycle() != ioList_[j]->getCycle()))
					THROWERROR("Error in startScheduling(): not all signals are at the same cycle: signal "
							+ ioList_[i]->getName() + " is at cycle " + vhdlize(ioList_[i]->getCycle())
							+ " but signal " + ioList_[j]->getName() + " is at cycle "
							+ vhdlize(ioList_[j]->getCycle()));
		//if the operator is already scheduled, then there is nothing else to do
		if(isOperatorScheduled())
			return;

		//check if this is a global operator
		//	if this is a global operator, and this is the global copy, then schedule it
		//	if this is a global operator, but a local copy, then use the
		//	scheduling of the global copy
		//test if this operator is a global operator
		bool isGlobalOperator = false;
		string globalOperatorName = getName();

		//this might be a copy of the global operator, so try to remove the postfix
		if(globalOperatorName.find("_cpy_") != string::npos)
			globalOperatorName = globalOperatorName.substr(0, globalOperatorName.find("_cpy_"));

		//look in the global operator list for the operator
		for(unsigned int i=0; i<UserInterface::globalOpList.size(); i++)
			if(UserInterface::globalOpList[i]->getName() == globalOperatorName)
			{
				isGlobalOperator = true;
				break;
			}

		//if this is a global operator, and this is a copy, then just copy the schedule
		//	of the global copy
		if(isGlobalOperator && (getName().find("_cpy_") != string::npos))
		{
			Operator* originalOperator;
			int maxInputCycle = 0;

			//search for the global copy of the operator, which should already be scheduled
			for(unsigned int i=0; i<UserInterface::globalOpList.size(); i++)
				if(UserInterface::globalOpList[i]->getName() == globalOperatorName)
				{
					originalOperator = UserInterface::globalOpList[i];
					break;
				}

			//determine the maximum cycle of the inputs
			for(unsigned int i=0; i<ioList_.size(); i++)
				if((ioList_[i]->type() == Signal::in) && (ioList_[i]->getCycle() > maxInputCycle))
					maxInputCycle = ioList_[i]->getCycle();

			//set the timing for all the signals of the operator,
			//	based on the timing of the global copy of the operator
			for(unsigned int i=0; i<signalList_.size(); i++)
			{
				Signal *currentSignal = signalList_[i];

				//for input ports, only update the lifespan
				if(currentSignal->type() == Signal::in)
				{
					currentSignal->updateLifeSpan(originalOperator->getSignalByName(currentSignal->getName())->getLifeSpan());
					continue;
				}

				currentSignal->setCycle(maxInputCycle + originalOperator->getSignalByName(currentSignal->getName())->getCycle());
				currentSignal->setCriticalPath(originalOperator->getSignalByName(currentSignal->getName())->getCriticalPath());
				currentSignal->setCriticalPathContribution(originalOperator->getSignalByName(currentSignal->getName())->getCriticalPathContribution());
				currentSignal->updateLifeSpan(originalOperator->getSignalByName(currentSignal->getName())->getLifeSpan());
				currentSignal->setHasBeenImplemented(true);
			}

			//the operator is now scheduled
			//	mark it as scheduled, and return
			setIsOperatorScheduled(true);

			//start scheduling the signals connected to the output of the sub-component
			for(unsigned int i=0; i<ioList_.size(); i++)
				if(ioList_[i]->type() == Signal::out)
				{
					for(unsigned int j=0; j<ioList_[i]->successors()->size(); j++)
						scheduleSignal(ioList_[i]->successor(j));
				}

			return;
		}

		//extract the dependences between the operator's internal signals
		extractSignalDependences();

		//schedule each of the input signals
		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			Signal *currentSignal = ioList_[i];

			//schedule the current input signal
			if(currentSignal->type() == Signal::in)
				setSignalTiming(currentSignal);
		}

		//start the schedule on the children of the inputs
		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			Signal *currentSignal = ioList_[i];

			if(currentSignal->type() != Signal::in)
				continue;

			for(unsigned j=0; j<currentSignal->successors()->size(); j++)
					scheduleSignal(currentSignal->successor(j));
		}

		setIsOperatorScheduled(true);
	}

	void Operator::scheduleSignal(Signal *targetSignal)
	{
		//TODO: add more checks here
		//check if the signal has already been scheduled
		if(targetSignal->getHasBeenImplemented() == true)
			//there is nothing else to be done
			return;

		//check if all the signal's predecessors have been scheduled
		for(unsigned int i=0; i<targetSignal->predecessors()->size(); i++)
			if(targetSignal->predecessor(i)->getHasBeenImplemented() == false)
				//not all predecessors are scheduled, so thee signal cannot be scheduled
				return;

		//if the preconditions are satisfied, schedule the signal
		setSignalTiming(targetSignal);

		//update the lifespan of targetSignal's predecessors
		for(unsigned int i=0; i<targetSignal->predecessors()->size(); i++)
			targetSignal->predecessor(i)->updateLifeSpan(targetSignal->getCycle() - targetSignal->predecessor(i)->getCycle());

		//check if this is an input signal for a sub-component
		//	it's safe to test if the signal belongs to a sub-component by checking
		//	the signal type because the flow cannot get to this call for the main
		//	component
		if(targetSignal->type() == Signal::in)
		{
			//this is an input for a sub-component
			bool allInputsScheduled = true;

			//check if all the inputs of the operator have been implemented
			for(unsigned int i=0; i<targetSignal->parentOp()->getIOList()->size(); i++)
				if((targetSignal->parentOp()->getIOListSignal(i)->type() == Signal::in)
						&& (targetSignal->parentOp()->getIOListSignal(i)->getHasBeenImplemented() == false))
				{
					allInputsScheduled = false;
					break;
				}

			//check whether all the other inputs of the parent operator of
			//	this signal have been scheduled
			if(allInputsScheduled == false)
				//if not, then we can just stop
				return;

			//all the other inputs of the parent operator of this signal have been scheduled
			//	compute the maximum cycle of the inputs, and then synchronize
			//	the inputs to that cycle, and launch the scheduling for the parent operator of the signal
			int maxCycle = 0;

			//determine the maximum cycle
			for(unsigned int i=0; i<targetSignal->parentOp()->getIOList()->size(); i++)
				if(targetSignal->parentOp()->getIOListSignal(i)->getCycle() > maxCycle)
					maxCycle = targetSignal->parentOp()->getIOListSignal(i)->getCycle();
			//set all the inputs of the parent operator of targetSignal to the maximum cycle
			for(unsigned int i=0; i<targetSignal->parentOp()->getIOList()->size(); i++)
				if((targetSignal->parentOp()->getIOListSignal(i)->type() == Signal::in) &&
					(targetSignal->parentOp()->getIOListSignal(i)->getCycle() < maxCycle))
				{
					//if we have to delay the input, the we need to reset
					//	the critical path, as well
					//else, there is nothing else to do
					targetSignal->parentOp()->getIOListSignal(i)->setCycle(maxCycle);
					targetSignal->parentOp()->getIOListSignal(i)->setCriticalPath(0.0);
				}

			//start the scheduling of the parent operator of targetSignal
			targetSignal->parentOp()->startScheduling();
		}else
		{
			//this is a regular signal inside of the operator

			//try to schedule the successors of the signal
			for(unsigned int i=0; i<targetSignal->successors()->size(); i++)
				scheduleSignal(targetSignal->successor(i));
		}

	}

	void Operator::setSignalTiming(Signal* targetSignal)
	{
		int maxCycle = 0;
		double maxCriticalPath = 0.0, maxTargetCriticalPath;

		//check if the signal has already been scheduled
		if(targetSignal->getHasBeenImplemented() == true)
			//there is nothing else to be done
			return;

		//initialize the maximum cycle and critical path of the predecessors
		if(targetSignal->predecessors()->size() != 0)
		{
			maxCycle = targetSignal->predecessor(0)->getCycle() + targetSignal->predecessorPair(0)->second;
			maxCriticalPath = targetSignal->predecessor(0)->getCriticalPath();
		}

		//determine the maximum cycle and critical path of the signal's parents
		for(unsigned int i=1; i<targetSignal->predecessors()->size(); i++)
		{
			Signal* currentPred = targetSignal->predecessor(i);
			int currentPredCycleDelay = targetSignal->predecessorPair(i)->second;

			//constant signals are not taken into account
			if(currentPred->type() == Signal::constant)
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
		maxTargetCriticalPath = 1.0 / getTarget()->frequency();
		//check if the signal needs to pass to the next cycle,
		//	due to its critical path contribution
		if(maxCriticalPath+targetSignal->getCriticalPathContribution() > maxTargetCriticalPath)
		{
			double totalDelay = maxCriticalPath + targetSignal->getCriticalPathContribution();

			while((totalDelay+getTarget()->ffDelay()) > maxTargetCriticalPath)
			{
				// if maxCriticalPath+criticalPathContribution > 1/frequency, it may insert several pipeline levels.
				// This is what we want to pipeline block-RAMs and DSPs up to the nominal frequency by just passing their overall delay.
				maxCycle++;
				totalDelay -= maxTargetCriticalPath + getTarget()->ffDelay();
			}

			targetSignal->setCycle(maxCycle);
			targetSignal->setCriticalPath(0.0);
		}else
		{
			targetSignal->setCycle(maxCycle);
			targetSignal->setCriticalPath(maxCriticalPath+targetSignal->getCriticalPathContribution());
		}

		targetSignal->setHasBeenImplemented(true);
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
		return (subComponents_.find(s) != subComponents_.end());
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
		subComponents_              = op->getSubComponents();
		//instances_                  = op->getInstances();
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
		vhdl.vhdlCode.str(op->vhdl.vhdlCode.str());
		vhdl.vhdlCodeBuffer.str(op->vhdl.vhdlCodeBuffer.str());

		//vhdl.currentCycle_   = op->vhdl.currentCycle_;
		//vhdl.useTable        = op->vhdl.useTable;

//		vhdl.dependenceTable.clear();
//		vhdl.dependenceTable.insert(vhdl.dependenceTable.begin(), op->vhdl.dependenceTable.begin(), op->vhdl.dependenceTable.end());
		vhdl.dependenceTable = op->vhdl.dependenceTable;

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

		isOperatorImplemented_      = op->isOperatorImplemented();

		resourceEstimate.str(op->resourceEstimate.str());
		resourceEstimateReport.str(op->resourceEstimateReport.str());
		reHelper                    = op->reHelper;
		reActive                    = op->reActive;

		floorplan.str(op->floorplan.str());
		flpHelper                   = op->flpHelper;
	}


	void  Operator::deepCloneOperator(Operator *op){
		//start by coning the operator
		cloneOperator(op);

		//create deep copies of the signals, sub-components, instances etc.

		//create deep copies of the signals
		vector<Signal*> newSignalList;
		for(unsigned int i=0; i<signalList_.size(); i++)
		{
			if((signalList_[i]->type() == Signal::in) || (signalList_[i]->type() == Signal::out))
				continue;

			Signal* tmpSignal = new Signal(this, signalList_[i]);
			newSignalList.push_back(tmpSignal);
		}
		signalList_.clear();
		signalList_.insert(signalList_.begin(), newSignalList.begin(), newSignalList.end());

		//create deep copies of the inputs/outputs
		vector<Signal*> newIOList;
		for(unsigned int i=0; i<ioList_.size(); i++)
		{
			Signal* tmpSignal = new Signal(this, ioList_[i]);
			newIOList.push_back(tmpSignal);
		}
		ioList_.clear();
		ioList_.insert(ioList_.begin(), newIOList.begin(), newIOList.end());
		signalList_.insert(signalList_.end(), newIOList.begin(), newIOList.end());

		//recreate the signal dependences, for each of the signals
		for(unsigned int i=0; i<signalList_.size(); i++)
		{
			vector<pair<Signal*, int>> newPredecessors, newSuccessors;

			//no need to recreate the dependences for the inputs/outputs, as this is done in instance
			if((signalList_[i]->type() == Signal::in) || (signalList_[i]->type() == Signal::out))
				continue;

			//create the new list of predecessors for the signal currently treated
			for(unsigned int j=0; j<op->signalList_[i]->predecessors()->size(); j++)
			{
				pair<Signal*, int> tmpPair = *(op->signalList_[i]->predecessorPair(j));

				//if the signal is connected to the output of a subcomponent,
				//	then just skip this predecessor, as it will be added later on
				if((tmpPair.first->type() == Signal::out)
						&& (tmpPair.first->parentOp()->getName() != op->signalList_[i]->parentOp()->getName()))
					continue;

				newPredecessors.push_back(make_pair(getSignalByName(tmpPair.first->getName()), tmpPair.second));
			}
			//replace the old list of predecessors with the new one
			resetPredecessors(signalList_[i]);
			addPredecessors(signalList_[i], newPredecessors);

			//create the new list of successors for the signal currently treated
			for(unsigned int j=0; j<op->signalList_[i]->successors()->size(); j++)
			{
				pair<Signal*, int> tmpPair = *(op->signalList_[i]->successorPair(j));

				//if the signal is connected to the input of a subcomponent,
				//	then just skip this successor, as it will be added later on
				if((tmpPair.first->type() == Signal::in)
						&& (tmpPair.first->parentOp()->getName() != op->signalList_[i]->parentOp()->getName()))
					continue;

				newSuccessors.push_back(make_pair(getSignalByName(tmpPair.first->getName()), tmpPair.second));
			}
			//replace the old list of predecessors with the new one
			resetSuccessors(signalList_[i]);
			addSuccessors(signalList_[i], newSuccessors);
		}

		//update the signal map
		signalMap_.clear();
		for(unsigned int i=0; i<signalList_.size(); i++)
			signalMap_[signalList_[i]->getName()] = signalList_[i];

		//no need to recreate the signal dependences for each of the input/output signals,
		//	as this is either done by instance, or it is done by the parent operator of this operator
		//	or, they may not need to be set at all (e.g. when you just copy an operator)

		//create deep copies of the subcomponents
		//	and connect their inputs and outputs to the corresponding signals
		vector<Operator*> newOpList;
		for(unsigned int i=0; i<op->getOpList().size(); i++)
		{
			Operator* tmpOp = new Operator(op->getOpList()[i]->getTarget());

			tmpOp->deepCloneOperator(op->getOpList()[i]);

			//connect the inputs/outputs of the subcomponent
			for(unsigned int j=0; j<tmpOp->getIOList()->size(); j++)
			{
				Signal *currentIO = tmpOp->getIOListSignal(j), *originalIO;

				//clear the IO's predecessor and successor list
				resetPredecessors(currentIO);
				resetSuccessors(currentIO);

				//recreate the predecessor and successor list, as it is in
				//	the original operator, which we are cloning
				originalIO = op->getSignalByName(currentIO->getName());
				//recreate the predecessor/successor (for the input/output) list
				for(unsigned int k=0; k<originalIO->predecessors()->size(); k++)
				{
					pair<Signal*, int> tmpPair = *(originalIO->predecessorPair(k));
					if(currentIO->type() == Signal::in)
					{
						addPredecessor(currentIO, getSignalByName(tmpPair.first->getName()), tmpPair.second);
						addSuccessor(getSignalByName(tmpPair.first->getName()), currentIO, tmpPair.second);
					}else if(currentIO->type() == Signal::out)
					{
						addPredecessor(currentIO, tmpOp->getSignalByName(tmpPair.first->getName()), tmpPair.second);
						addSuccessor(tmpOp->getSignalByName(tmpPair.first->getName()), currentIO, tmpPair.second);
					}
				}
				//recreate the successor/predecessor (for the input/output) list
				for(unsigned int k=0; k<originalIO->successors()->size(); k++)
				{
					pair<Signal*, int> tmpPair = *(originalIO->successorPair(k));
					addSuccessor(currentIO, getSignalByName(tmpPair.first->getName()), tmpPair.second);
					addPredecessor(getSignalByName(tmpPair.first->getName()), currentIO, tmpPair.second);
				}
			}

			//add the new subcomponent to the subcomponent list
			newOpList.push_back(tmpOp);
		}
		oplist.clear();
		oplist = newOpList;

		//create deep copies of the instances
		//	replace the references in the instances to references to
		//	the newly created deep copies
		//the port maps for the instances do not need to be modified
		for(map<string, Operator*>::iterator it=subComponents_.begin(); it!=subComponents_.end(); it++)
		{
			string opName = it->first;
			Operator *tmpOp = it->second;

			//look for the instance with the same name
			for(unsigned int j=0; j<oplist.size(); j++)
				if(oplist[j]->getName() == tmpOp->getName())
					subComponents_[opName] = oplist[j];
		}

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


	void Operator::parseVHDL(int parseType)
	{
		if(parseType == 1)
		{
			//trigger the first code parse
			//	call str(), which will trigger the necessary calls
			REPORT(FULL, "  1st PARSING PASS");
			getFlopocoVHDLStream()->str();
		}else
		{
			//trigger the second parse
			//	for sequential and combinatorial operators as well
			REPORT(FULL, "  2nd PARSING PASS");
			parse2();
		}

		//trigger the first code parse for the operator's subcomponents
		for(vector<Operator*>::iterator it=oplist.begin(); it!=oplist.end(); it++)
		{
			(*it)->parseVHDL(parseType);
		}
	}


	void Operator::outputVHDLToFile(vector<Operator*> &oplist, ofstream& file)
	{
		string srcFileName = "Operator.cpp"; // for REPORT

		for(vector<Operator*>::iterator it=oplist.begin(); it!=oplist.end(); it++)
		{
			try {
				// check for subcomponents
				if(! (*it)->getOpListR().empty() ){
					//recursively call to print subcomponents
					outputVHDLToFile((*it)->getOpListR(), file);
				}

				//output the vhdl code to file
				//	for global operators, this is done only once
				if(!(*it)->isOperatorImplemented())
				{
					(*it)->outputVHDL(file);
					(*it)->setIsOperatorImplemented(true);
				}
			} catch (std::string &s) {
					cerr << "Exception while generating '" << (*it)->getName() << "': " << s << endl;
			}
		}
	}




#if 1
	void Operator::outputVHDLToFile(ofstream& file){
		vector<Operator*> oplist;

		REPORT(DEBUG, "Entering outputVHDLToFile");

		//build a copy of the global oplist hidden in Target (if it exists):
		for (unsigned i=0; i<UserInterface::globalOpList.size(); i++)
			oplist.push_back(UserInterface::globalOpList[i]);
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
			for(; it != testMemory.end () && firstEqual ; it++ ){
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


