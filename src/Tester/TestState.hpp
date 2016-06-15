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
		* Remove a parameter from the TestState using its name
		* @param name Name of the parameter
		*/
		void removeParam(string name);

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
		void nextIteration();

		/**
		* Set the number of iteration
		* @param pIterationNumber Number of iteration
		*/
		void setIterationNumber(int pIterationNumber);

		/**
		* Set the number of tests for the TestBench
		* @param pTestBenchNumber Number of tests for the TestBench
		*/
		void setTestBenchNumber(int pTestBenchNumber);

		/**
		* Get the number of tests for the TestBench
		* @return Number of tests for the TestBench
		*/
		int getTestBenchNumber();

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
		bool canIterate();

		/**
		* Check if the TestState is empty
		* @return true or false
		*/
		bool isEmpty();

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
		int iterationNumber;

		/**
		* Index of the current iteration
		*/
		int iterationIndex;

		/**
		* Number of tests for the TestBench
		*/
		int testBenchNumber;

	};
};

#endif
