/*
  A generic class for tables of values

  Author : Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.

 */


#include <iostream>
#include <sstream>
#include <cstdlib>
#include "utils.hpp"
#include "Table.hpp"

using namespace std;


namespace flopoco{


	int Table::double2input(double x){
		throw string("Error, double2input is being used and has not been overriden");
	}
	
	double Table::input2double(int x) {
		throw string("Error, input2double is being used and has not been overriden");
	}

	mpz_class Table::double2output(double x){
		throw string("Error, double2output is being used and has not been overriden");
	}

	double Table::output2double(mpz_class x) {
		throw string("Error, output2double is being used and has not been overriden");
	}

#if 0 // TODO some day
	mpz_class Table::mpfr2output(mpfr_t x){
		throw string("Error, mpfr2output is being used and has not been overriden");
	}

	void Table::output2mpfr(mpz_class x, mpfr_t y) {
		throw string("Error, output2mpfr is being used and has not been overriden");
	}
#endif

	mpz_class Table::call_function(int x)
	{
		return function(x);
	}



	Table::Table(Target* target_, int _wIn, int _wOut, int _minIn, int _maxIn, int _logicTable, map<string, double> inputDelays) :
		Operator(target_),
		wIn(_wIn), wOut(_wOut), minIn(_minIn), maxIn(_maxIn)
	{
		srcFileName = "Table";
		THROWERROR("\n\n Deprecated constructor.\n Please replace whatever inheritance you may have with a plain Table.\n See FPDiv for an example. " << endl);
		if(wIn<0){
			THROWERROR("wIn="<<wIn<<"; Input size cannot be negative"<<endl);
		}
		if(wOut<0){
			THROWERROR("wOut="<<wOut<<"; Output size cannot be negative"<<endl);
		}
		setCopyrightString("Florent de Dinechin, Matei Istoan (2007-2016)");

		if(_logicTable==1)
			logicTable=true;
		else if (_logicTable==-1)
			logicTable=false;
		else { // the constructor should decide
			logicTable = (wIn <= getTarget()->lutInputs())  ||  (wOut * (mpz_class(1) << wIn) < 0.5*getTarget()->sizeOfMemoryBlock());
			if(!logicTable)
				REPORT(DETAILED, "This table will be implemented in memory blocks");
		}
		
		// Set up the IO signals
		addInput ("X"  , wIn, true);
		addOutput ("Y"  , wOut, 1, true);

		if(maxIn==-1)
			maxIn=(1<<wIn)-1;
		if(minIn<0) {
			cerr<<"ERROR in Table::Table, minIn<0\n";
			exit(EXIT_FAILURE);
		}
		if(maxIn>=(1<<wIn)) {
			cerr<<"ERROR in Table::Table, maxIn too large\n";
			exit(EXIT_FAILURE);
		}
		if((minIn==0) && (maxIn==(1<<wIn)-1))
			full=true;
		else
			full=false;
		if (wIn > 10)
		  REPORT(0, "WARNING: FloPoCo is building a table with " << wIn << " input bits, it will be large.");
	}


	

	Table::Table(OperatorPtr parentOp_, Target* target_, vector<mpz_class> _values, int _wIn, int _wOut, string _name, int _logicTable, int _minIn, int _maxIn) :
		Operator(parentOp_, target_),
		wIn(_wIn), wOut(_wOut), minIn(_minIn), maxIn(_maxIn), values(_values)
	{
		srcFileName = "Table";
		setNameWithFreqAndUID(_name);
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007-2018)");

		//sanity checks: can't fill the table if there are no values to fill it with
		if(values.size() == 0)
			THROWERROR("Error in table: the set of values to be written in the table is empty" << endl);

		//set wIn
		if(wIn < 0){
			REPORT(DEBUG, "WARNING: wIn value not set, will be inferred from the values which are to be written in the table.");
			//set the value of wIn
			wIn = intlog2(values.size());
		}
		else if(((unsigned)1<<wIn) < values.size()) {
			REPORT(DEBUG, "WARNING: wIn set to a value lower than the number of values which are to be written in the table.");
			//set the value of wIn
			wIn = intlog2(values.size());
		}

		//determine the lowest and highest values stored in the table
		mpz_class maxValue = values[0], minValue = values[0];
		
		//this assumes that the values stored in the values array are all positive
		for(unsigned int i=0; i<values.size(); i++)		{
			if(values[i] < 0)
				THROWERROR("Error in table: value stored in table is negative: " << values[i] << endl);
			if(maxValue < values[i])
				maxValue = values[i];
			if(minValue > values[i])
				minValue = values[i];
		}

		//set wOut
		if(wOut < 0){
			REPORT(DEBUG, "WARNING: wOut value not set, will be inferred from the values which are to be written in the table.");
			//set the value of wOut
			wOut = intlog2(maxValue);
		}
		else if(wOut < intlog2(maxValue))  {
			REPORT(DEBUG, "WARNING: wOut value set to a value lower than the size of the values which are to be written in the table.");
			//set the value of wOut
			wOut = intlog2(maxValue);
		}

		// if this is just a Table
		if(_name == "")
			setNameWithFreqAndUID(srcFileName+"_"+vhdlize(wIn)+"_"+vhdlize(wOut));

		//checks for logic table
		if(_logicTable == 0)
			logicTable = false;
		else if(_logicTable == 1)
			logicTable = true;
		else
			logicTable = (wIn <= getTarget()->lutInputs())  ||  (wOut * (mpz_class(1) << wIn) < 0.5*getTarget()->sizeOfMemoryBlock());

