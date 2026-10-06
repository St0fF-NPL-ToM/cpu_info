/*
	This file serves a single purpose: in case of debugging, it shall 
	tell the natvis parser, which libc is used in the backend.

	Todo that, a simple const variable will be inserted wherever you
	#include this file.
*/
#ifndef _NDEBUG
		const bool msvc{
	#ifdef _WIN32
			true
	#else
			false
	#endif
		};
#endif
