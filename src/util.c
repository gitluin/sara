/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "util.h"


void
die(const char* e, ...){
	fprintf(stdout, "sara: %s\n", e);
	exit(1);
}

void*
ecalloc(size_t nmemb, size_t size){
	void* p;

	if ( !(p = calloc(nmemb, size)) )
		die("ecalloc failed");

	return p;
}

void
estrtoi(const char* s, Arg* arg){
	arg->i = (int) strtol(s, (char**) NULL, 10);
}

void
estrtof(const char* s, Arg* arg){
	arg->f = (float) strtof(s, (char**) NULL);
}

/* more flexible strtoul */
unsigned int
estrtoul(char* str, int nchar, int base){
	char* check, * tmp, * local_str, * saveptr;
	char** tmp_ops;
	int i, j;
	unsigned int to_store;
	unsigned int* tmp_vals;
	int nops = 0, nvals = 0, panic = 0;
	char** ops = ecalloc(1, sizeof(char*));
	unsigned int ret = 0;
	unsigned int* vals = ecalloc(1, sizeof(unsigned int));

	local_str = strndup(str, nchar*sizeof(char));

	if (base != 0){
		ret = strtoul(local_str, &check, base);

		/* if entire string was valid, all good */
		if (*local_str != '\0' && *check == '\0')
			return ret;
	}
	
	if ( (tmp = strtok_r(local_str, " ", &saveptr)) ){
		to_store = (unsigned int) strtoul(tmp, &check, 10);

		/* if you don't start with a value, ya dun fer */
		if ( !(*tmp != '\0' && *check == '\0') )
			return 0;

		vals[0] = to_store;
		nvals++;
	}

	while ( (tmp = strtok_r(NULL, " ", &saveptr)) ){
		printf("tmp is: %s\n", tmp);
	
		if (strcmp(tmp, ">>") == 0 || strcmp(tmp, "<<") == 0){
			/* if double operators, we're done */
			if (ops[nops] && (strcmp(ops[nops], ">>") == 0 || strcmp(ops[nops], "<<") == 0)){
				break;

			} else {
				nops++;

				if ( (tmp_ops = realloc(ops, nops * sizeof(char *))) ){
					ops = tmp_ops;
					ops[nops-1] = tmp;

				} else {
					panic = 1;
				}
			}

		} else if ( (to_store = strtoul(tmp, (char**) NULL, 10)) ){
			nvals++;

			if ( (tmp_vals = realloc(vals, nvals * sizeof(int))) ){
				vals = tmp_vals;
				vals[nvals-1] = to_store;

			} else {
				panic = 1;
			}
		}

		if (panic){
			free(ops);
			free(vals);
			free(local_str);
			fprintf(stderr, "estrtoul: failed to realloc ops or vals\n");
			return 0;
		}
	}

	/* trim any excess
	 * will prevent catastrophic failure from malformed value input, but
	 * junk will ensue!
	 */
	while (nvals > nops + 1 && nvals > 0)
		nvals--;

	while (nops > nvals - 1 && nops > 0)
		nops--;

	for (i=0,j=i-1;i < nvals;i++,j++){
		if (i == 0){
			ret = vals[i];
			continue;
		}

		if (strcmp(ops[j], ">>") == 0)
			ret >>= vals[i];
		else if (strcmp(ops[j], "<<") == 0)
			ret <<= vals[i];
	}

	free(ops);
	free(vals);
	free(local_str);

	return ret;
}

int
slen(const char* str){
	int i = 0;

	for (;*str;str++,i++);

	return i;
}

/* convert 11011110 to "01111011"
 * for this example, len = 8
 * dest must be a calloc'd char* that you free() afterwards
 */
void
uitos(unsigned int ui, int len, char* dest){
	int i, j, res;
	int bytearray[len];
	char bytestr[len + 1];

	/* reverse the array, as tags are printed left to right, not right to left */
	for (i=0;i < len;i++)
		bytearray[i] = ui >> i & 1;

	for (i=0, j=0;
	(i < (len + 1)) && (res = snprintf(bytestr + j, (len + 1) - j, "%d", bytearray[i])) > 0;
	i++)
		j += res;

	snprintf(dest, len + 1, "%s", bytestr);
}
