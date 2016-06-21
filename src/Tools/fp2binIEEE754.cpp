/*
 * Utility for converting a FP number into its binary representation,
 * for testing etc
 *
 * Author : Florent de Dinechin
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <mpfr.h>
#include <cstdlib>

#include"../utils.hpp"
using namespace std;
using namespace flopoco;


static void usage(char *name){
  cerr << "\nUsage: "<<name<<" precision x" <<endl ;
  cerr << "  precision can take following values: single /"<<endl;
  cerr << "    double / double-extended"<<endl;
  cerr << "  x is input as an arbitrary precision decimal number" <<endl ;
  cerr << "    and will be rounded to the nearest IEEE754_FP(wE,wF) number." <<endl ;
  exit (EXIT_FAILURE);
}




string check_prescision_allowed(char* s, char* cmd) {
  string p=string(s);
  if (!(p=="single" || p=="double" || p=="double-extended")){
    cerr << "ERROR: got "<<p<<", expected precision (single/double/double-extended)"<<endl;
    usage(cmd);
  }
  return p;
}



int main(int argc, char* argv[] )
{

  if(argc != 3) usage(argv[0]);
  string precision = check_prescision_allowed(argv[1], argv[0]);
  int wE=0;
  int wF=0;

  if(precision=="single"){
    wE=8;
    wF=23;
  }
  else if(precision=="double"){
    wE=11;
    wF=52;
  }
  else if(precision=="double-extended"){
    wE=15;
    wF=64;
  }
  else
    usage(argv[0]);

  mpfr_t mpx;

  mpfr_init2 (mpx, wF+1);
  mpfr_set_str (mpx, argv[2], 10, GMP_RNDN);

  cout<< fp2binIEEE754(mpx, wE, wF) << endl;
}
