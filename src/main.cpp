/* vim: set tabstop=8 softtabstop=2 shiftwidth=2: */
/*
 * The FloPoCo command-line interface
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

#include "Operator.hpp"
#include "Shifters.hpp"
#include "LZOC.hpp"
#include "IntAdder.hpp"
#include "IntMultiplier.hpp"
#include "FPMultiplier.hpp"
#include "LongAcc.hpp"
#include "Wrapper.hpp"
#include "TestBench.hpp"
#include "BigTestBench.hpp"
#include "ConstMult/IntConstMult.hpp"
#include "ConstMult/FPConstMult.hpp"
#include "Target.hpp"
#include "Targets/VirtexIV.hpp"

#ifdef HAVE_HOTBM
#include "HOTBM.hpp"
#endif

using namespace std;


// Global variables, useful through most of FloPoCo

vector<Operator*> oplist;

string filename="flopoco.vhdl";

int verbose=0;

Target* target;



static void usage(char *name){
  cerr << "\nUsage: "<<name<<" <operator specification list>\n" ;
  cerr << "  Each operator specification is one of\n";
  cerr << "    LeftShifter  wIn  MaxShift\n";
  cerr << "    RightShifter wIn  MaxShift\n";
  cerr << "    LZOC wIn wOut\n";
  cerr << "    IntAdder wIn\n";
  cerr << "      Integer adder, possibly pipelined to arbitrary frequency (almost)\n";
  cerr << "    LongAcc wE_in wF_in MaxMSB_in LSB_acc MSB_acc\n";
  cerr << "      Long fixed-point accumulator\n";
  cerr << "    IntConstMult w c\n";
  cerr << "      integer constant multiplier: w is input size, c is the constant.\n";
  cerr << "    FPConstMult wE_in wF_in wE_out wF_out cst_sgn cst_exp cst_int_sig\n";
  cerr << "      floating-point constant multiplier.\n";
  cerr << "      The constant is provided as integral significand and integral exponent.\n";
  cerr << "    IntMultiplier wInX wInY \n";
  cerr << "      integer multiplier of two integers X and Y of sizes wInX and wInY \n";	
  cerr << "    FPMultiplier wEX wFX wEY wFY wER wFR normalize\n";
  cerr << "      floating-point multiplier \n";
  cerr << "      normalize can be either 0 or 1. \n";     
#ifdef HAVE_HOTBM
  cerr << "    HOTBM function wI wO n\n";
  cerr << "      High-order table-based method for generating a given function\n";
  cerr << "      wI - input width, wO - output width, n - degree of minimax\n";
  cerr << "      function - sollya-syntaxed function to implement (must be escaped)\n";
#endif // HAVE_HOTBM
  cerr << "    TestBench n\n";
  cerr << "       produce a behavorial test bench for the preceding operator\n";
  cerr << "       This test bench will include standard tests, plus n random tests.\n";
#if 0 // hidden in 0.5beta release
  cerr << "    BigTestBench n\n";
  cerr << "       Same as above, but generates a more VHDL efficient test bench.\n";
#endif
  cerr << "    Wrapper entity_name\n";
  cerr << "       produce a wrapper named entity_name for the preceding operator\n";
  cerr << "       (useful to get synthesis results without having the operator optimised out)\n";
  cerr << "  In addition several options affect the operators following them:\n";
  cerr << "   -outputfile=<output file name>\n";
  cerr << "       sets the filename (default flopoco.vhdl)\n";
  cerr << "   -lut=<3|4|5|6|7|8>\n";
  cerr << "       sets the input size of the LUTs of the target architecture (default 4)\n";
  cerr << "   -verbose=<1|2|3>\n";
  cerr << "       sets the verbosity level (default 0)\n";
  cerr << "   -pipeline=<yes|no>\n";
  cerr << "       pipeline the operators (default=yes)\n";
  cerr << "   -frequency=<frequency in MHz>\n";
  cerr << "       target frequency (default=400)\n";
  cerr << "   -DSP_blocks=<yes|no>\n";
  cerr << "       optimize for the use of DSP blocks (default=yes)\n";

  //  cerr << "    FPAdd wE wF\n";
  exit (EXIT_FAILURE);
}




int check_strictly_positive(char* s, char* cmd) {
  int n=atoi(s);
  if (n<=0){
    cerr<<"ERROR: got "<<s<<", expected strictly positive number."<<endl;
    usage(cmd);
  }
  return n;
}

int check_positive_or_null(char* s, char* cmd) {
  int n=atoi(s);
  if (n<0){
    cerr<<"ERROR: got "<<s<<", expected positive-or-null number."<<endl;
    usage(cmd);
  }
  return n;
}

int check_sign(char* s, char* cmd) {
  int n=atoi(s);
  if (n!=0 && n!=1) {
    cerr<<"ERROR: got "<<s<<", expected a sign bit (0 or 1)."<<endl;
    usage(cmd);
  }
  return n;
}



bool parse_command_line(int argc, char* argv[]){
  if (argc<2) {
    usage(argv[0]); 
    return false;
  }

  Operator* op;
  int i=1;
  do {
    string opname(argv[i++]);
    if(opname[0]=='-'){
      string::size_type p = opname.find('=');
      if (p == string::npos) {
	// 	bool yes = s.size() <= 6 || s.substr(2, 3) != "no-";
	// 	std::string o = s.substr(yes ? 2 : 5);
	// 	if (o == "reverted-fma") parameter_rfma = yes;
	// 	else
	cerr << "ERROR: Option missing an = : "<<opname<<endl; 
	return false;
      } else {
	string o = opname.substr(1, p - 1), v = opname.substr(p + 1);
	if (o == "outputfile") {
	  if(oplist.size()!=0)
	    cerr << "WARNING: global option "<<o<<" should come before any operator specification" << endl; 
	  filename=v;
	}
  	else if (o == "lut") {
 	  if(oplist.size()!=0)
 	    cerr << "WARNING: global option "<<o<<" should come before any operator specification" << endl; 
 	  int lutsize = atoi(v.c_str()); // there must be a more direct method of string
 	  if (lutsize<3 || lutsize>8) {
 	    cerr<<"ERROR: Are there really FPGAs whose LUTs have "<<lutsize<< " inputs ?"<<endl; 
 	    usage(argv[0]);
 	  }
	  target->set_lut_inputs(lutsize);
	  if (verbose) {
	    cerr<< "LUT size set to "<< lutsize <<endl;
	  }
 	}
 	else if (o == "verbose") {
	  verbose = atoi(v.c_str()); // there must be a more direct method of string
	  if (verbose<0 || verbose>3) {
	    cerr<<"ERROR: verbose should be 1, 2 or 3,    got "<<v<<"."<<endl;
	    usage(argv[0]);
	  }
	}
 	else if (o == "pipeline") {
	  if(v=="yes") target->set_pipelined();
	  else if(v=="no")  target->set_not_pipelined();
	  else {
	    cerr<<"ERROR: pipeline option should be yes or no,    got "<<v<<"."<<endl; 
	    usage(argv[0]);
	  }
	}
 	else if (o == "frequency") {
	  int freq = atoi(v.c_str());
	  if (freq>1 && freq<10000) {
	    target->set_frequency(1e6*(double)freq);
	    if(verbose) 
	      cerr << "Frequency set to "<<target->frequency()<< "MHz" <<endl; 
	  }
	  else {
	    cerr<<"WARNING: frequency out of reasonible range, ignoring it."<<endl; 
	  }
	}
	else if (o == "DSP_blocks") {
	  if(v=="yes") target->set_use_hard_multipliers(true);
	  else if(v=="no")  target->set_use_hard_multipliers(false);
	  else {
	    cerr<<"ERROR: DSP_blocks option should be yes or no,    got "<<v<<"."<<endl; 
	    usage(argv[0]);
	  }
	}
	else { 	
	  cerr << "Unknown option "<<o<<endl; 
	  return false;
	}
      }
    }
    else if(opname=="IntConstMult"){
      int nargs = 2;
      if (i+nargs > argc)
	usage(argv[0]);
      else {
	int w = atoi(argv[i++]);
	mpz_class mpc(argv[i++]);
	cerr << "> IntConstMult , w="<<w<<", c="<<mpc<<"\n";
	op = new IntConstMult(target, w, mpc);
	oplist.push_back(op);
      }        
    }
    else if(opname=="FPConstMult"){
      int nargs = 7;
      if (i+nargs > argc)
	usage(argv[0]);
      else {
	int wE_in = check_strictly_positive(argv[i++], argv[0]);
	int wF_in = check_strictly_positive(argv[i++], argv[0]);
	int wE_out = check_strictly_positive(argv[i++], argv[0]);
	int wF_out = check_strictly_positive(argv[i++], argv[0]);
	int cst_sgn  = check_sign(argv[i++], argv[0]); 
	int cst_exp  = atoi(argv[i++]); // TODO no check on this arg
	mpz_class cst_sig(argv[i++]);
	cerr << "> FPConstMult, wE_in="<<wE_in<<", wF_in="<<wF_in
	     <<", wE_out="<<wE_out<<", wF_out="<<wF_out
	     <<", cst_sgn="<<cst_sgn<<", cst_exp="<<cst_exp<< ", cst_sig="<<cst_sig<<endl;
	op = new FPConstMult(target, wE_in, wF_in, wE_out, wF_out, cst_sgn, cst_exp, cst_sig);
	oplist.push_back(op);
      }        
    } 	
     else if(opname=="LeftShifter"){
       int nargs = 2;
       if (i+nargs > argc)
	 usage(argv[0]);
       else {
	 int wIn = check_strictly_positive(argv[i++], argv[0]);
	 int maxShift = check_strictly_positive(argv[i++], argv[0]);
	 cerr << "> LeftShifter, wIn="<<wIn<<", maxShift="<<maxShift<<"\n";
	 op = new Shifter(target, wIn, maxShift, Left);
	 oplist.push_back(op);
       }    
     }    
     else if(opname=="RightShifter"){
       int nargs = 2;
       if (i+nargs > argc)
	 usage(argv[0]);
       else {
	 int wIn = check_strictly_positive(argv[i++], argv[0]);
	 int maxShift = check_strictly_positive(argv[i++], argv[0]);
	 cerr << "> RightShifter, wIn="<<wIn<<", maxShift="<<maxShift<<"\n";
	 op = new Shifter(target, wIn, maxShift, Right);
	 oplist.push_back(op);	
       }    
     }
     else if(opname=="LZOC"){
       int nargs = 3;
       if (i+nargs > argc)
	 usage(argv[0]);
       else {
	 int wIn = check_strictly_positive(argv[i++], argv[0]);
	 int wOut = check_strictly_positive(argv[i++], argv[0]);
	 cerr << "> LZOC, wIn="<<wIn<<", wOut="<<wOut<<"\n";
	 op = new LZOC(target, wIn, wOut);
	 oplist.push_back(op);	
       }    
     }
     else if(opname=="FPMultiplier"){
       int nargs = 7; // was 7
      if (i+nargs > argc)
	usage(argv[0]);
       else {
	 int wEX = check_strictly_positive(argv[i++], argv[0]);
	 int wFX = check_strictly_positive(argv[i++], argv[0]);
	 int wEY = check_strictly_positive(argv[i++], argv[0]);
	 int wFY = check_strictly_positive(argv[i++], argv[0]);
	 int wER = check_strictly_positive(argv[i++], argv[0]);
	 int wFR = check_strictly_positive(argv[i++], argv[0]);
	 int norm = atoi(argv[i++]);

        cerr << "> FPMultiplier , wEX="<<wEX<<", wFX="<<wFX<<", wEY="<<wEY<<", wFY="<<wFY<<", wER="<<wER<<", wFR="<<wFR<< " Normalized="<< norm<<" \n";
        
	if ((norm==0) or (norm==1))
	{
		if ((wEX==wEY) && (wEX==wER)) 
		{
			op = new FPMultiplier(target, wEX, wFX, wEY, wFY, wER, wFR, norm);
			oplist.push_back(op);
		}
		else
		{
			cerr<<"(For now) the inputs must have the same size"<<endl;
		}
	}
	else       
        {
			cerr<<"normalized must be 0 or 1"<<endl;
	}  

	}//end else    
     } 
     else if(opname=="IntAdder"){ // toy 
      int nargs = 1;
      if (i+nargs > argc)
	usage(argv[0]);
       else {
	 int wIn = check_strictly_positive(argv[i++], argv[0]);
	cerr << "> IntAdder , wIn="<<wIn<<endl  ;
	op = new IntAdder(target, wIn);
	oplist.push_back(op);
       }    
     }   
     else if(opname=="IntMultiplier"){
      int nargs = 2;
      if (i+nargs > argc)
	usage(argv[0]);
       else {
	 int wInX = check_strictly_positive(argv[i++], argv[0]);
	 int wInY = check_strictly_positive(argv[i++], argv[0]);
	cerr << "> IntMultiplier , wInX="<<wInX<<", wInY="<<wInY<<"\n";
	op = new IntMultiplier(target, wInX, wInY);
	oplist.push_back(op);
       }    
     }   
     else if(opname=="LongAcc"){
      int nargs = 5;
      if (i+nargs > argc)
	usage(argv[0]);
       else {
 	int wEX = check_strictly_positive(argv[i++], argv[0]);
 	int wFX = check_strictly_positive(argv[i++], argv[0]);
 	int MaxMSBX = atoi(argv[i++]); // may be negative
 	int LSBA = atoi(argv[i++]); // may be negative
 	int MSBA = atoi(argv[i++]); // may be negative
 	cerr << "> Long accumulator , wEX="<<wEX<<", wFX="<<wFX<<", MaxMSBX="<<MaxMSBX<<", LSBA="<<LSBA<<", MSBA="<<MSBA<<"\n";
	op = new LongAcc(target, wEX, wFX, MaxMSBX, LSBA, MSBA);
	oplist.push_back(op);
       }    
     }
      // hidden and undocumented
     else if(opname=="LongAccPrecTest"){
       int nargs = 6; // same as LongAcc, plus an iteration count
      if (i+nargs > argc)
	usage(argv[0]);
       else {
 	int wEX = atoi(argv[i++]);
 	int wFX = atoi(argv[i++]);
 	int MaxMSBX = atoi(argv[i++]);
 	int LSBA = atoi(argv[i++]);
 	int MSBA = atoi(argv[i++]);
	int n = atoi(argv[i++]);
 	cerr << "> Test of long accumulator accuracy, wEX="<<wEX<<", wFX="<<wFX<<", MaxMSBX="<<MaxMSBX<<", LSBA="<<LSBA<<", MSBA="<<MSBA<<", "<< n << " tests\n";
	LongAcc * op = new LongAcc(target, wEX, wFX, MaxMSBX, LSBA, MSBA);
	// op->test_precision(n);
	op->test_precision2();
       }    
     }    
    else if (opname == "Wrapper") {
      int nargs = 1;
      if (i+nargs > argc)
	usage(argv[0]);
      else {
      if(oplist.empty()){
	    cerr<<"ERROR: Wrapper has no operator to wrap (it should come after the operator it wraps)"<<endl;
	    usage(argv[0]);
	  }
	  Operator* toWrap = oplist.back();
	  cerr << "> Wrapper for " << toWrap->unique_name << " as entity "<< argv[i] <<endl;
	  oplist.push_back(new Wrapper(target, toWrap, argv[i++]));
	}
    }
    else if (opname == "TestBench") {
      int nargs = 1;
      if (i+nargs > argc)
	usage(argv[0]); // and exit
      if(oplist.empty()){
	  cerr<<"ERROR: TestBench has no operator to wrap (it should come after the operator it wraps)"<<endl;
	  usage(argv[0]); // and exit
	}
      int n = check_strictly_positive(argv[i++], argv[0]);
      Operator* toWrap = oplist.back();
      cerr << "> TestBench for " << toWrap->unique_name  <<endl;
      oplist.push_back(new TestBench(target, toWrap, n));
    }
    else if (opname == "BigTestBench") {
      int nargs = 1;
      if (i+nargs > argc)
	usage(argv[0]); // and exit
      if(oplist.empty()){
	  cerr<<"ERROR: BigTestBench has no operator to wrap (it should come after the operator it wraps)"<<endl;
	  usage(argv[0]); // and exit
	}
      int n = check_strictly_positive(argv[i++], argv[0]);
      Operator* toWrap = oplist.back();
      cerr << "> BigTestBench for " << toWrap->unique_name  <<endl;
      oplist.push_back(new BigTestBench(target, toWrap, n));
    }
#ifdef HAVE_HOTBM
    else if (opname == "HOTBM") {
      int nargs = 4;
      if (i+nargs > argc)
	usage(argv[0]); // and exit
      string func = argv[i++];
      int wI = check_strictly_positive(argv[i++], argv[0]);
      int wO = check_strictly_positive(argv[i++], argv[0]);
      int n  = check_strictly_positive(argv[i++], argv[0]);
      cerr << "> HOTBM func='" << func << "', wI=" << wI << ", wO=" << wO <<endl;
      op = new HOTBM(target, func, wI, wO, n);
      oplist.push_back(op);
    }
#endif // HAVE_HOTBM
//     else if(opname=="FPAdd"){
//       // 2 arguments
//       if (argc < i+2)
// 	usage(argv[0]);
//       else {
// 	int wE = atoi(argv[i+1]);
// 	int wF = atoi(argv[i+2]);
// 	cerr << "> FPAdd, wE="<<wE<<", wF="<<wF<<"\n";
// 	cerr << "  NOT IMPLEMENTED"<<endl;
// 	i+=3;
//       }        
//     }
    else  {
      cerr << "ERROR: Problem parsing input line, exiting";
      usage(argv[0]);
    }
  } while (i<argc);
  return true;
}




int main(int argc, char* argv[] )
{
  target = new VirtexIV();

  try {
    bool command_line_OK =  parse_command_line(argc, argv);
  } catch (std::string s) {
    cerr << "Exception while parsing command line: " << s << endl;
    return 1;
  }

 if(oplist.empty()) {
   cerr << "Nothing to generate, exiting\n";
   exit(EXIT_FAILURE);
 }

  ofstream file;
  file.open(filename.c_str(), ios::out);
  cerr<< "Output file: " << filename <<endl;
  
  for(int i=0; i<oplist.size(); i++) {
    try {
      oplist[i]->output_vhdl(file);
    } catch (std::string s) {
      cerr << "Exception while generating '" << oplist[i]->unique_name << "': " << s <<endl;
    }
  }
  file.close();
  
  cout << endl<<"Final report:"<<endl;
  for(int i=0; i<oplist.size(); i++) {
    oplist[i]->output_final_report();
  }
  return 0;
}
