#include "library.hpp"

int someFunction( int _in ) {
	int * pIn = & _in;
	pIn = 0;

	return *pIn + 3;
}