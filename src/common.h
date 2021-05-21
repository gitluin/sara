/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#ifndef COMMON_H
#define COMMON_H

#define INPUTSOCK			"/tmp/sara.sock"
/* longest is "changemsize -0.05" at 17, +1 for '\0' */
#define MAXBUFF				18
/* max length of a progs command */
#define MAXLEN				256

void die(const char* e, ...);
int slen(const char* str);

#endif
