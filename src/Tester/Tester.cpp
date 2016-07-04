#include "Tester.hpp"
#include "TestState.hpp"
#include "UserInterface.hpp"

#include <vector>
#include <map>
#include <set>

#include <iostream>

namespace flopoco
{

	OperatorPtr Tester::parseArguments(Target *target , vector<string> &args)
	{
		string opName;
		bool testDependences;
		UserInterface::parseBoolean(args, "Dependences", &testDependences);
		UserInterface::parseString(args, "Operator", &opName);

		Tester tester(opName,testDependences);

		return nullptr;
	}

	void Tester::registerFactory()
	{
		UserInterface::add("AutoTest", // name
			"A tester for operators.",
			"Tester",
			"", //seeAlso
			"Operator(string)=All: name of the operator to test, All if we need to test all the operators;\
			Dependences(bool)=false: test the operator's dependences;",
			"",
			Tester::parseArguments
			) ;
	}

	Tester::Tester(string opName, bool testDependences)
	{
		system("src/Tester/initTests.sh");

		OperatorFactoryPtr opFact;	
		TestState * currentTestState = new TestState();
		string commandLine;
		string commandLineTestBench;
		set<string> testedOperator;
		set<string>::iterator itOperator;
		vector<string> paramNames;
		vector<string>::iterator itParam;
		map<string,string> testStateParam;
		map<string,string>::iterator itMap;

			// Test all the operators ?
		if(opName == "All")
		{

			int importantTestStateNumber = 5;

				// We get the operators' names to add them in the testedOperator set
			unsigned nbFactory = UserInterface::getFactoryCount();

			for (unsigned i=0; i<nbFactory ; i++)
			{
				opFact = UserInterface::getFactoryByIndex(i);
				if(opFact->name() != "AutoTest")
					testedOperator.insert(opFact->name());
			}

				// Test the first important TestState for each Operator before testing all the TestStates
			for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
			{
				system(("src/Tester/initOpTest.sh " + (*itOperator)).c_str());
				cout << "Testing first tests for Operator : " << (*itOperator) << endl;
				opFact = UserInterface::getFactoryByName(*itOperator);
				paramNames = opFact->param_names();

				// Fetch all parameters and default values for readability
				for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
				{
					currentTestState->addParam(*itParam,opFact->getDefaultParamVal(*itParam));
				}

				opFact->nextTestStateGenerator(currentTestState);

				// Is the TestState is unchanged, meaning the NextState method is not implemented
				if(!currentTestState->isUnchanged())
				{
					while(currentTestState->getIterationIndex()<importantTestStateNumber)
					{
							// Get the next state and create the flopoco command corresponding
						commandLine = "src/Tester/testScript.sh " + (*itOperator);

						// Get the map containing the parameters
						testStateParam = currentTestState->getMap();

						for(itMap = testStateParam.begin(); itMap != testStateParam.end(); itMap++)
						{
							commandLine += " " + itMap->first + "=" + itMap->second;
						}

						commandLineTestBench = " TestBench n=" + to_string(currentTestState->getTestBenchSize());

						system((commandLine + commandLineTestBench).c_str());
						currentTestState->nextIteration();
						if(currentTestState->canIterate())
							opFact->nextTestStateGenerator(currentTestState);
					}

						// Clean all temporary file
					system(("src/Tester/cleanTest.sh " + (*itOperator)).c_str());
				}
				else
				{
					cout << "No nextTestState method defined" << endl;
				}

				currentTestState->clean();
			}

			currentTestState->setIterationIndex(importantTestStateNumber + 1 );
		}
		else
		{
			opFact = UserInterface::getFactoryByName(opName);
			testedOperator.insert(opFact->name());

				// Do we check for dependences ?
			if(testDependences)		
			{
					// Find all the dependences of the Operator opName
					// Add dependences to the set testedOperator
					// Then we do the same on all Operator in the set testedOperator

				unsigned nbFactory = UserInterface::getFactoryCount();
				set<string> allOperator;

				for (unsigned i=0; i<nbFactory ; i++)
				{
					opFact = UserInterface::getFactoryByIndex(i);
					allOperator.insert(opFact->name());
				}

				for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
				{

					FILE * in;
					char buff[512];
					bool success = true;

						// string grepCommand = "grep '" + *itOperator + "\.o' CMakeFiles/FloPoCoLib.dir/depend.make | awk -F/ '{print $NF}' | awk -F. '{print $1}' | grep ^.*$";

						// Command to get the name of the Operator using the depend file of CMake
					string grepCommand = "grep '" + *itOperator + "\.hpp' CMakeFiles/FloPoCoLib.dir/depend.make | grep -o '.*\.o' | awk -F/ '{print $NF}' | awk -F. '{print $1}'";

					if(!(in = popen(grepCommand.c_str(), "r")))
					{
						success = false;
					}

					if(success)
					{
						while(fgets(buff, sizeof(buff), in) != NULL )
						{
							string opFile = string(buff);
							opFile.erase(opFile.find("\n"));
							if(allOperator.find(opFile) != allOperator.end())
							{
								testedOperator.insert(opFile);
							}
						}
						pclose(in);
					}
				}
			}
		}

			// For each tested Operator, we run a number of tests defined by the Operator		
		for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
		{
			system(("src/Tester/initOpTest.sh " + (*itOperator)).c_str());
			cout << "Testing Operator : " << (*itOperator) << endl;
			opFact = UserInterface::getFactoryByName(*itOperator);
			paramNames = opFact->param_names();

				// Fetch all parameters and default values for readability
			for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
			{
				currentTestState->addParam(*itParam,opFact->getDefaultParamVal(*itParam));
			}

			opFact->nextTestStateGenerator(currentTestState);

				// Is the TestState is unchanged, meaning the NextState method is not implemented
			if(!currentTestState->isUnchanged())
			{
				while(currentTestState->canIterate())
				{
						// Get the next state and create the flopoco command corresponding
					commandLine = "src/Tester/testScript.sh " + (*itOperator);

						// Get the map containing the parameters
					testStateParam = currentTestState->getMap();

					for(itMap = testStateParam.begin(); itMap != testStateParam.end(); itMap++)
					{
						commandLine += " " + itMap->first + "=" + itMap->second;
					}
					commandLineTestBench = " TestBench n=" + to_string(currentTestState->getTestBenchSize());

					system((commandLine + commandLineTestBench).c_str());						
					currentTestState->nextIteration();
					if(currentTestState->canIterate())
						opFact->nextTestStateGenerator(currentTestState);
				}

					// Clean all temporary file
				system(("src/Tester/cleanTest.sh " + (*itOperator)).c_str());
			}
			else
			{
				cout << "No nextTestState method defined" << endl;
			}

			currentTestState->clean();
		}
		cout << "Tests are finished" << endl;
		delete currentTestState;
		exit(EXIT_SUCCESS);
	}
};
