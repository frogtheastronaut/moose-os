/*
    MooseOS Assertion code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "assert/assert.h"
#include "print/debug.h"

void assert(int condition) {
	if (!condition) {
		debugf("[MOOSE] Assertion failed!\n");
		for(;;);
	}
}