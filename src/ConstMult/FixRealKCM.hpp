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
							 double targetUlpError = 1.0
							 );

		/**
		 * @brief Incorporated version of KCM. 
		 * @param parentOp : operator frow which the KCM is a subentity
		 * @param multiplicandX : signal which will be KCM input (must be a fixed-point signal)
		 * @param lsbOut : desired output precision i.e. output least 
		 * 					significant bit has a weight of 2^lsbOut
		 * @param constant : string that describes the constant with sollya
		 * 					  syntax
		 * @param targetUlpError : exiged error bound on result. Difference
		 * 							between result and real value should be
		 * 							lesser than targetUlpError * 2^lsbOut.
		 * 							Value has to be in ]0.5 ; 1] (if 0.5 wanted,
		 * 							please consider to create a one bit more
		 * 							precise KCM with a targetUlpError of 1 and
		 * 							truncate the result
		 * @param g : if -1, do a dry run to compute g (for internal use only): g is computed but no vhdl is generated.
     *            otherwise take g as it is given (do not compute it). Bit 0 of the bit heap has weight lsbOut-g, and the bit heap must be large enough.
		 *
		 *  /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
		 * 	WARNING : nothing is added to the bitHeap when constant is set to
		 * 	zero. Your bitHeap must have been fed with another signal despite of
		 * 	what it will not produce an output signal (bitHeap problem)
		 *  /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
		 */
		FixRealKCM(
							 Operator* parentOp, 
							 string multiplicandX,
							 bool signedInput,
							 int msbIn,
							 int lsbIn,
							 int lsbOut, 
							 string constant,
							 bool addRoundBit,
							 double targetUlpError = 1.0
							 );

		/** return the computed number of guard bits */
		int getGuardBits();
		
		
		/**
		 * @brief handle operator initialisation, constant parsing, computation of the needed guard bits
		 */
		void init();




		/**
		 * @brief do the actual VHDL generation for the bitHeap KCM.
		 */
		void addToBitHeap(BitHeap* bitHeap, int g);



		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);

		static OperatorPtr parseArguments(
				Target* target,
				vector<string>& args
			);

		static void registerFactory();
		
		Operator*	parentOp; 		/**< The operator which envelops this constant multiplier */
		bool signedInput;
		bool signedOutput; /**< computed: true if the constant is negative or the input is signed */
		int msbIn;
		int lsbIn;
		int msbOut;
		int lsbOut;
		int lsbInOrig; /**< currently always equal to lsbIn, but for really small constants, we could use fewer bits of the input */
		string constant;
		float targetUlpError;
		bool addRoundBit; /**< If false, do not add the round bit to the bit heap: somebody else will */ 
		mpfr_t mpC;
		mpfr_t absC;
		int msbC;
		int g;
		bool negativeConstant;
		bool constantRoundsToZero;
		bool constantIsPowerOfTwo;
		float errorInUlps; /**< These are ulps at position lsbOut-g. 0 if the KCM is exact, 0.5 if it has one table, more if there are more tables. computed by init(). */



		/** The heap of weighted bits that will be used to do the additions */
		BitHeap*	bitHeap;    	

		/** The input signal. */
		string inputSignalName;
		
		int numberOfTables;
		vector<int> m; /**< MSB of chunk i; m[0] == msbIn */
		vector<int> l; /**< LSB of chunk i; l[numberOfTables-1] = lsbIn, or maybe >= lsbIn if not all the input bits are used due to a small constant */


	private:
		bool specialCasesForBitHeap();
		void buildTablesForBitHeap();

	};



	class FixRealKCMTable : public Table
	{
		public:
			FixRealKCMTable(
					Target* target, 
					FixRealKCM* mother, 
					int i
					);

			mpz_class function(int x);
			FixRealKCM* mother;
			int index;
			//Weight of input lsb
			int lsbInWeight;
			bool negativeSubproduct;
	};

}


#endif
