#include <vector>

#include "FixFFT.hpp"
#include <iostream>
using namespace std;

namespace flopoco{
	
	int FixFFT::getTwiddleExp(int layer, int signal){
		return signal % (1 << layer);
	}

	bool FixFFT::isTop(int layer, int signal){
		return !(signal & (1<<layer));
	}

	pair<int, int> FixFFT::pred(int layer, int signal){
		int pastlay = layer - 1;
		if(isTop(pastlay, signal))
			return make_pair(signal, signal + (1<<pastlay));
		else
			return make_pair(signal - (1<<pastlay), signal);
	}

	FixFFT::CompType FixFFT::getComponent(int layer, int signal){
		int root(1<<(layer+1)); //the main root of unit used for this layer
		int exp(FixFFT::getTwiddleExp(signal, layer));
		//its exponant for the 
		if(isTop(layer, signal))
			return Trunc;
		if(exp && (exp*4) != root)
			return Cmplx;
		return Exact;
	}

	pair<FixFFT::fftSize, FixFFT::fftError> calcDim(FixFFT::fftPrec &fft){
		unsigned int nbLay = fft.size(); // the layers of the FFT
		unsigned int nbPts = 1<<lay; // the points of the FFT
		if (fft[0].size() != pts)
			throw "not an FFT";
		
		FixFFT::fftSize sizes(lay, FixFFT::laySize(pts, pair<int,int> (0,0)));
		FixFFT::fftError errors(lay, vector<float>(pts, 0.0));

		for(int lay=1; lay<=nbLay; lay++){
			for(int pt=0; pt<nbPts; pt++){
				
			}
		}
		
		return make_pair(sizes,errors);
	}
}
