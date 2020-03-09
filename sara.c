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

#define BUTTONMASK              	(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK               	(BUTTONMASK|PointerMotionMask)
#define EACHCLIENT(_I)			(ic=_I;ic;ic=ic->next) /* ic is a global */
#define EACHMON(_M)			(im=_M;im;im=im->next) /* im is a global */
#define ISOUTSIDE(PX,PY,X,Y,W,H)	((PX > X + W || PX < X || PY > Y + H || PY < Y))
#define ISVISIBLE(C)			((C->desks & C->mon->seldesks))
#define MAX(A,B)               		((A) > (B) ? (A) : (B))
#define POSTOINT(X)			((int)(ceil(log2(X)) == floor(log2(X)) ? ceil(log2(X)) : 0))
#define TABLENGTH(X)    		(sizeof(X)/sizeof(*X))
#define TEXTW(M,X)			(gettextwidth(M, X, slen(X)) + lrpad)

enum { SchNorm, SchSel };
enum { ColFg, ColBg };
enum { SymLeft, SymRight };
enum { AnyVis, OnlyVis };
enum { NoZoom, YesZoom };
enum { NoFocus, YesFocus };
enum { ClkPlcHldr, ClkWin };
enum { WantMove, WantResize };
enum { NoFloat, YesFloat };


/* ---------------------------------------
 * Structs
 * ---------------------------------------
 */

typedef struct bar bar;
typedef struct client client;
typedef struct drw drw;
typedef struct desktop desktop;
typedef struct monitor monitor;

typedef union {
	const int i;
	const unsigned int ui;
	const float f;
	const void* v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int btn;
	void (*func)(const Arg arg);
	const Arg arg;
} button;

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
	int isfloat, isfull;
	int monitor;
} rule;

struct bar {
	int w, h;
	Window win;
};

struct client {
	int x, y, w, h;
	unsigned int desks;
	unsigned int iscur;
	/* being in monocle is not considered floating */
	int isfloat;
	int isfull;
	/* prior to togglefs */
	int oldfloat;
	Window win;
	client* next;
	monitor* mon;
}; 

