#define BOOST_TEST_DYN_LINK   
#define BOOST_TEST_MODULE NumberFormatTest 

#include <boost/test/unit_test.hpp>
#include <gmpxx.h>
#include "TestBenches/PositNumber.hpp"
#ifdef SOFTPOSIT
#include "softposit.h"
#endif

using namespace flopoco;

BOOST_AUTO_TEST_CASE(PositTestIntConvertTwoWaysNonZeroES) {
	mpz_class z;	
	int width = 12;
	int eS = 3;
	PositNumber p(width, eS);
	for (int i = 0 ; i < (1 << width) ; ++i) {
		z = i;
		p = z;
		auto ret = p.getSignalValue();
		BOOST_REQUIRE_MESSAGE(
				ret == z, 
				"PositTestIntConvertTwoWays:: error for idx " << i << "\n value is " <<
				ret << "(" << ret.get_str(2) << ")"
			);
	}
}

BOOST_AUTO_TEST_CASE(PositTestIntConvertTwoWaysZeroES) {
	mpz_class z;	
	int width = 12;
	int eS = 0;
	PositNumber p(width, eS);
	for (int i = 0 ; i < 1 << width ; ++i) {
		z = i;
		p = z;
		auto ret = p.getSignalValue();
		BOOST_REQUIRE_MESSAGE(
				ret == z, 
				"PositTestIntConvertTwoWays:: error for idx " << i << "\n value is " <<
				ret << "(" << ret.get_str(2) << ")"
			);
	}
}

#ifdef SOFTPOSIT
BOOST_AUTO_TEST_CASE(PositTestMPFRConvertTwoWaysZeroES) {
	posit8_t pos8;
	PositNumber p(8, 0);
	uint8_t counter = 0;
	mpfr_t mpstore;
	mpfr_init2(mpstore, 53);
	do {
		pos8 = castP8(counter);
		double pos8val = convertP8ToDouble(pos8);
		//Tetsing int -> mpfr
		mpz_class tmp(counter);
		p = tmp;
		p.getMPFR(mpstore);
		double flopocoPositVal = mpfr_get_d(mpstore, MPFR_RNDN);
		BOOST_REQUIRE_MESSAGE(flopocoPositVal == pos8val, "Error for value " << 
				(int)(counter)); 
		
		//Testing mpfr -> int
		p = pos8val;
		auto sigVal = p.getSignalValue();
		BOOST_REQUIRE_MESSAGE(sigVal == counter, "Error for cast of mpfr for value " <<
			   (int)(counter));	
		counter++;
	} while (counter != 0);

	mpfr_clear(mpstore);
}

BOOST_AUTO_TEST_CASE(PositTestMPFRConvertTwoWaysPosit16) {
	posit16_t posit;
	PositNumber p(16, 1);
	uint16_t counter = 0;
	mpfr_t mpstore;
	mpfr_init2(mpstore, 53);
	do {
		posit = castP16(counter);
		double positVal = convertP16ToDouble(posit);
		mpz_class tmp(counter);
		p = tmp;
		p.getMPFR(mpstore);
		double flopocoPositVal = mpfr_get_d(mpstore, MPFR_RNDN);
		BOOST_REQUIRE_MESSAGE(flopocoPositVal == positVal, "Error for value " << 
				(int)(counter)); 
		p = positVal;
		auto sigVal = p.getSignalValue();
		BOOST_REQUIRE_MESSAGE(sigVal == counter, "Error for cast of mpfr for value " <<
			   (int)(counter));	
		counter++;
	} while (counter != 0);
	mpfr_clear(mpstore);
}

#ifdef POSIT32TEST
BOOST_AUTO_TEST_CASE(PositTestMPFRConvertTwoWaysPosit32) {
	posit32_t posit;
	PositNumber p(32, 2);
	uint32_t counter = 0;
	mpfr_t mpstore;
	mpfr_init2(mpstore, 53);
	do {
		posit = castP32(counter);
		double positVal = convertP32ToDouble(posit);
		mpz_class tmp(counter);
		p = tmp;
		p.getMPFR(mpstore);
		double flopocoPositVal = mpfr_get_d(mpstore, MPFR_RNDN);
		BOOST_REQUIRE_MESSAGE(flopocoPositVal == positVal, "Error for value " << 
				(int)(counter)); 
		p = positVal;
		auto sigVal = p.getSignalValue();
		BOOST_REQUIRE_MESSAGE(sigVal == counter, "Error for cast of mpfr for value " <<
			   (int)(counter));	
		counter++;
	} while (counter != 0);
	mpfr_clear(mpstore);
}
#endif
#endif
