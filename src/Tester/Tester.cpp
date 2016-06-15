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
			set<string> testedOperator;
			set<string>::iterator itOperator;
			vector<string> paramNames;
			vector<string>::iterator itParam;
			map<string,string> testStateParam;
			map<string,string>::iterator itMap;

			// Test all the operators ?
			if(opName == "All")
			{
				// We get the operators' names
				unsigned nbFactory = UserInterface::getFactoryCount();

				for (unsigned i=0; i<nbFactory ; i++)
				{
					opFact = UserInterface::getFactoryByIndex(i);
					testedOperator.insert(opFact->name());
				} 
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
			for(itOperator = testedOperator.begin(); itOperator != testedOperator.end(); ++itOperator)
			{
				cout << "Testing Operator : " << (*itOperator) << endl;
				opFact = UserInterface::getFactoryByName(*itOperator);
				paramNames = opFact->param_names();
				currentTestState->clean();
				opFact->nextTestStateGenerator(currentTestState);

				// Is the TestState empty, meaning the NextState method is not implemented
				if(!currentTestState->isEmpty())
				{
					while(currentTestState->canIterate())
					{
						commandLine = "./testScript.sh " + (*itOperator);

					// Fetch the default values for readability
						for(itParam = paramNames.begin(); itParam != paramNames.end(); ++itParam)
						{
							if(currentTestState->getValue(*itParam) == "NotFound")
							{
								currentTestState->addParam(*itParam,opFact->getDefaultParamVal(*itParam));
							}
						}

						// Get the map containing the parameters
						testStateParam = currentTestState->getMap();

						for(itMap = testStateParam.begin(); itMap != testStateParam.end(); itMap++)
						{
							commandLine += " " + itMap->first + "=" + itMap->second;
						}

						commandLine += " TestBench n=" + to_string(currentTestState->getTestBenchNumber());

						system(commandLine.c_str());
						currentTestState->nextIteration();
					}
				}
				else
				{
					cout << "No nextTestState method defined" << endl;
				}
			}
			cout << "Tests are finished" << endl;
		}
		else
		{
			cout << "Make failed" << endl;
		}
	}
};
