/*
 * program.h
 *
 *  Created on: 17 nov. 2017
 *      Author: eatit
 */

#ifndef MEM_PROGRAM_H_
#define MEM_PROGRAM_H_

#include <map>
#include <queue>

#include "variable.h"
#include "instruction.h"

#define PROGRAM_FUNCTION 1
#define PROGRAM_GLOBAL 2

class Function;

class Program {

public:

	// Initializor, pass the parent program as parameter
	Program(Program * parent_program, const int program_type = PROGRAM_GLOBAL);

	/*
	 * Defines the parent program, for instance the main program for a global function
	 */
	Program * parent_program;
	const int program_type;

	// Defines a set of variables in the current program scope
	std::unordered_map<std::string, Variable *> * variables;

	// Get the variable [name] in current program
	// return 0 if it does not exist
	virtual Variable * get_variable(std::string name);

	// Add variable [name] to the current program
	void push_variable(Variable * var);

	// Get the next instruction
	Instruction * get_next_instruction();

	// Push a new instruction on to the instruction queue
	void push_instruction(Instruction * instruction);

private:

	// Defines a set of instructions following the FIFO format
	std::queue<Instruction *> * instructions;

};

// Defines a global program, for instance a new file
class GlobalProgram : public Program {

public:
	GlobalProgram();

	// Defines a set of functions
	std::map<std::string, Function *> * functions;

	// Get the function in global program with [name]
	// return 0 if no function was found
	Function * get_function(std::string name);
	Function * get_function(char * name);

	// Push a function to the function map
	void push_function(const char * name, Function * function);


private:

};
#endif /* MEM_PROGRAM_H_ */
