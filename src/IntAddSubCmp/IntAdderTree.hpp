#ifndef INTADDERTREE_HPP
#define INTADDERTREE_HPP
#include "../Operator.hpp"
#include "../BitHeap/BitheapNew.hpp"



namespace flopoco{

	class IntAdderTree : public Operator
	{
	public:

		/**
		 * @brief 
		 */
		IntAdderTree(
								 OperatorPtr parentOp,
								 Target* target, 
								 int wIn,
								 int n,
								 bool signedInput
								 );
		

		void emulate(TestCase* tc);

		static OperatorPtr parseArguments(OperatorPtr parentOp, Target* target, vector<string>& args			);

		static TestList unitTest(int index);

		static void registerFactory();
		

	private:
		int wIn;
		int n;
		bool signedInput;
		int wOut;
	};

}


#endif
