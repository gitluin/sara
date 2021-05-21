/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#include <stdio.h>
#include <stdlib.h>

void
die(const char* e, ...){
	fprintf(stdout, "sara: %s\n", e);
	exit(1);
}

int
slen(const char* str){
	int i = 0;

	for (;*str;str++,i++);

	return i;
}
