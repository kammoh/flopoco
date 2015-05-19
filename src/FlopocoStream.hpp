#ifndef FlopocoStream_HPP
#define FlopocoStream_HPP
#include <vector>
#include <sstream>
#include <utility>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include <string>

//#include "VHDLLexer.hpp"
#include "LexerContext.hpp"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

namespace flopoco{


	/** 
	 * The FlopocoStream class.
	 * Segments of code having the same pipeline informations are scanned 
	 * on-the-fly using flex++ to find the VHDL signal IDs. The found IDs
	 * are augmented with pipeline depth information __IDName__PipelineDepth__
	 * for example __X__2__.
	 */
	class FlopocoStream{
	
		/* Methods for overloading the output operator available on streams */
		/*friend FlopocoStream& operator <= (FlopocoStream& output, string s) {

			output.vhdlCodeBuffer << " <= " << s;
			return output;
		}*/


		template <class paramType> friend FlopocoStream& operator <<(FlopocoStream& output, paramType c) {
			output.vhdlCode << c;
			output.codeParsed = false;
			return output;
		}

		
		friend FlopocoStream & operator<<(FlopocoStream& output, FlopocoStream fs) {
			output.vhdlCode << fs.str();
			output.codeParsed = false;
			return output; 
		}
		

		friend FlopocoStream& operator<<( FlopocoStream& output, UNUSED(ostream& (*f)(ostream& fs)) ){
			output.vhdlCode << std::endl;
			output.codeParsed = false;
			return output;
		}
		
		public:
			/**
			 * FlopocoStream constructor. 
			 * Initializes the two streams: vhdlCode and vhdlCodeBuffer
			 */
			FlopocoStream();

			/**
			 * FlopocoStream destructor
			 */
			~FlopocoStream();

			/**
			 * method that does the similar thing as str() does on ostringstream objects.
			 * Processing is done using the vhdl code buffer (vhdlCode).
			 * The output code is = newly transformed code;
			 * The transformation on vhdlCode is done when the buffer is
			 * flushed 
			 * @return the augmented string encapsulated by FlopocoStream  
			 */
			string str();

			/** 
			 * Resets both the code stream.
			 * @return returns empty string for compatibility issues.
			 */ 
			string str(string UNUSED(s) );
			
			/** 
			 * Function used to flush the buffer
			 * 	- save the code in the temporary buffer
			 * 	- parse the code and build the dependenceTree and annotate the code
			 */ 
			void flush();
			
			/**
			 * Parse the VHDL code in the buffer.
			 * Extract the dependencies between the signals.
			 * Annotate the signal names, for the second phase. All signals on the right-hand side of signal assignments
			 * will be transformed from signal_name to @signal_name@.
			 * At the end of lines with annotated signal names, the name of the left-hand side signal is written, after the semi-colon:
			 * @@lhs_name@@.
			 * @return the string containing the parsed and annotated VHDL code
			 */
			string parseCode();

			/**
			 * The dependenceTable created by the lexer is used
			 * to update a dependenceTable contained locally as a member variable of
			 * the FlopocoStream class
			 * @param[in] tmpDependenceTable a vector of pairs which will be copied
			 *            into the member variable dependenceTable
			 */
			void updateDependenceTable(vector<pair<string, string> > tmpDependenceTable);

			/**
			 * Member function used to set the code resulted after a second parsing
			 * was performed
			 * @param[in] code the 2nd parse level code 
			 */
			void setSecondLevelCode(string code);
			
			/**
			 * Returns the dependenceTable
			 */  
			vector<pair<string, string> > getDependenceTable();


			void disableParsing(bool s);
			
			bool isParsing();
			
			bool isEmpty();

			/**
			 * The dependence table should contain pairs of the form (lhsName, RhsName),
			 * where lhsName and rhsName are the names of the left-hand side and right-hand side
			 * of an assignment.
			 * Because of the parsing stage, lhsName might be of the form (lhsName1, lhsName2, ...),
			 * which must be fixed.
			 */
			void cleanupDependenceTable();


			ostringstream vhdlCode;              			/**< the vhdl code */

			vector<pair<string, string>> dependenceTable;	/**< table containing the left hand side- right hand side dependences */

		protected:
		
			bool disabledParsing;
			bool codeParsed;
	};
}
#endif