		// Sanity check: the table is built using a RAM, but is underutilized
		if(!logicTable
				&& ((wIn <= getTarget()->lutInputs())  ||  (wOut*(mpz_class(1) << wIn) < 0.5*getTarget()->sizeOfMemoryBlock())))
			REPORT(0, "Warning: the table is built using a RAM block, but is underutilized");

		// Logic tables are shared by default, large tables are unique because they can have a multi-cycle schedule.
		if(logicTable)
			setShared();

		// Set up the IO signals -- this must come after the setShared()
		addInput("X", wIn, true);
		addOutput("Y", wOut, 1, true);

		//set minIn
		if(minIn < 0){
			REPORT(DEBUG, "WARNING: minIn not set, or set to an invalid value; will use the value determined from the values to be written in the table.");
			minIn = 0;
		}
		//set maxIn
		if(maxIn < 0){
			REPORT(DEBUG, "WARNING: maxIn not set, or set to an invalid value; will use the value determined from the values to be written in the table.");
			maxIn = values.size()-1;
		}

		if((1<<wIn) < maxIn){
			cerr << "ERROR: in Table constructor: maxIn too large\n";
			exit(EXIT_FAILURE);
		}

		//determine if this is a full table
		if((minIn==0) && (maxIn==(1<<wIn)-1))
			full=true;
		else
			full=false;

		//user warnings
		if(wIn > 10)
			REPORT(FULL, "WARNING: FloPoCo is building a table with " << wIn << " input bits, it will be large.");


		//create the code for the table
		REPORT(DEBUG,"Table.cpp: Filling the table");
		std::string tableAttributes;

		//set the table attributes
		if(getTarget()->getID() == "Virtex6")
			tableAttributes =  "attribute ram_extract: string;\nattribute ram_style: string;\nattribute ram_extract of Y0: signal is \"yes\";\nattribute ram_style of Y0: signal is ";
		else if(getTarget()->getID() == "Virtex5")
			tableAttributes =  "attribute rom_extract: string;\nattribute rom_style: string;\nattribute rom_extract of Y0: signal is \"yes\";\nattribute rom_style of Y0: signal is ";
		else
			tableAttributes =  "attribute ram_extract: string;\nattribute ram_style: string;\nattribute ram_extract of Y0: signal is \"yes\";\nattribute ram_style of Y0: signal is ";

		if((logicTable == 1) || (wIn <= getTarget()->lutInputs())){
			//logic
			if(getTarget()->getID() == "Virtex6")
				tableAttributes += "\"pipe_distributed\";";
			else
				tableAttributes += "\"distributed\";";
		}else{
			//block RAM
			tableAttributes += "\"block\";";
		}

		if(logicTable){
			int lutsPerBit;
			if(wIn < getTarget()->lutInputs())
				lutsPerBit = 1;
			else
				lutsPerBit = 1 << (wIn-getTarget()->lutInputs());
			REPORT(DETAILED, "Building a logic table that uses " << lutsPerBit << " LUTs per output bit");
		}
		cpDelay = getTarget()->tableDelay(wIn, wOut, logicTable);

		
		vhdl << tab << "with X select " << declare(cpDelay, "Y0", wOut) << " <= " << endl;;
		getSignalByName("Y0") -> setTableAttributes(tableAttributes);
		
		for(unsigned int i=minIn.get_ui(); i<=maxIn.get_ui(); i++)
			vhdl << tab << tab << "\"" << unsignedBinary(values[i-minIn.get_ui()], wOut) << "\" when \"" << unsignedBinary(i, wIn) << "\"," << endl;
		vhdl << tab << tab << "\"";
		for(int i=0; i<wOut; i++)
			vhdl << "-";
		vhdl <<  "\" when others;" << endl;

		if((!logicTable) && (cpDelay < (double)(1.0/getTarget()->frequency())))
		{
			REPORT(FULL,"Warning: delaying output by 1 cycle to allow implementation as Block RAM" << endl);
			vhdl << tab << "Y <= " << delay("Y0", 1) << ";" << endl;
		}
		else
			vhdl << tab << "Y <= Y0;" << endl;
	}




	Table::Table(OperatorPtr parentOp, Target* target) :
		Operator(parentOp, target){
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007, 2018)");
	}


	int Table::size_in_LUTs() {
		return wOut*int(intpow2(wIn-getTarget()->lutInputs()));
	}

	//FOR TEST PURPOSES ONLY
	mpz_class Table::function(int x){
		return 0;
	}


	//FOR TEST PURPOSES ONLY
	OperatorPtr Table::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int wIn_;
		int wOut_;
		int logicTable_;
		vector<mpz_class> values_;

		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn_, false);
		UserInterface::parseStrictlyPositiveInt(args, "wOut", &wOut_, false);
		UserInterface::parseInt(args, "logicTable", &logicTable_, false);

		for(int i=0; i<(1<<wIn_); i++){
			mpz_class tmpMPZ = mpz_class(random()) * mpz_class(random()) * mpz_class(random()) * mpz_class(random()) * mpz_class(random());
			values_.push_back( (tmpMPZ*tmpMPZ+mpz_class(random())) % (mpz_class(1)<<wOut_) );
		}

		return new Table(parentOp, target, values_, wIn_, wOut_, "", logicTable_);
	}

	//FOR TEST PURPOSES ONLY
	void Table::registerFactory(){
		UserInterface::add("Table", // name
				"A generic table. FOR TEST PURPOSES ONLY.",
				"Miscellaneous", // category
				"",
				"wIn(int): input width;\
				wOut(int): output width;\
				logicTable(int): logic-based/RAM-based table",
				"",
				Table::parseArguments
		);

	}

}
