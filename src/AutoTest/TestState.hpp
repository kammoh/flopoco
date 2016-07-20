#ifndef TestState_hpp
#define TestState_hpp

#include <stdio.h>
#include <map>
#include <string>

using namespace std;

namespace flopoco
{

	class TestState
	{
	public:

		/**
		* Add a parameter to the TestState
		* @param name Name of the parameter
		* @param value Value of the parameter
		*/
		void addParam(string name, string value);

		/**
		* Clean the TestState, clear all the parameters and iteration values.
		*/
		void clean();

		/**
		* Change the value of a parameter of the TestState
		* @param name Name of the parameter
		* @param value New value of the paremeter
		*/
		void changeValue(string name, string value);

		/**
		* Increment the iteration index
		*/
		void nextTest();

		/**
		* Set the number of iteration
		* @param pIterationNumber Number of iteration
		*/
		void setTestsNumber(int pTestsNumber);

		void setIndex(int pIndex);

		void setTestBenchSize(int pTestBenchSize);

		/**
		* Get the index of the current iteration
		* @return Index of the current iteration
		*/
		int getIndex();

		int getTestBenchSize();

		/**
		* Get the value of a parameter using its name
		* If the parameter is not present, return "NotFound"
		* @param name Name of the parameter
		* @return Value of the parameter
		*/
		string getValue(string name);

		/**
		* Get the map of parameters
		* @return Map of parameters
		*/
		map<string,string> getMap();

		/**
		* Check if it is possible to iterate
		* @return true or false
		*/
		bool canTest();

		/**
		* Check if the TestState's parameters are all unspecified
		* @return true or false
		*/
		bool isUnchanged();

		/**
		* Constructor of a TestState
		* Set iteration number and index to 0
		* Set the number of tests for the TestBench
		*/
		TestState();

	private:

		/**
		* Map of parameters for this TestState
		*/
		map<string,string> testParam;

		/**
		* Number of iteration for this Operator
		*/
		int testsNumber;

		/**
		* Index of the current iteration
		*/
		int index;

		int testBenchSize;

	};
};

#endif
