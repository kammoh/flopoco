#ifndef FIX_COMPLEX_KCM_HPP
#define FIX_COMPLEX_KCM_HPP
#include <vector>
#include <string>

namespace flopoco {

	/**
	 * @brief a Fix FFT in FloPoCo
	 */	
	class FixFFT{
	public:
		/**
		 * @brief The type of a component
		 */
		typedef enum
			{ Trunc, /**< a truncature, top of the butterfly */
			  Exact, /**< an exact multiplication (4th roots of unit) */
			  Cmplx, /**< a complex multiplication (other roots) */
			} CompType;
		
		typedef std::vector<std::vector<std::pair<int, int>>> desc;
		typedef std::vector<std::vector<unsigned int>> prec;
		typedef std::vector<std::vector<float>> approx;

		/**
		 * @brief get the exponant of the twiddle factor of a butterfly
		 * @param[in] signal the line of the signal coming into the butterfly
		 * @param[in] layer the layer of the signal

		 * @return p such as \f$\omega_{2^{\ell}}^{p} \f$ is the twiddle factor
		 * of the butterfly, with \f$\ell\f$ the layer.
		 *
		 * @pre layer < size
		 */
		
		static int getTwiddleExp(int signal, int layer);

		/**
		 * @brief gets the component in which the signal enter
		 * @param[in] signal the line of the signal coming into the butterfly
		 * @param[in] layer the layer of the signal
		 
		 * @ret the component in which the signal enter
		 */

		static CompType getComponent(int signal, int layer);
		
		/**
		 * @brief computes the error made on each signal in an FFT
		 * @param[in] fft the precision of each element (KCM or Truncate)
		 * @return the error made on each element 
	 
		 fft dimensions must be (2**p, p)
		*/
	
		static approx calcErr(prec fft);
	};
}//namespace
#endif