struct drw {
	int w, h;
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

void die(const char* e, ...){
	fprintf(stdout, "sara: %s\n", e);
	exit(1);
}

void* ecalloc(size_t nmemb, size_t size){
	void* p;

	if ( !(p = calloc(nmemb,size)) ) die("ecalloc failed");

	return p;
}

int slen(const char* str){
	int i = 0;

	while (*str){ str++; i++; }

	return i;
}


/* ---------------------------------------
 * Main Function Declarations
 * ---------------------------------------
 */

/* X Event Processing */
static void buttonpress(XEvent* e);
static void configurenotify(XEvent* e);
static void configurerequest(XEvent* e);
static void destroynotify(XEvent* e);
static void enternotify(XEvent* e);
static void expose(XEvent* e);
static void focusin(XEvent* e);
static void keypress(XEvent* e);
static void maprequest(XEvent* e);
static void motionnotify(XEvent* e);
static void propertynotify(XEvent* e);
/* Client & Linked List Manipulation */
static void adjustcoords(client* c);
static void applyrules(client* c);
static void attachaside(client* c);
static void changecurrent(client* c, monitor* m, unsigned int deskmask);
static void configure(client* c);
static void detach(client* c);
static void killclient();
static void manage(Window parent, XWindowAttributes* wa);
static void manipulate(const Arg arg);
static void mapclients();
static void moveclient(const Arg arg);
static void moveclientup(client* c, int wantzoom);
static void movefocus(const Arg arg);
static void raisefloats();
static void refocus(client* n, client* p, monitor* m);
static void resizeclient(client* c, int x, int y, int w, int h);
static void sendmon(client* c, monitor* m);
static void todesktop(const Arg arg);
static void toggledesktop(const Arg arg);
static void togglefloat();
static void togglefs();
static void tomon(const Arg arg);
static void unmanage(client* c);
static void updatefocus();
static void zoom();
/* Monitor Manipulation */
static void changemon(monitor* m, int wantfocus);
static void cleanupmon(monitor* m);
static monitor* createmon(int num, int x, int y, int w, int h);
static monitor* dirtomon(int dir);
static monitor* findmon(Window w);
static void focusmon(const Arg arg);
static void updategeom();
/* Client Interfacing */
static client* findclient(Window w);
static client* findcurrent();
static client* findprevclient(client* c, int wantvis, int wantfloat);
static client* findvisclient(client* c, int wantfloat);
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
/* Backend */
static void cleandrw();
static void cleanup();
static XftColor* createscheme(const char* clrnames[], size_t clrcount);
static int getptrcoords(int* x, int* y);
static void grabbuttons(client* c, int focused);
static void grabkeys();
static void initdrw();
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static int xerror(Display* dis, XErrorEvent* e);
static int xerrordummy(Display* dis, XErrorEvent* e);
static int xsendkill(Window w);
static void youviolatedmymother();


/* Make the above known */
#include "config.h"


/* ---------------------------------------
 * Globals
 * ---------------------------------------
 */

/* X Interfacing */
static Cursor cursor;
static Display* dis;
static Window root;
static int screen;
static int sh;
static int sw;
/* Monitor Interfacing */
static monitor* curmon;
static monitor* mhead;
/* Backend */
static client* ic; /* for EACHCLIENT iterating */
static monitor* im; /* for EACHMON iterating */
static int justmanage;
static int justmanageunder;
static int justswitch;
static int lrpad;
static int running;
static XftColor** scheme;
static drw* sdrw;
static char xsetr_text[256];

/* Events array */
static void (*events[LASTEvent])(XEvent* e) = {
	[ButtonPress] = buttonpress,
	[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify
};


/* ---------------------------------------
 * X Event Processing
 * ---------------------------------------
 */

/* dwm copypasta */
void buttonpress(XEvent* e){
	unsigned int click = 0;
	int i;
	client* c;
	monitor* m;
	XButtonPressedEvent* ev = &e->xbutton;

	if ( (m = findmon(ev->window)) && m != curmon)
		changemon(m, NoFocus);

	if ( (c = findclient(ev->window)) ){
		if (c != c->mon->current){
			changecurrent(c, c->mon, c->mon->curdesk);
			updatefocus();
		}
		XAllowEvents(dis, ReplayPointer, CurrentTime);
		click = ClkWin;
	}

	for (i = 0; i < TABLENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func
		&& buttons[i].btn == ev->button
		&& buttons[i].mask == ev->state)
			buttons[i].func(buttons[i].arg);

	justswitch = 0;
}

/* dwm copypasta */
void configurenotify(XEvent* e){
	XConfigureEvent* ev = &e->xconfigure;

	if (ev->window == root){
		sw = ev->width; sh = ev->height;
		if (sdrw && (sdrw->w != sw || sdrw->h != sh)){
			cleandrw();
			initdrw();
		}
		updategeom();

		for EACHMON(mhead){
			for EACHCLIENT(im->head) if (ic->isfull){
					resizeclient(ic, im->x, im->y - im->bar->h, im->w, im->h + im->bar->h);
				}

			im->curlayout->arrange(im);
		}
		updatefocus();
	}
}

/* dwm copypasta */
void configurerequest(XEvent* e){
	client* c;
	monitor* m;
	XConfigureRequestEvent* ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ( (c = findclient(ev->window)) ){
		if (c->isfloat){
			m = c->mon;
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

/* some dwm copypasta */
void enternotify(XEvent* e){
	client* c;
	monitor* m;
	XCrossingEvent* ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;

	if ( !(c = findclient(ev->window)) )
		return;

	/* if focus is pulled by manage, and ptr enters that client, allow new events */
	if (c == curmon->current){
		if (justmanage)
			justmanage = justswitch = 0;
		return;
	}

	/* prevent focus from freaking out if you spawn a client underneath a ptr-focused float */
	if (justmanageunder){
		justmanageunder = justswitch = 0;
		return;
	}

	/* if enternotify is generated by moveclient, movefocus, or view, ignore it */
	if (justswitch){
		justswitch = 0;
		return;
	}

	if ( (m = c->mon) && m != curmon )
		changemon(m, NoFocus);

	changecurrent(c, c->mon, c->mon->curdesk);
	updatefocus();
}

void expose(XEvent* e){
	XExposeEvent* ev = &e->xexpose;

	if (ev->count == 0 && findmon(ev->window))
		drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void focusin(XEvent* e){
	XFocusChangeEvent* ev = &e->xfocus;

	if (curmon->current && ev->window != curmon->current->win)
		updatefocus();
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
	XMotionEvent* ev = &e->xmotion;

	if (ev->window != root)
		return;

	if (ISOUTSIDE(ev->x_root, ev->y_root, curmon->x, curmon->y - curmon->bar->h, curmon->w, curmon->h + curmon->bar->h)){
		for EACHMON(mhead){
			if (im != curmon){
				changemon(im, YesFocus);
				return;
			}
		}
	}
}

/* dwm copypasta */
void propertynotify(XEvent* e){
	XPropertyEvent* ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
}


/* ---------------------------------------
 * Client & Linked List Manipulation
 * ---------------------------------------
 */

void adjustcoords(client* c){
	if (ISOUTSIDE(c->x, c->y, c->mon->x, c->mon->y - c->mon->bar->h, c->mon->w, c->mon->h + c->mon->bar->h)){
		/* find which one it is inside */
		for EACHMON(mhead)
			if (!ISOUTSIDE(c->x, c->y, im->x, im->y - im->bar->h, im->w, im->h + im->bar->h)){
				c->x += (im->x < c->mon->x) ? c->mon->x : -im->x;
				c->y += (im->y < c->mon->y) ? c->mon->y : -im->y;
				break;
			}
	}
}

/* mostly dwm copypasta */
void applyrules(client* c){
	const char* class, * instance;
	int i;
	const rule* r;
	XClassHint ch = { NULL, NULL };
	XTextProperty tp;

	c->isfloat = c->desks = 0;

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
			c->isfull = r->isfull;
			c->desks |= r->desks;
			for EACHMON(mhead) if (im->num == r->monitor) break;
			if (im) c->mon = im;
		}
	}
	if (ch.res_class) XFree(ch.res_class);
	if (ch.res_name) XFree(ch.res_name);

	c->desks = c->desks ? c->desks : c->mon->seldesks;
}

void attachaside(client* c){
	client* l;

	if (!c->mon->head){
		c->mon->head = c;

	} else {
		/* If not the first on this desktop */
		if (c->mon->current){
			c->next = c->mon->current->next;
			c->mon->current->next = c;

		} else {
			for (l=c->mon->head;l->next;l=l->next);
			l->next = c;
		}
	}

	if (c->mon->current && c->mon->current->isfloat) justmanageunder = 1;

	changecurrent(c, c->mon, c->mon->curdesk);
}

void changecurrent(client* c, monitor* m, unsigned int deskmask){
	if (c){
		c->iscur ^= deskmask;
		grabbuttons(c, 1);
	}
	
	for EACHCLIENT(m->head) if (ic != c && (ic->iscur & deskmask)){
			ic->iscur ^= deskmask;
			grabbuttons(ic, 0);
		}

	m->current = c;
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

	/* before disconnecting c
	 * written this way so function keys don't move your focus
	 */
	if (c->desks & c->mon->curdesk) refocus(c->next, c, c->mon);
	else changecurrent(findprevclient(c, OnlyVis, YesFloat), c->mon, c->mon->curdesk);

	/* For both, if NULL, then we're still okay */
	if ( (p = findprevclient(c, AnyVis, YesFloat)) )
		p->next = c->next;
	else
		c->mon->head = c->next;
}

/* dwm copypasta */
void killclient(){
	if (!curmon->current)
		return;
	if (!xsendkill(curmon->current->win)){
		XGrabServer(dis);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dis, DestroyAll);
		XKillClient(dis, curmon->current->win);
		XSync(dis, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dis);
	}
}

void manage(Window parent, XWindowAttributes* wa){
	client* c, * t;
	Window trans = None;

	if ( !(c = ecalloc(1, sizeof(client))) )
		die("Error while callocing new client!");

	c->win = parent;
	c->isfloat = c->oldfloat = c->isfull = c->iscur = 0;
	c->x = wa->x; c->y = wa->y;
	c->w = wa->width; c->h = wa->height;

	if (XGetTransientForHint(dis, parent, &trans) && (t = findclient(trans))){
		c->desks = t->desks;
		c->mon = t->mon;

	} else {
		c->mon = curmon;
		applyrules(c);
	}
	if (!c->isfloat) c->isfloat = c->oldfloat = (trans != None);

	configure(c);
	XSelectInput(dis, c->win, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);

	attachaside(c);
	adjustcoords(c);
	c->y = (c->y < c->mon->y) ? c->mon->y : c->y;

	/* move out of the way until told otherwise */
	XMoveResizeWindow(dis, c->win, c->x + 2*sw, c->y, c->w, c->h);
	XMapWindow(dis, c->win);

	/* applyrules */
	if (c->isfull && c == curmon->current){
		c->isfull = !c->isfull;
		togglefs();
	}

	justmanage = 1;
	c->mon->curlayout->arrange(c->mon);
	if (c->desks & curmon->seldesks) updatefocus();
}

void mapclients(monitor* m){
	for EACHCLIENT(m->head) if ISVISIBLE(ic) XMapWindow(dis, ic->win); else XUnmapWindow(dis, ic->win);
}

void moveclient(const Arg arg){
	client* c;

	if (!curmon->current || curmon->current->isfull)
		return;

	/* Up stack */
	if (arg.i > 0)
		moveclientup(curmon->current, NoZoom);
	/* Down stack - equivalent to moving next tiled client up */
	else if ( arg.i < 0 && (c = findvisclient(curmon->current->next, NoFloat)) )
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
		if ( !(target = findvisclient(curmon->head, NoFloat)) || target == c )
			return;
	} else {
		if ( !(target = findprevclient(c, 0, NoFloat)) )
			return;
	}

	p = findprevclient(c, AnyVis, YesFloat);
	ptarget = findprevclient(target, AnyVis, YesFloat);

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

		/* if curmon->current was highest, go to the bottom */
		if (!c) for (;j;j=j->next) if ISVISIBLE(j) c = j;

	/* down stack */
	} else {
		if ( !(c = findvisclient(curmon->current->next, YesFloat)) )
			for (c=curmon->head;c && !ISVISIBLE(c);c=c->next);
	}

	justswitch = 1;
	changecurrent(c, curmon, curmon->curdesk);
	updatefocus();
}

/* dwm copypasta */
void manipulate(const Arg arg){
	int x, y, ocx, ocy, nx, ny, nw, nh, trytoggle = 0;
	client* c;
	monitor* m;
	XEvent ev;
	Time lasttime = 0;

	if ( !(c = curmon->current) || c->isfull )
		return;
	if (XGrabPointer(dis, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getptrcoords(&x, &y))
		return;
	if (arg.i == WantResize){
		if ( !(m = c->mon) )
			return;
		XWarpPointer(dis, None, c->win, 0, 0, 0, 0, c->w + 1, c->h + 1);
	}

	ocx = c->x; ocy = c->y;
	do {
		XMaskEvent(dis, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type){
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			events[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			if (arg.i == WantResize){
				nw = MAX(ev.xmotion.x - ocx + 1, 1);
				nh = MAX(ev.xmotion.y - ocy + 1, 1);
				/* if c extends beyond the boundaries of its monitor, make it a float */
				if (m->x + nw >= curmon->x && m->x + nw <= curmon->x + curmon->w
				&& m->y + nh >= curmon->y && m->y + nh <= curmon->y + curmon->h)
					trytoggle = 1;
				nx = c->x; ny = c->y;

			} else {
				nx = ocx + (ev.xmotion.x - x);
				ny = ocy + (ev.xmotion.y - y);
				/* if c is within snap pixel of the monitor borders, then snap */
				if (abs(curmon->x - nx) < snap)
					nx = curmon->x;
				else if (abs((curmon->x + curmon->w) - (nx + c->w)) < snap)
					nx = curmon->x + curmon->w - c->w;
				if (abs(curmon->y - ny) < snap)
					ny = curmon->y;
				else if (abs((curmon->y + curmon->h) - (ny + c->h)) < snap)
					ny = curmon->y + curmon->h - c->h;
				trytoggle = 1;
				nw = c->w; nh = c->h;
			}
			if (trytoggle)
				if (!c->isfloat && ((abs(nw - c->w) > snap || abs(nh - c->h) > snap)
				|| (abs(nx - c->x) > snap || abs(ny - c->y) > snap)))
					togglefloat();
			if (c->isfloat)
				resizeclient(c, nx, ny, nw, nh);
			break;
		}
	} while (ev.type != ButtonRelease);
	if (arg.i == WantResize)
		XWarpPointer(dis, None, c->win, 0, 0, 0, 0, c->w + 1, c->h + 1);
	XUngrabPointer(dis, CurrentTime);

	if (arg.i == WantResize)
		while (XCheckMaskEvent(dis, EnterWindowMask, &ev));

	if (ISOUTSIDE(c->x, c->y, curmon->x, curmon->y - curmon->bar->h, curmon->w,
				curmon->h + curmon->bar->h))
	{
		for EACHMON(mhead){
			if (im != curmon && !ISOUTSIDE(c->x, c->y, im->x, im->y - im->bar->h,
						im->w, im->h + im->bar->h))
			{
				sendmon(c, im);
				changemon(im, YesFocus);
				return;
			}
		}
	}
}

void raisefloats(){
	for EACHCLIENT(curmon->head) if (ISVISIBLE(ic) && ic->isfloat){
			resizeclient(ic, ic->x, ic->y, ic->w, ic->h);
			XRaiseWindow(dis, ic->win);
		}
}

/* focus moves down if possible, else up */
void refocus(client* n, client* p, monitor* m){
	client* vis;
	vis = (vis = findvisclient(n, YesFloat)) ? vis : findprevclient(p, OnlyVis, YesFloat);
	changecurrent(vis, m, m->curdesk);
}

void resizeclient(client* c, int x, int y, int w, int h){
	XWindowChanges wc;

	c->x = wc.x = x;
	c->y = wc.y = y;
	c->w = wc.width = w;
	c->h = wc.height = h;
	XConfigureWindow(dis, c->win, CWX|CWY|CWWidth|CWHeight, &wc);
	XSync(dis, False);
}

/* mostly dwm copypasta */
void sendmon(client* c, monitor* m){
	if (c->mon == m || c->isfull) return;

	detach(c);
	c->mon = m;
	c->desks = m->seldesks;

	c->next = NULL;
	attachaside(c);
	if (c->isfloat) adjustcoords(c);

	curmon->current = findcurrent();
	for EACHMON(mhead){
		mapclients(im);
		im->curlayout->arrange(im);
	}
	changemon(c->mon, YesFocus);
}

void todesktop(const Arg arg){
	if (curmon->curdesk & arg.ui) return;

	curmon->current->desks = arg.ui;
	curmon->current->iscur = 0;
	changecurrent(curmon->current, curmon, arg.ui);

	XUnmapWindow(dis, curmon->current->win);
	XSync(dis, False);

	refocus(curmon->current->next, curmon->current, curmon);
	curmon->curlayout->arrange(curmon);
	updatefocus();
}

void toggledesktop(const Arg arg){
	unsigned int newdesks;

	if (!curmon->current)
		return;
	if ( (newdesks = curmon->current->desks ^ arg.ui) ){
		curmon->current->desks = newdesks;
		changecurrent(curmon->current, curmon, arg.ui);

		if ( !(ISVISIBLE(curmon->current)) ){
			XUnmapWindow(dis, curmon->current->win);
			XSync(dis, False);
			refocus(curmon->current->next, curmon->current, curmon);
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

		for EACHCLIENT(curmon->head)
			if (ic != curmon->current)
				XConfigureWindow(dis, ic->win, CWSibling|CWStackMode, &wc);

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

		XMoveResizeWindow(dis, curmon->current->win, curmon->x, curmon->y - curmon->bar->h,
				curmon->w, curmon->h + curmon->bar->h);
		XRaiseWindow(dis, curmon->current->win);
		XUnmapWindow(dis, curmon->bar->win);

	} else {
		justswitch = 1;
		curmon->current->isfloat = curmon->current->oldfloat;
		curmon->curlayout->arrange(curmon);

		XMapRaised(dis, curmon->bar->win);
		drawbars();
	}
}

void tomon(const Arg arg){
	if (!curmon->current || !mhead->next)
		return;
	sendmon(curmon->current, dirtomon(arg.i));
}

void unmanage(client* c){
	monitor* m = c->mon;

	detach(c);
	XUngrabButton(dis, AnyButton, AnyModifier, c->win);
	if (c) free(c);
	m->curlayout->arrange(m);
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

void changemon(monitor* m, int wantfocus){
	if (curmon && curmon->current) grabbuttons(curmon->current, 0);
	curmon = m;
	if (wantfocus) updatefocus();
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

/* dwm copypasta */
monitor* dirtomon(int dir){
	monitor* m;

	if (dir > 0){
		if ( !(m = curmon->next) ) m = mhead;

	} else if (curmon == mhead){
		for (m=mhead;m->next;m=m->next);

	} else {
		for (m=mhead;m->next != curmon;m=m->next);
	}

	return m;
}

monitor* findmon(Window w){
	for EACHMON(mhead)
		for EACHCLIENT(im->head)
			if (ic->win == w)
				return im;

	return curmon;
}

void focusmon(const Arg arg){
	monitor* m;

	if (!mhead->next)
		return;
	if ((m = dirtomon(arg.i)) == curmon)
		return;
	changemon(m, YesFocus);
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

/* dwm copypasta - use the dwm 6.1 approach */
void updategeom(){
	int x, y;

#ifdef XINERAMA
	if (XineramaIsActive(dis)){
		int i, j, ns;
		client* c;
		monitor* m, * oldmhead = mhead;

		XineramaScreenInfo* info = XineramaQueryScreens(dis, &ns);
		XineramaScreenInfo* unique;

      		/* only consider unique geometries as separate screens */
		unique = ecalloc(ns, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < ns; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		
		mhead = m = createmon(0, unique[0].x_org, unique[0].y_org,
				unique[0].width, unique[0].height);
		for (i=1;i < j;i++){
			m->next = createmon(i, unique[i].x_org, unique[i].y_org,
					unique[i].width, unique[i].height);
			m = m->next;
		}

		free(unique);

		/* reattach any old clients to the new mhead */
		while ( (m = oldmhead) ){
			while ( (c = m->head) ){
				m->head = c->next;
				detach(c);
				c->mon = mhead;
				attachaside(c);
			}
			oldmhead = m->next;
			cleanupmon(m);
		}

	} else
#endif
	{
		if (mhead) cleanupmon(mhead);
		mhead = createmon(0, 0, 0, sw, sh);
	}

	/* focus monitor that has the pointer inside it */
	for EACHMON(mhead)
		if (getptrcoords(&x, &y) && !ISOUTSIDE(x, y, im->x, im->y - im->bar->h, im->w, im->h)){
			changemon(im, YesFocus);
			break;
		}
}


/* ---------------------------------------
 * Client Interfacing
 * ---------------------------------------
 */

client* findclient(Window w){
	for EACHMON(mhead)
		for EACHCLIENT(im->head)
			if (ic->win == w)
				return ic;

	return NULL;
}

client* findcurrent(){
	for EACHCLIENT(curmon->head)
		if (ISVISIBLE(ic) && (ic->iscur & curmon->curdesk))
			return ic;

	return NULL;
}

client* findprevclient(client* c, int onlyvis, int wantfloat){
	client* ret;

	for EACHCLIENT(c->mon->head){
		if (ic == c) break;
		if (onlyvis){
			if (ISVISIBLE(ic)) ret = ic;
		} else if (!wantfloat){
			if (ISVISIBLE(ic) && !ic->isfloat) ret = ic;
		}
		if (ic->next == c) return (onlyvis || !wantfloat) ? ret : ic;
	}

	return NULL;
}

client* findvisclient(client* c, int wantfloat){
	for EACHCLIENT(c){
		if ISVISIBLE(ic){
			if (!wantfloat){
				if (!ic->isfloat) return ic;
			} else {
				return ic;
			}
		}
	}
	
	return NULL;
}


/* ---------------------------------------
 * Bar
 * ---------------------------------------
 */

/* part dwm copypasta */
void drawbar(monitor* m){
	int j, x = 2; /* 2px left padding */
	int xsetr_text_w, is_sel, dodraw = 1;
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
		/* do not draw vacant tags, but do draw selected tags */
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
	for EACHMON(mhead) drawbar(im);
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

	XDefineCursor(dis, sbar->win, cursor);
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
	for EACHCLIENT(m->head)
		if (ISVISIBLE(ic) && !ic->isfloat && !ic->isfull)
			resizeclient(ic, m->x, m->y, m->w, m->h);

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
	unsigned int i, tagmask;

	if (arg.ui == ~0) tagmask = ~(curmon->seldesks);
	else tagmask = arg.ui;

	/* if this would leave nothing visible */
	if ((curmon->seldesks ^ tagmask) == 0) return;

	curmon->seldesks ^= tagmask;
	mapclients(curmon);

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

	loaddesktop(POSTOINT(arg.ui));
	curmon->seldesks = curmon->curdesk = arg.ui;

	mapclients(curmon);

	/* changecurrent(findcurrent(), curmon->curdesk) toggles off findcurrent() if not NULL */
	if ( !(curmon->current = findcurrent()) && (c = findvisclient(curmon->head, YesFloat)) )
		changecurrent(c, curmon, curmon->curdesk);

	justswitch = 1;
	curmon->curlayout->arrange(curmon);
	updatefocus();
}


/* ---------------------------------------
 * Backend
 * ---------------------------------------
 */


void cleandrw(){
	XftDrawDestroy(sdrw->xd);
	XFreeGC(dis, sdrw->gc);
	XftFontClose(dis, sdrw->xfont);
	XFreePixmap(dis, sdrw->d);
	free(sdrw);
}

/* Kill off any remaining clients
 * Free all the things
 */
void cleanup(){
	int i;
	const Arg arg = {.ui = ~0};
	monitor* m, * tm;

	toggleview(arg);
	for EACHMON(mhead){
		changemon(im, NoFocus);
		while (curmon->current)
			unmanage(curmon->current);
	}

	XUngrabKey(dis, AnyKey, AnyModifier, root);

	m = mhead;
	while (m){
		tm = m->next;
		cleanupmon(m);
		m = tm;
	}

	for (i=0;i < TABLENGTH(colors);i++) free(scheme[i]);
	free(scheme);
	XFree(&cursor);
	cleandrw();

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

/* dwm copypasta */
int getptrcoords(int* x, int* y){
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dis, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

/* dwm copypasta */
void grabbuttons(client* c, int focused){
	unsigned int i, j;
	unsigned int modifiers[] = { 0, LockMask };

	XUngrabButton(dis, AnyButton, AnyModifier, c->win);
	if (!focused)
		XGrabButton(dis, AnyButton, AnyModifier, c->win, False,
			BUTTONMASK, GrabModeSync, GrabModeSync, None, None);

	for (i = 0; i < TABLENGTH(buttons); i++)
		if (buttons[i].click == ClkWin)
			for (j = 0; j < TABLENGTH(modifiers); j++)
				XGrabButton(dis, buttons[i].btn,
					buttons[i].mask | modifiers[j],
					c->win, False, BUTTONMASK,
					GrabModeAsync, GrabModeSync, None, None);
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

	sdrw->w = sw; sdrw->h = sh;
	sdrw->d = XCreatePixmap(dis, root, sdrw->w, sdrw->h, DefaultDepth(dis, screen));
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

	cursor = XCreateFontCursor(dis, 68);

	scheme = ecalloc(TABLENGTH(colors), sizeof(XftColor*));
	for (i=0;i < TABLENGTH(colors);i++) scheme[i] = createscheme(colors[i], 2);

	running = 1;

	mhead = NULL;
	curmon = NULL;

	initdrw();
	updategeom();
	loaddesktop(0);

	wa.cursor = cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dis, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dis, root, wa.event_mask);

	grabkeys();
	updatestatus();
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

/* dwm copypasta of sendevent */
int xsendkill(Window w){
	int n, exists = 0;
	XEvent ev;
	Atom destproto = XInternAtom(dis, "WM_DELETE_WINDOW", False);
	Atom* protocols;

	if (XGetWMProtocols(dis, w, &protocols, &n)){
		while (!exists && n--)
			exists = (protocols[n] == destproto);
		XFree(protocols);
	}

	if (exists){
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = destproto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dis, w, False, NoEventMask, &ev);
	}

	return exists;
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
