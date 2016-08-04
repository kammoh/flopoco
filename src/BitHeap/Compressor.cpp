
#include "Compressor.hpp"


using namespace std;

namespace flopoco{


	Compressor::Compressor(Target * target_, vector<int> heights_, bool compactView_)
		: Operator(target_), heights(heights_), compactView(compactView_)
	{
		ostringstream name;
		stringstream nm, xs;

		//remove the zero columns at the lsb
		while(heights[0] == 0)
		{
			heights.erase(heights.begin());
		}

		name << "Compressor_";

		for(int i=heights.size()-1; i>=0; i--)
		{
			wIn    += heights[i];
			maxVal += intpow2(i) * heights[i];
			name << heights[i];
		}

		wOut = intlog2(maxVal);

		name << "_" << wOut;
		setName(name.str());

		for(int i=heights.size()-1; i>=0; i--)
		{
			addInput(join("X",i), heights[i]);

			xs << "X" << i << " ";
			if(i != 0)
				xs << "& ";
		}

		addOutput("R", wOut);

		vhdl << tab << declare("X", wIn) << " <= " << xs.str() << ";" << endl;
		vhdl << tab << "with X select R <= " << endl;

		vector<vector<mpz_class>> values(1<<wOut);

		for(mpz_class i=0; i<(1 << wIn); i++)
		{
			mpz_class ppcnt = 0;
			mpz_class ii = i;

			for(unsigned j=0; j<heights.size(); j++)
			{
				ppcnt += popcnt( ii - ((ii>>heights[j]) << heights[j]) ) << j;
				ii = ii >> heights[j];
			}

			if(!compactView)
			{
				vhdl << tab << tab << "\"" << unsignedBinary(ppcnt, wOut) << "\" when \""
						<< unsignedBinary(i, wIn) << "\", \n";
			}else{
				values[ppcnt.get_ui()].push_back(i);
			}
		}

		if(compactView)
		{
			for(unsigned i=0; i<values.size(); i++)
			{
				if(values[i].size() > 0)
				{
					vhdl << tab << tab << "\"" << unsignedBinary(mpz_class(i), wOut) << "\" when \""
							<< unsignedBinary(mpz_class(values[i][0]), wIn) << "\"";
					for(unsigned j=1; j<values[i].size(); j++)
					{
						vhdl << " | \"" << unsignedBinary(mpz_class(values[i][j]), wIn) << "\"";
					}
					vhdl << ";" << endl;
				}
			}
		}

		vhdl << tab << tab << "\"" << std::string(wOut, '-') << "\" when others;" << endl;

		REPORT(DEBUG, "Generated " << name.str());
	}




	Compressor::~Compressor(){
	}

	unsigned Compressor::getColumnSize(int column)
	{
		if(column >= (signed)heights.size())
			return 0;
		else
			return heights[column];
	}

	int Compressor::getOutputSize()
	{
		return wOut;
	}

	void Compressor::emulate(TestCase * tc)
	{
		mpz_class r = 0;

		for(unsigned i=0; i<heights.size(); i++)
		{
			mpz_class sx = tc->getInputValue(join("X", i));
			mpz_class p  = popcnt(sx);

			r += p<<i;
		}

		tc->addExpectedOutput("R", r);
	}

	OperatorPtr Compressor::parseArguments(Target *target, vector<string> &args) {
		string in;
		bool compactView_;
		vector<int> heights_;

		UserInterface::parseString(args, "columnHeights", &in);
		UserInterface::parseBoolean(args, "compactView", &compactView_);

		// tokenize the string, with ':' as a separator
		stringstream ss(in);
		while(ss.good())
		{
			string substr;

			getline(ss, substr, ':');
			heights_.insert(heights_.begin(), stoi(substr));
		}

		return new Compressor(target, heights_, compactView_);
	}

	void Compressor::registerFactory(){
		UserInterface::add("Compressor", // name
				"A basic compressor.",
				"BasicInteger", // categories
				"",
				"columnHeights(string): colon-separated list of heights for the columns of the compressor, \
in decreasing order of the weight. Example: columnHeights=\"2:3\"; \
				compactView(bool)=false: whether the VHDL code is printed in a more compact way, or not",
				"",
				Compressor::parseArguments
		) ;
	}
}




