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

typedef struct bar bar;
typedef struct client client;
typedef struct desktop desktop;
typedef struct monitor monitor;

struct key {
	unsigned int mod;
	KeySym keysym;
	void (*function)(const Arg arg);
	const Arg arg;
};

struct bar {
	int w, h;
	XftColor* scheme;
	Drawable d;
	XftFont* xfont;
	GC gc;
	Window win;
	XftDraw* xd;
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
	//int nofocus;
}; 

typedef struct {
	const char* symbol;
	void (*arrange)(monitor*);
} layout;

struct desktop {
	float msize;
	layout* curlayout;
};

typedef struct {
	const char* title;
	int deskmask;
	int isfloat;
} rule;

typedef struct {
	int x, y;
} point;

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

	if (!(p = calloc(nmemb,size))) die("ecalloc failed");

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
//static void configurenotify(XEvent* e);
static void configurerequest(XEvent* e);
static void destroynotify(XEvent* e);
static void enternotify(XEvent* e);
static void expose(XEvent* e);
static void keypress(XEvent* e);
static void maprequest(XEvent* e);
static void motionnotify(XEvent* e);
static void propertynotify(XEvent* e);
//static void unmapnotify(XEvent* e);

/* Client & Linked List Manipulation */
static void applyrules(client* c);
static void attachaside(client* c);
static void changecurrent(client* c, unsigned int deskmask);
static void detach(client* c);
static void killclient();
static void manage(Window parent, XWindowAttributes* wa);
static void mapclients();
static void moveclient(const Arg arg);
static void movefocus(const Arg arg);
static void raisefloats();
static void refocus(client* n, client* p);
static void resizeclient(client* c, int x, int y, int w, int h);
static void swapmaster();
static void todesktop(const Arg arg);
static void toggledesktop(const Arg arg);
static void togglefloat();
static void togglefs();
static void unmanage(client* c);
static void updatefocus();

/* Monitor Manipulation */
static void changemon(monitor* m, int focus);
static void createmon(monitor* m, int num, int x, int y, int w, int h);
static monitor* findmon(Window w);
static void focusmon(const Arg arg);
static void initmons();

/* Client Interfacing */
static client* findcurrent();
static client* findclient(Window w);
static client* findvisclient(client* c);
static client* findprevclient(client* c, int wantvis);
static void updateprev(client* c);

/* Bar */
static void drawbar(monitor* m);
static void drawbars();
static int drawbartext(monitor* m, int x, int y, int w, int h, unsigned int lpad, const char* text);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static int gettextwidth(monitor* m, const char* str, int len);
static bar* initbar(monitor* m);
static void updatestatus();

/* Desktop Interfacing */
static void changedesktop(const Arg arg);
static void changemsize(const Arg arg);
static void loaddesktop(int i);
static void monocle(monitor* m);
static void setlayout(const Arg arg);
static void tile(monitor* m);
static void toggleview(const Arg arg);
static void viewall();

/* Backend */
static void cleanup();
static void grabkeys();
static XftColor* scheme_create(const char* clrnames[], size_t clrcount);
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static int xerror(Display* dis, XErrorEvent* ee);
static void xsendkill(Window w);
static void youviolatedmymother();


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
static client* preventer;
static int justswitch;

/* Monitor Interfacing */
static monitor* mhead;
static monitor* curmon;

/* Backend */
static client* ic; /* for EACHCLIENT iterating */
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
	//[UnmapNotify] = unmapnotify
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

	if ( (m = findmon(ev->window)) && m != curmon)
		changemon(m, 0);

	if ( (c = findclient(ev->window)) ){
		updateprev(c);
		changecurrent(c, curmon->curdesk);
		updatefocus();
		XAllowEvents(dis, ReplayPointer, CurrentTime);
	}
}

/* dwm copypasta */
//void configurenotify(XEvent* e){
//	XConfigureEvent* ev = &e->xconfigure;
//
//	if (ev->window == root){
//		curmon->bar->w = ev->width;
//		XFreePixmap(dis, curmon->bar->d);
//		curmon->bar->d = XCreatePixmap(dis, root, curmon->bar->w,
//				curmon->bar->h, DefaultDepth(dis,screen));
//
//		XMoveResizeWindow(dis, curmon->bar->win, 0, 0, curmon->bar->w,
//				curmon->bar->h);
//		drawbar(curmon);
//	}
//}

