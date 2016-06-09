#ifndef FIXREALKCM_HPP
#define FIXREALKCM_HPP
#include "../Operator.hpp"
#include "../Table.hpp"

#include "../BitHeap/BitHeap.hpp"

namespace flopoco{


	class FixRealKCMTable;

	class FixRealKCM : public Operator
	{
	public:

		/**
		 * @brief Standalone version of KCM. Input size will be msbIn-lsbIn+1
		 * @param target : target on which we want the KCM to run
		 * @param signedInput : true if input are 2'complement fixed point numbers
		 * @param msbin : power of two associated with input msb. For unsigned 
		 * 				  input, msb weight will be 2^msb, for signed input, it
		 * 				  will be -2^msb
		 * 	@param lsbIn : power of two of input least significant bit
		 * 	@param lsbOut : Weight of the least significant bit of the output
		 * 	@param constant : string that describes the constant in sollya syntax
		 * 	@param targetUlpError : exiged error bound on result. Difference
		 * 							between result and real value should be
		 * 							lesser than targetUlpError * 2^lsbOut.
		 * 							Value has to be in ]0.5 ; 1] (if 0.5 wanted,
		 * 							please consider to create a one bit more
		 * 							precise KCM with a targetUlpError of 1 and
		 * 							truncate the result
		 */
		FixRealKCM(
				Target* target, 
				bool signedInput, 
				int msbIn, 
				int lsbIn, 
				int lsbOut, 
				string constant, 
				double targetUlpError = 1.0, 
				map<string, double> inputDelays = emptyDelayMap
			);

		/**
		 * @brief Incorporated version of KCM. 
		 * @param parentOp : operator frow which the KCM is a subentity
		 * @param target : target on which we want the KCM to run
		 * @param multiplicandX : signal which will be KCM input
		 * @param signedInput : true if input are considered as 2'complemented
		 * 						relative fixed point numbers
		 * 						false if they are considered as positive number
		 * @param msbin : power of two associated with input msb. For unsigned 
		 * 				  input, msb weight will be 2^msb, for signed input, it
		 * 				  will be -2^msb
		 * 	@param lsbIn : power of two of input least significant bit
		 * 	@param lsbOut : desired output precision i.e. output least 
		 * 					significant bit has a weight of 2^lsbOut
		 * 	@param constant : string that describes the constant with sollya
		 * 					  syntax
		 * 	@param bitHeap : bit heap on which the KCM should throw is result
		 * 	@param targetUlpError : exiged error bound on result. Difference
		 * 							between result and real value should be
		 * 							lesser than targetUlpError * 2^lsbOut.
		 * 							Value has to be in ]0.5 ; 1] (if 0.5 wanted,
		 * 							please consider to create a one bit more
		 * 							precise KCM with a targetUlpError of 1 and
		 * 							truncate the result
		 *
		 *  /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
		 * 	WARNING : nothing is added to the bitHeap when constant is set to
		 * 	zero. Your bitHeap must have been fed with another signal despite of
		 * 	what it will not produce an output signal (bitHeap problem)
		 *  /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
		 */
		FixRealKCM(
				Operator* parentOp, 
				Target* target, 
				Signal* multiplicandX, /**<  must be a fixed-point signal */
				int lsbOut, 
				string constant,
				double targetUlpError = 1.0, 
				map<string, double> inputDelays = emptyDelayMap
			);


		/**
		 * @brief handle operator initialisation and constant parsing
		 */
		void init();

		void buildForBitHeap();

		
		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);

		static OperatorPtr parseArguments(
				Target* target,
				vector<string>& args
			);

		static void registerFactory();
		
		bool signedInput;
		int msbIn;
		int lsbIn;
		int msbOut;
		int lsbOut;
		int wOut;
		int lsbInOrig; // for really small constants, we will use fewer bits of the input
		string constant;
		float targetUlpError;
		mpfr_t mpC;
		mpfr_t absC;
		int msbC;
		int g;
		bool negativeConstant;
		bool constantRoundsToZero;
		bool constantIsPowerOfTwo;


		/* The heap of weighted bits that will be used to do the additions */
		BitHeap*	bitHeap;    	

		/* The operator which envelops this constant multiplier */
		Operator*	parentOp;
		
		int numberOfTables;
		vector<int> m; /**< MSB of chunk i; m[0] == msbIn */
		vector<int> l; /**< LSB of chunk i; l[numberOfTables-1] = lsbIn, or maybe >= lsbIn if not all the input bits are used due to a small constant */
		
	};



	class FixRealKCMTable : public Table
	{
		public:
			FixRealKCMTable(
					Target* target, 
					FixRealKCM* mother, 
					int i, 
					int weight, 
					int wIn, 
					int wOut, 
					//if true, table result will be input*C - 1 in order to
					//avoid bit wasting for sign bit
					//e.g. : if cste is -2 and input is 0001 result will be 
					// 11101.
					// For sign extension if output width is 9 we need to add
					// 111100001 and that way we doesn't waste a useless sign bit
					// (as we know that se subproduct sign is the constant sign)
					bool negativeSubproduct, 
					bool last, 
					int logicTable = 1
					);

			mpz_class function(int x);
			FixRealKCM* mother;
			int index;
			//Weight of input lsb
			int weight;
			bool negativeSubproduct;
			bool last;
	};

}


#endif
