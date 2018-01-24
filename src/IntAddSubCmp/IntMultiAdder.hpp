#ifndef INTMULTIADDER_HPP
#define INTMULTIADDER_HPP
#include "../Operator.hpp"
#include "../BitHeap/BitheapNew.hpp"



namespace flopoco{

	class IntMultiAdder : public Operator
	{
	public:

		/**
		 * @brief 
		 */
		IntMultiAdder(
								 OperatorPtr parentOp,
								 Target* target, 
								 unsigned int wIn,
								 unsigned int n,
								 bool signedInput,
								 unsigned int wOut=0
								 );
		

		void emulate(TestCase* tc);

		static OperatorPtr parseArguments(OperatorPtr parentOp, Target* target, vector<string>& args			);

		static TestList unitTest(int index);

		static void registerFactory();
		

	private:
		unsigned int wIn;
		unsigned int n;
		bool signedInput;
		unsigned int wOut;
	};

}


#endif
