/*
 * Parser.cpp
 *
 *  Created on: 13 nov. 2017
 *      Author: timmy.lindholm
 */

#include <stack>
#include <iostream>

#include "parser.h"
#include "lexer.h"
#include "error.h"

// Defines an operator with correct precedence
typedef struct {
	char * name;
	int precedence;
} Operator;

Parser::Parser() {

	this->lexer = new Lexer;
	this->buffer = NULL;
	this->global_program = NULL;
	this->program = NULL;
}

/*
 * Parses the code which resides in buffer
 * May be called recursively, thus the non-static program
 * pointer
*/
void Parser::parse(Program * program) {
	Token * tok;
	FunctionCall * funccall;

	this->program = program;
	tok = lexer->next_token();

	while(tok->type != TOK_NULL && tok->type != TOK_RIGHT_CBRACK) {
		// Entry points for keywords and instructions
		switch(tok->type) {

		// Function or variable name
		case TOK_NAME:
			const char * name;

			name = tok->value;
			tok = lexer->next_token();

			// Function call
			if(tok->type == TOK_LEFT_PAR) {
				parse_function_call(name, &funccall);
			}

			// Function definition
			else if(tok->type == TOK_COLON) {
				if(program != global_program)
					ERROR(T_CRIT, "function definitions may only occur in the global scope, on line %d.", lexer->n_lines);

				parse_function_definition(name);
			}

			// Assignment operation
			else if(IS_ASSIGNMENT(tok->type)) {
				parse_assignment_operation(name, tok->type);
			}

			// Unknown
			else {
				ERROR(T_CRIT, "unknown token %s on line %d.", name, lexer->n_lines);
			}

			break;

		// If or else if
		case TOK_IF:
			parse_if_statement();
			break;

		case TOK_ELSE_IF:
			break;

		// While loop
		case TOK_WHILE:
			break;

		// For all
		case TOK_UNI_QUANT:
			break;

		// Return statement
		case TOK_RETURN:
			if(program->program_type != PROGRAM_FUNCTION) {
				if(program->parent_program->program_type != PROGRAM_FUNCTION)
					ERROR(T_CRIT, "return statements may only be used inside functions, on line %d.", lexer->n_lines);
			}

			parse_return_operation();
			break;

		case TOK_AT:
			parse_inline_code_operation();

			break;

		default:
			break;
		}

		tok = lexer->next_token();
	}
}

// Parse an inline code operation
void Parser::parse_inline_code_operation() {
	Token * tok;
	int left_curly;
	std::vector<Token *> * code;

	left_curly = 0;
	code = new std::vector<Token *>();

	tok = lexer->next_token();

	// Make sure token is of left curly
	if(tok->type != TOK_LEFT_CBRACK)
		ERROR(T_CRIT, "unexpected token %s on line %d, expected {", tok->value, lexer->n_lines);

	// Fetch all tokens until right curly is reached
	tok = lexer->next_token();

	while(tok->type != TOK_NULL) {

		// Look for end of script
		if(tok->type == TOK_RIGHT_CBRACK) {
			if(left_curly)
				left_curly--;
			else
				break;
		}

		else if(tok->type == TOK_LEFT_CBRACK)
			left_curly++;

		code->push_back(tok);
		tok = lexer->next_token();
	}

	// No ending curly
	if(tok->type == TOK_NULL)
		ERROR(T_CRIT, "unexpected end of file on line %d", lexer->n_lines);

	// Add inline injection instruction
	InlineInjection * instruction = new InlineInjection(code);

	program->push_instruction(instruction);
}

// Parse an assignment operation and create the corresponding memory layout
void Parser::parse_assignment_operation(const char * name, int assign_type) {
	std::vector<Token *> * expression;
	int type;

	// Get the expression
	expression = parse_expression(type);

	for(auto x : *expression) {
		std::cerr << x->value << ": " << x->type << " ";
	}

	std::cerr << std::endl;

	// Create the variable and add it to the current program
	Variable * variable = new Variable(name, expression, type);
	program->push_variable(variable);

	// Create assignment instruction
	Assignment * assignment = new Assignment(variable, assign_type);
	program->push_instruction(assignment);
}

