/*
 * main.cpp
 *
 *  Created on: 22 aug. 2017
 *      Author: timmy
 */

#include <iostream>
#include "Compiler.h"

int main() {
	Compiler compiler;

	compiler.compile((d_string) "func : [x, y] -> { ret (x + y). }");
	return 0;
}


