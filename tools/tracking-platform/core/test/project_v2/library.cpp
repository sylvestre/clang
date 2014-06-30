#include "library.hpp"

int someFunction( int _in ) {
	int * pIn = & _in;

	return *pIn + 3;
}