// Convert infix expression to postfix expression for convenience
// and determine the type of the expression, return in standard form
std::vector<Token *> * Parser::parse_expression(int &type, int tok_delim) {
	Token * tok;
	int left_pars;
	std::stack<Token *> op_stack;
	std::vector<Token *> * expression, * exp;
	std::unordered_map<int, int> operators;

	operators[TOK_MULT] = 3;
	operators[TOK_DIV] = 3;
	operators[TOK_PLUS] = 2;
	operators[TOK_MINUS] = 2;
	operators[TOK_LEFT_PAR] = 1;

	expression = new std::vector<Token *>;
	exp = new std::vector<Token *>;

	left_pars = 0;
	type = TOK_NULL;

	// Loop through tokens until dot or end of line
	tok = lexer->next_token();

	while(tok->type != TOK_NULL && tok->type != tok_delim) {
		if(tok->type == TOK_RIGHT_PAR && ! left_pars)
			break;

		exp->push_back(tok);

		// Numeric operand
		if(tok->type == TOK_INT || tok->type == TOK_FLOAT) {
			if(tok->type == TOK_FLOAT)
				type = TOK_FLOAT;
			else if(tok->type == TOK_INT && type != TOK_FLOAT && type != TOK_STRING)
				type = TOK_INT;

			expression->push_back(tok);
		}

		// String operand
		else if(tok->type == TOK_STRING) {
			type = TOK_STRING;
			expression->push_back(tok);
		}

		// Function or variable operand
		else if(tok->type == TOK_NAME) {
			Token * name = tok;
			FunctionCall * instruction = nullptr;
			int first = 1;
			tok = lexer->next_token();

			// Function
			if(tok->type == TOK_LEFT_PAR) {
				auto func = parse_function_call(name->value, &instruction);

				// Check if function has a certain return type
				if(type != TOK_NULL) {
					if((! func->check_return_type(type)) && type != TOK_STRING)
						ERROR(T_CRIT, "invalid return value of function %s on line %d.", name->value, lexer->n_lines);
				}

				else
					type = func->get_return_type();

				expression->push_back(new Token((char * const) "@", TOK_LEFT_PAR));
				exp->push_back(new Token((char * const) "(", TOK_LEFT_PAR));

				std::vector<Token *> * arg;
				while((arg = instruction->get_next_argument())) {

					if(first)
						first = 0;
					else {
						exp->push_back(new Token((char * const) ",", TOK_LEFT_PAR));
					}

					for(auto tok_ : *arg) {
						exp->push_back(tok_);
					}
				}

				exp->push_back(new Token((char * const) ")", TOK_RIGHT_PAR));
				tok = lexer->next_token();
			}

			// Variable
			else {
				auto var = program->get_variable(name->value);

				// Determine if variable exists
				if(! var)
					ERROR(T_CRIT, "undefined variable %s on line %d.", name->value, lexer->n_lines);

				// Determine variable type
				if(type != TOK_NULL) {
					if(var->type != type && type != TOK_STRING)
						ERROR(T_CRIT, "invalid type of variable %s on line %d.", name->value, lexer->n_lines);
				}

				else
					type = var->type;
			}

			expression->push_back(name);

			continue;
		}

		else if(tok->type == TOK_LEFT_PAR) {
			left_pars++;
			op_stack.push(tok);
		}

		else if(tok->type == TOK_RIGHT_PAR) {
			Token * op;
			op = NULL;

			// Pop stack until corresponding left paranthesis is found
			while((! op_stack.empty()) && (op = op_stack.top())->type != TOK_LEFT_PAR) {
				expression->push_back(op);
				op_stack.pop();
			}

			// Matching paranthesis
			if(op != NULL && op->type == TOK_LEFT_PAR) {
				op_stack.pop();
			}

			else {
				ERROR(T_CRIT, "faulty expression on line %d", lexer->n_lines);
			}

			left_pars--;
		}

		// Operator
		else if(IS_OPERATOR(tok->type)) {
			Token * op;
			int prec;

			op = NULL;
			prec = operators[tok->type];

			// Add operators with higher precedence
			while(! op_stack.empty() && prec <= operators[(op = op_stack.top())->type]) {
				expression->push_back(op);
				op_stack.pop();
			}

			op_stack.push(tok);
		}

		// Invalid token
		else
			ERROR(T_CRIT, "unexpected '%s' on line %d", tok->value, lexer->n_lines);

		tok = lexer->next_token();
	}

	// Check if no ending delimiter
	if(tok->type == TOK_NULL)
		ERROR(T_CRIT, "unexpected end of file on line %d", lexer->n_lines);

	// Add remaining operators to expression
	while(! op_stack.empty()) {
		expression->push_back(op_stack.top());
		op_stack.pop();
	}

	delete expression;
	return exp;
}

