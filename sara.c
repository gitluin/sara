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

#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define TABLENGTH(X)    		(sizeof(X)/sizeof(*X))
#define ISVISIBLE(C)			((C->desktops & curmon->seldesks))
#define TEXTW(M,X)			(gettextwidth(M, X, slen(X)) + lrpad)
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

typedef struct monitor monitor;
struct monitor {
	/* monitor */
	int x, y, h, w;
	int num;
	bar* bar;
	monitor* next;
	/* desktops */
	float master_size;
	unsigned int seldesks;
	unsigned int curdesk;
	client* current;
	client* head;
	client* prev_enter;
	desktop* desktops;
	layout* current_layout;
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

/* Monitor Manipulation */
static void changemon(monitor* m, int focus);
static void createmon(monitor* m, int num, int x, int y, int w, int h);
static monitor* findmon(Window w);
static void focusmon(const Arg arg);
static void initmons();
static void setcurrentmon(monitor* m);

/* Client Interfacing */
static client* findcurrent();
static client* findclient();
static client* findvisclient(client* c);
static client* findprevclient(client* c, int is_vis);
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
static void monocle();
static void setlayout(const Arg arg);
static void tile();
static void toggleview(const Arg arg);
static void viewall();

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
static client* prev_enter;
static int just_switched;

/* Backend */
static client* ic; /* for EACHCLIENT iterating */
static int lrpad;
static int running;
static XftColor** scheme;
static char xsetr_text[256];

/* Monitor Interfacing */
static monitor* mhead;
static monitor* curmon;

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
	[PropertyNotify] = propertynotify,
	//[UnmapNotify] = unmapnotify
};


/* ---------------------------------------
 * X Event Processing
 * ---------------------------------------
 */

/* dwm copypasta */
void buttonpress(XEvent* e){
	client *c;
	monitor* m;
	XButtonPressedEvent* ev = &e->xbutton;

	/* focus monitor if necessary */
	if ( (m = findmon(ev->window)) && m != curmon )
		changemon(m, 1);

	if ( (c = findclient(ev->window)) ){
		updateprev(c);
		changecurrent(c, curmon->curdesk);
		updatefocus();
//		XAllowEvents(dis, ReplayPointer, CurrentTime);
	}
}

/* dwm copypasta */
//void configurenotify(XEvent* e){
//	XConfigureEvent* ev = &e->xconfigure;
//
//	if (ev->window == root){
//		curmon->bar->width = ev->width;
//		XFreePixmap(dis, curmon->bar->d);
//		curmon->bar->d = XCreatePixmap(dis, root, curmon->bar->width,
//				curmon->bar->height, DefaultDepth(dis,screen));
//
//		XMoveResizeWindow(dis, curmon->bar->win, 0, 0, curmon->bar->width,
//				curmon->bar->height);
//		drawbar(curmon);
//	}
//}

