/*
 * Compiler.cpp
 *
 *  Created on: 13 nov. 2017
 *      Author: timmy.lindholm
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "compiler.h"
#include "error.h"

#define READ_FILE_ERROR 1

Compiler::Compiler() {

	this->parser = new Parser();
	this->translator = new Translator();
	this->buffer = NULL;
	this->program = NULL;

}

Compiler::~Compiler() {
	// TODO Auto-generated destructor stub
}

// Compile a file specified by the argument
int Compiler::compile(char * file_name) {
	try {
		// Fetch code file
		this->buffer = this->read_file(file_name);
	} catch(int e) {
		ERROR(T_CRIT, "failed to read from file %s.", file_name);
	}

	std::cerr << "INFO:\n" << std::endl;

	// Pass buffer to parser and parse the file
	this->parser->set_line_start(this->line_start);
	this->parser->set_input_code(this->buffer);
	program = this->parser->parse();

	translator->translate(program);

	return 1;
}

/* Opens a file and reads the content into a buffer
 * Throws an error if file could not be opened
 */
char * Compiler::read_file(char * file_name) {
	FILE * file_stream;
	char * buffer, * pbuffer;

	if(! (file_stream = fopen(file_name, "r")))
		throw READ_FILE_ERROR;

	buffer = (char *) malloc(200 * sizeof(char));
	pbuffer = buffer;

	int n_total_read = 0;
	int n_read = 0;
	int i = 1;

	while((n_read = fread(pbuffer, sizeof(char), 200, file_stream)) == 200) {
		n_total_read += n_read;

		buffer = (char *) realloc(buffer, ++i * 200 * sizeof(char) + 1);
		pbuffer = buffer + n_total_read;
	}

	n_total_read += n_read;
	buffer[n_total_read] = '\0';

	fclose(file_stream);
	return buffer;
}
