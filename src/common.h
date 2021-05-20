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
/* longest I accounted for is "r firefox NULL some-thing '1 << 8' 1 1 -1"
 * at 41, +1 for '\0'
 */
#define MAXBUFF				42*sizeof(char) 
/* max size of r->class, r->instance, r->title
 * when parsing rules from sarasock, this will never fill up beyond what
 * MAXBUFF allows
 */
#define MAXLEN				256

void die(const char* e, ...);

#endif
