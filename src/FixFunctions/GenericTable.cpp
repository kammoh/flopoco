/*
  This file is part of the FloPoCo project

  Author : Florent de Dinechin, Matei Istoan

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2014.

  All rights reserved.
*/

#include "GenericTable.hpp"


using namespace std;

namespace flopoco{

	// A generic table, filled with values passed to the constructor
	GenericTable::GenericTable(Target* target_, int wIn_, int wOut_, vector<mpz_class> values_, bool logicTable_) :
			Table(target_, wIn_, wOut_, 0, values_.size()-1, logicTable_), values(values_)
	{
		ostringstream name;

		srcFileName = "GenericTable";
		name << "GenericTable_" << wIn << "_" << wOut;
		setName(name.str());
	}

	GenericTable::~GenericTable() {}

	mpz_class GenericTable::function(int x)
	{
		if((unsigned)x < values.size())
			return  values[x];
		else
			return 0;
	}

}