// Parse a logical expression and return the associated tokens in standard
// notation
std::vector<Token *> * Parser::parse_logical_expression(int tok_delim) {
	Token * tok;
	int last_type, comparison;
	std::vector<Token *> * exp;
	std::vector<Token *> * expression;
	std::stack<Token *> op_stack;
	std::unordered_map<int, int> operators;

	operators[TOK_GREATER] = 4;
	operators[TOK_LESSER] = 4;
	operators[TOK_GREATER_EQUAL] = 4;
	operators[TOK_LESSER_EQUAL] = 4;
	operators[TOK_AND] = 3;
	operators[TOK_OR] = 2;
	operators[TOK_LEFT_PAR] = 1;

	expression = new std::vector<Token *>;
	exp = new std::vector<Token *>;


	last_type = TOK_NULL;
	comparison = 0;

	tok = lexer->next_token();

	// Loop through expression token by token
	while(tok->type != TOK_NULL && tok->type != tok_delim) {
		exp->push_back(tok);

		// Whether the comparsion flag is set, make sure types are comparable
		if(comparison) {
			if(tok->type != TOK_NAME && last_type != tok->type) {
				ERROR(T_CRIT, "comparison of different types, on line %d", lexer->n_lines);
			}

			else if(tok->type != TOK_NAME)
				comparison = 0;
		}

		// Numeric operand
		if(tok->type == TOK_INT || tok->type == TOK_FLOAT) {
			if(tok->type == TOK_FLOAT)
				last_type = TOK_FLOAT;
			else
				last_type = TOK_INT;

			expression->push_back(tok);
		}

		// String operand
		else if(tok->type == TOK_STRING) {
			last_type = TOK_STRING;
			expression->push_back(tok);
		}

		// Function or variable operand
		else if(tok->type == TOK_NAME) {
			Token * name = tok;
			FunctionCall * funccall;
			int last_type_cp;

			last_type_cp = last_type;
			tok = lexer->next_token();

			// Function
			if(tok->type == TOK_LEFT_PAR) {
				auto func = parse_function_call(name->value, &funccall);
				last_type = func->get_return_type();

				expression->push_back(new Token((char * const) "@", TOK_AT));
				exp->push_back(new Token((char * const) "@", TOK_AT));

				tok = lexer->next_token();
			}

			// Variable
			else {
				auto var = program->get_variable(name->value);

				// Determine if variable exists
				if(! var)
					ERROR(T_CRIT, "undefined variable %s on line %d.", name->value, lexer->n_lines);

				// Determine variable type
				last_type = var->type;
				std::cerr << var->type << std::endl;
			}

			// Whether the comparsion flag is set, make sure types are comparable
			if(comparison) {
				if(last_type != last_type_cp)
					ERROR(T_CRIT, "comparison of different types, on line %d", lexer->n_lines);

				comparison = 0;
			}

			expression->push_back(name);
			continue;
		}

		else if(tok->type == TOK_LEFT_PAR) {
			op_stack.push(tok);
		}

		else if(tok->type == TOK_RIGHT_PAR) {
			Token * op;
			op = NULL;

			// Pop stack until corresponding left paranthesis is found
			while((! op_stack.empty()) && (op = op_stack.top())->type != TOK_LEFT_PAR) {
				expression->push_back(op);
				op_stack.pop();
			}

			// Matching paranthesis
			if(op != NULL && op->type == TOK_LEFT_PAR) {
				op_stack.pop();
			}

			else
				ERROR(T_CRIT, "faulty expression on line %d", lexer->n_lines);

		}

		// Comparison operator
		else if(IS_COMPARISON(tok->type)) {
			Token * op;
			int prec;

			comparison = 1;
			op = NULL;
			prec = operators[tok->type];

			// Add operators with higher precedence
			while(! op_stack.empty() && prec <= operators[(op = op_stack.top())->type]) {
				expression->push_back(op);
				op_stack.pop();
			}

			op_stack.push(tok);
		}

		// Logical and or or
		else if(tok->type == TOK_AND || tok->type == TOK_OR) {
			Token * op;
			int prec;

			op = NULL;
			prec = operators[tok->type];

			// Add operators with higher precedence
			while(! op_stack.empty() && prec <= operators[(op = op_stack.top())->type]) {
				expression->push_back(op);
				op_stack.pop();
			}

			op_stack.push(tok);
		}

		// Invalid token
		else
			ERROR(T_CRIT, "unexpected '%s' on line %d", tok->value, lexer->n_lines);

		tok = lexer->next_token();
	}

	// Check if no ending delimiter
	if(tok->type == TOK_NULL)
		ERROR(T_CRIT, "unexpected end of file on line %d", lexer->n_lines);

	// Add remaining operators to expression
	while(! op_stack.empty()) {
		expression->push_back(op_stack.top());
		op_stack.pop();
	}

	delete expression;
	return exp;
}

