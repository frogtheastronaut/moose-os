malloc:
	clang -dynamiclib -arch arm64 -arch arm64e -o malloc.dylib malloc.c
	export DYLD_INSERT_LIBRARIES=$(PWD)/malloc.dylib
unset:
	unset DYLD_INSERT_LIBRARIES
remove:
	rm malloc.dylib 
	rm -rf malloc.dylib.dSYM