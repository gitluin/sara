/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch              
 * Copyright (c) 2020, This Fackin Guy, gitluin on github (no email for you!)     
 *                                                                                
 * Please refer to the MIT license for details on usage: https://mit-license.org/
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
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define TABLENGTH(X)    		(sizeof(X)/sizeof(*X))
#define ISVISIBLE(C)			((C->desks & curmon->seldesks))
#define TEXTW(M,X)			(gettextwidth(M, X, slen(X)) + lrpad)
#define EACHCLIENT(_I)			(ic=_I;ic;ic=ic->next) /* ic is a global */
#define ISOUTSIDE(PX,PY,X,Y,W,H)	((PX > X + W || PX < X || PY > Y + H || PY < Y))
#define POSTOINT(X)			((int)(ceil(log2(X)) == floor(log2(X)) ? ceil(log2(X)) : 0))

enum { SchNorm, SchSel };
enum { ColFg, ColBg };
enum { SymLeft, SymRight };
enum { AnyVis, OnlyVis };
enum { NoZoom, YesZoom };
enum { NoDetach, YesDetach };
enum { NoFocus, YesFocus };
enum { NoStay, YesStay };

typedef union {
	const int i;
	const float f;
	const void* v;
} Arg;


/* ---------------------------------------
 * Structs
 * ---------------------------------------
 */

typedef struct bar bar;
typedef struct client client;
typedef struct drw drw;
typedef struct desktop desktop;
typedef struct monitor monitor;

struct key {
	unsigned int mod;
	KeySym keysym;
	void (*function)(const Arg arg);
	const Arg arg;
};

typedef struct {
	const char* symbol;
	void (*arrange)(monitor*);
} layout;

typedef struct {
	const char* class, * instance, * title;
	int desks;
	int isfloat;
	int monitor;
} rule;

struct bar {
	int w, h;
	Window win;
};

struct client {
	client* next;
	Window win;
	unsigned int desks;
	unsigned int iscur;
	int x, y, w, h;
	/* being in monocle is not considered floating */
	int isfloat;
	int isfull;
	/* prior to togglefs */
	int oldfloat;
}; 

struct drw {
	Drawable d;
	GC gc;
	XftColor* scheme;
	XftDraw* xd;
	XftFont* xfont;
};

struct desktop {
	float msize;
	layout* curlayout;
};

struct monitor {
	/* monitor */
	int x, y, h, w;
	int num;
	bar* bar;
	monitor* next;
	/* desks */
	float msize;
	unsigned int seldesks;
	unsigned int curdesk;
	client* current;
	client* head;
	desktop* desks;
	layout* curlayout;
};


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

	if ( !(p = calloc(nmemb,size)) ) die("ecalloc failed");

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
static void buttonpress(XEvent* e);
static void configurerequest(XEvent* e);
static void destroynotify(XEvent* e);
static void enternotify(XEvent* e);
static void expose(XEvent* e);
static void keypress(XEvent* e);
static void maprequest(XEvent* e);
static void motionnotify(XEvent* e);
static void propertynotify(XEvent* e);
static void unmapnotify(XEvent* e);

/* Client & Linked List Manipulation */
static void applyrules(client* c);
static void attachaside(client* c);
static void changecurrent(client* c, unsigned int deskmask);
static void configure(client* c);
static void detach(client* c);
static void killclient();
static void manage(Window parent, XWindowAttributes* wa);
static void mapclients();
static void moveclient(const Arg arg);
static void moveclientup(client* c, int wantzoom);
static void movefocus(const Arg arg);
static void raisefloats();
static void refocus(client* n, client* p);
static void resizeclient(client* c, int x, int y, int w, int h);
static void sendtomon(client* c, monitor* oldmon, monitor* newmon, int wantdetach, int wantstay, int wantfocus);
static void todesktop(const Arg arg);
static void toggledesktop(const Arg arg);
static void togglefloat();
static void togglefs();
static void tomon(const Arg arg);
static void unmanage(client* c);
static void updatefocus();
static void zoom();

/* Monitor Manipulation */
static void changemon(monitor* m, int focus);
static void cleanupmon(monitor* m);
//static monitor* createmon(int num, int x, int y, int w, int h);
static monitor* createmon();
static monitor* findmon(Window w);
static monitor* findprevmon(monitor* m);
static void focusmon(const Arg arg);
static void initmons();
static void isortmons();
static void swapmon(monitor* x, monitor* y);
//static void updategeom();

/* Client Interfacing */
static client* findclient(Window w);
static client* findcurrent();
static client* findvisclient(client* c);
static client* findprevclient(client* c, int wantvis);

/* Bar */
static void drawbar(monitor* m);
static void drawbars();
static int drawbartext(monitor* m, int x, int y, int w, int h, unsigned int lpad, const char* text);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static int gettextwidth(monitor* m, const char* str, int len);
static bar* initbar(monitor* m);
static void updatestatus();

/* Desktop Interfacing */
static void changemsize(const Arg arg);
static void loaddesktop(int i);
static void monocle(monitor* m);
static void setlayout(const Arg arg);
static void tile(monitor* m);
static void toggleview(const Arg arg);
static void view(const Arg arg);
static void viewall();

