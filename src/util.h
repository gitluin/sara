/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#ifndef SARA_UTIL_H
#define SARA_UTIL_H

#include <string.h>

#include "types.h"


#define NUMTAGS				9
#define MOUSEMOD			Mod4Mask
#define MASTER_SIZE     		0.55
#define SAFEPARG(A,B)			((A < parg.i && parg.i < B))
#define BUTTONMASK              	(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK               	(BUTTONMASK|PointerMotionMask)
#define EACHCLIENT(I)			(ic=I;ic;ic=ic->next) /* ic is a global */
#define EACHMON(M)			(im=M;im;im=im->next) /* im is a global */
#define ISOUTSIDE(PX,PY,X,Y,W,H)	((PX > X + W || PX < X || PY > Y + H || PY < Y))
#define ISVISIBLE(C)			((C->desks & C->mon->seldesks))
#define MAX(A,B)               		((A) > (B) ? (A) : (B))
#define STREQ(A,B)			((strcmp(A,B) == 0))
#define TABLENGTH(X)    		(sizeof(X)/sizeof(*X))
#define INPUTSOCK			"/tmp/sara.sock"
// TODO: reasonable max for rules, configs?
#define MAXBUFF				18*sizeof(char) /* longest is "changemsize -0.05" at 17, +1 for '\0' */
#define MAXLEN				256


enum { AnyVis,     OnlyVis };
enum { NoZoom,     YesZoom };
enum { NoFocus,    YesFocus };
enum { WantMove,   WantResize };
enum { WantTiled,  WantFloating };
enum { WantInt,    WantFloat,	NumTypes};


void die(const char* e, ...);
void* ecalloc(size_t nmemb, size_t size);
void estrtoi(const char* s, Arg* arg);
void estrtof(const char* s, Arg* arg);
unsigned int estrtoul(char* str, int nchar, int base);
int slen(const char* str);
void uitos(unsigned int ui, int len, char* dest);

/* for EACHCLIENT/EACHMON iterating */
extern client* ic;
extern monitor* im;

#endif
