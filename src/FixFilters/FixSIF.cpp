#include "FixSIF.hpp"


using namespace std;


namespace flopoco {

	FixSIF::FixSIF(Target* target, string paramFileName_) :
		Operator(target), paramFileName(paramFileName_)
	{
		srcFileName="FixSIF";
		setCopyrightString ("Anastasia Volkova, Matei Istoan (2016)");
		useNumericStd_Unsigned();

		ostringstream name;
		name << "FixSIF";
		setNameWithFreqAndUID( name.str() );

		//TODO: initialize all the parameters of the constructor from the file given as parameter

		//create the u signals
		for(int i=0; i<n_u; i++)
			addInput(join("U_", i), (m_u[i]-l_u[i]+1));
		//create the t signals
		for(int i=0; i<n_t; i++)
			declare(join("T_", i), (m_t[i]-l_t[i]+1));
		//create the x signals
		setCycle(0, true);
		for(int i=0; i<n_x; i++){
			setCycle(0, true);
			declare(join("X_", i, "_next"), (m_x[i]-l_x[i]+1));
			setCycle(1, true);
			declare(join("X_", i), (m_x[i]-l_x[i]+1));
		}
		setCycle(0, true);
		//create the y signals
		for(int i=0; i<n_y; i++)
			addOutput(join("Y_", i), (m_y[i]-l_y[i]+1));

		//the SOPC operator
		FixSOPC* fixSOPC;

		//create vectors of all the sizes of the
		vector<int> msbVector, lsbVector;

		//add the msbs and lsbs of the t signals
		msbVector.insert(msbVector.end(), m_t.begin(), m_t.end());
		lsbVector.insert(msbVector.end(), l_t.begin(), l_t.end());
		//add the msbs and lsbs of the x signals
		msbVector.insert(msbVector.end(), m_x.begin(), m_x.end());
		lsbVector.insert(msbVector.end(), l_x.begin(), l_x.end());
		//add the msbs and lsbs of the u signals
		msbVector.insert(msbVector.end(), m_u.begin(), m_u.end());
		lsbVector.insert(msbVector.end(), l_u.begin(), l_u.end());

		//create the SOPCs computing the T signals
		addComment("---------------  SOPCs for the T signals  ---------------");
		for(int i=0; i<n_t; i++){
			addComment(join("creating the sum of products for T_", i));
			//create the SOPC specific to this t
			fixSOPC = new FixSOPC(getTarget(), msbVector, lsbVector, m_t[i], l_t[i], Z[i], -1); // -1 means: faithful
			addSubComponent(fixSOPC);
			//connect the inputs of the SOPC
			for(int j=0; j<n_t; j++) {
				inPortMap(fixSOPC, join("X",j), join("T_", j));
			}
			for(int j=0; j<n_x; j++) {
				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j, "_next"));
			}
			for(int j=0; j<n_u; j++) {
				inPortMap(fixSOPC, join("X",n_t+n_u+j), join("U_", j));
			}
			//connect the output
			outPortMap(fixSOPC, "R", join("T_", i), false);
			//create the instance
			vhdl << tab << instance(fixSOPC, join("fixSOPC_T_", i)) << endl;
		}
		addComment("---------------------------------------------------------");

		//create the SOPCs computing the X signals
		addComment("---------------  SOPCs for the X signals  ---------------");
		for(int i=0; i<n_x; i++){
			addComment(join("creating the sum of products for X_", i));
			//create the SOPC specific to this t
			fixSOPC = new FixSOPC(getTarget(), msbVector, lsbVector, m_x[i], l_x[i], Z[n_t+i], -1); // -1 means: faithful
			addSubComponent(fixSOPC);
			//connect the inputs of the SOPC
			for(int j=0; j<n_t; j++) {
				inPortMap(fixSOPC, join("X",j), join("T_", j));
			}
			for(int j=0; j<n_x; j++) {
				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j));
			}
			for(int j=0; j<n_u; j++) {
				inPortMap(fixSOPC, join("X",n_t+n_u+j), join("U_", j));
			}
			outPortMap(fixSOPC, "R", join("X_", i, "_next"));
			vhdl << tab << instance(fixSOPC, join("fixSOPC_X_", i)) << endl;

			setCycle(1, true);
			vhdl << tab << "X_" << i << " <= X_" << i << "_next;" << endl;
			setCycle(0), true;
		}
		addComment("---------------------------------------------------------");

		//create the SOPCs computing the X signals
		addComment("---------------  SOPCs for the Y signals  ---------------");
		for(int i=0; i<n_y; i++){
			addComment(join("creating the sum of products for Y_", i));
			//create the SOPC specific to this t
			fixSOPC = new FixSOPC(getTarget(), msbVector, lsbVector, m_x[i], l_x[i], Z[n_t+n_x+i], -1); // -1 means: faithful
			addSubComponent(fixSOPC);
			//connect the inputs of the SOPC
			for(int j=0; j<n_t; j++) {
				inPortMap(fixSOPC, join("X",j), join("T_", j));
			}
			for(int j=0; j<n_x; j++) {
				inPortMap(fixSOPC, join("X",n_t+j), join("X_", j, "_next"));
			}
			for(int j=0; j<n_u; j++) {
				inPortMap(fixSOPC, join("X",n_t+n_u+j), join("U_", j));
			}
			outPortMap(fixSOPC, "R", join("Y_", i), false);
			vhdl << tab << instance(fixSOPC, join("fixSOPC_Y_", i)) << endl;
		}
		addComment("---------------------------------------------------------");



	};



	FixSIF::~FixSIF(){

	};




	void FixSIF::emulate(TestCase * tc){

	};

	void FixSIF::buildStandardTestCases(TestCaseList* tcl){};

	OperatorPtr FixSIF::parseArguments(Target *target, vector<string> &args) {
		string paramFile;
		UserInterface::parseString(args, "paramFileName", &paramFile);

		return new FixSIF(target, paramFile);
	}


	void FixSIF::registerFactory(){
		UserInterface::add("FixSIF", // name
				"A fix-point SIF implementation.",
				"FiltersEtc", // categories
				"",
				"",
				"",
				FixSIF::parseArguments
		) ;
	}

}
