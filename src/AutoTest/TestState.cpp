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
		testsNumber = 0;
		index = 0;
		testBenchSize = 1000;
	}

	void TestState::changeValue(string name, string value)
	{
		map<string,string>::iterator it = testParam.find(name);

		if(it != testParam.end())
			it->second = value;
	}

	void TestState::nextTest()
	{
		index++;
	}

	void TestState::setTestsNumber(int pTestsNumber)
	{
		testsNumber = pTestsNumber;
	}

	void TestState::setIndex(int pIndex)
	{
		index = pIndex;
	}

	void TestState::setTestBenchSize(int pTestBenchSize)
	{
		testBenchSize = pTestBenchSize;
	}

	int TestState::getIndex()
	{
		return index;
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

	bool TestState::canTest()
	{
		return (index<testsNumber);
	}

	bool TestState::isUnchanged()
	{
		bool unchanged = true;

		map<string,string>::iterator itMap;

		for(itMap = testParam.begin(); itMap != testParam.end(); ++itMap)
		{
			if(itMap->second != "" && testsNumber != 0)
			{
				unchanged = false;
			}
		}

		return unchanged;
	}

	TestState::TestState()
	{
		index = 0;
		testsNumber = 0;
		testBenchSize = 1000;
	}

};