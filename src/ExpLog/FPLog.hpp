#ifndef __FPLOG_HPP
#define __FPLOG_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Table.hpp"


namespace flopoco{

#define MAXRRSTAGES 2000 // 4000 bits of accuracy should be enough for anybody

	class FPLog : public Operator
	{
	protected:

		class FirstInvTable : public Table
		{
		public:
			FirstInvTable(Target* target, int p1, int w1);
			~FirstInvTable();
			mpz_class function(int x);
			//  int check_accuracy(int wF);
			int    double2input(double x);
			double input2double(int x);
			mpz_class double2output(double x);
			double output2double(mpz_class x);
			double maxMulOut;
			double minMulOut;
		};

		class FirstLogTable : public Table
		{
		public:
			FirstLogTable(Target *target, int p1, int w1,  FirstInvTable* fit, FPLog* op_);
			~FirstLogTable();
			FirstInvTable* fit;
			mpz_class function(int x);
			int    double2input(double x);
			double input2double(int x);
			mpz_class double2output(double x);
			double output2double(mpz_class x);
		private:
			FPLog* op;
		};


		
	public:
		FPLog(Target* target, int wE, int wF);
		~FPLog();

		//		Overloading the virtual functions of Operator
		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		/**Overloading the function of Operator with a function that tests only positive FP numbers (full range)*/
		TestCase* buildRandomTestCase(int i);
		// User-interface stuff
		/** Factory method */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);
		static void registerFactory();

		/** Create the next TestState based on the previous TestState */
		static void nextTestState(TestState * previousTestState);

		int wE, wF;

	};
}
#endif
