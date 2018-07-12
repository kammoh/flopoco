#include "FixFFT.hpp"

using namespace std;

namespace flopoco{
	int FixFFT::getTwiddleExp(int signal, int layer){
		return signal % (1 << layer);
	}

	FixFFT::CompType FixFFT::getComponent(int signal, int layer){
		int exp(FixFFT::getTwiddleExp(signal, layer));
		int root(1<<(layer+1));
		if(!(signal & (1 << layer))) //the non-multiplied entry of the butterfly
			return Trunc;
		if(exp && (exp*4) != root)
			return Cmplx;
		return Exact;
	}
}
