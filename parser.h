/*
 * Parser.h
 *
 *  Created on: 13 nov. 2017
 *      Author: timmy.lindholm
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <vector>

#include "lexer.h"
#include "mem/program.h"
#include "mem/function.h"

class Parser {

public:
	Parser();

	// Parse the code which resides in buffer, use the program passed
	// as argument
	void parse(Program * program);

	// Entry point, call this function to set up a new program and
	// to initialize a new parser
	Program * parse();

	// Sets the internal buffer to the code pointed to by the argument
	void set_input_code(char * buffer);

	// Set lexer line start
	void set_line_start(int start);

private:
	Lexer * lexer;
	char * buffer;

	// Global program and current program
	GlobalProgram * global_program;
	Program * program;

	// Parse an inline code operation, C/C++ code
	void parse_inline_code_operation();

	// Parse an assignment operation, for instace = or +=
	void parse_assignment_operation(const char * name, int assign_type);

	// Convert infix expression to postfix expression
	std::vector<Token *> * parse_expression(int &type, int tok_delim = TOK_DOT);

	// Convert infix logical expression to postfix expression
	std::vector<Token *> * parse_logical_expression(int tok_delim);

	// Parse a call for a function with name [name]
	// return a pointer to the function itself
	Function * parse_function_call(const char * func_name, FunctionCall ** function_call = nullptr);

	// Parse a function definition with name [name]
	// return a pointer to the fuction itself
	Function * parse_function_definition(const char * func_name);

	// Parse a return operation
	void parse_return_operation();

	// Parse an if statement
	void parse_if_statement();
};

#endif /* PARSER_H_ */
