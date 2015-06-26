/*
 * An instance of an Operator.
 * Instance is to Operator what a VHDL architecture is to an entity
 */

#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <vector>
#include <map>

#include "utils.hpp"

#include "Operator.hpp"

using namespace std;

namespace flopoco {

class Instance {

public:

	/**
	 * Return the name of the operator
	 */
	std::string getName();

	/**
	 * Set the name of the operator
	 */
	void setName();

	/**
	 * Return the the operator for this instance
	 */
	std::string getOperator();

	/**
	 * Add a new input port connection
	 */
	std::string addInputPort(std::string portName, Signal* connectedSignal);

	/**
	 * Remove an input port connection
	 */
	bool removeInputPort(std::string portName);

	/**
	 * Clear the input port connections
	 */
	void clearInputPorts();

	/**
	 * Add a new output port connection
	 */
	std::string addOutputPort(std::string portName, Signal* connectedSignal);

	/**
	 * Remove an output port connection
	 */
	bool removeOutputPort(std::string portName);

	/**
	 * Clear the output port connections
	 */
	void clearOutputPorts();


	std::string name;                           /*< the name of the instance */
	Operator* op;                               /*< the operator for this instance */
	map<std::string, Signal*> inPortMap;        /*< the port mapping for the inputs of the operator */
	map<std::string, Signal*> outPortMap;       /*< the port mapping for the outputs of the operator */

protected:

private:

};

} //namespace

#endif /* INSTANCE_HPP */
