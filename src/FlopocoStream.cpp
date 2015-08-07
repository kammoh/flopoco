/**
  FloPoCo Stream for VHDL code.
  Once parsed, it will include timing information (c.f. header for more details).

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon

  Authors :   Bogdan Pasca, Nicolas Brunie

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.
*/


#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>
#include <cstdlib>
#include <gmpxx.h>
#include "utils.hpp"

#include "FlopocoStream.hpp"


using namespace std;

namespace flopoco{

	/**
	 * The FlopocoStream class constructor
	 */
	FlopocoStream::FlopocoStream(){
		vhdlCode.str("");
		vhdlCodeBuffer.str("");
		dependenceTable.clear();
		disabledParsing = false;
		codeParsed = false;
	}


	FlopocoStream::~FlopocoStream(){
	}


	string FlopocoStream::str(){
		if(!codeParsed)
			flush();

		return vhdlCode.str();
	}

	string FlopocoStream::str(string UNUSED(s) ){
		vhdlCode.str("");
		vhdlCodeBuffer.str("");
		dependenceTable.clear();
		codeParsed = false;
		return "";
	}


	void FlopocoStream::flush(){
		ostringstream bufferCode;

		/* parse the buffer if it is not empty */
		if(vhdlCodeBuffer.str() != string(""))
		{
			/* scan the code buffer and build the dependence table and annotate the code */
			bufferCode.str("");
			bufferCode << parseCode();

			/* fix the dependence table, to the correct content type */
			cleanupDependenceTable();

			/* the newly processed code is appended to the existing one */
			vhdlCode << bufferCode.str();
		}
	}


	string FlopocoStream::parseCode()
	{
		ostringstream vhdlO;
		istringstream in(vhdlCodeBuffer.str());

		//instantiate the flex++ object  for lexing the buffer info
		LexerContext* lexer = new LexerContext(&in, &vhdlO);

		//call the FlexLexer++ on the buffer. The lexing output is
		//	in the variable vhdlO. Additionally, a temporary table containing
		//	the triplets <lhsName, rhsName, delay> is created
		lexer->lex();

		//the temporary table is used to update the member of FlopocoStream
		updateDependenceTable(lexer->dependenceTable);

		//set the flag for code parsing and reset the vhdl code buffer
		codeParsed = true;
		vhdlCodeBuffer.str("");

		//the annotated string is returned
		return vhdlO.str();
	}


	void FlopocoStream::updateDependenceTable(vector<triplet<string, string, int>> tmpDependenceTable){
		vector<triplet<string, string, int>>::iterator iter;

		for(iter = tmpDependenceTable.begin(); iter!=tmpDependenceTable.end();++iter){
			dependenceTable.push_back(make_triplet(iter->first, iter->second, iter->third));
		}
	}


	void FlopocoStream::setSecondLevelCode(string code){
		vhdlCodeBuffer.str("");
		vhdlCode.str("");
		vhdlCode << code;
		codeParsed = true;
	}


	vector<triplet<string, string, int>> FlopocoStream::getDependenceTable(){
		return dependenceTable;
	}


	void FlopocoStream::disableParsing(bool s){
		disabledParsing = s;
	}


	bool FlopocoStream::isParsing(){
		return !disabledParsing;
	}


	bool FlopocoStream::isEmpty(){
		return (((vhdlCode.str()).length() == 0) && ((vhdlCodeBuffer.str()).length() == 0));
	}


	void FlopocoStream::cleanupDependenceTable()
	{
		vector<triplet<string, string, int>> newDependenceTable;

		for(int i=0; (unsigned)i<dependenceTable.size(); i++)
		{
			string lhsName = dependenceTable[i].first;
			string rhsName = dependenceTable[i].second;
			string newRhsName;
			int rhsDelay = 0;

			//search for dependence edges where the right-hand side can be
			//	a delayed signal. Delayed signals are of the form
			//	ID_Name^cycle_delays
			//the signal name must be extracted and when a new edge is added,
			//	it should be added with the corresponding delay
			if(rhsName.find("^") != string::npos)
			{
				//this is a delayed signal
				newRhsName = rhsName.substr(0, rhsName.find("^"));
				rhsDelay = (int)strtol((rhsName.substr(rhsName.find("^")+1, string::npos)).c_str(), NULL, 10);
			}else
			{
				//this is a regular signal name
				//	nothing to be done
				newRhsName = rhsName;
			}

			//search for dependence edges where the left-hand side can be
			//	of the form (ID_Name_1, ID_Name_2, ..., ID_Name_n)
			//	and split it into several triplets
			if(lhsName.find("(") || lhsName.find(")") || lhsName.find(",") || lhsName.find("\t") || lhsName.find("\n") || lhsName.find(" "))
			{
				//split the lhsName into several names,
				//	without any separating characters (,\n\t\ ())
				string delimiters = " \t\n,()";
				ostringstream newLhsName;
				int count;

				count = 0;
				while((unsigned)count<lhsName.size())
				{
					//initialize the new lhs name
					newLhsName.str("");
					//skip characters as long as they are delimiters
					while((delimiters.find(lhsName[count]) != string::npos)
							&& ((unsigned)count<lhsName.size()))
						count++;
					//add the characters as long as they are not delimiters
					while((delimiters.find(lhsName[count]) == string::npos)
							&& ((unsigned)count<lhsName.size()))
					{
						newLhsName << lhsName[count];
						count++;
					}

					newDependenceTable.push_back(make_triplet(newLhsName.str(), newRhsName, rhsDelay));
				}
			}else
			{
				newDependenceTable.push_back(make_triplet(lhsName, newRhsName, rhsDelay));
			}
		}

		//clear the old values in the dependence table
		dependenceTable.clear();

		//add the new values to the dependence table
		for(unsigned int i=0; i<newDependenceTable.size(); i++)
		{
			dependenceTable.push_back(newDependenceTable[i]);
		}
	}

}













