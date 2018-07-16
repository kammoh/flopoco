#ifndef FIX_COMPLEX_KCM_HPP
#define FIX_COMPLEX_KCM_HPP
#include <vector>
#include <string>

#include <Operator.hpp>

using namespace std;


/* Radix-2 FFT
   A n=2^k points FFT is represented as a bidimentional array of size (k+1, n);
   each cell represents a signal. The first layer is the input, and the layer
   k is the output.   
 */

namespace flopoco {
	
	/**
	 * @brief a Fix FFT in FloPoCo
	 */	
	class FixFFT : public Operator{
	public:
		/**
		 * @brief The type of a component
		 */
		typedef enum
			{ Trunc, /**< a truncature, top of the butterfly */
			  Exact, /**< an exact multiplication (4th roots of unit) */
			  Cmplx, /**< a complex multiplication (other roots) */
			} CompType;

		/** the size of each bus on a layer */
		typedef vector<pair<int, int>> laySize;

		/** the size of each bus of the FFT */
		typedef vector<laySize> fftSize;
		
		/** the precision required for each bus of the FFT */
		typedef vector<vector<unsigned int>> fftPrec;
		/** the error accumulated from the beginning for each bus of the FFT */
		typedef vector<vector<float>> fftError;

		/**
		 * @brief get the exponant of the twiddle factor of a butterfly
		 * @param[in] layer the layer of the signal
		 * @param[in] signal the line of the signal coming into the butterfly

		 * @return p such as \f$\omega_{2^{\ell}}^{p} \f$ is the twiddle factor
		 * of the butterfly, with \f$\ell\f$ the layer.
		 *
		 * @pre layer < size
		 */
		
		static int getTwiddleExp(int layer, int signal);


		/**
		 * @brief say if the signal enters in the non-multiplied(top) part
		 * of its outcoming Butterfly
		 * @param[in] layer the layer of the signal
		 * @param[in] signal the line of the signal coming into the butterfly
		 * @ret true IFF the signal is the input of the top part of its
		 * outcoming Butterfly
		 */
		static bool isTop(int layer, int signal);
		
		/**
		 * @brief given one output signal of a butterfly, returns its two input
		 * signals 
		 * @param[in] layer the layer of the butterfly's output
		 * @param[in] signal the line of the butterfly's output

		 */

		static pair<int, int> pred(int layer, int signal);
				
		/**
		 * @brief gets the component in which the signal enter
		 * @param[in] layer the layer of the signal
		 * @param[in] signal the line of the signal coming into the butterfly
		 
		 * @ret the component in which the signal enter
		 */

		static CompType getComponent(int layer, int signal);
		
		/**
		 * @brief computes the error made on each signal in an FFT
		 * @param[in] fft the precision of each element (KCM or Truncate)
		 * @return the error made on each element 
	 
		 fft dimensions must be (p, 2**p)
		*/
	
		static pair<fftSize,fftError> calcDim(fftPrec &fft);
	};
}//namespace
#endif
