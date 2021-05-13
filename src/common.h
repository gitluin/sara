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
// TODO: reasonable max for rules, configs?
#define MAXBUFF				18*sizeof(char) /* longest is "changemsize -0.05" at 17, +1 for '\0' */

void die(const char* e, ...);

#endif
