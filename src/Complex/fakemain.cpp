#include <iostream>
#include "FixFFT.hpp"

using namespace flopoco;
using namespace std;

int main(){
	for(int i=0; i<16; cout << get<0>(FixFFT::pred(2,i))
		    << " " << get<1>(FixFFT::pred(2,i++)) << endl){}
	return 0;
}
