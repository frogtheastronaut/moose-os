#include "assert/assert.h"
#include "print/debug.h"

void assert(int condition) {
	if (!condition) {
		debugf("[MOOSE] Assertion failed!\n");
		for(;;);
	}
}