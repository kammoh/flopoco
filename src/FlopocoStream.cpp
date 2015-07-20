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
		dependenceTable.clear();
		disabledParsing = false;
		codeParsed = false;
	}


	FlopocoStream::~FlopocoStream(){
	}


	string FlopocoStream::str(){

		if(!codeParsed)
		{
			flush();
		}
		return vhdlCode.str();
	}

	string FlopocoStream::str(string UNUSED(s) ){
		vhdlCode.str("");
		dependenceTable.clear();
		codeParsed = false;
		return "";
	}


	void FlopocoStream::flush(){
		ostringstream bufferCode;

		/* parse the buffer if it is not empty */
		if(vhdlCode.str() != string(""))
		{
			/* scan the code buffer and build the dependence table and annotate the code */
			bufferCode << parseCode();

			/* fix the dependence table, to the correct content type */
			cleanupDependenceTable();

			/* the newly processed code is appended to the existing one */
			vhdlCode.str(bufferCode.str());
		}
	}


	string FlopocoStream::parseCode()
	{
		ostringstream vhdlO;
		istringstream in(vhdlCode.str());

		/*
		 * instantiate the flex++ object  for lexing the buffer info
		 */
		LexerContext* lexer = new LexerContext(&in, &vhdlO);

		/*
		 * call the FlexLexer ++ on the buffer. The lexing output is
		 * in the variable vhdlO. Additionally, a temporary table containing
		 * the tuples <lhsName, rhsName> is created
		 */
		lexer->lex();

		/*
		 * the temporary table is used to update the member of the FlopocoStream
		 * class dependenceTable
		 */
		updateDependenceTable(lexer->dependenceTable);

		/*
		 * set the flag for code parsing
		 */
		codeParsed = true;

		/* the annotated string is returned */
		return vhdlO.str();
	}


	void FlopocoStream::updateDependenceTable(vector<triplet<string, string, int>> tmpDependenceTable){
		vector<triplet<string, string, int>>::iterator iter;

		for (iter = tmpDependenceTable.begin(); iter!=tmpDependenceTable.end();++iter){
			triplet<string, string, int> tmp;

			tmp.first  = (*iter).first;
			tmp.second = (*iter).second;
			tmp.third  = (*iter).third;
			dependenceTable.push_back(tmp);
		}
	}


	void FlopocoStream::setSecondLevelCode(string code){
		vhdlCode.str("");
		vhdlCode << code;
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
		return ((vhdlCode.str()).length() == 0);
	}


	void FlopocoStream::cleanupDependenceTable()
	{
		vector<triplet<string, string, int>> newDependenceTable;

		for(int i=0; (unsigned)i<dependenceTable.size(); i++)
		{
			string lhsName = dependenceTable[i].first;
			string rhsName = dependenceTable[i].second;
			string newRhsName;
			int rhsDelay = -1;

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
			}
			else
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
				while(count<lhsName.size())
				{
					newLhsName.str("");
					while(delimiters.find(lhsName[count]) != string::npos)
						count++;
					while((delimiters.find(lhsName[count]) == string::npos)
							&& (count<lhsName.size()))
					{
						newLhsName << lhsName[count];
						count++;
					}

					triplet<string, string, int> tmpTriplet;
					tmpTriplet.first  = newLhsName.str();
					tmpTriplet.second = newRhsName;
					tmpTriplet.third  = (rhsDelay == -1) ? 0 : rhsDelay;
					newDependenceTable.push_back(tmpTriplet);
				}
			}else
			{
				triplet<string, string, int> tmpTriplet;
				tmpTriplet.first  = lhsName;
				tmpTriplet.second = newRhsName;
				tmpTriplet.third  = (rhsDelay == -1) ? 0 : rhsDelay;
				newDependenceTable.push_back(tmpTriplet);
			}
		}

		dependenceTable.clear();

		for(unsigned int i=0; i<newDependenceTable.size(); i++)
		{
			dependenceTable.push_back(newDependenceTable[i]);
		}
	}

}













