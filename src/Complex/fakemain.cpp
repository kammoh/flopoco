#include <iostream>
#include "FixFFT.hpp"

using namespace flopoco;
using namespace std;

int main(){
	for(int i=0;i<16;i++)
		cout << FixFFT::getComponent(i,3) << '\n';
	return 0;
}