/* dwm copypasta */
void configurerequest(XEvent* e){
	client* c;
	monitor* m;
	XConfigureRequestEvent* ev = &e->xconfigurerequest;

	/* only honor requests if they should be honored */
	if ( (c = findclient(ev->window)) && c->is_float){
		m = findmon(c->win);
		if (ev->value_mask & CWX) c->x = m->x + ev->x;
		if (ev->value_mask & CWY) c->y = (ev->y < m->bar->height) ? m->bar->height : ev->y;
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
	XCrossingEvent* ev = &e->xcrossing;

	if ( (ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root )
		return;

	if ( !(c = findclient(ev->window)) || just_switched ){
		just_switched = 0;
		return;
	}

	/* if we haven't moved, and the window is the same */
	if (prev_enter && !ISOUTSIDE(getpointcoords(0), getpointcoords(1), prev_enter->x,
				prev_enter->y, prev_enter->w, prev_enter->h))
			//&& prev_enter->win == c->win)
		return;

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
		curmon->current_layout->arrange();
		updatefocus();
	}
}

//void motionnotify(XEvent* e){
//	Monitor *m;
//	XMotionEvent *ev = &e->xmotion;
//
//	if (ev->window != root)
//		return;
//
//	if ( (m = recttomon(ev->x_root, ev->y_root, 1, 1)) && m != curmon)
//		changemon(m, 1);
//}

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

void changecurrent(client* c, unsigned int desktopmask){
	if (c){
		c->is_current ^= desktopmask;
		XUngrabButton(dis, Button1, AnyModifier, c->win);
	}
	
	for EACHCLIENT(curmon->head) if ( ic != c && (ic->is_current & desktopmask) ){
			ic->is_current ^= desktopmask;
			XGrabButton(dis, Button1, 0, ic->win, False, BUTTONMASK,
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
	XEvent ev;

	if (curmon->current){
		w = curmon->current->win;
		unmanage(curmon->current);

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
	c->desktops = curmon->curdesk;

	c->x = wa->x; c->y = wa->y;
	c->w = wa->width; c->h = wa->height;

	if (!(prev_enter->win) || !findclient(prev_enter->win))
		updateprev(c);

	applyrules(c);
	attachaside(c);
	/* if (c->no_focus){
	 * 	hide_client(c);
	 *	refocus(curmon->current->next, curmon->current);
	 * }
	 * curmon->current_layout->arrange();
	 */
	XMapWindow(dis, c->win);
	drawbars();
}

void mapclients(){
	for EACHCLIENT(curmon->head) if ISVISIBLE(ic) XMapWindow(dis, ic->win); else XUnmapWindow(dis, ic->win);
}

void moveclient(const Arg arg){
	client* p, * mp, * n;

	if (!curmon->current || curmon->current->is_full)
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

	curmon->current_layout->arrange();
	updatefocus();
	drawbars();
}

/* some dwm copypasta */
void movefocus(const Arg arg){
	client* j, * c = NULL;

	if ( !curmon->current || curmon->current->is_full )
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
	for EACHCLIENT(curmon->head) if (ISVISIBLE(ic) && ic->is_float){
			XMoveResizeWindow(dis, ic->win, ic->x, ic->y, ic->w, ic->h);
			XRaiseWindow(dis, ic->win);
		}
}

/* focus moves down if possible, else up */
void refocus(client* n, client* p){
	client* vis;
	vis = (vis = findvisclient(n)) ? vis : findprevclient(p,YesVis);
	changecurrent(vis, curmon->curdesk);
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

		curmon->current_layout->arrange();
		updatefocus();
	}
}

void todesktop(const Arg arg){
	if (curmon->curdesk & 1 << arg.i) return;

	curmon->current->desktops = 1 << arg.i;
	curmon->current->is_current = 0;
	changecurrent(curmon->current, 1 << arg.i);

	XUnmapWindow(dis, curmon->current->win);
	XSync(dis, False);

	refocus(curmon->current->next, curmon->current);
	curmon->current_layout->arrange();
	updatefocus();
	updatestatus();
}

void toggledesktop(const Arg arg){
	unsigned int new_desktops;

	if (!curmon->current) return;

	new_desktops = curmon->current->desktops ^ 1 << arg.i;
	if (new_desktops){
		curmon->current->desktops = new_desktops;
		curmon->current->is_current = 0;
		changecurrent(curmon->current, 1 << arg.i);

		if ( !(ISVISIBLE(curmon->current)) ){
			XUnmapWindow(dis, curmon->current->win);
			XSync(dis, False);
			refocus(curmon->current->next, curmon->current);
		}

		curmon->current_layout->arrange();
		updatefocus();
		updatestatus();
	}
}

void togglefloat(){
	XWindowChanges wc;

	if (!curmon->current || curmon->current->is_full) return;

	curmon->current->is_float = !curmon->current->is_float;
	curmon->current_layout->arrange();

	if (curmon->current->is_float){
		wc.sibling = curmon->current->win;
		wc.stack_mode = Below;

		for EACHCLIENT(curmon->head) XConfigureWindow(dis, ic->win, CWSibling|CWStackMode, &wc);
	}

	if (!curmon->current->is_float){
		wc.sibling = curmon->bar->win;
		wc.stack_mode = Below;
		XConfigureWindow(dis, curmon->current->win, CWSibling|CWStackMode, &wc);
	}
}

void togglefs(){
	if (!curmon->current) return;

	curmon->current->is_full = !curmon->current->is_full;
	/* a pecularity of my implementation - will remain as such unless I decide to implement oldx, oldy, etc. for clients */
	curmon->current->is_float = 0;

	if (curmon->current->is_full){
		curmon->current->x = curmon->x;
		curmon->current->y = 0;
		curmon->current->w = curmon->w;
		curmon->current->h = curmon->h + curmon->bar->height;
		XMoveResizeWindow(dis, curmon->current->win, curmon->current->x, curmon->current->y,
				curmon->current->w, curmon->current->h);
		XRaiseWindow(dis, curmon->current->win);

		XUnmapWindow(dis, curmon->bar->win);

	} else {
		curmon->current_layout->arrange();

		XMapRaised(dis, curmon->bar->win);
		drawbars();
	}
}

/* TODO: Change monitors if necessary */
void unmanage(client* c){
	detach(c);
	free(c);
	curmon->current_layout->arrange();
	updatefocus();
}

/* TODO: Weirdness when killing last client on another monitor */
void updatefocus(){
	for EACHCLIENT(curmon->head) if (ic == curmon->current){
			XSetInputFocus(dis, ic->win, RevertToPointerRoot, CurrentTime);
			XRaiseWindow(dis, ic->win);
		}
}


/* ---------------------------------------
 * Monitors
 * ---------------------------------------
 */

void changemon(monitor* m, int focus){
	/* save current */
	curmon->prev_enter = prev_enter;

	/* select m */
	setcurrentmon(m);

	if (focus) updatefocus();
	// this causes a crash
	//updatestatus();
}

void createmon(monitor* m, int num, int x, int y, int w, int h){
	int i;

	m->num = num;
	m->x = x; m->y = y;
	m->w = w; m->h = h;

	m->bar = initbar(m);

	m->y = y + m->bar->height;
	m->h = h - m->bar->height;

	/* Default to first layout */
	m->current_layout = (layout*) &layouts[0];
	m->master_size = m->w * MASTER_SIZE;

	m->desktops = ecalloc(TABLENGTH(tags), sizeof(desktop));
	for (i=0;i < TABLENGTH(tags);i++){
		m->desktops[i].current_layout = m->current_layout;
		m->desktops[i].master_size = m->master_size;
	}

	/* Default to first desktop */
	m->seldesks = m->curdesk = 1 << 0;

	m->head = NULL;
	m->current = NULL;
	m->prev_enter = prev_enter;
}

monitor* findmon(Window w){
	monitor* im, * old_monitor = curmon;

	for (im=mhead;im;im=im->next){
		changemon(im, 0);
		for EACHCLIENT(curmon->head){
			if (ic->win == w){
				changemon(old_monitor, 1);
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

	if (m) changemon(m, 1);
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

		setcurrentmon(mhead);
//		free(unique);
		XFree(info);
	}
#endif
	if (!mhead){
		m = ecalloc(1, sizeof(monitor));
		mhead = m;
		createmon(m, 0, 0, 0, sw, sh);
		setcurrentmon(m);
	}
}

void setcurrentmon(monitor* m){
	prev_enter = m->prev_enter;
	just_switched = 0;
	
	curmon = m;
}


/* ---------------------------------------
 * Client Interfacing
 * ---------------------------------------
 */

client* findcurrent(){
	for EACHCLIENT(curmon->head) if ( ISVISIBLE(ic) && (ic->is_current & curmon->curdesk) ){
			return ic;
		}

	return NULL;
}

client* findclient(Window w){
	monitor* im, * old_monitor = curmon;

	for (im=mhead;im;im=im->next){
		changemon(im, 0);
		for EACHCLIENT(curmon->head){
			if (ic->win == w){
				changemon(old_monitor, 1);
				return ic;
			}
		}
	}
	changemon(old_monitor, 1);

	return NULL;
}

client* findprevclient(client* c, int is_vis){
	client* i, * ret = NULL;
	for (i=curmon->head;i && i != c;i=i->next){
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
void drawbar(monitor* m){
	int j, x = 2, xsetr_text_w = 0, is_sel; /* 2px left padding */
	unsigned int occ = 0;

	/* draw background */
	m->bar->scheme = scheme[SchNorm];
	XSetForeground(dis, m->bar->gc, m->bar->scheme[ColBg].pixel);
	XFillRectangle(dis, m->bar->d, m->bar->gc, 0, 0, sw, m->bar->height);

	/* draw status */
	xsetr_text_w = TEXTW(m, xsetr_text) - lrpad + 2; /* 2px right padding */
	drawbartext(m, m->bar->width - xsetr_text_w, 0, xsetr_text_w, m->bar->height, 0, xsetr_text);

	for EACHCLIENT(m->head) occ |= ic->desktops;

	/* draw tags */
	for (j=0;j<TABLENGTH(tags);j++){
		/* do not draw vacant tags, but do draw selected tags regardless */
		is_sel = m->seldesks & 1 << j;
		if ( !(occ & 1 << j) && !is_sel )
			continue;

		m->bar->scheme = scheme[is_sel ? SchSel : SchNorm];

		x = drawbartext(m, x, 0, TEXTW(m, syms[SymLeft]) + 1, m->bar->height, 0,
				is_sel ? syms[SymLeft] : " ") - lrpad;
		x = drawbartext(m, x, 0, TEXTW(m, tags[j]), m->bar->height, 0, tags[j]) - lrpad + 2;
		x = drawbartext(m, x, 0, TEXTW(m, syms[SymRight]), m->bar->height, 0,
				is_sel ? syms[SymRight] : " ") - lrpad / 2;
	}
	x -= lrpad / 2;
	m->bar->scheme = scheme[SchNorm];
	drawbartext(m, x, 0, TEXTW(m, curmon->current_layout->symbol), m->bar->height, lrpad / 2, curmon->current_layout->symbol);

	XCopyArea(dis, m->bar->d, m->bar->win, m->bar->gc, 0, 0, m->bar->width, m->bar->height, 0, 0);
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

	sbar->height = sbar->xfont->ascent + sbar->xfont->descent + 2;
	sbar->width = m->w;
	sbar->d = XCreatePixmap(dis, root, m->w, m->h, DefaultDepth(dis, screen));
	sbar->gc = XCreateGC(dis, sbar->d, 0, NULL);
	sbar->xd = XftDrawCreate(dis, sbar->d, DefaultVisual(dis,screen), DefaultColormap(dis,screen));

	sbar->win = XCreateWindow(dis, root, m->x, m->y, sbar->width, sbar->height, 0, 
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

//	if (curmon->curdesk & 1 << arg.i) return;

	loaddesktop(arg.i);
	curmon->seldesks = curmon->curdesk = 1 << arg.i;

	mapclients();

	/* why is this not changecurrent? */
	if ( !(curmon->current = findcurrent()) && (c = findvisclient(curmon->head)) )
		curmon->current = c;

	curmon->current_layout->arrange();
	just_switched = 1;
	updatefocus();
	updatestatus();
}

void changemsize(const Arg arg){
	int mw = curmon->w;
	curmon->master_size += ( ((curmon->master_size < 0.95 * mw) && (arg.f > 0))
			|| ((curmon->master_size > 0.05 * mw) && (arg.f < 0))  ) ? arg.f * mw : 0;

	curmon->current_layout->arrange();
}

void loaddesktop(int i){
	curmon->desktops[POSTOINT(curmon->curdesk)].master_size = curmon->master_size;
	curmon->desktops[POSTOINT(curmon->curdesk)].current_layout = curmon->current_layout;

	curmon->master_size = curmon->desktops[i].master_size;
	curmon->current_layout = curmon->desktops[i].current_layout;
}

void monocle(){
	//int mh = sh - sbar->height;
	int mh = curmon->h;
	int mw = curmon->w;
	int y = curmon->y;

	for EACHCLIENT(curmon->head) if (ISVISIBLE(ic) && !ic->is_float){
			ic->x = 0; ic->y = y;
			ic->w = mw; ic->h = mh;
			XMoveResizeWindow(dis, ic->win, ic->x, ic->y, ic->w, ic->h);
		} 

	raisefloats();
}

void setlayout(const Arg arg){
	curmon->current_layout = (layout*) arg.v;
	curmon->current_layout->arrange();
	drawbars();
}

void tile(){
	client* nf = NULL;
	int n = 0;
	//int mh = sh - sbar->height;
	int mh = curmon->h;
	int mw = curmon->w;
	int y = curmon->y;
	int x = curmon->x;
	//int y = sbar->height;

	/* Find the first non-floating, visible window and tally non-floating, visible windows */
	for EACHCLIENT(curmon->head) if (!ic->is_float && ISVISIBLE(ic)){
			nf = (!nf) ? ic : nf;
			n++;
		}

	if (nf && n == 1){
		nf->x = x; nf->y = y;
		nf->w = mw; nf->h = mh;
		XMoveResizeWindow(dis, nf->win, nf->x, nf->y, nf->w, nf->h);

	} else if (nf){
		/* so having a master doesn't affect stack splitting */
		n--;

		/* Master window */
		nf->x = x; nf->y = y;
		nf->w = curmon->master_size; nf->h = mh;
		XMoveResizeWindow(dis, nf->win, nf->x, nf->y, nf->w, nf->h);

		/* Stack */
		for EACHCLIENT(nf->next){
			if (ISVISIBLE(ic) && !ic->is_float && !ic->is_full){
				ic->x = x + curmon->master_size; ic->y = y;
				ic->w = mw - curmon->master_size; ic->h = mh / n;
				XMoveResizeWindow(dis, ic->win, ic->x, ic->y, ic->w, ic->h);

				y += mh / n;
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
				curmon->curdesk = curmon->seldesks;
				break;
			}
		}
	}

	curmon->current_layout->arrange();
	updatefocus();
	updatestatus();
}

void viewall(){
	int i;
	curmon->seldesks = 0;
	for (i=0;i < TABLENGTH(tags);i++) curmon->seldesks ^= 1 << i;

	mapclients();
	raisefloats();

	curmon->current_layout->arrange();
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

	m = mhead;
	while (m){
		while (curmon->current) killclient();

		free(m->desktops);
		XftDrawDestroy(m->bar->xd);
		XFreeGC(dis, m->bar->gc);
		XftFontClose(dis, m->bar->xfont);
		XFreePixmap(dis, m->bar->d);
		XUnmapWindow(dis, m->bar->win);
		XDestroyWindow(dis, m->bar->win);
		free(m->bar);

		tmp = m->next;
		free(m);
		m = tmp;
		setcurrentmon(m);
	}

	/* This ain't pretty, but it gets the job done (Without memory leak? Not sure) */
//	XUnmapWindow(dis, sbar->win);
//	XDestroyWindow(dis, sbar->win);
	XUngrabKey(dis, AnyKey, AnyModifier, root);

//	XftDrawDestroy(sbar->xd);
//	XFreeGC(dis, sbar->gc);
//	XftFontClose(dis, sbar->xfont);
//	XFreePixmap(dis, sbar->d);
//	free(sbar);

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
	prev_enter = ecalloc(1, sizeof(client));

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
