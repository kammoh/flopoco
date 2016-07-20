#include "FixSIF.hpp"


using namespace std;


namespace flopoco {

	FixSIF::FixSIF(Target* target) :
		Operator(target)
	{
		srcFileName="FixSIF";
		setCopyrightString ( "Anastasia Volkova, Matei Istoan (2016)" );
		useNumericStd_Unsigned();


		ostringstream name;
		name << "FixSIF";
		setNameWithFreqAndUID( name.str() );

#if HAVE_WCPG
//			REPORT(INFO, "computing worst-case peak gain");
//			if (!WCPG_tf(&H, coeffb_d, coeffa_d, n, m))
//				THROWERROR("Could not compute WCPG");
//			REPORT(INFO, "Worst-case peak gain is H=" << H);
#else
//			THROWERROR("WCPG was not found (see cmake output), cannot compute worst-case peak gain H. Either provide H, or compile FloPoCo with WCPG");
#endif

	};



	FixSIF::~FixSIF(){

	};




	void FixSIF::emulate(TestCase * tc){

	};

	void FixSIF::buildStandardTestCases(TestCaseList* tcl){};

	OperatorPtr FixSIF::parseArguments(Target *target, vector<string> &args) {

		return new FixSIF(target);
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
