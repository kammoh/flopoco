// general c++ library for manipulating streams
#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
  There will be used in the emulate function to manipulate arbitraly large
  entries */
#include "gmp.h"
#include "mpfr.h"
#include <gmpxx.h>

// include the header of the Operator
#include "NewCompressorTree.hpp"
#include "PopCount.hpp"

using namespace std;


// personalized parameter
//string NewCompressorTree::operatorInfo = "UserDefinedInfo param0 param1 <options>";


// the length of the result is the same as the one of vops
// the result is truncated on overflow
NewCompressorTree::NewCompressorTree(Target * target, vector<unsigned> vops)
	:Operator(target), w(vops.size()), vert_operands(vops)
{
	{
		// definition of the name of the operator
		ostringstream name;
		name << "NewCompressorTree_" << w;
		vector<unsigned>::const_iterator it = vops.begin();
		for (; it != vops.end(); it++) {
			name << '_' << *it;
		}
		setName(name.str());
	}
	setCopyrightString("Guillaume Sergent 2012");

	for (unsigned i = 0; i < w; i++) {
		addInput (join("X", i), vops[i]);
	}
	addOutput ("R", w);

	unsigned level = 0, max_height;
	unsigned n = target->lutInputs();
	vector<Operator*> popcounts (n+1, (Operator*) 0);
	// first construct _level0 identifiers
	for (unsigned i = 0; i < w; i++) {
		ostringstream l0;
		l0 << "X_" << i << "_level0";
		vhdl << tab << declare (l0.str(), vops[i]) << " <= X" << i << ";\n";
	}
	for (;;) {
		// sync new level_i signals together
		if (w) {
			setCycleFromSignal (join("X_",0,"_level",level));
			for (unsigned i = 1; i < w; i++) {
				syncCycleFromSignal (join("X_",i,"_level",level));
			}
		}
		vector<unsigned>::iterator it;
		max_height = 0;
		for (it = vops.begin(); it != vops.end(); it++) {
			if (max_height < *it)
				max_height = *it;
		}
		bool exit_the_loop = false; // break will only exit the switch
		switch (max_height) {
		case 0:
			if (w)
				vhdl << "R <= \"0\";";
			exit_the_loop = true;
			break;
		case 1:
			// nothing to add
			vhdl << "R <= ";
			// enumerate in reverse since IR is litte-endian and
			// flopoco's vhdl is big-endian
			for (int i = w-1; i >= 0; i--) {
				if (i < w-1)
					vhdl << " & ";
				if (vops[i]) {
					vhdl << "X_" << i << "_level" << level
					     << of(0);
				} else {
					vhdl << "\"0\"";
				}
			}
			vhdl << ";" << endl;
			exit_the_loop = true;
			break;
		case 2:
			// final (binary) addition in the general case
			vhdl << tab << declare ("R_1", w) << " <= ";
			// enumerate in reverse since IR is litte-endian and
			// flopoco's vhdl is big-endian
			for (int i = w-1; i >= 0; i--) {
				if (i < w-1)
					vhdl << " & ";
				if (vops[i]) {
					vhdl << "X_" << i << "_level" << level
					     << of(0);
				} else {
					vhdl << "\"0\"";
				}
			}
			vhdl <<";" << endl;
			vhdl << tab << declare ("R_2", w) << " <= ";
			for (int i = w-1; i >= 0; i--) {
				if (i < w-1)
					vhdl << " & ";
				if (vops[i] > 1) {
					vhdl << "X_" << i << "_level" << level
					     << of(1);
				} else {
					vhdl << "\"0\"";
				}
			}
			vhdl << ";\n";
			vhdl << "R <= R_1 + R_2;" << endl;
			exit_the_loop = true;
			break;
		default:
			vector<unsigned> vops_new (w, 0);
			for (unsigned i = 0; i < w; i++) {
				unsigned inputs = vops[i];
				while (inputs > n) {
					if (!popcounts[n]) {
						popcounts[n] = new PopCount
							(target, n);
						oplist.push_back(popcounts[n]);
					}
					ostringstream in;
					in << "X_" << i << "_level"
					   << level << "_" << vops[i]-1
					   << "_" << vops[i]-n;
					string out = in.str() + "_popcnt";

					vhdl << tab << declare (in.str(), n)
					     << " <= X_" << i << "_level"
					     << level
					     << range(vops[i]-1,vops[i]-n)
					     << ";" << endl;
					outPortMap (popcounts[n],"R",out);
					inPortMap(popcounts[n],"X",in.str());
					vhdl << instance (popcounts[n],
					                  out + "_calc");
					for (int j = 0; j < intlog2(n); j++) {
						if (i+j >= w)
							break;
						ostringstream bit;
						bit << "X_" << i+j << "_level"
						    << (level+1) << "_bit"
						    << vops_new[i+j];
						vhdl << tab << declare (bit.str())
						     << " <= " << out
						     << of(j) << ";\n";
						vops_new[i+j]++;
					}
					vops[i] -= n;
					inputs = vops[i];
				}
				// if it's compressible
				if (inputs > 2) {
					if (!popcounts[inputs]) {
						popcounts[inputs] = new
							PopCount (target,
							          inputs);
						oplist.push_back (popcounts
								[inputs]);
					}
					ostringstream in;
					in << "X_" << i << "_level"
					   << level << "_" << vops[i]-1 << "_0";
					string out = in.str() + "_popcnt";

					vhdl << tab << declare (in.str(), inputs)
					     << " <= X_" << i << "_level"
					     << level
					     // vops[i] == inputs
					     << range(vops[i]-1, 0)
					     << ";" << endl;
					outPortMap (popcounts[inputs],"R",out);
					inPortMap (popcounts[inputs],"X",
					           in.str());
					vhdl << instance (popcounts[inputs],
					                  out + "_calc");
					for (int j = 0;
					         j < intlog2(inputs);
						 j++) {
						if (i+j >= w)
							break;
						ostringstream bit;
						bit << "X_" << i+j << "_level"
						    << (level+1) << "_bit"
						    << vops_new[i+j];
						vhdl << tab << declare (bit.str())
						     << " <= " << out
						     << of(j) << ";\n";
						vops_new[i+j]++;
					}
				} else {
					for (unsigned j = 0; j < inputs; j++) {
						ostringstream bit;
						bit << "X_" << i << "_level"
						    << (level+1) << "_bit"
						    << vops_new[i];
						vhdl << tab << declare (bit.str())
						     << " <= X_" << i
						     << "_level" << level
						     << of(j) << ";" << endl;
						vops_new[i]++;
					}
				}
			}
			// and now we constructed every bit of the next level
			// so we are ready to construct the real new inputs
			for (unsigned i = 0; i < w; i++) {
				// do nothing if there is nothing left to add
				// of weight i
				if (!vops_new[i]) continue;

				ostringstream next_lvl;
				next_lvl << "X_" << i << "_level" << (level+1);
				vhdl << tab << declare (next_lvl.str(),vops_new[i]);
				// if it's of length 1 the rhs will be a std_logic
				if (vops_new[i] == 1)
					vhdl << of(0);
				vhdl << " <= ";
				// the order of filling of the level vectors
				// doesn't matter: here we fill in reverse
				for (unsigned j = 0; j < vops_new[i]; j++) {
					if (j)
						vhdl << " & ";
					vhdl << "X_" << i << "_level"
					     << (level+1) << "_bit"
					     << j;
				}
				vhdl << ";\n";
			}
			level++;
			//nextCycle();
			vops = vops_new;
			break;
		}
	if (exit_the_loop)
		break;
	}
};



void NewCompressorTree::emulate(TestCase * tc)
{
	std::vector<mpz_class> signals (w, mpz_class(0));
	for (unsigned int i = 0; i < w; i++) {
		signals[i] = tc->getInputValue(join("X",i));
	}
	mpz_class res;
	for (unsigned int i = 0; i < w; i++) {
		res += (popcnt (signals[i]) << i);
	}
	res &= ((mpz_class(1) << w) - 1);
	tc->addExpectedOutput("R", res);
}

