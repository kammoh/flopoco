/*
 * A generic class for tables of values
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
#include <cstdlib>
#include "utils.hpp"
#include "Table.hpp"

using namespace std;


namespace flopoco{


	int Table::double2input(double x){
		throw string("Error, double2input is being used and has not been overriden");
	}

	double Table::input2double(int x) {
		throw string("Error, input2double is being used and has not been overriden");
	}

	mpz_class Table::double2output(double x){
		throw string("Error, double2output is being used and has not been overriden");
	}

	double Table::output2double(mpz_class x) {
		throw string("Error, output2double is being used and has not been overriden");
	}

#if 0 // TODO some day
	mpz_class Table::mpfr2output(mpfr_t x){
		throw string("Error, mpfr2output is being used and has not been overriden");
	}

	void Table::output2mpfr(mpz_class x, mpfr_t y) {
		throw string("Error, output2mpfr is being used and has not been overriden");
	}
#endif



	Table::Table(Target* target, int _wIn, int _wOut, int _minIn, int _maxIn) : 
		Operator(target),
		wIn(_wIn), wOut(_wOut), minIn(_minIn), maxIn(_maxIn)
	{
		setCopyrightString("Florent de Dinechin (2007)");

		// Set up the IO signals
		addInput ("X"  , wIn, true);
		addOutput ("Y"  , wOut, 1, true);
		if ((target->getVendor()=="Xilinx")){
			setCombinatorial();
		}else
			nextCycle();
		
		if(maxIn==-1) maxIn=(1<<wIn)-1;
		if(minIn<0) {
			cerr<<"ERROR in Table::Table, minIn<0\n";
			exit(EXIT_FAILURE);
		}
		if(maxIn>=(1<<wIn)) {
			cerr<<"ERROR in Table::Table, maxIn too large\n";
			exit(EXIT_FAILURE);
		}
		if((minIn==0) && (maxIn==(1<<wIn)-1)) 
			full=true;
		else
			full=false;
		if (wIn > 10)
		  REPORT(0, "WARNING : FloPoCo is building a table with " << wIn << " input bits, it will be large.");
	}

Table::Table(Target* target) : 
		Operator(target){
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007, 2010)");
		if (false && (target->getVendor()=="Xilinx")){
			setCombinatorial();
		}else{
			nextCycle();
		}
	}

	// We have to define this method because the constructor of Table cannot use the (pure virtual) function()
	void Table::outputVHDL(std::ostream& o, std::string name) {

		if (wIn <= target_->lutInputs()){
			int i,x;
			mpz_class y;
			vhdl	<< "  with X select  Y <= " << endl;
			for (x = minIn; x <= maxIn; x++) {
				y=function(x);
				//if( y>=(1<<wOut) || y<0)
					//REPORT(0, "Output out of range" << "x=" << x << "  y= " << y );
				vhdl 	<< tab << "\"" << unsignedBinary(y, wOut) << "\" when \"" << unsignedBinary(x, wIn) << "\"," << endl;
			}
			vhdl << tab << "\"";
			for (i = 0; i < wOut; i++) 
				vhdl << "-";
			vhdl <<  "\" when others;" << endl;

			Operator::outputVHDL(o,  name);
		}
		else { 
			int x;
			mpz_class y;
			licence(o);

			o << "library ieee; " << endl;
			o << "use ieee.std_logic_1164.all;" << endl;
			o << "use ieee.numeric_std.all;" << endl;
			o << "library work;" << endl;
			outputVHDLEntity(o);
			newArchitecture(o,name);
			
			o << tab << "-- Build a 2-D array type for the RoM" << endl;

			if (maxIn-minIn<=256 && wOut>36){
				o << tab << "subtype word_t is std_logic_vector("<< (wOut%2==0?wOut/2-1:(wOut+1)/2-1) <<" downto 0);" << endl;
				o << tab << "type memory_t is array(0 to 2**"<<9<<"-1) of word_t;" << endl;
			}else{
				o << tab << "subtype word_t is std_logic_vector("<< wOut-1 <<" downto 0);" << endl;
				o << tab << "type memory_t is array(0 to 2**"<<wIn<<"-1) of word_t;" << endl;
			}
			
			o << tab <<"function init_rom" << endl;
			o << tab << tab << "return memory_t is " << endl;
			o << tab << tab << "variable tmp : memory_t := (" << endl;
			
			int left = (wOut%2==0?wOut/2:(wOut+1)/2);
			int right= wOut - left;
			if (maxIn-minIn <= 256 && wOut>36 /* TODO Replace with target->getBRAMWidth */){
				/*special BRAM packing */	
				//The first maxIn/2 go in the upper part of the table 
				for (x = minIn; x < (maxIn+1)/2; x++) {				
					y=function(x);
					o << tab << "\"" << unsignedBinary(y>>right, (wOut%2==0?wOut/2:(wOut+1)/2)) << "\"," << endl;
				}
				for (x = (maxIn+1)/2; x <= 255; x++) {				
					o << tab << "\"" << unsignedBinary(0, (wOut%2==0?wOut/2:(wOut+1)/2)) << "\"," << endl;
				}
				int d = maxIn -  (maxIn+1)/2;
				for (x = (maxIn+1)/2; x < maxIn; x++) {				
					y=function(x);
					y=y % (1 << right);
					o << tab << "\"" << unsignedBinary(y, (wOut%2==0?wOut/2:(wOut+1)/2)) << "\"," << endl;
				}
				for (x = d; x <= 255; x++) {				
					o << tab << "\"" << unsignedBinary(0, (wOut%2==0?wOut/2:(wOut+1)/2)) << "\"," << endl;
				}
			}else{
				for (x = minIn; x <= maxIn; x++) {
					y=function(x);
					//if( y>=(1<<wOut) || y<0)
						//REPORT(0, "Output out of range" << "x=" << x << "  y= " << y );
					o << tab << "\"" << unsignedBinary(y, wOut) << "\"," << endl;
				}
			}
			
			o << tab << tab << "others => (others => '0'));" << endl;
			o << tab << tab << "	begin " << endl;
			o << tab << tab << "return tmp;" << endl;
			o << tab << tab << "end init_rom;" << endl;
			
			o << "	signal rom : memory_t := init_rom;" << endl;
			if (maxIn-minIn <= 256 && wOut>36)
				o << tab << "signal Y1,Y0: std_logic_vector("<<(wOut%2==0?wOut/2-1:(wOut+1)/2-1) <<" downto 0);" << endl;
			
			beginArchitecture(o);		
			o << "	process(clk)" << endl;
			o << tab << "begin" << endl;
			o << tab << "if(rising_edge(clk)) then" << endl;
			if (maxIn-minIn <= 256 && wOut>36){
				o << tab << "	Y1 <= rom(  TO_INTEGER(unsigned(X))  );" << endl;
				o << tab << "	Y0 <= rom(  TO_INTEGER(unsigned(X)+256)  );" << endl;
			}else{
				o << tab << "	Y <= rom(  TO_INTEGER(unsigned(X))  );" << endl;
			}
			o << tab << "end if;" << endl;
			o << tab << "end process;" << endl;

			if (maxIn-minIn <= 256 && wOut>36){
				o << tab << " Y <= Y1 & Y0"<<range((wOut%2==0?wOut/2-1:(wOut-1)/2-1),0)<<";"<<endl; 
			}			
			
			endArchitecture(o);
		}
		
	}


	int Table::size_in_LUTs() {
		return wOut*int(intpow2(wIn-target_->lutInputs()));
	}

}
