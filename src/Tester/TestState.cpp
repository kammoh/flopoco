#include "TestState.hpp"

namespace flopoco
{

	void TestState::addParam(string name, string value)
	{
		testParam.insert(make_pair(name,value));
	}

	void TestState::removeParam(string name)
	{
		testParam.erase(name);
	}

	void TestState::clean()
	{
		testParam.clear();
		iterationNumber = 0;
		iterationIndex = 0;
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

	void TestState::setTestBenchNumber(int pTestBenchNumber)
	{
		testBenchNumber = pTestBenchNumber;
	}

	int TestState::getTestBenchNumber()
	{
		return testBenchNumber;
	}

	string TestState::getValue(string name)
	{
		if(testParam.find(name) != testParam.end())
			return testParam.find(name)->second;
		else
			return "NotFound";
	}

	map<string,string> TestState::getMap()
	{
		return testParam;
	}

	bool TestState::canIterate()
	{
		return (iterationIndex<iterationNumber);
	}

	bool TestState::isEmpty()
	{
		return testParam.empty();
	}

	TestState::TestState()
	{
		iterationIndex = 0;
		iterationNumber = 0;
		testBenchNumber = 100;
	}

};