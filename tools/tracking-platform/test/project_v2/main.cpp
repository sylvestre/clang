#include "library.hpp"
#include <iostream>

#define nulledptr( _name ) int * _name = 0;

int main( int, char** ) {
	
	int a = 3;
	nulledptr( pA );
	std::cout << "Result of somefunc: " 
			  << *pA + someFunction(3) << std::endl;
}



