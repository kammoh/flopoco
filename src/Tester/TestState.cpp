#include "TestState.hpp"

#include <iostream>

namespace flopoco
{

	void TestState::addParam(string name, string value)
	{
		testParam.insert(make_pair(name,value));
	}

	void TestState::clean()
	{
		testParam.clear();
		iterationNumber = 0;
		iterationIndex = 0;
		testBenchSize = 100;
	}

	void TestState::changeValue(string name, string value)
	{
		map<string,string>::iterator it = testParam.find(name);

		it->second = value;
	}

	void TestState::nextIteration()
	{
		iterationIndex++;
	}

	void TestState::setIterationNumber(int pIterationNumber)
	{
		iterationNumber = pIterationNumber;
	}

	void TestState::setIterationIndex(int pIterationIndex)
	{
		iterationIndex = pIterationIndex;
	}

	void TestState::setTestBenchSize(int pTestBenchSize)
	{
		testBenchSize = pTestBenchSize;
	}

	int TestState::getIterationIndex()
	{
		return iterationIndex;
	}

	int TestState::getTestBenchSize()
	{
		return testBenchSize;
	}

	string TestState::getValue(string name)
	{
		return testParam.find(name)->second;
	}

	map<string,string> TestState::getMap()
	{
		return testParam;
	}

	bool TestState::canIterate()
	{
		return (iterationIndex<iterationNumber);
	}

	bool TestState::isUnchanged()
	{
		bool unchanged = true;

		map<string,string>::iterator itMap;

		for(itMap = testParam.begin(); itMap != testParam.end(); ++itMap)
		{
			if(itMap->second != "" && iterationNumber != 0)
			{
				unchanged = false;
			}
		}

		return unchanged;
	}

	TestState::TestState()
	{
		iterationIndex = 0;
		iterationNumber = 0;
		testBenchSize = 100;
	}

};