/* Parse a call to a function, output an error if function for some reason
 * does not exist or have matches with arguments
 */
Function * Parser::parse_function_call(const char * func_name, FunctionCall ** function_call) {
	Function * func = nullptr;

	// Try to fetch function
	func = global_program->get_function(func_name);

	// Means function could not be found in program map
	if(! func)
		ERROR(T_CRIT, "unknown call to function %s, on line %d.", func_name, lexer->n_lines);

	*function_call = new FunctionCall(func);

	int args_size = func->get_arguments_size();
	int n = 0;

	if(args_size)
	while(lexer->last_token->type != TOK_RIGHT_PAR && lexer->last_token->type != TOK_NULL) {
		Argument * arg = func->get_next_argument();

		int type;

		arg->value = parse_expression(type, TOK_COMMA);
		arg->type = type;

		if(arg->type == TOK_STRING) {
			arg->value->insert(arg->value->begin(), new Token((char * const) "\"", TOK_QUOTE));
			arg->value->push_back(new Token((char * const) "\"", TOK_QUOTE));
		}

		(*function_call)->push_argument(arg->value);

		n++;
	}

	else
		lexer->next_token();

	// No closing right paranthesis
	if(lexer->last_token->type != TOK_RIGHT_PAR)
		ERROR(T_CRIT, "unexpected end of function %s on line %d.", func_name, lexer->n_lines);

	// Too many or too few arguments
	if(n != args_size)
		ERROR(T_CRIT, "invalid number of arguments in call to function %s, on line %d.", func_name, lexer->n_lines);

	// Update return statement in case that it depend on the arguments
	if(! func->get_return_type())
		func->set_return_type(TOK_AUTO);

	// Push function call to program instruction queue
	program->push_instruction(*function_call);

	return func;
}

