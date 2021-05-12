/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#ifndef SARA_TYPES_H
#define SARA_TYPES_H

// TODO: only needed for client.win
#include <X11/Xlib.h>


typedef struct client client;
typedef struct desktop desktop;
typedef struct monitor monitor;
typedef struct rule rule;

typedef union {
	int i;
	float f;
	/* received from sarasock */
	const char* s;
} Arg;

typedef struct {
	unsigned int mask;
	unsigned int btn;
	void (*func)(const Arg arg);
	const Arg arg;
} button;

typedef struct {
	const char letter;
	void (*arrange)(monitor*);
	/* for external layout setting */
	const char* name;
} layout;

struct client {
	int x, y, w, h;
	/* being in monocle is not considered floating */
	int isfloat;
	/* prior to togglefs */
	int oldfloat;
	int isfull;
	unsigned int desks;
	unsigned int iscur;
	client* next;
	monitor* mon;
	Window win;
}; 

struct desktop {
	float msize;
	layout* curlayout;
};

struct monitor {
	float msize;
	int curdesk;
	int mx, my, mh, mw, wy, wh;
	int num;
	unsigned int seldesks;
	client* current;
	//client* prev;
	client* head;
	desktop* desks;
	layout* curlayout;
	monitor* next;
};

struct rule {
	const char* class;
	const char* instance;
	const char* title;
	int desks;
	int isfloat;
	int isfull;
	int monitor;
	rule* next;
};


#endif
