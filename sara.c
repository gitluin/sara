/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch              
 * Copyright (c) 2020, This Fackin Guy, gitluin on github (no email for you!)     
 *                                                                                
 * Permission is hereby granted, free of charge, to any person obtaining a        
 * copy of this software and associated documentation files (the "Software"),     
 * to deal in the Software without restriction, including without limitation      
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,       
 * and/or sell copies of the Software, and to permit persons to whom the          
 * Software is furnished to do so, subject to the following conditions:           
 *                                                                                
 * The above copyright notice and this permission notice shall be included in     
 * all copies or substantial portions of the Software.                            
 *                                                                                
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL        
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING        
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER            
 * DEALINGS IN THE SOFTWARE.                                                      
 * 
 */ 

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xft/Xft.h>
//#ifdef XINERAMA
//#include <X11/extensions/Xinerama.h>
//#endif

#define TABLENGTH(X)    		(sizeof(X)/sizeof(*X))
#define ISVISIBLE(C)			((C->desktops & seldesks))
#define TEXTW(X)			(gettextwidth(X, slen(X)) + lrpad)
#define EACHCLIENT(_I)			(ic=_I;ic;ic=ic->next) /* ic is a global */
#define ISOUTSIDE(PX,PY,X,Y,W,H)	((PX > X + W || PX < X || PY > Y + H || PY < Y))
#define POSTOINT(X)			((int)(ceil(log2(X)) == floor(log2(X)) ? ceil(log2(X)) : 0))
#define ISPOSTOINT(X)			((X == 0 ? 0 : ceil(log2(X)) == floor(log2(X))))

enum { SchNorm, SchSel };
enum { ColFg, ColBg };
enum { SymLeft, SymRight };
enum { NoVis, YesVis };

typedef union {
	const int i;
	const float f;
	const void* v;
} Arg;


/* ---------------------------------------
 * Structs
 * ---------------------------------------
 */

struct key {
	unsigned int mod;
	KeySym keysym;
	void (*function)(const Arg arg);
	const Arg arg;
};

typedef struct bar bar;
struct bar {
	int width;
	int height;
	XftColor* scheme;
	Drawable d;
	XftFont* xfont;
	GC gc;
	Window win;
	XftDraw* xd;
};

typedef struct client client;
struct client {
	client* next;
	Window win;
	unsigned int desktops;
	unsigned int is_current;
	int x, y, w, h;
	/* being in monocle is not considered floating */
	int is_float;
	int is_full;
	//int no_focus;
}; 

typedef struct {
	const char* symbol;
	void (*arrange)();
} layout;

typedef struct desktop desktop;
struct desktop {
	float master_size;
	layout* current_layout;
};

typedef struct {
	const char* title;
	int desktopmask;
	int is_float;
} rule;

typedef struct {
	int x, y;
} point;

//typedef struct monitor monitor;
//struct monitor {
//	int x, y, h, w;
//	int num;
//	float master_size;
//	unsigned int seldesks;
//	client* head;
//	client* current;
//	desktop desktops[TABLENGTH(tags)];
//	layout* current_layout;
//	monitor* next;
//};


/* ---------------------------------------
 * Util Functions
 * ---------------------------------------
 */

static void die(const char *fmt, ...);
static void* ecalloc(size_t nmemb, size_t size);
static int slen(const char* str);

void die(const char* e, ...){
	fprintf(stdout,"sara: %s\n",e);
	exit(1);
}

void* ecalloc(size_t nmemb, size_t size){
	void* p;

	if ( !(p = calloc(nmemb,size)) ){
		die("ecalloc failed");
	}
	return p;
}

int slen(const char* str){
	int i = 0;
	const char* c = str;

	while (*c){ c++; i++; }

	return i;
}


/* ---------------------------------------
 * Main Function Declarations
 * ---------------------------------------
 */

/* X Event Processing */
//static void buttonpress(XEvent* e);
static void configurenotify(XEvent* e);
static void configurerequest(XEvent* e);
static void destroynotify(XEvent* e);
static void enternotify(XEvent* e);
//static void expose(XEvent* e);
static void keypress(XEvent* e);
static void maprequest(XEvent* e);
static void propertynotify(XEvent* e);
//static void unmapnotify(XEvent* e);