/* Backend */
static void cleanup();
static XftColor* createscheme(const char* clrnames[], size_t clrcount);
static int getptrcoords(int rety);
static void grabkeys();
static void initdrw();
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static int xerror(Display* dis, XErrorEvent* ee);
static int xerrordummy(Display* dis, XErrorEvent* e);
static void youviolatedmymother();


/* Make the above known */
#include "config.h"


/* ---------------------------------------
 * Globals
 * ---------------------------------------
 */

/* X Interfacing */
static Display* dis;
static Window root;
static int screen;
static int sh;
static int sw;

/* Client Interfacing */
static int justswitch;

/* Monitor Interfacing */
static monitor* curmon;
static monitor* mhead;

/* Backend */
static client* ic; /* for EACHCLIENT iterating */
static drw* sdrw;
static int lrpad;
static int running;
static XftColor** scheme;
static char xsetr_text[256];

/* Events array */
static void (*events[LASTEvent])(XEvent* e) = {
	[ButtonPress] = buttonpress,
	//[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[KeyPress] = keypress,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};


/* ---------------------------------------
 * X Event Processing
 * ---------------------------------------
 */

/* TODO: monitor support */
/* dwm copypasta */
void buttonpress(XEvent* e){
	client* c;
	monitor* m;
	XButtonPressedEvent* ev = &e->xbutton;

	if ( !(c = findclient(ev->window)) )
		return;

	if ( (m = findmon(ev->window)) && m != curmon)
		changemon(m, 0);

	changecurrent(c, curmon->curdesk);
	updatefocus();
	XAllowEvents(dis, ReplayPointer, CurrentTime);
}

/* dwm copypasta */
//void configurenotify(XEvent* e){
//	monitor* m;
//	client* c;
//	XConfigureEvent* ev = &e->xconfigure;
//
//	if (ev->window == root){
//		sw = ev->width; sh = ev->height;
//		updategeom();
//		if (sdrw){
//			if (sdrw->d) XFreePixmap(dis, sdrw->d);
//			sdrw->d = XCreatePixmap(dis, root, sw, sh, DefaultDepth(dis,screen));
//		}
//
//		for (m=mhead;m;m=m->next){
//			for EACHCLIENT(m->head) if (ic->isfull){
//					resizeclient(ic, m->x, m->y, m->w, m->h);
//				}
//
//			m->curlayout->arrange(m);
//		}
//		updatefocus();
//	}
//}

/* dwm copypasta */
void configurerequest(XEvent* e){
	client* c;
	monitor* m;
	XConfigureRequestEvent* ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ( (c = findclient(ev->window)) ){
		if (c->isfloat){
			m = findmon(c->win);
			if (ev->value_mask & CWX) c->x = m->x + ev->x;
			if (ev->value_mask & CWY) c->y = (ev->y < m->bar->h) ? m->bar->h : ev->y;
			if (ev->value_mask & CWWidth) c->w = ev->width;
			if (ev->value_mask & CWHeight) c->h = ev->height;
			if ISVISIBLE(c) XMoveResizeWindow(dis, c->win, c->x, c->y, c->w, c->h);

		} else {
			configure(c);
		}

	} else {
		wc.x = ev->x; wc.y = ev->y;
		wc.width = ev->width; wc.height = ev->height;
		wc.border_width = 0;
		wc.sibling = ev->above; wc.stack_mode = ev->detail;
		XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
	}

	XSync(dis, False);
}

void destroynotify(XEvent* e){
	client* c;
	XDestroyWindowEvent* ev = &e->xdestroywindow;

	if ( (c = findclient(ev->window)) )
		unmanage(c);
}

/* mostly dwm copypasta */
void enternotify(XEvent* e){
	client* c;
	monitor* m;
	XCrossingEvent* ev = &e->xcrossing;

	if ( (ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root )
		return;

	if ( !(c = findclient(ev->window)) || c == curmon->current )
		return;

	if (justswitch){
		justswitch = 0;
		return;
	}

	if ( (m = findmon(ev->window)) && m != curmon )
		changemon(m, 0);

	changecurrent(c, curmon->curdesk);
	updatefocus();
}

/* dwm copypasta */
void expose(XEvent* e){
	monitor* m;
	XExposeEvent* ev = &e->xexpose;

	if ( ev->count == 0 && (m = findmon(ev->window)) )
		drawbar(m);
}

void keypress(XEvent* e){
	int i;
	XKeyEvent ke = e->xkey;
	KeySym keysym = XKeycodeToKeysym(dis, ke.keycode, 0);

	for (i=0;i<TABLENGTH(keys);i++)
		if (keys[i].keysym == keysym && keys[i].mod == ke.state)
			keys[i].function(keys[i].arg);
}

/* dwm copypasta */
void maprequest(XEvent* e){
	XWindowAttributes wa;
	XMapRequestEvent* ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dis, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
	if (!findclient(ev->window))
		manage(ev->window, &wa);
}

void motionnotify(XEvent* e){
	monitor* m;
	XMotionEvent* ev = &e->xmotion;
	int isoutside = ISOUTSIDE(ev->x_root, ev->y_root, curmon->x, curmon->y, curmon->w, curmon->h);

	if (ev->window != root)
		return;

	for (m=mhead;m;m=m->next){
		if (m != curmon && isoutside){
			changemon(m, 1);
			return;
		}
	}
}

/* dwm copypasta */
void propertynotify(XEvent* e){
	XPropertyEvent* ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
}

void unmapnotify(XEvent* e){
	curmon->curlayout->arrange(curmon);
}


/* ---------------------------------------
 * Client & Linked List Manipulation
 * ---------------------------------------
 */

/* mostly dwm copypasta */
void applyrules(client* c){
	const char* class, * instance;
	int i;
	const rule* r;
	monitor* m;
	XClassHint ch = { NULL, NULL };
	XTextProperty tp;

	XGetWMName(dis, c->win, &tp);
	XGetClassHint(dis, c->win, &ch);
	class = ch.res_class ? ch.res_class : "broken";
	instance = ch.res_name  ? ch.res_name  : "broken";

	for (i=0;i<TABLENGTH(rules);i++){
		r = &rules[i];
		if ((!r->title || (tp.value && strstr(r->title, (const char*) tp.value)))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			c->isfloat = r->isfloat;
			//c->isfull = r->isfull;
			c->desks = 0; c->desks |= r->desks;
			for (m=mhead;m;m=m->next){
				if (m->num == r->monitor){
					sendtomon(c, curmon, m, NoDetach, NoFocus, YesStay);
					break;
				}
			}
		}
	}
	if (!c->desks) c->desks = curmon->seldesks;

	if (ch.res_class) XFree(ch.res_class);
	if (ch.res_name) XFree(ch.res_name);
}

void attachaside(client* c){
	client* l;

	/* if attached to a monitor already (applyrules) */
	if (findclient(c->win)) return;

	if (!curmon->head){
		curmon->head = c;

	} else {
		/* If not the first on this desktop */
		if (curmon->current){
			c->next = curmon->current->next;
			curmon->current->next = c;

		} else {
			for (l=curmon->head;l->next;l=l->next);
			l->next = c;
		}
	}

	XSelectInput(dis, c->win, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	changecurrent(c, curmon->curdesk);
}

void changecurrent(client* c, unsigned int deskmask){
	if (c){
		c->iscur ^= deskmask;
		XUngrabButton(dis, Button1, AnyModifier, c->win);
	}
	
	for EACHCLIENT(curmon->head) if (ic != c && (ic->iscur & deskmask)){
			ic->iscur ^= deskmask;
			XGrabButton(dis, Button1, 0, ic->win, False, ButtonPressMask,
					GrabModeAsync, GrabModeSync, None, None);
		}

	curmon->current = c;
}

void configure(client* c){
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dis;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x; ce.y = c->y;
	ce.width = c->w; ce.height = c->h;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dis, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void detach(client* c){
	client* p;
	/* Move the window out of the way first to hide it while it hangs around :) */
	XMoveWindow(dis, c->win, 2*sw, 0);

	/* before disconnecting c */
	refocus(c->next, c);

	/* For both, if NULL, then we're still okay */
	if ( (p = findprevclient(c, AnyVis)) )
		p->next = c->next;
	else
		curmon->head = c->next;
}

/* dwm copypasta */
void killclient(){
	if (!curmon->current)
		return;

	XGrabServer(dis);
	XSetErrorHandler(xerrordummy);
	XSetCloseDownMode(dis, DestroyAll);
	XKillClient(dis, curmon->current->win);
	XSync(dis, False);
	XSetErrorHandler(xerror);
	XUngrabServer(dis);
}

void manage(Window parent, XWindowAttributes* wa){
	client* c, * t;
	Window trans = None;

	if ( !(c = ecalloc(1, sizeof(client))) )
		die("Error while callocing new client!");

	c->win = parent;
	c->isfloat = c->oldfloat = c->isfull = c->iscur = 0;
	c->x = wa->x;
	c->y = (wa->y < curmon->y) ? curmon->y : wa->y;
	c->w = wa->width; c->h = wa->height;

	if (XGetTransientForHint(dis, parent, &trans) && (t = findclient(trans))){
		c->desks = t->desks;
	} else {
		c->desks = curmon->seldesks;
		applyrules(c);
	}
	if (!c->isfloat) c->isfloat = c->oldfloat = (trans != None);

	attachaside(c);

	/* move out of the way until told otherwise */
	XMoveResizeWindow(dis, c->win, c->x + 2*sw, c->y, c->w, c->h);
	XMapWindow(dis, c->win);

	justswitch = 1;
	curmon->curlayout->arrange(curmon);
	if (c->desks & curmon->seldesks) updatefocus();
}

void mapclients(){
	for EACHCLIENT(curmon->head) if ISVISIBLE(ic) XMapWindow(dis, ic->win); else XUnmapWindow(dis, ic->win);
}

void moveclient(const Arg arg){
	client* c;

	if (!curmon->current || curmon->current->isfull)
		return;

	/* Up stack */
	if (arg.i > 0)
		moveclientup(curmon->current, NoZoom);
	/* Down stack - equivalent to moving next visible client up */
	else if ( arg.i < 0 && (c = findvisclient(curmon->current->next)) )
		moveclientup(c, NoZoom);

	justswitch = 1;
	curmon->curlayout->arrange(curmon);
	updatefocus();
}

void moveclientup(client* c, int wantzoom){
	client* p, * target, * ptarget;

	if (!c) return;

	/* Go up only if not highest visible */
	if (wantzoom){
		if ( !(target = findvisclient(curmon->head)) || target == c )
			return;
	} else {
		if ( !(target = findprevclient(c, OnlyVis)) )
			return;
	}

	p = findprevclient(c, AnyVis);
	ptarget = findprevclient(target, AnyVis);

	/* if p == target, then we're still okay */
	p->next = c->next;
	c->next = target;
	if (ptarget) ptarget->next = c;
	else curmon->head = c;
}

/* some dwm copypasta */
void movefocus(const Arg arg){
	client* j, * c = NULL;

	if (!curmon->current || curmon->current->isfull)
		return;

	/* up stack */
	if (arg.i > 0){
		for (j=curmon->head;j != curmon->current;j=j->next)
			if ISVISIBLE(j) c = j;

		if (!c)
			/* if curmon->current was highest, go to the bottom */
			for (;j;j=j->next) if ISVISIBLE(j) c = j;

	/* down stack */
	} else {
		if ( !(c = findvisclient(curmon->current->next)) )
			for (c=curmon->head;c && !ISVISIBLE(c);c=c->next);
	}

	changecurrent(c, curmon->curdesk);
	updatefocus();
}

void raisefloats(){
	for EACHCLIENT(curmon->head) if (ISVISIBLE(ic) && ic->isfloat){
			resizeclient(ic, ic->x, ic->y, ic->w, ic->h);
			XRaiseWindow(dis, ic->win);
		}
}

/* focus moves down if possible, else up */
void refocus(client* n, client* p){
	client* vis;
	vis = (vis = findvisclient(n)) ? vis : findprevclient(p, OnlyVis);
	changecurrent(vis, curmon->curdesk);
}

void resizeclient(client* c, int x, int y, int w, int h){
	XWindowChanges wc;

	c->x = wc.x = x;
	c->y = wc.y = y;
	c->w = wc.width = w;
	c->h = wc.height = h;
	XConfigureWindow(dis, c->win, CWX|CWY|CWWidth|CWHeight, &wc);
}

void sendtomon(client* c, monitor* oldmon, monitor* newmon, int wantdetach, int wantfocus, int wantstay){
	if (wantdetach){
		changemon(oldmon, 0);
		detach(c);
		curmon->curlayout->arrange(curmon);
	}

	changemon(newmon, 0);
	attachaside(c);
	c->desks = curmon->seldesks;
	curmon->curlayout->arrange(curmon);

	if (wantstay){
		changemon(oldmon, wantfocus);
	}
}

void todesktop(const Arg arg){
	if (curmon->curdesk & 1 << arg.i) return;

	curmon->current->desks = 1 << arg.i;
	curmon->current->iscur = 0;
	changecurrent(curmon->current, 1 << arg.i);

	XUnmapWindow(dis, curmon->current->win);
	XSync(dis, False);

	refocus(curmon->current->next, curmon->current);
	curmon->curlayout->arrange(curmon);
	updatefocus();
}

void toggledesktop(const Arg arg){
	unsigned int newdesks;

	if (!curmon->current) return;

	newdesks = curmon->current->desks ^ 1 << arg.i;
	if (newdesks){
		curmon->current->desks = newdesks;
		changecurrent(curmon->current, 1 << arg.i);

		if ( !(ISVISIBLE(curmon->current)) ){
			XUnmapWindow(dis, curmon->current->win);
			XSync(dis, False);
			refocus(curmon->current->next, curmon->current);
		}

		curmon->curlayout->arrange(curmon);
		updatefocus();
	}
}

void togglefloat(){
	XWindowChanges wc;

	if (!curmon->current || curmon->current->isfull) return;

	if ( (curmon->current->isfloat = !curmon->current->isfloat) ){
		wc.sibling = curmon->current->win;
		wc.stack_mode = Below;

		for EACHCLIENT(curmon->head) if (ic != curmon->current) XConfigureWindow(dis, ic->win, CWSibling|CWStackMode, &wc);

	} else {
		wc.sibling = curmon->bar->win;
		wc.stack_mode = Below;
		XConfigureWindow(dis, curmon->current->win, CWSibling|CWStackMode, &wc);
	}

	curmon->curlayout->arrange(curmon);
}

void togglefs(){
	if (!curmon->current) return;

	if ( (curmon->current->isfull = !curmon->current->isfull) ){
		curmon->current->oldfloat = curmon->current->isfloat;
		curmon->current->isfloat = 0;

		XMoveResizeWindow(dis, curmon->current->win, curmon->x, 0, curmon->w, curmon->h + curmon->bar->h);
		XRaiseWindow(dis, curmon->current->win);

		XUnmapWindow(dis, curmon->bar->win);

	} else {
		curmon->current->isfloat = curmon->current->oldfloat;
		curmon->curlayout->arrange(curmon);

		XMapRaised(dis, curmon->bar->win);
		drawbars();
	}
}

void tomon(const Arg arg){
	client* c;
	monitor* m;

	if ( !(c = curmon->current) ) return;

	if (arg.i > 0){
		if (!curmon->next) return;
		else m = curmon->next;

	} else {
		for (m=mhead;m && m != curmon && m->next != curmon;m=m->next);
	}

	if (m && m != curmon) sendtomon(c, curmon, m, YesDetach, YesFocus, NoStay);
}

void unmanage(client* c){
	monitor* im;

	detach(c);
	if (c) free(c);
	curmon->curlayout->arrange(curmon);

	/* Change monitors if necessary */
	if (!curmon->head)
		for (im=mhead;im;im=im->next)
			if (im != curmon && im->head)
				changemon(im, 1);

	updatefocus();
}

void updatefocus(){
	if (curmon->current){
		XSetInputFocus(dis, curmon->current->win, RevertToPointerRoot, CurrentTime);
		XRaiseWindow(dis, curmon->current->win);

	} else {
		XSetInputFocus(dis, root, RevertToPointerRoot, CurrentTime);
	}

	raisefloats();
	drawbars();
}

void zoom(){
	moveclientup(curmon->current, YesZoom);

	justswitch = 1;
	curmon->curlayout->arrange(curmon);
	updatefocus();
}


/* ---------------------------------------
 * Monitor Manipulation
 * ---------------------------------------
 */

void changemon(monitor* m, int focus){
	justswitch = 0;
	curmon = m;
	if (focus) updatefocus();
}

void cleanupmon(monitor* m){
	free(m->desks);

	XUnmapWindow(dis, m->bar->win);
	XDestroyWindow(dis, m->bar->win);

	free(m->bar);
	free(m);
}

monitor* createmon(int num, int x, int y, int w, int h){
	monitor* m = ecalloc(1, sizeof(monitor));
	int i;

	m->num = num;
	m->x = x; m->y = y;
	m->w = w; m->h = h;

	m->bar = initbar(m);

	m->y += m->bar->h;
	m->h -= m->bar->h;

	/* Default to first layout */
	m->curlayout = (layout*) &layouts[0];
	m->msize = m->w * MASTER_SIZE;

	m->desks = ecalloc(TABLENGTH(tags), sizeof(desktop));
	for (i=0;i < TABLENGTH(tags);i++){
		m->desks[i].curlayout = m->curlayout;
		m->desks[i].msize = m->msize;
	}

	/* Default to first desktop */
	m->seldesks = m->curdesk = 1 << 0;
	m->head = NULL;
	m->current = NULL;

	return m;
}

//monitor* createmon(){
//	int i;
//	monitor* m = ecalloc(1, sizeof(monitor));
//
//	/* Default to first layout */
//	m->curlayout = (layout*) &layouts[0];
//	m->msize = m->w * MASTER_SIZE;
//
//	m->desks = ecalloc(TABLENGTH(tags), sizeof(desktop));
//	for (i=0;i < TABLENGTH(tags);i++){
//		m->desks[i].curlayout = m->curlayout;
//		m->desks[i].msize = m->msize;
//	}
//
//	/* Default to first desktop */
//	m->seldesks = m->curdesk = 1 << 0;
//	m->head = NULL;
//	m->current = NULL;
//
//	return m;
//}

monitor* findmon(Window w){
	monitor* im;

	for (im=mhead;im;im=im->next){
		for EACHCLIENT(im->head){
			if (ic->win == w){
				return im;
			}
		}
	}

	return curmon;
}

monitor* findprevmon(monitor* m){
	monitor* i;

	for (i=mhead;i && i != m;i=i->next){
		if (i->next == m)
			return i;
	}

	return NULL;
}

void focusmon(const Arg arg){
	monitor* m;

	if (arg.i > 0){
		if (!curmon->next) return;
		else m = curmon->next;

	} else {
		if (curmon == mhead)
			return;
		else
			for (m=mhead;m && m != curmon && m->next != curmon;m=m->next);
	}

	if (m && m != curmon) changemon(m, 1);
}

/* dwm copypasta */
#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo* unique, size_t n, XineramaScreenInfo* info){
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org)
			return 0;
	return 1;
}
#endif

/* TODO: Support adding/removing monitors and transferring the clients */
/* some dwm copypasta */
void initmons(){
	monitor* m;

#ifdef XINERAMA
	if (XineramaIsActive(dis)){
		int i, j, ns;

		XineramaScreenInfo* info = XineramaQueryScreens(dis, &ns);
		XineramaScreenInfo* unique;

      		/* only consider unique geometries as separate screens */
		unique = ecalloc(ns, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < ns; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		
		mhead = m = createmon(0, unique[0].x_org, unique[0].y_org, unique[0].width, unique[0].height);
		for (i=1;i < j;i++){
			m->next = createmon(i, unique[i].x_org, unique[i].y_org, unique[i].width, unique[i].height);
			m = m->next;
		}

		/* My laptop insists that the primary display is not
		 * :0. So reorder and renumber monitors if necessary.
		 * I'm an idiot and can't write a sorting method,
		 * good thing no human being uses lots of monitors!
		 */
		for (i=0;i < j;i++) isortmons();
		for (m=mhead, i=0;m;m=m->next, i++) m->num = i;

		free(unique);
		changemon(mhead, 0);
	}
#endif
	if (!mhead){
		mhead = createmon(0, 0, 0, sw, sh);
		changemon(mhead, 0);
	}
}

/* thanks to firmcodes.com */
void isortmons(){
	monitor* min, * i, * m = mhead;

	while (m && m->next){
		min = m; i = m->next;
		
		while (i){
			if (min->x > i->x) min = i;
			i = i->next;
		}

		swapmon(m, min);
		m = m->next;
	}
}

void swapmon(monitor* x, monitor* y){
	monitor* px = findprevmon(x), * py = findprevmon(y);

	if (x == y) return;

	if (x == mhead) mhead = y;
	if (px) px->next = y;
	if (py) py->next = y->next;
	y->next = x;
}

///* dwm copypasta - use the dwm 6.1 approach */
//void updategeom(){
//	monitor* m;
//
//#ifdef XINERAMA
//	if (XineramaIsActive(dis)) {
//		int i, j, ns;
//		monitor* im, * prev, * mprev, * oldmhead = mhead;
//		client* c;
//		XineramaScreenInfo* info = XineramaQueryScreens(dis, &ns);
//		XineramaScreenInfo* unique = NULL;
//
//		/* only consider unique geometries as separate screens */
//		unique = ecalloc(ns, sizeof(XineramaScreenInfo));
//		for (i=0, j=0;i < ns;i++)
//			if (isuniquegeom(unique, j, &info[i]))
//				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
//		XFree(info);
//
//		for (i=0;i < j;i++){
//			fprintf(stderr, "working on %d\n", i);
//			m = createmon();
//
//			m->x = unique[i].x_org;
//			m->y = unique[i].y_org;
//			m->w = unique[i].width;
//			m->h = unique[i].height;
//
//			m->bar = initbar(m);
//
//			m->y += m->bar->h;
//			m->h -= m->bar->h;
//
//			if (!mhead){
//				mhead = m;
//				fprintf(stderr, "just attached mhead\n");
//
//			} else {
//				for (im=mhead;im->next;im=im->next);
//				im->next = m;
//				fprintf(stderr, "just attached at the tail\n");
//			}
//		}
//
//		/* Reorder and renumber monitors if necessary.
//		 * I'm an idiot who can't write a sorting method,
//		 * good thing no human being uses lots of monitors!
//		 */
//		for (i=0;i < j;i++) isort();
//		for (m=mhead, i=0;m;m=m->next, i++){
//			m->num = i;
//			fprintf(stderr, "just numbered monitor #%d\n", m->num);
//		}
//		free(unique);
//
//		/* reattach any old clients to the new mhead */
//		m = oldmhead;
//		while (m){
//			fprintf(stderr, "reattaching old clients\n");
//			while ( (c = m->head) ){
//				m->head = c->next;
//				/* send client to mhead */
//				sendtomon(c, m, mhead, YesDetach, YesFocus, YesStay);
//			}
//			m = m->next;
//			cleanupmon(m);
//		}
//	} else
//#endif /* XINERAMA */
//	{	
//		fprintf(stderr, "no xinerama defined\n");
//
//		if (!mhead){
//			mhead = createmon();
//		}
//
//		if (mhead->w != sw || mhead->h != sh) {
//			mhead->x = mhead->y = 0;
//			mhead->w = sw; mhead->h = sh;
//
//			mhead->bar = initbar(mhead);
//
//			mhead->y += mhead->bar->h;
//			mhead->h -= mhead->bar->h;
//			changemon(mhead, 0);
//		}
//	}
//
//	for (m=mhead;m;m=m->next)
//		/* if pointer is inside this mon's boundaries */
//		if (!ISOUTSIDE(getptrcoords(0), getptrcoords(1), m->x, m->y, m->w, m->h)){
//			fprintf(stderr, "cursor is in, and changing to, monitor #%d\n", m->num);
//			changemon(m, 1);
//			break;
//		}
//}


/* ---------------------------------------
 * Client Interfacing
 * ---------------------------------------
 */

client* findclient(Window w){
	monitor* im;

	for (im=mhead;im;im=im->next){
		for EACHCLIENT(im->head){
			if (ic->win == w){
				return ic;
			}
		}
	}

	return NULL;
}

client* findcurrent(){
	for EACHCLIENT(curmon->head) if ( ISVISIBLE(ic) && (ic->iscur & curmon->curdesk) ){
			return ic;
		}

	return NULL;
}

client* findprevclient(client* c, int onlyvis){
	client* i, * ret = NULL;
	for (i=curmon->head;i && i != c;i=i->next){
		if (onlyvis)
			if ISVISIBLE(i) ret = i;

		if (i->next == c){
			if (onlyvis)
				return ret;
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


/* ---------------------------------------
 * Bar
 * ---------------------------------------
 */

/* part dwm copypasta */
void drawbar(monitor* m){
	int j, x = 2; /* 2px left padding */
	int xsetr_text_w = 0, is_sel, dodraw = 1;
	unsigned int occ = 0;

	/* draw background */
	sdrw->scheme = scheme[SchNorm];
	XSetForeground(dis, sdrw->gc, sdrw->scheme[ColBg].pixel);
	XFillRectangle(dis, sdrw->d, sdrw->gc, 0, 0, sw, m->bar->h);

	/* draw status */
	xsetr_text_w = TEXTW(m, xsetr_text) - lrpad + 2; /* 2px right padding */
	drawbartext(m, m->bar->w - xsetr_text_w, 0, xsetr_text_w, m->bar->h, 0, xsetr_text);

	for EACHCLIENT(m->head) occ |= ic->desks;

	/* draw tags */
	for (j=0;j<TABLENGTH(tags);j++){
		/* do not draw vacant tags, but do draw selected tags regardless */
		is_sel = m->seldesks & 1 << j;
		if ( !(occ & 1 << j) && !is_sel )
			continue;

		sdrw->scheme = scheme[is_sel ? SchSel : SchNorm];

		x = drawbartext(m, x, 0, TEXTW(m, syms[SymLeft]) + 1, m->bar->h, 0,
				is_sel ? syms[SymLeft] : " ") - lrpad;
		x = drawbartext(m, x, 0, TEXTW(m, tags[j]), m->bar->h, 0, tags[j]) - lrpad + 2;
		x = drawbartext(m, x, 0, TEXTW(m, syms[SymRight]), m->bar->h, 0,
				is_sel ? syms[SymRight] : " ") - lrpad / 2;
	}
	x -= lrpad / 2;
	sdrw->scheme = scheme[SchNorm];
	drawbartext(m, x, 0, TEXTW(m, m->curlayout->symbol), m->bar->h, lrpad / 2, m->curlayout->symbol);

	for EACHCLIENT(m->head) if (ISVISIBLE(ic) && ic->isfull){
		dodraw = 0;
		break;
	}
	if (dodraw) XMapRaised(dis, m->bar->win);
	XCopyArea(dis, sdrw->d, m->bar->win, sdrw->gc, 0, 0, m->bar->w, m->bar->h, 0, 0);
	XSync(dis, False);
}

void drawbars(){
	monitor* im;
	for (im=mhead;im;im=im->next) drawbar(im);
}

int drawbartext(monitor* m, int x, int y, int w, int h, unsigned int lpad, const char* text){
	int ty = y + (h - lrpad) / 2 + sdrw->xfont->ascent;
	XftDrawString8(sdrw->xd, &sdrw->scheme[ColFg], sdrw->xfont, x + lpad, ty, (XftChar8*)text, slen(text));

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
		if ( XmbTextPropertyToTextList(dis, &name, &list, &n) >= Success && n > 0 && *list ){
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);

	return 1;
}

/* does anything here need to be free() or XFree()? */
int gettextwidth(monitor* m, const char* str, int len){
	XGlyphInfo xgi;
	XftTextExtents8(dis, sdrw->xfont, (XftChar8*)str, len, &xgi);

	return xgi.width;
}

bar* initbar(monitor* m){
	bar* sbar;

	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ExposureMask
	};

	if ( !(sbar = ecalloc(1, sizeof(bar))))
		die("Could not ecalloc sbar!");

	sbar->h = lrpad + 2;
	sbar->w = m->w;

	sbar->win = XCreateWindow(dis, root, m->x, m->y, sbar->w, sbar->h, 0, 
			DefaultDepth(dis, screen), InputOutput, DefaultVisual(dis,screen), 
			CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);

	XDefineCursor(dis, sbar->win, XCreateFontCursor(dis, 68));
	XMapRaised(dis, sbar->win);

	return sbar;
}

/* dwm copypasta */
void updatestatus(){
	if (!gettextprop(root, XA_WM_NAME, xsetr_text, sizeof(xsetr_text)))
		strcpy(xsetr_text, "where's the leak, ma'am?");

	drawbars();
}


/* ---------------------------------------
 * Desktop Interfacing
 * ---------------------------------------
 */

void changemsize(const Arg arg){
	curmon->msize += ( ((curmon->msize < 0.95 * curmon->w) && (arg.f > 0))
			|| ((curmon->msize > 0.05 * curmon->w) && (arg.f < 0))  ) ? arg.f * curmon->w : 0;

	curmon->curlayout->arrange(curmon);
}

void loaddesktop(int i){
	curmon->desks[POSTOINT(curmon->curdesk)].msize = curmon->msize;
	curmon->desks[POSTOINT(curmon->curdesk)].curlayout = curmon->curlayout;

	curmon->msize = curmon->desks[i].msize;
	curmon->curlayout = curmon->desks[i].curlayout;
}

void monocle(monitor* m){
	for EACHCLIENT(m->head) if (ISVISIBLE(ic) && !ic->isfloat && !ic->isfull){
			resizeclient(ic, m->x, m->y, m->w, m->h);
		} 

	raisefloats();
}

void setlayout(const Arg arg){
	curmon->curlayout = (layout*) arg.v;
	curmon->curlayout->arrange(curmon);
	drawbars();
}

void tile(monitor* m){
	client* nf = NULL;
	int n = 0, x = m->x, y = m->y;

	/* Find the first non-floating, visible window and tally non-floating, visible windows */
	for EACHCLIENT(m->head) if (!ic->isfloat && ISVISIBLE(ic)){
			nf = (!nf) ? ic : nf;
			n++;
		}

	if (nf && n == 1){
		if (!nf->isfull) resizeclient(nf, x, y, m->w, m->h);

	} else if (nf){
		/* so having a master doesn't affect stack splitting */
		n--;

		/* Master window */
		if (!nf->isfull) resizeclient(nf, x, y, m->msize, m->h);

		/* Stack */
		for EACHCLIENT(nf->next){
			if (ISVISIBLE(ic) && !ic->isfloat && !ic->isfull){
				resizeclient(ic, x + m->msize, y, m->w - m->msize, m->h / n);

				y += m->h / n;
			}
		}
	}

	raisefloats();
}

void toggleview(const Arg arg){
	int i;

	/* if this would leave nothing visible */
	if ((curmon->seldesks ^ 1 << arg.i) == 0) return;

	curmon->seldesks ^= 1 << arg.i;
	mapclients();

	if (!(curmon->curdesk & curmon->seldesks)){
		for (i=0;i < TABLENGTH(tags);i++){
			if (curmon->seldesks & 1 << i){
				loaddesktop(i);
				curmon->curdesk = 1 << i;
				break;
			}
		}
	}

	curmon->curlayout->arrange(curmon);
	updatefocus();
}

void view(const Arg arg){
	client* c;

	loaddesktop(arg.i);
	curmon->seldesks = curmon->curdesk = 1 << arg.i;

	mapclients();

	if ( !(curmon->current = findcurrent()) && (c = findvisclient(curmon->head)) )
		changecurrent(c, curmon->curdesk);

	justswitch = 1;
	curmon->curlayout->arrange(curmon);
	updatefocus();
}

void viewall(){
	int i;
	curmon->seldesks = 0;
	for (i=0;i < TABLENGTH(tags);i++) curmon->seldesks ^= 1 << i;

	mapclients();

	curmon->curlayout->arrange(curmon);
	updatefocus();
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
	monitor* m, * tm;

	XUngrabKey(dis, AnyKey, AnyModifier, root);

	for (m=mhead;m;m=m->next){
		changemon(m, 0);

		while (curmon->current)
			unmanage(curmon->current);
	}

	m = mhead;
	while (m){
		tm = m->next;
		cleanupmon(m);
		m = tm;
	}

	XftDrawDestroy(sdrw->xd);
	XFreeGC(dis, sdrw->gc);
	XftFontClose(dis, sdrw->xfont);
	XFreePixmap(dis, sdrw->d);

	for (i=0;i < TABLENGTH(colors);i++) free(scheme[i]);
	free(scheme);

	XSync(dis, False);
	XSetInputFocus(dis, PointerRoot, RevertToPointerRoot, CurrentTime);
}

/* y'all need to free() this bad boi when you cleanup */
XftColor* createscheme(const char* clrnames[], size_t clrcount){
	int i;
	XftColor* sch;

	if ( !(sch = ecalloc(clrcount, sizeof(XftColor))) )
		die("Error while trying to ecalloc for createscheme");

	for (i=0;i < clrcount;i++){
		if ( !XftColorAllocName(dis, DefaultVisual(dis,screen), DefaultColormap(dis,screen), clrnames[i], &sch[i]) )
			die("Error while trying to allocate color '%s'", clrnames[i]);
	}

	return sch;
}

int getptrcoords(int rety){
	int x, y, di;
	unsigned int dui;
	Window dummy;

	XQueryPointer(dis, root, &dummy, &dummy, &x, &y, &di, &di, &dui);

	return rety ? y : x;
}

void grabkeys(){
	int i;
	KeyCode code;

	for (i=0;i < TABLENGTH(keys);i++){
		if ( (code = XKeysymToKeycode(dis, keys[i].keysym)) )
			XGrabKey(dis, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
	}
}

void initdrw(){
	if ( !(sdrw = ecalloc(1, sizeof(drw))) )
		die("could not ecalloc a drw!");

	if ( !(sdrw->xfont = XftFontOpenName(dis, screen, fontname)) )
		die("The font you tried to use was not found. Check the name.");

	lrpad = sdrw->xfont->ascent + sdrw->xfont->descent;

	sdrw->d = XCreatePixmap(dis, root, sw, sh, DefaultDepth(dis, screen));
	sdrw->gc = XCreateGC(dis, sdrw->d, 0, NULL);
	sdrw->xd = XftDrawCreate(dis, sdrw->d, DefaultVisual(dis,screen), DefaultColormap(dis,screen));
}

void setup(){
	int i;
	sigchld(0);

	XSetWindowAttributes wa;

	screen = DefaultScreen(dis);
	root = RootWindow(dis, screen);

	sw = XDisplayWidth(dis, screen);
	sh = XDisplayHeight(dis, screen);

	scheme = ecalloc(TABLENGTH(colors), sizeof(XftColor*));
	for (i=0;i < TABLENGTH(colors);i++) scheme[i] = createscheme(colors[i], 2);

	running = 1;

	mhead = NULL;
	curmon = NULL;

	initdrw();
	initmons();
	//updategeom();
	loaddesktop(0);

	wa.cursor = XCreateFontCursor(dis, 68);
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dis, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dis, root, wa.event_mask);

	grabkeys();
	updatestatus();
	drawbars();
}

/* dwm copypasta */
void sigchld(int unused){
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	
	while (0 < waitpid(-1, NULL, WNOHANG));
}

/* dwm copypasta */
void spawn(const Arg arg){
	if (arg.v == dmenucmd) dmenumon[0] = '0' + curmon->num;

	if (fork() == 0){
		if (dis) close(ConnectionNumber(dis));
		setsid();
		execvp( ((char **) arg.v)[0], (char **) arg.v);

		fprintf(stderr, "sara: execvp %s", ((char **) arg.v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
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

int xerrordummy(Display* dis, XErrorEvent* e){
	return 0;
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
        XCloseDisplay(dis);

	return 0;
}
