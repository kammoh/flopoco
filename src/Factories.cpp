/*
  the FloPoCo command-line interface

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Authors : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr
			Bogdan Pasca, Bogdan.Pasca@ens-lyon.org

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, INSA-Lyon
  2008-2014.
  All rights reserved.

*/



#include "FloPoCo.hpp"

using namespace flopoco;


void UserInterface::registerFactories()
{
	try {

		AutoTest::registerFactory();

		Compressor::registerFactory();
		BitheapTest::registerFactory();

		// Florent has commented out all the operators for now.
		// Uncomment them only to bring them up to the new framework.

		// Fix2FP::registerFactory();
		// FP2Fix::registerFactory();
		// InputIEEE::registerFactory();
		// OutputIEEE::registerFactory();

		Shifter::registerFactory();
		LZOC::registerFactory();
		LZOCShifterSticky::registerFactory();

		ShiftReg::registerFactory();

		IntAdder::registerFactory();
		
#if 0 // Plug them back some day
		IntAdderClassical::registerFactory();
		IntAdderAlternative::registerFactory();
		IntAdderShortLatency::registerFactory();
#endif
		
#if 0 // Plug them for debug purpose only
		IntAdderSpecific::registerFactory();
		LongIntAdderAddAddMuxGen1::registerFactory();
		LongIntAdderAddAddMuxGen2::registerFactory();
		LongIntAdderCmpAddIncGen1::registerFactory();
		LongIntAdderCmpAddIncGen2::registerFactory();
		LongIntAdderCmpCmpAddGen1::registerFactory();
		LongIntAdderCmpCmpAddGen2::registerFactory();
		LongIntAdderMuxNetwork::registerFactory();
		IntComparatorSpecific::registerFactory();
#endif
		// IntComparator::registerFactory();
		IntDualAddSub::registerFactory();
		IntMultiAdder::registerFactory();
		DSPBlock::registerFactory();
		//IntKaratsubaRectangular::registerFactory();
		IntMultiplier::registerFactory();
		// IntSquarer::registerFactory();

		// IntConstMult::registerFactory(); // depends on BH, but in currently unplugged code
		// FPConstMult::registerFactory();
		// FPRealKCM::registerFactory();
		// IntConstDiv::registerFactory(); // depends on IntConstMult
		// FPConstDiv::registerFactory();
		FixFunctionByTable::registerFactory();
		// FixFunctionBySimplePoly::registerFactory();
		// FixFunctionByPiecewisePoly::registerFactory();
		// FixFunctionByMultipartiteTable::registerFactory(); // depends on BH
		// BasicPolyApprox::registerFactory();
		// PiecewisePolyApprox::registerFactory();
		FixRealKCM::registerFactory();
		TestBench::registerFactory();
		Wrapper::registerFactory();
		FPAdd::registerFactory();
		// FPAddSub::registerFactory();
		// FPAdd3Input::registerFactory();
		// FPAddSinglePath::registerFactory();
		// FPMult::registerFactory();
		// //FPMultKaratsuba::registerFactory();
		// FPSquare::registerFactory();
		FPDiv::registerFactory();
		// FPDiv::NbBitsMinRegisterFactory();
		// FPSqrt::registerFactory();

		// FPLargeAcc::registerFactory();
		// LargeAccToFP::registerFactory();
		// FPDotProduct::registerFactory();

		// FPExp::registerFactory();
		// IterativeLog::registerFactory();
		// FPPow::registerFactory();
		// FixSinCos::registerFactory();
		// CordicSinCos::registerFactory();
		// FixAtan2::registerFactory();
		// //FixedComplexAdder::registerFactory();
		FixSOPC::registerFactory();
	  FixFIR::registerFactory();
		FixIIR::registerFactory();

		// hidden for now
		// Fix2DNorm::registerFactory();

		Xilinx_GenericAddSub::registerFactory();
		Xilinx_Comparator::registerFactory();
		Xilinx_TernaryAdd_2State::registerFactory();
		XilinxGPC::registerFactory();
		XilinxFourToTwoCompressor::registerFactory();

		// TargetModel::registerFactory();
		// For new developers to play within FloPoCo operator development
	  TutorialOperator::registerFactory();
	}
	catch (const std::exception &e) {
		cerr << "Error while registering factories: " << e.what() <<endl;
		exit(EXIT_FAILURE);
	}
}