/* Client & Linked List Manipulation */
static void applyrules(client* c);
static void attachaside(client* c);
static void changecurrent(client* c, unsigned int desktopmask);
static void detach(client* c);
static void killclient();
static void manage(Window parent, XWindowAttributes* wa);
static void mapclients();
static void moveclient(const Arg arg);
static void movefocus(const Arg arg);
static void raisefloats();
static void refocus(client* n, client* p);
static void swapmaster();
static void todesktop(const Arg arg);
static void toggledesktop(const Arg arg);
static void togglefloat();
static void togglefs();
static void unmanage(client* c);
static void updatefocus();

/* Client Interfacing */
static client* findclient(Window w);
static client* findcurrent();
static client* findvisclient(client* c);
static client* findprevclient(client* c, int is_vis);
static void updateprev(client* c);

/* Bar */
static void drawbar();
static int drawbartext(int x, int y, int w, int h, unsigned int lpad, const char* text);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static int gettextwidth(const char* str, int len);
static void initbar();
static void updatestatus();

/* Desktop Interfacing */
static void changedesktop(const Arg arg);
static void changemsize(const Arg arg);
static void loaddesktop(int i);
static void monocle();
static void setlayout(const Arg arg);
static void tile();
static void view(const Arg arg);

/* Backend */
static void cleanup();
static int getpointcoords(int ret_y);
static void grabkeys();
static XftColor* scheme_create(const char* clrnames[], size_t clrcount);
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static void youviolatedmymother();
static int xerror(Display* dis, XErrorEvent* ee);


/* Make the above known */
#include "config.h"


/* ---------------------------------------
 * Globals
 * ---------------------------------------
 */

/* X Interfacing */
static int screen;
static int sh;
static int sw;
static Display* dis;
static Window root;

/* Client Interfacing */
static client* head;
static client* current;
static client* prev_enter;

/* Bar */
static bar* sbar;

/* Desktop Interfacing */
static layout* current_layout;
static desktop desktops[TABLENGTH(tags)];
static float master_size;
static unsigned int current_desktop;
static unsigned int seldesks;

/* Backend */
static client* ic; /* for EACHCLIENT iterating */
static int lrpad;
static int running;
static XftColor** scheme;
static char xsetr_text[256];

/* Monitor Interfacing */
//static monitor* head_mon;
//static monitor* current_mon;