/* dwm copypasta */
void configurerequest(XEvent* e){
	client* c;
	monitor* m;
	XConfigureRequestEvent* ev = &e->xconfigurerequest;

	if ((c = findclient(ev->window))){
		m = findmon(c->win);
		if (ev->value_mask & CWX) c->x = m->x + ev->x;
		if (ev->value_mask & CWY) c->y = (ev->y < m->bar->h) ? m->bar->h : ev->y;
		if (ev->value_mask & CWWidth) c->w = ev->width;
		if (ev->value_mask & CWHeight) c->h = ev->height;
		if ISVISIBLE(c) XMoveResizeWindow(dis, c->win, c->x, c->y, c->w, c->h);
	}

	XSync(dis, False);
}

void destroynotify(XEvent* e){
	client* c;
	XDestroyWindowEvent* ev = &e->xdestroywindow;

	if ( (c = findclient(ev->window)) ){
		unmanage(c);
		drawbars();
	}
}

void enternotify(XEvent* e){
	client* c;
	monitor* m;
	XCrossingEvent* ev = &e->xcrossing;

	if ( (ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root )
		return;

	if ( !(c = findclient(ev->window)) || justswitch ){
		justswitch = 0;
		return;
	}

	/* if we haven't moved from the confines of the window */
	if (preventer && !ISOUTSIDE(ev->x_root, ev->y_root, preventer->x,
				preventer->y, preventer->w, preventer->h))
		return;

	if ( (m = findmon(ev->window)) && m != curmon )
		changemon(m, 0);

	updateprev(c);
	changecurrent(c, curmon->curdesk);
	updatefocus();
}

/* dwm copypasta */
void expose(XEvent* e){
	XExposeEvent* ev = &e->xexpose;

	if (ev->count == 0 && findclient(ev->window))
		drawbars();
}

void keypress(XEvent* e){
	int i;
	XKeyEvent ke = e->xkey;
	KeySym keysym = XKeycodeToKeysym(dis, ke.keycode, 0);

	for (i=0;i<TABLENGTH(keys);i++)
		if (keys[i].keysym == keysym && keys[i].mod == ke.state)
			keys[i].function(keys[i].arg);
}

void maprequest(XEvent* e){
	XWindowAttributes wa;
	XMapRequestEvent* ev = &e->xmaprequest;

	if ( XGetWindowAttributes(dis, ev->window, &wa) && !wa.override_redirect && !findclient(ev->window) ){
		manage(ev->window, &wa);
		curmon->curlayout->arrange(curmon);
		updatefocus();
	}
}

void motionnotify(XEvent* e){
	monitor* m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;

	for (m=mhead;m;m=m->next){
		if (m != curmon && !ISOUTSIDE(ev->x_root, ev->y_root, m->x, m->y, m->w, m->h)){
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
			if (r->deskmask > 0) c->desks = 1 << r->deskmask;
			c->isfloat = r->isfloat;
		}
	}

	/* need to free something, but idk what or how, because everything accidentallys */
	//XFree(&tp.value);
}	

void attachaside(client* c){
	client* l;

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
	
	for EACHCLIENT(curmon->head) if ( ic != c && (ic->iscur & deskmask) ){
			ic->iscur ^= deskmask;
			XGrabButton(dis, Button1, 0, ic->win, False, ButtonPressMask,
					GrabModeAsync, GrabModeSync, None, None);
		}

	curmon->current = c;
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
		curmon->head = c->next;
}

void killclient(){
	Window w;

	if (curmon->current){
		w = curmon->current->win;
		unmanage(curmon->current);
		xsendkill(w);
	}

	updatestatus();
}

void manage(Window parent, XWindowAttributes* wa){
	client* c;

	if ( !(c = ecalloc(1, sizeof(client))) )
		die("Error while callocing new client!");

	c->win = parent;
	c->isfloat = c->isfull = c->iscur = 0;
	c->desks = curmon->seldesks;

	c->x = wa->x; c->y = wa->y;
	c->w = wa->width; c->h = wa->height;

	if (!(preventer->win) || !findclient(preventer->win))
		updateprev(c);

	applyrules(c);
	attachaside(c);
	/* if (c->nofocus){
	 * 	hideclient(c);
	 *	refocus(curmon->current->next, curmon->current);
	 * }
	 * curmon->curlayout->arrange(curmon);
	 */
	XMapWindow(dis, c->win);
	drawbars();
}

void mapclients(){
	for EACHCLIENT(curmon->head) if ISVISIBLE(ic) XMapWindow(dis, ic->win); else XUnmapWindow(dis, ic->win);
}

void moveclient(const Arg arg){
	client* p, * mp, * n;

	if (!curmon->current || curmon->current->isfull)
		return;

	p = findprevclient(curmon->current, NoVis);

	/* Up stack if not head */
	if (arg.i > 0 && curmon->current != curmon->head){
		if (p == curmon->head){
			swapmaster();

		} else {
			mp = findprevclient(p, NoVis);

			mp->next = curmon->current;
			p->next = curmon->current->next;
			curmon->current->next = p;
		}

	/* Down stack if not tail */
	} else if (arg.i < 0 && curmon->current->next){
		n = curmon->current->next;
		curmon->current->next = n->next;
		n->next = curmon->current;

		if (curmon->current == curmon->head)
			curmon->head = n;
		else
			p->next = n;
	}

	curmon->curlayout->arrange(curmon);
	updatefocus();
	drawbars();
}

/* some dwm copypasta */
void movefocus(const Arg arg){
	client* j, * c = NULL;

	if ( !curmon->current || curmon->current->isfull )
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

	updateprev(c);
	changecurrent(c, curmon->curdesk);
	updatefocus();
	drawbars();
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
	vis = (vis = findvisclient(n)) ? vis : findprevclient(p, YesVis);
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

void swapmaster(){
	client* tmp;
	client* p;

	if (curmon->head && curmon->current && curmon->current != curmon->head){
		tmp = (curmon->head->next == curmon->current) ? curmon->head : curmon->head->next;
		p = findprevclient(curmon->current, NoVis);

		/* if p is head, this gets overwritten - saves an if statement */
		p->next = curmon->head;
		curmon->head->next = curmon->current->next;
		curmon->current->next = tmp;
		curmon->head = curmon->current;

		curmon->curlayout->arrange(curmon);
		updatefocus();
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
	updatestatus();
}

void toggledesktop(const Arg arg){
	unsigned int newdesks;

	if (!curmon->current) return;

	newdesks = curmon->current->desks ^ 1 << arg.i;
	if (newdesks){
		curmon->current->desks = newdesks;
		curmon->current->iscur = 0;
		changecurrent(curmon->current, 1 << arg.i);

		if ( !(ISVISIBLE(curmon->current)) ){
			XUnmapWindow(dis, curmon->current->win);
			XSync(dis, False);
			refocus(curmon->current->next, curmon->current);
		}

		curmon->curlayout->arrange(curmon);
		updatefocus();
		updatestatus();
	}
}

void togglefloat(){
	XWindowChanges wc;

	if (!curmon->current || curmon->current->isfull) return;

	curmon->current->isfloat = !curmon->current->isfloat;
	curmon->curlayout->arrange(curmon);

	if (curmon->current->isfloat){
		wc.sibling = curmon->current->win;
		wc.stack_mode = Below;

		for EACHCLIENT(curmon->head) XConfigureWindow(dis, ic->win, CWSibling|CWStackMode, &wc);
	}

	if (!curmon->current->isfloat){
		wc.sibling = curmon->bar->win;
		wc.stack_mode = Below;
		XConfigureWindow(dis, curmon->current->win, CWSibling|CWStackMode, &wc);
	}
}

void togglefs(){
	if (!curmon->current) return;

	curmon->current->isfull = !curmon->current->isfull;
	/* a pecularity of my implementation - will remain as such unless I decide to implement oldx, oldy, etc. for clients */
	curmon->current->isfloat = 0;

	if (curmon->current->isfull){
		resizeclient(curmon->current, curmon->x, 0, curmon->w, curmon->h + curmon->bar->h);
		XRaiseWindow(dis, curmon->current->win);

		XUnmapWindow(dis, curmon->bar->win);

	} else {
		curmon->curlayout->arrange(curmon);

		XMapRaised(dis, curmon->bar->win);
		drawbars();
	}
}

/* TODO: Change monitors if necessary */
void unmanage(client* c){
	detach(c);
	if (c) free(c);
	curmon->curlayout->arrange(curmon);
	updatefocus();
}

/* TODO: Weirdness when killing last client on another monitor */
void updatefocus(){
	if (curmon->current){
		XSetInputFocus(dis, curmon->current->win, RevertToPointerRoot, CurrentTime);
		XRaiseWindow(dis, curmon->current->win);

	} else {
		XSetInputFocus(dis, root, RevertToPointerRoot, CurrentTime);
	}
}


/* ---------------------------------------
 * Monitors
 * ---------------------------------------
 */

void changemon(monitor* m, int focus){
	justswitch = 0;
	curmon = m;
	if (focus) updatefocus();
}

void createmon(monitor* m, int num, int x, int y, int w, int h){
	int i;

	m->num = num;
	m->x = x; m->y = y;
	m->w = w; m->h = h;

	m->bar = initbar(m);

	m->y = y + m->bar->h;
	m->h = h - m->bar->h;

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
}

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

void focusmon(const Arg arg){
	monitor* m = NULL;

	if (arg.i > 0){
		if (curmon->next) m = curmon->next;

	} else {
		for (m=mhead;m && m->next != curmon;m=m->next);
	}

	if (m){
		changemon(m, 1);
	}
}

/* TODO: Is this required for mirroring displays to work? */
/* dwm copypasta */
#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo* unique, size_t n, XineramaScreenInfo* info){
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
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
		monitor* im, * tmp;

		XineramaScreenInfo* info = XineramaQueryScreens(dis, &ns);
//		XineramaScreenInfo* unique;

		/* only consider unique geometries as separate screens */
//		unique = ecalloc(ns, sizeof(XineramaScreenInfo));
//		for (i = 0, j = 0; i < ns; i++)
//			if (isuniquegeom(unique, j, &info[i]))
//				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
//		XFree(info);
		
		for (i=0;i < ns;i++){
			m = ecalloc(1, sizeof(monitor));
			createmon(m, i, info[i].x_org, info[i].y_org, info[i].width, info[i].height);
	
			if (!mhead){
				mhead = m;
	
			/* works for me because I only use 2 monitors */
			} else if (m->x < mhead->x){
				tmp = mhead;
				mhead = m;
				mhead->next = tmp;

			} else {
				for (im=mhead;im->next;im=im->next);
				im->next = m;
			}
		}

		changemon(mhead, 0);
//		free(unique);
		XFree(info);
	}
#endif
	if (!mhead){
		m = ecalloc(1, sizeof(monitor));
		mhead = m;
		createmon(m, 0, 0, 0, sw, sh);
		changemon(m, 0);
	}
}


/* ---------------------------------------
 * Client Interfacing
 * ---------------------------------------
 */

client* findcurrent(){
	for EACHCLIENT(curmon->head) if ( ISVISIBLE(ic) && (ic->iscur & curmon->curdesk) ){
			return ic;
		}

	return NULL;
}

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

client* findprevclient(client* c, int wantvis){
	client* i, * ret = NULL;
	for (i=curmon->head;i && i != c;i=i->next){
		if (wantvis)
			if ISVISIBLE(i) ret = i;

		if (i->next == c){
			if (wantvis)
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
	preventer->win = c->win;
	preventer->x = c->x; preventer->y = c->y;
	preventer->w = c->w; preventer->h = c->h;
}


/* ---------------------------------------
 * Bar
 * ---------------------------------------
 */

/* part dwm copypasta */
void drawbar(monitor* m){
	int j, x = 2, xsetr_text_w = 0, is_sel; /* 2px left padding */
	unsigned int occ = 0;

	/* draw background */
	m->bar->scheme = scheme[SchNorm];
	XSetForeground(dis, m->bar->gc, m->bar->scheme[ColBg].pixel);
	XFillRectangle(dis, m->bar->d, m->bar->gc, 0, 0, sw, m->bar->h);

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

		m->bar->scheme = scheme[is_sel ? SchSel : SchNorm];

		x = drawbartext(m, x, 0, TEXTW(m, syms[SymLeft]) + 1, m->bar->h, 0,
				is_sel ? syms[SymLeft] : " ") - lrpad;
		x = drawbartext(m, x, 0, TEXTW(m, tags[j]), m->bar->h, 0, tags[j]) - lrpad + 2;
		x = drawbartext(m, x, 0, TEXTW(m, syms[SymRight]), m->bar->h, 0,
				is_sel ? syms[SymRight] : " ") - lrpad / 2;
	}
	x -= lrpad / 2;
	m->bar->scheme = scheme[SchNorm];
	drawbartext(m, x, 0, TEXTW(m, m->curlayout->symbol), m->bar->h, lrpad / 2, m->curlayout->symbol);

	XCopyArea(dis, m->bar->d, m->bar->win, m->bar->gc, 0, 0, m->bar->w, m->bar->h, 0, 0);
	XSync(dis, False);
}

void drawbars(){
	monitor* im;
	for (im=mhead;im;im=im->next) drawbar(im);
}

int drawbartext(monitor* m, int x, int y, int w, int h, unsigned int lpad, const char* text){
	int ty = y + (h - (m->bar->xfont->ascent + m->bar->xfont->descent)) / 2 + m->bar->xfont->ascent;
	XftDrawString8(m->bar->xd, &m->bar->scheme[ColFg], m->bar->xfont, x + lpad, ty, (XftChar8*)text, slen(text));

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
	XftTextExtents8(dis, m->bar->xfont, (XftChar8*)str, len, &xgi);

	return xgi.width;
}

bar* initbar(monitor* m){
	bar* sbar;

	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ExposureMask
	};

	sbar = ecalloc(1, sizeof(bar));
	if ( !(sbar->xfont = XftFontOpenName(dis, screen, fontname)) )
		die("The font you tried to use was not found. Check the name.");

	lrpad = sbar->xfont->ascent + sbar->xfont->descent;

	sbar->h = sbar->xfont->ascent + sbar->xfont->descent + 2;
	sbar->w = m->w;
	sbar->d = XCreatePixmap(dis, root, m->w, m->h, DefaultDepth(dis, screen));
	sbar->gc = XCreateGC(dis, sbar->d, 0, NULL);
	sbar->xd = XftDrawCreate(dis, sbar->d, DefaultVisual(dis,screen), DefaultColormap(dis,screen));

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

void changedesktop(const Arg arg){
	client* c;

	loaddesktop(arg.i);
	curmon->seldesks = curmon->curdesk = 1 << arg.i;

	mapclients();

	if ( (c = findcurrent()) || (c = findvisclient(curmon->head)) )
		changecurrent(c, curmon->curdesk);

	curmon->curlayout->arrange(curmon);
	justswitch = 1;
	updatefocus();
	updatestatus();
}

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
	updatestatus();
}

void viewall(){
	int i;
	curmon->seldesks = 0;
	for (i=0;i < TABLENGTH(tags);i++) curmon->seldesks ^= 1 << i;

	mapclients();
	raisefloats();

	curmon->curlayout->arrange(curmon);
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
	monitor* m, * tmp;
	client* tc;
	Window w;

	XUngrabKey(dis, AnyKey, AnyModifier, root);

	m = mhead;
	while (m){
		while (m->head){
			w = m->head->win;
			tc = m->head->next;

			free(m->head);

			m->head = tc;
			xsendkill(w);
		}

		free(m->desks);

		XUnmapWindow(dis, m->bar->win);
		XDestroyWindow(dis, m->bar->win);
		XftDrawDestroy(m->bar->xd);
		XFreeGC(dis, m->bar->gc);
		XftFontClose(dis, m->bar->xfont);
		XFreePixmap(dis, m->bar->d);

		free(m->bar);

		tmp = m->next;
		free(m);
		m = tmp;
		changemon(m, 0);
	}

	free(preventer);

	for (i=0;i < TABLENGTH(colors);i++) free(scheme[i]);
	free(scheme);

	fprintf(stdout, "sara: Thanks for using!\n");
	XDestroySubwindows(dis, root);

	XSync(dis, False);
	XSetInputFocus(dis, PointerRoot, RevertToPointerRoot, CurrentTime);

        XCloseDisplay(dis);
}

void grabkeys(){
	int i;
	KeyCode code;

	for (i=0;i < TABLENGTH(keys);i++){
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

	scheme = ecalloc(TABLENGTH(colors), sizeof(XftColor*));
	for (i=0;i < TABLENGTH(colors);i++) scheme[i] = scheme_create(colors[i], 2);

	running = 1;

	mhead = NULL;
	curmon = NULL;
	preventer = ecalloc(1, sizeof(client));

	initmons();
	loaddesktop(0);

	wa.cursor = XCreateFontCursor(dis, 68);
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dis, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dis, root, wa.event_mask);

	grabkeys();
//	drawbar();
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

void xsendkill(Window w){
	XEvent ev;

	ev.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(dis, w, False, NoEventMask, &ev);
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