// Parse a function definition with name [name]
// error will occur if function does already exist
// return pointer to function
Function * Parser::parse_function_definition(const char * func_name) {
	Function * function;

	if(global_program->get_function(func_name))
		ERROR(T_CRIT, "redefinition of function %s on line %d.", func_name, lexer->n_lines);

	function = new Function(program);
	function->name = func_name;

	lexer->next_token();

	if(lexer->last_token->type != TOK_LEFT_PAR)
		ERROR(T_CRIT, "unexpected %s on line %d.", lexer->last_token->value, lexer->n_lines);

	// Parse arguments
	lexer->next_token();

	const char * var_name;
	int default_value_type;
	std::vector<Token *> * default_value;

	var_name = NULL;
	default_value = NULL;
	default_value_type = TOK_NULL;

	while(lexer->last_token->type != TOK_RIGHT_PAR && lexer->last_token->type != TOK_NULL) {
		// Add variable to function
		if(lexer->last_token->type == TOK_COMMA) {
			Argument * argument = new Argument(var_name, default_value, default_value_type);

			var_name = NULL;
			default_value = NULL;
			default_value_type = TOK_NULL;

			// Add argument to function
			function->push_argument(argument);
		}

		// Set default value for argument
		else if(lexer->last_token->type == TOK_EQUAL) {
			Token * value = lexer->next_token();

			default_value = new std::vector<Token *>;
			default_value->push_back(value);

			default_value_type = value->type;
		}

		// Argument
		else if(lexer->last_token->type == TOK_NAME && var_name == NULL) {
			var_name = lexer->last_token->value;
		}

		else
			ERROR(T_CRIT, "unexpected %s on line %d.", lexer->last_token->value, lexer->n_lines);

		lexer->next_token();
	}

	if(lexer->last_token->type == TOK_NULL)
		ERROR(T_CRIT, "no ending delimiter for argument list of function %s on line %d.", func_name, lexer->n_lines);

	// Push last argument to function
	if(var_name != NULL) {
		Argument * argument = new Argument(var_name, default_value, default_value_type);
		function->push_argument(argument);
	}

	lexer->next_token();
	if(lexer->last_token->type != TOK_IMPLIES)
		ERROR(T_CRIT, "unexpected %s on line %d.", lexer->last_token->value, lexer->n_lines);

	lexer->next_token();
	if(lexer->last_token->type != TOK_LEFT_CBRACK)
		ERROR(T_CRIT, "function definition requries a { } block, function %s on line %d.", func_name, lexer->n_lines);

	// Call parse recursevily to parse function instructions
	// Save current program to restore it after parsing
	Program * current = program;
	parse(function);
	program = current;

	// No ending curly bracket
	if(lexer->last_token->type == TOK_NULL)
		ERROR(T_CRIT, "unexpected end of function %s, missing '}' on line %d.", func_name, lexer->n_lines);

	// Push function to program
	global_program->push_function(func_name, function);

	return function;
}

// Parse an if statement
// Create an if instruction and push it to the current program
void Parser::parse_if_statement() {
	std::vector<Token *> * expression;

	// Get logical expression
	expression = parse_logical_expression(TOK_IMPLIES);

	lexer->next_token();
	if(lexer->last_token->type != TOK_LEFT_CBRACK)
		ERROR(T_CRIT, "code execution definition requries a { } block, if statement on line %d.", lexer->n_lines);

	// Call parse recursevily to parse code block instructions
	// Save current program to restore it after parsing
	Program * if_program = new Program(program);
	Program * current = program;

	parse(if_program);
	program = current;

	// No ending curly bracket
	if(lexer->last_token->type != TOK_RIGHT_CBRACK)
		ERROR(T_CRIT, "unexpected end of code block, missing '}' on line %d.", lexer->n_lines);

	// Push instruction to program
	IfStatement * if_statement = new IfStatement(expression, if_program);

	program->push_instruction(if_statement);
}

// Parse a return operation and set the corresponding
// return type of the function
void Parser::parse_return_operation() {
	int type;
	std::vector<Token *> * value;

	// Make sure in the correct program
	Program * prog = program;
	Program * current_program = program;

	if(program->program_type != PROGRAM_FUNCTION)
	while((prog = prog->parent_program)->program_type != PROGRAM_FUNCTION);

	program = prog;

	value = parse_expression(type);

	Function * func = static_cast<Function *>(program);
	func->set_return_type(type);

	// Return to current program
	program = current_program;

	// Push instruction to current program
	ReturnOperation * return_op = new ReturnOperation(value);
	program->push_instruction(return_op);
}

// Calls the main parse function with the global program as argument
Program * Parser::parse() {
	global_program = new GlobalProgram;

	this->parse(global_program);

	return global_program;
}

/*
 * Sets the internal code buffer to the argument
 * This way we do not have to pass the buffer to the lexer
 * Each call.
 */
void Parser::set_input_code(char * buffer) {
	this->buffer = buffer;
	this->lexer->set_input_code(buffer);
}

// Set the lexer line start
void Parser::set_line_start(int start) {
	this->lexer->n_lines = -1 * start;
}