/* Events array */
static void (*events[LASTEvent])(XEvent* e) = {
	//[ButtonPress] = buttonpress,
	[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	//[Expose] = expose,
	[KeyPress] = keypress,
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
	//[UnmapNotify] = unmapnotify
};


/* ---------------------------------------
 * X Event Processing
 * ---------------------------------------
 */

/* dwm copypasta */
//void buttonpress(XEvent* e){
//	client *c;
//	XButtonPressedEvent* ev = &e->xbutton;
//
//	/* focus monitor if necessary */
////	if ((m = wintomon(ev->window)) && m != selmon) {
////		unfocus(selmon->sel, 1);
////		selmon = m;
////		focus(NULL);
////	}
//
//	if ( (c = findclient(ev->window)) ){
////		updateprev(c);
//		changecurrent(c, current_desktop);
//		updatefocus();
////		XAllowEvents(dis,ReplayPointer,CurrentTime);
//	}
//}

/* dwm copypasta */
void configurenotify(XEvent* e){
	XConfigureEvent* ev = &e->xconfigure;

	if (ev->window == root){
		sbar->width = ev->width;
		XFreePixmap(dis,sbar->d);
		sbar->d = XCreatePixmap(dis, root, sbar->width, sbar->height, DefaultDepth(dis,screen));

		XMoveResizeWindow(dis, sbar->win, 0, 0, sbar->width, sbar->height);
		drawbar();
	}
}

/* dwm copypasta */
void configurerequest(XEvent* e){
	client* c;
	XConfigureRequestEvent* ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ( (c = findclient(ev->window)) && c->is_float ){
		if (ev->value_mask & CWX) c->x = 0 + ev->x;
		if (ev->value_mask & CWY) /* c->y = sbar->height + ev->y; */ c->y = ev->y;
		if (ev->value_mask & CWWidth) c->w = ev->width;
		if (ev->value_mask & CWHeight) c->h = ev->height;
		if ISVISIBLE(c) XMoveResizeWindow(dis, c->win, c->x, c->y, c->w, c->h);

	} else {
		wc.x = ev->x; wc.y = ev->y; wc.width = ev->width;
		wc.height = ev->height; wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
	}

	XSync(dis, False);
}

void destroynotify(XEvent* e){
	client* c;
	XDestroyWindowEvent* ev = &e->xdestroywindow;

	if ( (c = findclient(ev->window)) ){
		unmanage(c);
		drawbar();
	}
}

void enternotify(XEvent* e){
	client* c;
	XCrossingEvent* ev = &e->xcrossing;

	if ( (ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root )
		return;

	if ( !(c = findclient(ev->window)) )
		return;

	/* if we haven't moved */
	if (prev_enter && !ISOUTSIDE(getpointcoords(0), getpointcoords(1), prev_enter->x, prev_enter->y, prev_enter->w, prev_enter->h))
		return;

	updateprev(c);
	changecurrent(c, current_desktop);
	updatefocus();
}

/* dwm copypasta */
//void expose(XEvent* e){
//	XExposeEvent* ev = &e->xexpose;
//
//	if (ev->count == 0 && findclient(ev->window))
//		drawbar();
//}

void keypress(XEvent* e){
	int i;
	XKeyEvent ke = e->xkey;
	KeySym keysym = XKeycodeToKeysym(dis, ke.keycode, 0);

	for (i=0;i<TABLENGTH(keys);i++){
		if (keys[i].keysym == keysym && keys[i].mod == ke.state)
			keys[i].function(keys[i].arg);
	}
}

void maprequest(XEvent* e){
	XWindowAttributes wa;
	XMapRequestEvent* ev = &e->xmaprequest;

	if ( XGetWindowAttributes(dis, ev->window, &wa) && !wa.override_redirect && !findclient(ev->window) ){
		manage(ev->window, &wa);
		current_layout->arrange();
		updatefocus();
	}
}

/* dwm copypasta */
void propertynotify(XEvent* e){
	XPropertyEvent* ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
}

//void unmapnotify(XEvent* e){
//	XUnmapEvent* ev = &e->xunmap;
//
//}


/* ---------------------------------------
 * Client & Linked List Manipulation
 * ---------------------------------------
 */

void applyrules(client* c){
	int i;
	const rule* r;
	XTextProperty tp;
	XGetWMName(dis, c->win, &tp);

	for (i=0;i<TABLENGTH(rules);i++){
		r = &rules[i];
		if ( !r->title || (tp.value && strstr(r->title, (const char*)tp.value)) ){
			if (r->desktopmask > 0) c->desktops = 1 << r->desktopmask;
			c->is_float = r->is_float;
		}
	}

	/* need to free something, but idk what or how, because everything accidentallys */
	//XFree(&tp.value);
}	

void attachaside(client* c){
	client* l;

	if (!head){
		head = c;

	} else {
		/* If not the first on this desktop */
		if (current){
			c->next = current->next;
			current->next = c;

		} else {
			for (l=head;l->next;l=l->next);
			l->next = c;
		}
	}

	XSelectInput(dis, c->win, EnterWindowMask|FocusChangeMask|StructureNotifyMask);
	changecurrent(c, current_desktop);
}

void changecurrent(client* c, unsigned int desktopmask){
	if (c) c->is_current ^= desktopmask;
	
	for EACHCLIENT(head) if ( ic != c && (ic->is_current & desktopmask) ){
			ic->is_current ^= desktopmask;
		}

	current = c;
}

void detach(client* c){
	client* p;
	/* Move the window out of the way first to hide it while it hangs around :) */
	XWindowAttributes wa;
	XGetWindowAttributes(dis, c->win, &wa);
	XMoveWindow(dis, c->win, -2*wa.width, wa.y);

	/* before disconnecting c */
	refocus(c->next, c);

	/* For both, if NULL, then we're still okay */
	if ( (p = findprevclient(c, NoVis)) )
		p->next = c->next;
	else
		head = c->next;
}

void killclient(){
	Window w;
	XEvent ev;

	if (current){
		w = current->win;
		unmanage(current);

		/* send X Kill signal */
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dis, w, False, NoEventMask, &ev);
	}

	updatestatus();
}

void manage(Window parent, XWindowAttributes* wa){
	client* c;

	if ( !(c = ecalloc(1, sizeof(client))) )
		die("Error while callocing new client!");

	c->win = parent;
	c->is_float = 0;
	c->is_full = 0;
	c->is_current = 0;
	c->desktops = current_desktop;

	c->x = wa->x; c->y = wa->y;
	c->w = wa->width; c->h = wa->height;

	if (!(prev_enter->win) || !findclient(prev_enter->win))
		updateprev(c);

	applyrules(c);
	attachaside(c);
	/* if (c->no_focus){
	 * 	hide_client(c);
	 *	refocus(current->next, current);
	 * }
	 * current_layout->arrange();
	 */
	XMapWindow(dis, c->win);
	drawbar();
}

void mapclients(){
	for EACHCLIENT(head) if ISVISIBLE(ic) XMapWindow(dis, ic->win); else XUnmapWindow(dis, ic->win);
}

void moveclient(const Arg arg){
	client* p, * mp, * n;

	if (!current || current->is_full)
		return;

	p = findprevclient(current, NoVis);

	/* Up stack if not head */
	if (arg.i > 0 && current != head){
		if (p == head){
			swapmaster();

		} else {
			mp = findprevclient(p, NoVis);

			mp->next = current;
			p->next = current->next;
			current->next = p;
		}

	/* Down stack if not tail */
	} else if (arg.i < 0 && current->next){
		n = current->next;
		current->next = n->next;
		n->next = current;

		if (current == head)
			head = n;
		else
			p->next = n;
	}

	current_layout->arrange();
	updatefocus();
	drawbar();
}

/* some dwm copypasta */
void movefocus(const Arg arg){
	client* j, * c = NULL;

	if ( !current || current->is_full )
		return;

	/* up stack */
	if (arg.i > 0){
		for (j=head;j != current;j=j->next)
			if ISVISIBLE(j) c = j;

		if (!c)
			/* if current was highest, go to the bottom */
			for (;j;j=j->next) if ISVISIBLE(j) c = j;

	/* down stack */
	} else {
		if ( !(c = findvisclient(current->next)) )
			for (c=head;c && !ISVISIBLE(c);c=c->next);
	}

	updateprev(c);
	changecurrent(c, current_desktop);
	updatefocus();
	drawbar();
}

void raisefloats(){
	for EACHCLIENT(head) if (ISVISIBLE(ic) && ic->is_float){
			XMoveResizeWindow(dis, ic->win, ic->x, ic->y, ic->w, ic->h);
			XRaiseWindow(dis, ic->win);
		}
}

/* focus moves down if possible, else up */
void refocus(client* n, client* p){
	client* vis;
	vis = (vis = findvisclient(n)) ? vis : findprevclient(p,YesVis);
	changecurrent(vis, current_desktop);
}

void swapmaster(){
	client* tmp;
	client* p;

	if (head && current && current != head){
		tmp = (head->next == current) ? head : head->next;
		p = findprevclient(current, NoVis);

		/* if p is head, this gets overwritten - saves an if statement */
		p->next = head;
		head->next = current->next;
		current->next = tmp;
		head = current;

		current_layout->arrange();
		updatefocus();
	}
}

void todesktop(const Arg arg){
	if (current_desktop & 1 << arg.i) return;

	current->desktops = 1 << arg.i;
	current->is_current = 0;
	changecurrent(current, 1 << arg.i);

	XUnmapWindow(dis, current->win);
	XSync(dis, False);

	refocus(current->next, current);
	current_layout->arrange();
	updatefocus();
	updatestatus();
}

void toggledesktop(const Arg arg){
	unsigned int new_desktops;

	if (!current) return;

	new_desktops = current->desktops ^ 1 << arg.i;
	if (new_desktops){
		current->desktops = new_desktops;
		current->is_current = 0;
		changecurrent(current, 1 << arg.i);

		if ( !(ISVISIBLE(current)) ){
			XUnmapWindow(dis, current->win);
			XSync(dis, False);
			refocus(current->next, current);
		}

		current_layout->arrange();
		updatefocus();
		updatestatus();
	}
}

void togglefloat(){
	XWindowChanges wc;

	if (!current) return;

	current->is_float = !current->is_float;
	current_layout->arrange();

	if (current->is_float){
		wc.sibling = current->win;
		wc.stack_mode = Below;

		for EACHCLIENT(head) XConfigureWindow(dis, ic->win, CWSibling|CWStackMode, &wc);
	}

	if (!current->is_float){
		wc.sibling = sbar->win;
		wc.stack_mode = Below;
		XConfigureWindow(dis, current->win, CWSibling|CWStackMode, &wc);
	}
}

void togglefs(){
	if (!current) return;

	current->is_full = !current->is_full;
	/* a pecularity of my implementation - will remain as such unless I decide to implement oldx, oldy, etc. for clients */
	current->is_float = 0;

	if (current->is_full){
		current->x = 0; current->y = 0;
		current->w = sw; current->h = sh;
		XMoveResizeWindow(dis, current->win, current->x, current->y, current->w, current->h);
		XRaiseWindow(dis, current->win);

		XUnmapWindow(dis, sbar->win);

	} else {
		current_layout->arrange();

		XMapRaised(dis, sbar->win);
		drawbar();
	}
}

void unmanage(client* c){
	detach(c);
	free(c);
	current_layout->arrange();
	updatefocus();
}

void updatefocus(){
	for EACHCLIENT(head) if (ic == current){
			XSetInputFocus(dis, ic->win, RevertToPointerRoot, CurrentTime);
			XRaiseWindow(dis, ic->win);
		}
}


/* ---------------------------------------
 * Client Interfacing
 * ---------------------------------------
 */

client* findclient(Window w){
	for EACHCLIENT(head) if (ic->win == w) return ic;

	return NULL;
}

client* findcurrent(){
	for EACHCLIENT(head) if ( ISVISIBLE(ic) && (ic->is_current & current_desktop) ){
			return ic;
		}

	return NULL;
}

client* findprevclient(client* c, int is_vis){
	client* i, * ret = NULL;
	for (i=head;i && i != c;i=i->next){
		if (is_vis)
			if ISVISIBLE(i) ret = i;

		if (i->next == c){
			if (is_vis)
				return ISVISIBLE(i) ? i : ret;
			else
				return i;
		}
	}

	return NULL;
}

client* findvisclient(client* c){
	for EACHCLIENT(c) if ISVISIBLE(ic) return ic;
	
	return NULL;
}

void updateprev(client* c){
	prev_enter->win = c->win;
	prev_enter->x = c->x; prev_enter->y = c->y;
	prev_enter->w = c->w; prev_enter->h = c->h;
}


/* ---------------------------------------
 * Bar
 * ---------------------------------------
 */

/* part dwm copypasta */
void drawbar(){
	int j, x = 2, xsetr_text_w = 0, is_sel; /* 2px left padding */
	unsigned int occ = 0;

	/* draw background */
	sbar->scheme = scheme[SchNorm];
	XSetForeground(dis, sbar->gc, sbar->scheme[ColBg].pixel);
	XFillRectangle(dis, sbar->d, sbar->gc, 0, 0, sw, sbar->height);

	/* draw status */
	xsetr_text_w = TEXTW(xsetr_text) - lrpad + 2; /* 2px right padding */
	drawbartext(sbar->width - xsetr_text_w, 0, xsetr_text_w, sbar->height, 0, xsetr_text);

	for EACHCLIENT(head) occ |= ic->desktops;

	/* draw tags */
	for (j=0;j<TABLENGTH(tags);j++){
		/* do not draw vacant tags, but do draw selected tags regardless */
		is_sel = seldesks & 1 << j;
		if ( !(occ & 1 << j) && !is_sel )
			continue;

		sbar->scheme = scheme[is_sel ? SchSel : SchNorm];

		x = drawbartext(x, 0, TEXTW(syms[SymLeft]) + 1, sbar->height, 0, is_sel ? syms[SymLeft] : " ") - lrpad;
		x = drawbartext(x, 0, TEXTW(tags[j]), sbar->height, 0, tags[j]) - lrpad + 2;
		x = drawbartext(x, 0, TEXTW(syms[SymRight]), sbar->height, 0, is_sel ? syms[SymRight] : " ") - lrpad / 2;
	}
	x -= lrpad / 2;
	sbar->scheme = scheme[SchNorm];
	drawbartext(x, 0, TEXTW(current_layout->symbol), sbar->height, lrpad / 2, current_layout->symbol);

	XCopyArea(dis, sbar->d, sbar->win, sbar->gc, 0, 0, sbar->width, sbar->height, 0, 0);
	XSync(dis, False);
}

int drawbartext(int x, int y, int w, int h, unsigned int lpad, const char* text){
	int ty = y + (h - (sbar->xfont->ascent + sbar->xfont->descent)) / 2 + sbar->xfont->ascent;
	XftDrawString8(sbar->xd, &sbar->scheme[ColFg], sbar->xfont, x + lpad, ty, (XftChar8*)text, slen(text));

	return x + w;
}

/* dwm copypasta */
int gettextprop(Window w, Atom atom, char *text, unsigned int size){
	char** list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;

	text[0] = '\0';

	if ( !XGetTextProperty(dis, w, &name, atom) || !name.nitems )
		return 0;

	if (name.encoding == XA_STRING){
		strncpy(text, (char *) name.value, size - 1);

	} else {
		if ( XmbTextPropertyToTextList(dis,&name,&list,&n) >= Success && n > 0 && *list ){
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);

	return 1;
}

/* does anything here need to be free() or XFree()? */
int gettextwidth(const char* str, int len){
	XGlyphInfo xgi;
	XftTextExtents8(dis, sbar->xfont, (XftChar8*)str, len, &xgi);

	return xgi.width;
}

void initbar(){
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ExposureMask
	};

	if ( !(sbar->xfont = XftFontOpenName(dis, screen, fontname)) ){
		die("The font you tried to use was not found. Check the name.");
	}

	sbar->height = sbar->xfont->ascent + sbar->xfont->descent + 2;
	sbar->width = sw;
	sbar->d = XCreatePixmap(dis, root, sw, sh, DefaultDepth(dis, screen));
	sbar->gc = XCreateGC(dis, sbar->d, 0, NULL);
	sbar->xd = XftDrawCreate(dis, sbar->d, DefaultVisual(dis,screen), DefaultColormap(dis,screen));

	sbar->win = XCreateWindow(dis, root, 0, 0, sbar->width, sbar->height, 0, 
			DefaultDepth(dis, screen), InputOutput, DefaultVisual(dis,screen), 
			CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);

	XDefineCursor(dis, sbar->win, XCreateFontCursor(dis, 68));
	XMapRaised(dis, sbar->win);
}

/* dwm copypasta */
void updatestatus(){
	if (!gettextprop(root, XA_WM_NAME, xsetr_text, sizeof(xsetr_text)))
		strcpy(xsetr_text, "where's the leak, ma'am?");

	drawbar();
}


/* ---------------------------------------
 * Desktop Interfacing
 * ---------------------------------------
 */

void changedesktop(const Arg arg){
	client* c;

	if (current_desktop & 1 << arg.i) return;

	loaddesktop(arg.i);
	seldesks = current_desktop = 1 << arg.i;

	mapclients();

	/* why is this not changecurrent? */
	if ( !(current = findcurrent()) && (c = findvisclient(head)) )
		current = c;

	current_layout->arrange();
	updatefocus();
	updatestatus();
}

void changemsize(const Arg arg){
	master_size += ( ((master_size < 0.95 * sw) && (arg.f > 0))
			|| ((master_size > 0.05 * sw) && (arg.f < 0))  ) ? arg.f * sw : 0;

	current_layout->arrange();
}

void loaddesktop(int i){
	desktops[POSTOINT(current_desktop)].master_size = master_size;
	desktops[POSTOINT(current_desktop)].current_layout = current_layout;

	master_size = desktops[i].master_size;
	current_layout = desktops[i].current_layout;
}

void monocle(){
	int mh = sh - sbar->height;

	raisefloats();
	for EACHCLIENT(head) if (ISVISIBLE(ic) && !ic->is_float){
			ic->x = 0; ic->y = sbar->height;
			ic->w = sw;
			ic->h = mh;
			XMoveResizeWindow(dis, ic->win, ic->x, ic->y, ic->w, ic->h);
		} 
}

void setlayout(const Arg arg){
	current_layout = (layout*) arg.v;
	current_layout->arrange();
	drawbar();
}

void tile(){
	client* nf = NULL;
	int n = 0;
	int mh = sh - sbar->height;
	int y = sbar->height;

	/* Find the first non-floating, visible window and tally non-floating, visible windows */
	for EACHCLIENT(head) if (!ic->is_float && ISVISIBLE(ic)){
			nf = (!nf) ? ic : nf;
			n++;
		}

	raisefloats();

	if (nf && n == 1){
		nf->x = 0; nf->y = y;
		nf->w = sw; nf->h = mh;
		XMoveResizeWindow(dis, nf->win, nf->x, nf->y, nf->w, nf->h);

	} else if (nf){
		/* so having a master doesn't affect stack splitting */
		n--;

		/* Master window */
		nf->x = 0; nf->y = y;
		nf->w = master_size; nf->h = mh;
		XMoveResizeWindow(dis, nf->win, nf->x, nf->y, nf->w, nf->h);

		/* Stack */
		for EACHCLIENT(nf->next){
			if (ISVISIBLE(ic) && !ic->is_float){
				ic->x = master_size;
				ic->y = y;
				ic->w = sw - master_size;
				ic->h = mh / n;
				XMoveResizeWindow(dis, ic->win, ic->x, ic->y, ic->w, ic->h);

				y += mh / n;
			}
		}
	}
}

void view(const Arg arg){
	int i;

	/* if this would leave nothing visible */
	if ((seldesks ^ 1 << arg.i) == 0) return;

	seldesks ^= 1 << arg.i;
	mapclients();

	if (!(current_desktop & seldesks)){
		for (i=0;i < TABLENGTH(tags);i++){
			if (seldesks & 1 << i){
				loaddesktop(i);
				current_desktop = seldesks;
				break;
			}
		}
	}

	current_layout->arrange();
	updatefocus();
	updatestatus();
}


/* ---------------------------------------
 * Backend
 * ---------------------------------------
 */


/* Kill off any remaining clients
 * Free all the things
 */
void cleanup(){
	int i;

	while (current) killclient();

//	m = head_mon;
//	while (m){
//		free(m->desktops);
//		tmp = m->next;
//		free(m);
//		m = tmp;
//	}

	/* This ain't pretty, but it gets the job done (Without memory leak? Not sure) */
	XUnmapWindow(dis, sbar->win);
	XDestroyWindow(dis, sbar->win);
	XUngrabKey(dis, AnyKey, AnyModifier, root);

	XftDrawDestroy(sbar->xd);
	XFreeGC(dis, sbar->gc);
	XftFontClose(dis, sbar->xfont);
	XFreePixmap(dis, sbar->d);
	free(sbar);

	free(prev_enter);

	for (i=0;i < TABLENGTH(colors);i++) free(scheme[i]);
	free(scheme);

	fprintf(stdout, "sara: Thanks for using!\n");
	XDestroySubwindows(dis, root);

	XSync(dis, False);
	XSetInputFocus(dis, PointerRoot, RevertToPointerRoot, CurrentTime);

        XCloseDisplay(dis);
}

int getpointcoords(int ret_y){
	int x, y, dwx, dwy;
	unsigned int dmr;
	Window drret, dcret;

	XQueryPointer(dis, root, &drret, &dcret, &x, &y, &dwx, &dwy, &dmr);

	return ret_y ? y : x;
}

void grabkeys(){
	int i;
	KeyCode code;

	for (i=0;i<TABLENGTH(keys);i++){
		if ( (code = XKeysymToKeycode(dis, keys[i].keysym)) )
			XGrabKey(dis, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
	}
}

/* y'all need to free() this bad boi when you cleanup */
XftColor* scheme_create(const char* clrnames[], size_t clrcount){
	int i;
	XftColor* sch;

	if ( !(sch = ecalloc(clrcount, sizeof(XftColor))) )
		die("Error while trying to ecalloc for scheme_create");

	for (i=0;i < clrcount;i++){
		if ( !XftColorAllocName(dis, DefaultVisual(dis,screen), DefaultColormap(dis,screen), clrnames[i], &sch[i]) )
			die("Error while trying to allocate color '%s'", clrnames[i]);
	}

	return sch;
}

void setup(){
	int i;
	sigchld(0);

	XSetWindowAttributes wa;

	screen = DefaultScreen(dis);
	root = RootWindow(dis, screen);

	sw = XDisplayWidth(dis, screen);
	sh = XDisplayHeight(dis, screen);
	master_size = sw * MASTER_SIZE;

	/* Default to first layout */
	current_layout = (layout*) &layouts[0];

	scheme = ecalloc(TABLENGTH(colors), sizeof(XftColor*));
	for (i=0;i < TABLENGTH(colors);i++) scheme[i] = scheme_create(colors[i], 2);

	running = 1;

	/* set up bar */
	sbar = ecalloc(1, sizeof(bar));
	initbar();
	lrpad = sbar->xfont->ascent + sbar->xfont->descent;

	head = NULL;
	current = NULL;
	prev_enter = ecalloc(1, sizeof(client));

	/* Set up all desktops, default to 0 */
	for (i=0;i < TABLENGTH(tags);i++){
		desktops[i].master_size = master_size;
		desktops[i].current_layout = current_layout;
	}
	seldesks = current_desktop = 1 << 0;
	loaddesktop(0);
	
	// initmons();

	wa.cursor = XCreateFontCursor(dis, 68);
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dis, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dis, root, wa.event_mask);

	grabkeys();
	drawbar();
}

/* dwm copypasta */
void sigchld(int unused){
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	
	while (0 < waitpid(-1, NULL, WNOHANG));
}

/* dwm copypasta */
void spawn(const Arg arg){
	if (fork()) return;
	if (dis) close(ConnectionNumber(dis));

	setsid();
	execvp( ((char**) arg.v)[0], (char**) arg.v);
	
	fprintf(stderr, "sara: execvp %s", ((char **) arg.v)[0]);
	perror(" failed");
	exit(EXIT_SUCCESS);
}

/* dwm copypasta */
void start(){
	XEvent ev;

	XSync(dis, False);
	while ( running && !XNextEvent(dis, &ev) ){
		if (events[ev.type])
			events[ev.type](&ev);
	}
}

/* dwm copypasta */
int xerror(Display* dis, XErrorEvent* e){
	if (e->error_code == BadWindow
	|| (e->request_code == X_SetInputFocus && e->error_code == BadMatch)
	|| (e->request_code == X_PolyText8 && e->error_code == BadDrawable)
	|| (e->request_code == X_PolyFillRectangle && e->error_code == BadDrawable)
	|| (e->request_code == X_PolySegment && e->error_code == BadDrawable)
	|| (e->request_code == X_ConfigureWindow && e->error_code == BadMatch)
	|| (e->request_code == X_GrabButton && e->error_code == BadAccess)
	|| (e->request_code == X_GrabKey && e->error_code == BadAccess)
	|| (e->request_code == X_CopyArea && e->error_code == BadDrawable))
		return 0;

	die("xerror handler had trouble! I'm too lazy to tell you what went wrong.");
	return -1;
}

void youviolatedmymother(){
	running = 0;
}

int main(){
	if ( !(dis = XOpenDisplay(NULL)) ) die("Cannot open display!");
	XSetErrorHandler(xerror);
	setup();

	start();

	cleanup();

	return 0;
}

//void initmons(){
//	int i, ns;
//	monitor* m;
//
//	XineramaScreenInfo* info = XineramaQueryScreens(dis, &ns);
//	
//	/* what is the "unique geometries" problem in dwm?" */
//	for (i=0;i < ns;i++){
//		m = ecalloc(1, sizeof(monitor));
//		m->x = info[i].x_org;
//		m->y = info[i].y_org;
//		m->w = info[i].width;
//		m->h = info[i].height;
//		m->num = i; 
//
//		m->bar = sbar;
//
//		/* will be filled in when you save for the first time */
//		m->desktops = ecalloc(TABLENGTH(tags), sizeof(desktop));
//		m->seldesks = seldesks;
//
//		m->current_desktop = current_desktop;
//		m->current_layout = current_layout;
//		m->master_size = master_size;
//
//		m->head = head;
//		m->current = current;
//		m->prev_enter = prev_enter;
//
//		attach_mon(m);
//	}
//
//	XFree(info);
//}

//void loadmon(monitor* m){
//	/* save current */
//	current_mon->desktops = desktops;
//	current_mon->seldesks = seldesks;
//
//	current_mon->current_desktop = current_desktop;
//	current_mon->current_layout = current_layout;
//	current_mon->master_size = master_size;
//
//	current_mon->head = head;
//	current_mon->current = current;
//	current_mon->prev_enter = prev_enter;
//
//	/* select m */
//	desktops = m->desktops;
//	seldesks = m->seldesks;
//
//	current_desktop = m->current_desktop;
//	current_layout = m->current_layout;
//	master_size = m->master_size;
//	
//	head = m->head;
//	current = m->current;
//	prev_enter = m->prev_enter;
//}
