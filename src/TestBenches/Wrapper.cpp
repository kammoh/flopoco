/*
  A wrapper generator for FloPoCo.

  A wrapper is a VHDL entity that places registers before and after
  an operator, so that you can synthesize it and get delay and area,
  without the synthesis tools optimizing out your design because it
  is connected to nothing.

  Author: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.
 */

#include <iostream>
#include <sstream>
#include "Operator.hpp"
#include "Wrapper.hpp"

namespace flopoco{

	Wrapper::Wrapper(Target* target, Operator *op):
		Operator(target), op_(op)
	{
		string idext;

		setCopyrightString("Florent de Dinechin (2007)");
		//the name of the Wrapped operator consists of the name of the operator to be
		//	wrapped followd by _Wrapper
		setName(op_->getName() + "_Wrapper");

		//this operator is a sequential one	even if Target is unpipelined
		setSequential();

		//copy the signals of the wrapped operator
		// this replaces addInputs and addOutputs
		for(int i=0; i<op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);

			if(s->type() == Signal::in){
				//copy the input
				addInput(s->getName(), s->width(), s->isBus());
				//connect the input to an intermediary signal
				idext = "i_" + s->getName();
				vhdl << tab << declare(idext, s->width(), s->isBus()) << " <= " << delay(s->getName()) << ";" << endl;
				//connect the port of the wrapped operator
				inPortMap (op, s->getName(), idext);
			}else if(s->type() == Signal::out){
				//copy the output
				addOutput(s->getName(), s->width(), s->isBus());
				//connect the output
				idext = "o_" + s->getName();
				outPortMap (op, s->getName(), idext);
			}
		}

		// The VHDL for the instance
		vhdl << instance(op, "test");

		// copy the outputs
		for(int i=0; i<op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);

			if(s->type() == Signal::out) {
				string idext = "o_" + s->getName();
				vhdl << tab << s->getName() << " <= " << delay(idext) << ";" << endl;
			}
		}
	}

	Wrapper::~Wrapper() {
	}


}
