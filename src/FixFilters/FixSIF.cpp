#include "FixSIF.hpp"


using namespace std;


namespace flopoco {

	FixSIF::FixSIF(Target* target, string paramFileName_, string testsFileName_) :
		Operator(target), paramFileName(paramFileName_), testsFileName(testsFileName_)
	{
		srcFileName="FixSIF";
		setCopyrightString ("Anastasia Volkova, Matei Istoan (2016)");
		useNumericStd_Unsigned();

		ostringstream name;
		name << "FixSIF";
		setNameWithFreqAndUID( name.str() );

		//parse the coefficients for the SIF from the parameter file
		parseCoeffFile();

		//deactivate parsing
		setCombinatorial();
		target->setPipelined(false);

		//create the u signals
		for(int i=0; i<n_u; i++)
			addInput(join("U_", i), (m_u[i]-l_u[i]+1));
		//create the t signals
		for(int i=0; i<n_t; i++)
			declare(join("T_", i), (m_t[i]-l_t[i]+1));
		//create the x signals
		//activate parsing
		setSequential();
		setCycle(0, false);
		for(int i=0; i<n_x; i++){
			setCycle(0, false);
			declare(join("X_", i, "_next"), (m_x[i]-l_x[i]+1), true, Signal::registeredWithAsyncReset);
			//create a temporary signal for X
//			declare(join("X_", i, "_int"), (m_x[i]-l_x[i]+1));
			setCycle(1, false);
			declare(join("X_", i), (m_x[i]-l_x[i]+1));
		}
		setCycle(0, false);
		//deactivate parsing
		setCombinatorial();
		//create the y signals
		for(int i=0; i<n_y; i++)
			addOutput(join("Y_", i), (m_y[i]-l_y[i]+1));

		//the SOPC operator
		FixSOPC* fixSOPC;

		//create vectors of all the sizes of the
		vector<int> msbVector, lsbVector;

		//add the msbs and lsbs of the t signals
		msbVector.insert(msbVector.end(), m_t.begin(), m_t.end());
		lsbVector.insert(lsbVector.end(), l_t.begin(), l_t.end());
		//add the msbs and lsbs of the x signals
		msbVector.insert(msbVector.end(), m_x.begin(), m_x.end());
		lsbVector.insert(lsbVector.end(), l_x.begin(), l_x.end());
		//add the msbs and lsbs of the u signals
		msbVector.insert(msbVector.end(), m_u.begin(), m_u.end());
		lsbVector.insert(lsbVector.end(), l_u.begin(), l_u.end());

		//create the SOPCs computing the T signals
		vhdl << endl;
		addComment("---------------  SOPCs for the T signals  ---------------");
		for(int i=0; i<n_t; i++){
			addComment(join("creating the sum of products for T_", i));
			//create the SOPC specific to this t
			fixSOPC = new FixSOPC(getTarget(), msbVector, lsbVector, m_t[i], l_t[i], Z[i], -1); // -1 means: faithful
			fixSOPC->setCombinatorial();
			addSubComponent(fixSOPC);
			//connect the inputs of the SOPC
			for(int j=0; j<n_t; j++) {
				inPortMap(fixSOPC, join("X",j), join("T_", j));
			}
			for(int j=0; j<n_x; j++) {
//				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j, "_int"));
				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j));
			}
			for(int j=0; j<n_u; j++) {
				inPortMap(fixSOPC, join("X",n_t+n_x+j), join("U_", j));
			}
			//connect the output
			outPortMap(fixSOPC, "R", join("T_", i), false);
			//create the instance
			vhdl << instance(fixSOPC, join("fixSOPC_T_", i)) << endl;
		}
		addComment("---------------------------------------------------------");
		vhdl << endl;

		//create the SOPCs computing the X signals
		vhdl << endl;
		addComment("---------------  SOPCs for the X signals  ---------------");
		for(int i=0; i<n_x; i++){
			addComment(join("creating the sum of products for X_", i));
			//create the SOPC specific to this x
			fixSOPC = new FixSOPC(getTarget(), msbVector, lsbVector, m_x[i], l_x[i], Z[n_t+i], -1); // -1 means: faithful
			fixSOPC->setCombinatorial();
			addSubComponent(fixSOPC);
			//connect the inputs of the SOPC
			for(int j=0; j<n_t; j++) {
				inPortMap(fixSOPC, join("X",j), join("T_", j));
			}
			for(int j=0; j<n_x; j++) {
//				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j, "_int"));
				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j));
			}
			for(int j=0; j<n_u; j++) {
				inPortMap(fixSOPC, join("X",n_t+n_x+j), join("U_", j));
			}
			outPortMap(fixSOPC, "R", join("X_", i, "_next"), false);
			vhdl << instance(fixSOPC, join("fixSOPC_X_", i)) << endl;

			//activate parsing
			setSequential();
