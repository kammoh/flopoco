/*
  A model of Virtex Ultrascale+(16nm) FPGA (exact part: xcvu3p-2-FFVC1517)

  Author : Ledoux Louis

  Additional comment:
    - note the speed grade of -2
    - CLB topology : https://www.xilinx.com/support/documentation/user_guides/ug574-ultrascale-clb.pdf
    - Primitive : https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_1/ug974-vivado-ultrascale-libraries.pdf
    - Max Freq : - https://www.xilinx.com/support/documentation/data_sheets/ds923-virtex-ultrascale-plus.pdf
                 - 1.3GHz on DSP : https://www.xilinx.com/support/documentation/ip_documentation/ru/c-addsub.html

  This file is part of the FloPoCo project

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, INSA-Lyon
  2008-2016.
  All rights reserved.
*/

#include "VirtexUltrascalePlus.hpp"
#include <iostream>
#include <sstream>
#include "../utils.hpp"

namespace flopoco{

	VirtexUltrascalePlus::VirtexUltrascalePlus(): Target()	{
		id_              = "VirtexUltrascalePlus";
		vendor_          = "Xilinx";

		// 775 MHz is clock buffer and network switching characteristics for speed grade -2 @ 0.85V
		maxFrequencyMHz_ = 775;

		/////// Architectural parameters
		lutInputs_ = 5;
		possibleDSPConfig_.push_back(make_pair(25,18));
		whichDSPCongfigCanBeUnsigned_.push_back(false);
		sizeOfBlock_ = 36864;  // The blocks are 36kb configurable as dual 18k so I don't know.

		// See also all the constant parameters at the end of VirtexUltrascalePlus.hpp
	}

	VirtexUltrascalePlus::~VirtexUltrascalePlus() {};

	double VirtexUltrascalePlus::logicDelay(int inputs){
		double delay;
		do {
			if(inputs <= 5) {
				delay= addRoutingDelay(lut5Delay_);
				inputs=0;
			}
			else {
				delay=addRoutingDelay(lut6Delay_);
				inputs -= 6;
			}
		}
		while(inputs>0);
		TARGETREPORT("logicDelay(" << inputs << ") = " << delay*1e9 << " ns.");
		return  delay;
	}


	double VirtexUltrascalePlus::adderDelay(int size, bool addRoutingDelay_) {
		double delay = adderConstantDelay_ + ((size)/4 -1)* carry4Delay_;
		if(addRoutingDelay_) {
			delay=addRoutingDelay(delay);
			TARGETREPORT("adderDelay(" << size << ") = " << delay*1e9 << " ns.");
		}
		return  delay;
	};


	double VirtexUltrascalePlus::eqComparatorDelay(int size){
		// TODO Refine
		return addRoutingDelay( lut5Delay_ + double((size-1)/(lutInputs_/2)+1)/4*carry4Delay_);
	}

	double VirtexUltrascalePlus::eqConstComparatorDelay(int size){
		// TODO refine
		return addRoutingDelay( lut5Delay_ + double((size-1)/lutInputs_+1)/4*carry4Delay_ );
	}
	double VirtexUltrascalePlus::ffDelay() {
		return ffDelay_;
	};

	double VirtexUltrascalePlus::addRoutingDelay(double d) {
		return(d+ typicalLocalRoutingDelay_);
	};


	double VirtexUltrascalePlus::fanoutDelay(int fanout){
		double delay= fanoutConstant_*fanout;
		TARGETREPORT("fanoutDelay(" << fanout << ") = " << delay*1e9 << " ns.");
		return delay;
	};

	double VirtexUltrascalePlus::lutDelay(){
		return lut5Delay_;
	};

	long VirtexUltrascalePlus::sizeOfMemoryBlock()
	{
		return sizeOfBlock_;
	};

	double VirtexUltrascalePlus::lutConsumption(int lutInputSize) {
		if (lutInputSize <= 5) {
			return .5;
		}
		switch (lutInputSize) {
			case 6:
				return 1.;
			case 7:
				return 2.;
			case 8:
				return 4.;
			default:
				return -1.;
		}
	}

	double VirtexUltrascalePlus::tableDelay(int wIn, int wOut, bool logicTable){
		if(logicTable) {
			return logicDelay(wIn);
		}
		else {
			return RAMDelay_;
		}
	}


	bool VirtexUltrascalePlus::suggestSubaddSize(int &x, int wIn){
		int chunkSize = 4* ((int)floor( (1./frequency() - (adderConstantDelay_ + ffDelay())) / carry4Delay_ ));
		x = min(chunkSize, wIn);
		if (x > 0)
			return true;
		else {
			x = min(2,wIn);
			return false;
		}
	};


	void VirtexUltrascalePlus::delayForDSP(MultiplierBlock* multBlock, double currentCp, int& cycleDelay, double& cpDelay)
	{
		double targetPeriod, totalPeriod;

		targetPeriod = 1.0/frequency();
		totalPeriod = currentCp + DSPMultiplierDelay_;

		cycleDelay = floor(totalPeriod/targetPeriod);
		cpDelay = totalPeriod-targetPeriod*cycleDelay;
	}

}
