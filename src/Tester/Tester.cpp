#include "Tester.hpp"
#include "TestState.hpp"
#include "UserInterface.hpp"

#include <vector>
#include <map>
#include <set>

#include <iostream>

namespace flopoco
{

	Tester::Tester(string opName, bool testDependences)
	{
		cout << "Call to make" << endl;
		int status = system("./initTest.sh");

		if(status == 0)
		{
			cout << "Make succeeded" << endl;
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
					testedOperator.insert(opFact->name());
				}

				// Test the first important TestState for each Operator before testing all the TestStates
				for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
				{
					cout << "Testing first tests for Operator : " << (*itOperator) << endl;
					opFact = UserInterface::getFactoryByName(*itOperator);
					paramNames = opFact->param_names();

				// Fetch all parameters and default values for readability
					for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
					{
						currentTestState->addParam(*itParam,opFact->getDefaultParamVal(*itParam));
					}
					currentTestState->addParam("n","100");

					opFact->nextTestStateGenerator(currentTestState);

				// Is the TestState is unchanged, meaning the NextState method is not implemented
					if(!currentTestState->isUnchanged())
					{
						while(currentTestState->getIterationIndex()<importantTestStateNumber)
						{
							// Get the next state and create the flopoco command corresponding
							commandLine = "./testScript.sh " + (*itOperator);

						// Get the map containing the parameters
							testStateParam = currentTestState->getMap();

							for(itMap = testStateParam.begin(); itMap != testStateParam.end(); itMap++)
							{
								if(itMap->first=="n")
								{
									commandLineTestBench = " TestBench " + itMap->first + "=" + itMap->second;
								}
								else
									commandLine += " " + itMap->first + "=" + itMap->second;
							}

							system((commandLine + commandLineTestBench).c_str());
							currentTestState->nextIteration();
							if(currentTestState->canIterate())
								opFact->nextTestStateGenerator(currentTestState);
						}
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
				cout << "Testing Operator : " << (*itOperator) << endl;
				opFact = UserInterface::getFactoryByName(*itOperator);
				paramNames = opFact->param_names();

				// Fetch all parameters and default values for readability
				for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
				{
					currentTestState->addParam(*itParam,opFact->getDefaultParamVal(*itParam));
				}
				currentTestState->addParam("n","100");

				opFact->nextTestStateGenerator(currentTestState);

				// Is the TestState is unchanged, meaning the NextState method is not implemented
				if(!currentTestState->isUnchanged())
				{
					while(currentTestState->canIterate())
					{
						// Get the next state and create the flopoco command corresponding
						commandLine = "./testScript.sh " + (*itOperator);

						// Get the map containing the parameters
						testStateParam = currentTestState->getMap();

						for(itMap = testStateParam.begin(); itMap != testStateParam.end(); itMap++)
						{
							if(itMap->first=="n")
							{
								commandLineTestBench = " TestBench " + itMap->first + "=" + itMap->second;
							}
							else
								commandLine += " " + itMap->first + "=" + itMap->second;
						}

						system((commandLine + commandLineTestBench).c_str());
						currentTestState->nextIteration();
						if(currentTestState->canIterate())
							opFact->nextTestStateGenerator(currentTestState);
					}
				}
				else
				{
					cout << "No nextTestState method defined" << endl;
				}

				currentTestState->clean();
			}
			cout << "Tests are finished" << endl;
			delete currentTestState;
			// Clean all temporary file
			system("./cleanTest.sh");
		}
		else
		{
			cout << "Make failed" << endl;
		}
	}
};