//			vhdl << tab << "X_" << i << "_int <= X_" << i << ";" << endl;
			setCycle(1, true);
			vhdl << tab << "X_" << i << " <= X_" << i << "_next;" << endl;
			setCycle(0, true);
			//deactivate parsing
			setCombinatorial();
			vhdl << endl;
		}
		addComment("---------------------------------------------------------");
		vhdl << endl;

		//create the SOPCs computing the Y signals
		vhdl << endl;
		addComment("---------------  SOPCs for the Y signals  ---------------");
		for(int i=0; i<n_y; i++){
			addComment(join("creating the sum of products for Y_", i));
			//compute the max. abs error
			// 	parse the coeffs from the string, with Sollya parsing
			sollya_obj_t node;
			mpfr_t err_y_parsed;
			double err_y_parsed_d;

			node = sollya_lib_parse_string(err_y[i].c_str());
			// 	If conversion did not succeed (i.e. parse error)
			if(node == 0)
				THROWERROR(srcFileName << ": Unable to parse string " << err_y[i] << " as a numeric constant");

			mpfr_init2(err_y_parsed, 10000);
			sollya_lib_get_constant(err_y_parsed, node);
			sollya_lib_clear_obj(node);
			err_y_parsed_d = mpfr_get_d(err_y_parsed, GMP_RNDN);
			mpfr_clear(err_y_parsed);
			//create the SOPC specific to this y
			fixSOPC = new FixSOPC(getTarget(), msbVector, lsbVector, m_y[i], l_y[i], Z[n_t+n_x+i], -1, err_y_parsed_d); // -1 means: faithful
			fixSOPC->setCombinatorial();
			addSubComponent(fixSOPC);
			//connect the inputs of the SOPC
			for(int j=0; j<n_t; j++) {
				inPortMap(fixSOPC, join("X",j), join("T_", j));
			}
			for(int j=0; j<n_x; j++) {
//				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j, "_int"));
				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j));
			}
			for(int j=0; j<n_u; j++) {
				inPortMap(fixSOPC, join("X",n_t+n_x+j), join("U_", j));
			}
			outPortMap(fixSOPC, "R", join("Y_", i), false);
			vhdl << instance(fixSOPC, join("fixSOPC_Y_", i)) << endl;
		}
		addComment("---------------------------------------------------------");
		vhdl << endl;


		//activate parsing
		setSequential();
		target->setPipelined(true);

		//set the operator as having only one cycle
		setCycle(0);
		setPipelineDepth(0);

	};



	FixSIF::~FixSIF(){

	};



	void FixSIF::parseCoeffFile()
	{
		ifstream file(paramFileName);
		string line, tempStr;

		if(file.is_open())
		{
			//read n_u
			if(getline(file, line).rdstate() != ios::goodbit){
				THROWERROR("Error: parseCoeffFile: error while reading the value of n_u");
			}else{
				n_u = stoi(line);
			}
			//read n_t
			if(getline(file, line).rdstate() != ios::goodbit){
				THROWERROR("Error: parseCoeffFile: error while reading the value of n_t");
			}else{
				n_t = stoi(line);
			}
			//read n_x
			if(getline(file, line).rdstate() != ios::goodbit){
				THROWERROR("Error: parseCoeffFile: error while reading the value of n_x");
			}else{
				n_x = stoi(line);
			}
			//read n_y
			if(getline(file, line).rdstate() != ios::goodbit){
				THROWERROR("Error: parseCoeffFile: error while reading the value of n_y");
			}else{
				n_y = stoi(line);
			}
			//read the msb and lsb vector for u
			for(int i=0; i<n_u; i++)
			{
				//read a line of the matrix
				if(getline(file, line).rdstate() != ios::goodbit){
					THROWERROR("Error: parseCoeffFile: error while reading the " <<
							"values of the msb and lsb of u index " << i);
				}
				//extract the msb and lsb
				m_u.push_back(stoi(line.substr(0, line.find(' '))));
				l_u.push_back(stoi(line.substr(line.find(' ')+1, line.size()-line.find(' ')-1)));
			}
			//read the msb and lsb vector for t
			for(int i=0; i<n_t; i++)
			{
				//read a line of the matrix
				if(getline(file, line).rdstate() != ios::goodbit){
					THROWERROR("Error: parseCoeffFile: error while reading the " <<
							"values of the msb and lsb of t index " << i);
				}
				//extract the msb and lsb
				m_t.push_back(stoi(line.substr(0, line.find(' '))));
				l_t.push_back(stoi(line.substr(line.find(' ')+1, line.size()-line.find(' ')-1)));
			}
			//read the msb and lsb vector for x
			for(int i=0; i<n_x; i++)
			{
				//read a line of the matrix
				if(getline(file, line).rdstate() != ios::goodbit){
					THROWERROR("Error: parseCoeffFile: error while reading the " <<
							"values of the msb and lsb of x index " << i);
				}
				//extract the msb and lsb
				m_x.push_back(stoi(line.substr(0, line.find(' '))));
				l_x.push_back(stoi(line.substr(line.find(' ')+1, line.size()-line.find(' ')-1)));
			}
			//read the msb and lsb vector for y
			//	and their corresponding maximum error
			for(int i=0; i<n_y; i++)
			{
				//read a line of the matrix
				if(getline(file, line).rdstate() != ios::goodbit){
					THROWERROR("Error: parseCoeffFile: error while reading the " <<
							"values of the msb and lsb of y index " << i);
				}
				//extract the msb and lsb
				m_y.push_back(stoi(line.substr(0, line.find(' '))));
				l_y.push_back(stoi(line.substr(line.find(' ')+1, line.size()-line.find(' ')-1)));
				//extract the error
				err_y.push_back(line.substr(line.rfind(' ')+1, line.size()-line.rfind(' ')-1));
			}
			//read the matrix Z
			for(int i=0; i<n_t+n_x+n_y; i++)
			{
				//read a line of the matrix
				if(getline(file, line).rdstate() != ios::goodbit){
					THROWERROR("Error: parseCoeffFile: error while reading the " <<
							"values of the Z matrix index " << i);
				}
				//split it to a vector of strings
				vector<string> matLine;
				stringstream ss(line);

				while(ss >> tempStr)
				{
					matLine.push_back(tempStr);
				}
				Z.push_back(matLine);
			}

			file.close();
		}else{
			THROWERROR("Error: FixSIF: unable to open the parameter file " << paramFileName << "!");
		}
	}




	void FixSIF::emulate(TestCase * tc){
		THROWERROR("Error: emulate has not yet been implemented for the SIF operator. " <<
				"For now, there is only support for pre-generated tests.");
	};

	void FixSIF::buildStandardTestCases(TestCaseList* tcl){
		int nbTests = 0;

		if(testsFileName == ""){
			THROWERROR("Error: no test file has been specified");
		}else{
			ifstream file(testsFileName);
			string line, tempStr;

			if(file.is_open())
			{
				//read the number of tests in the file
				if(getline(file, line).rdstate() != ios::goodbit){
					THROWERROR("Error: parseCoeffFile: error while reading the value of n_x");
				}else{
					nbTests = stoi(line);
				}
				for(int i=0; i<nbTests; i++)
				{
					vector<string> testInputVector;
					mpfr_t* testInputsMpfr;
					vector<string> testOutputVector;
					mpfr_t* testOutputsMpfr;

					testInputsMpfr = (mpfr_t*)malloc(n_u*sizeof(mpfr_t));
					testOutputsMpfr = (mpfr_t*)malloc(n_u*sizeof(mpfr_t));
					//read the inputs
					if(getline(file, line).rdstate() != ios::goodbit){
						THROWERROR("Error: buildStandardTestCases: error while reading the values of the inputs from the tests file");
					}
					//split the line into a vector of values
					for(int j=0; j<n_u; j++)
					{
						//split it to a vector of strings
						stringstream ss(line);

						while(ss >> tempStr)
						{
							testInputVector.push_back(tempStr);
						}
					}
					//parse the inputs with Sollya
					for(int j=0; j<n_u; j++)
					{
						// parse the coeffs from the string, with Sollya parsing
						sollya_obj_t node;

						node = sollya_lib_parse_string(testInputVector[j].c_str());
						// If conversion did not succeed (i.e. parse error)
						if(node == 0)
							THROWERROR(srcFileName << ": Unable to parse string " << testInputVector[j] << " as a numeric constant");

						mpfr_init2(testInputsMpfr[j], 10000);
						sollya_lib_get_constant(testInputsMpfr[j], node);
						sollya_lib_clear_obj(node);
					}

					//read the outputs
					if(getline(file, line).rdstate() != ios::goodbit){
						THROWERROR("Error: buildStandardTestCases: error while reading the values of the outputs from the tests file");
					}
					//split the line into a vector of values
					for(int j=0; j<n_y; j++)
					{
						//split it to a vector of strings
						stringstream ss(line);

						while(ss >> tempStr)
						{
							testOutputVector.push_back(tempStr);
						}
					}
					//parse the outputs with Sollya
					for(int j=0; j<n_y; j++)
					{
						// parse the coeffs from the string, with Sollya parsing
						sollya_obj_t node;

						node = sollya_lib_parse_string(testOutputVector[j].c_str());
						// If conversion did not succeed (i.e. parse error)
						if(node == 0)
							THROWERROR(srcFileName << ": Unable to parse string " << testOutputVector[j] << " as a numeric constant");

						mpfr_init2(testOutputsMpfr[j], 10000);
						sollya_lib_get_constant(testOutputsMpfr[j], node);
						sollya_lib_clear_obj(node);
					}

					//create the test
					TestCase *tc;

					//create the test case
					tc = new TestCase(this);
					//create the inputs
					for(int j=0; j<n_u; j++){
						mpfr_t tmpMpfr;
						mpz_class tmpMpz;

						//create a temporary variable
						mpfr_init2(tmpMpfr, 10000);
						//initialize the variable to the value of the current input
						mpfr_set(tmpMpfr, testInputsMpfr[j], GMP_RNDN);
						//convert the variable to an integer
						mpfr_mul_2si(tmpMpfr, tmpMpfr, -l_u[j], GMP_RNDN);
						//convert the mpfr to an mpz
						mpfr_get_z (tmpMpz.get_mpz_t(), tmpMpfr, GMP_RNDN);
						tmpMpz = signedToBitVector(tmpMpz, 1+m_u[j]-l_u[j]);

						tc->addInput(join("U_", j), tmpMpz);
					}
					//create the outputs
					for(int j=0; j<n_y; j++){
						mpfr_t tmpMpfr;
						mpz_class tmpMpz_u, tmpMpz_d;

						//create a temporary variable
						mpfr_init2(tmpMpfr, 10000);
						//initialize the variable to the value of the current input
						mpfr_set(tmpMpfr, testOutputsMpfr[j], GMP_RNDN);
						//convert the variable to an integer
						mpfr_mul_2si(tmpMpfr, tmpMpfr, -l_y[j], GMP_RNDN);
						//convert the mpfr to an mpz
						//	rounded up and down
						mpfr_get_z (tmpMpz_u.get_mpz_t(), tmpMpfr, GMP_RNDU);
						tmpMpz_u = signedToBitVector(tmpMpz_u, 1+m_y[j]-l_y[j]);
						mpfr_get_z (tmpMpz_d.get_mpz_t(), tmpMpfr, GMP_RNDD);
						tmpMpz_d = signedToBitVector(tmpMpz_d, 1+m_y[j]-l_y[j]);

						tc->addExpectedOutput(join("Y_", j), tmpMpz_u);
						tc->addExpectedOutput(join("Y_", j), tmpMpz_d);
					}
					//add the test
					tcl->add(tc);

					//mpfr cleanup
					for(int j=0; j<n_u; j++){
						mpfr_clear(testInputsMpfr[j]);
					}
					for(int j=0; j<n_y; j++){
						mpfr_clear(testOutputsMpfr[j]);
					}
					free(testInputsMpfr);
					free(testOutputsMpfr);
				}
				file.close();
			}else{
				THROWERROR("Error: failed to open the tests file " << testsFileName);
			}
		}


	};



	OperatorPtr FixSIF::parseArguments(Target *target, vector<string> &args) {
		string paramFile;
		UserInterface::parseString(args, "paramFileName", &paramFile);
		string testsFile;
		UserInterface::parseString(args, "testsFileName", &testsFile);

		return new FixSIF(target, paramFile, testsFile);
	}



	void FixSIF::registerFactory(){
		UserInterface::add("FixSIF", // name
				"A fix-point SIF implementation.",
				"FiltersEtc", // categories
				"",
				"paramFileName(string): the name of the file containing the parameters for the SIF;\
				testsFileName(string)=\"\": the name of the file containing the tests for the SIF",
				"",
				FixSIF::parseArguments
		) ;
	}

}
