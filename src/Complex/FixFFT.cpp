#include <vector>
#include "FixFFT.hpp"
#include <iostream>
using namespace std;

int intlog2(int number)
{
	int result = 0;
	number--;
	while(number){
		number >>= 1;
		result++;
	}
	return result;
}

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
	
	//WIP, let's see more theoretic versions first
	//Warning : the use of ints limits the fft size, even if I doubt that 
	// someone will ever need a 4294967296-points FFT
	
	pair<FixFFT::fftSize, FixFFT::fftError> calcDim(FixFFT::fftPrec &fft){
		int nbLay = fft.size() - 1; // the layers of the FFT
		int nbPts = 1<<nbLay; // the points of the FFT
		if (fft[0].size() != (long unsigned)nbPts)
			throw "not an FFT";
		
		FixFFT::fftSize sizes(nbLay + 1,
		                      FixFFT::laySize(nbPts, pair<int,int> (0,0)));

		FixFFT::fftError errors(nbLay, vector<float>(nbPts, 0.0));

		for(int lay=1; lay<=nbLay; lay++){
			for(int pt=0; pt<nbPts; pt++){
				//WIP, not hard, just need to know this sqrt(2) problem
				
			}
		}
		
		return make_pair(sizes,errors);
	}
	
	pair<int, int> FixFFT::sizeSignal(int msbIn, int lsbIn, int lsbOut,
	                                  int nbLay, int lay){
		if(!lay)
			return make_pair(msbIn, lsbIn);

		return make_pair(2*lay + 1, lsbOut - 1 - 2*nbLay - intlog2(nbLay)
		                 + 2*lay);
	}
	
	FixFFT::fftSize FixFFT::calcDim4(int msbIn, int lsbIn, int lsbOut,
	                                 int nbLay){
		 // the layers of the FFT
		int nbPts = 1<<(nbLay*2); // the points of the FFT
		FixFFT::fftSize size(nbLay+1,
		                     FixFFT::laySize(nbPts, pair<int,int> (0,0)));
		
		for(int lay=0; lay <= nbLay; lay++){
			for(int pt=0; pt < nbPts; pt++){
				size[lay][pt] = FixFFT::sizeSignal(msbIn, lsbIn, lsbOut,
				                                   nbLay, lay);
			}
		}
		return size;
	}	
}

