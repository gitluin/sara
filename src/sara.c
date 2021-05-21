/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

/* general */
#include <stdio.h>
#include <stdlib.h>
/* sockets */
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
/* Xlib */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/shape.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "common.h"


#define SAFEPARG(A,B)			((A <= parg.i && parg.i <= B))
#define BUTTONMASK              	(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK               	(BUTTONMASK|PointerMotionMask)
#define EACHCLIENT(I)			(ic=I;ic;ic=ic->next) /* ic is a global */
#define EACHMON(M)			(im=M;im;im=im->next) /* im is a global */
#define ISOUTSIDE(PX,PY,X,Y,W,H)	((PX > X + W || PX < X || PY > Y + H || PY < Y))
#define ISVISIBLE(C)			((C->desks & C->mon->seldesks))
#define MAX(A,B)               		((A) > (B) ? (A) : (B))
#define STREQ(A,B)			((strcmp(A,B) == 0))
#define TABLENGTH(X)    		(sizeof(X)/sizeof(*X))


enum { AnyVis,     OnlyVis };
enum { NoZoom,     YesZoom };
enum { NoFocus,    YesFocus };
enum { WantMove,   WantResize };
enum { WantTiled,  WantFloating };
enum { WantInt,    WantFloat,	NumTypes};


/* ---------------------------------------
 * Typedefs
 * ---------------------------------------
 */

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
	char* class;
	char* instance;
	char* title;
	unsigned int desks;
	int isfloat;
	int isfull;
	int monitor;
	rule* next;
};


/* ---------------------------------------
 * Util Functions
 * ---------------------------------------
 */

void*
ecalloc(size_t nmemb, size_t size){
	void* p;

	if ( !(p = calloc(nmemb, size)) )
		die("ecalloc failed");

	return p;
}

void
estrtoi(const char* s, Arg* arg){
	arg->i = (int) strtol(s, (char**) NULL, 10);
}

void
estrtof(const char* s, Arg* arg){
	arg->f = (float) strtof(s, (char**) NULL);
}

/* convert 11011110 to "01111011"
 * for this example, len = 8
 * dest must be a calloc'd char* that you free() afterwards
 */
void
uitos(unsigned int ui, int len, char* dest){
	int i, j, res;
	int bytearray[len];
	char bytestr[len + 1];

	/* reverse the array, as tags are printed left to right, not right to left */
	for (i=0;i < len;i++)
		bytearray[i] = ui >> i & 1;

	for (i=0, j=0;
	(i < (len + 1)) && (res = snprintf(bytestr + j, (len + 1) - j, "%d", bytearray[i])) > 0;
	i++)
		j += res;

	snprintf(dest, len + 1, "%s", bytestr);
}


/* ---------------------------------------
 * Main Function Declarations
 * ---------------------------------------
 */

/* Clients */
static void adjustcoords(client* c);
static void applyrules(client* c);
static void attachaside(client* c);
static void changecurrent(client* c, monitor* m, int desk, int refocused);
static void configure(client* c);
static void detach(client* c);
static client* findclient(Window w);
static client* findcurrent(monitor* m);
static client* findprevclient(client* c, int wantvis, int wantfloat);
static client* findvisclient(client* c, int wantfloat);
static void killclient(const Arg arg);
static void manage(Window parent, XWindowAttributes* wa);
static void manipulate(const Arg arg);
static void moveclient(const Arg arg);
static void moveclientup(client* c, int wantzoom);
static void movefocus(const Arg arg);
static void resizeclient(client* c, int x, int y, int w, int h);
static void restack(monitor* m);
static void sendmon(client* c, monitor* m);
static void showhide(monitor* m);
static void todesktop(const Arg arg);
static void toggledesktop(const Arg arg);
static void togglefloat(const Arg arg);
static void togglefs(const Arg arg);
static void tomon(const Arg arg);
static void unmanage(client* c, int destroyed);
static void updatefocus(monitor* m);
static void zoom(const Arg arg);
/* Desktops */
static void arrange(monitor* m);
static void changemsize(const Arg arg);
static void floaty(monitor* m);
static void loaddesktop(int i);
static void monocle(monitor* m);
static void setlayout(const Arg arg);
static void tile(monitor* m);
static void toggleview(const Arg arg);
static void view(const Arg arg);
/* Monitor Manipulation */
static void changemon(monitor* m, int wantfocus);
static void cleanupmon(monitor* m);
static monitor* coordstomon(int x, int y);
static monitor* createmon(int num, int x, int y, int w, int h);
static monitor* dirtomon(int dir);
static monitor* findmon(Window w);
static void focusmon(const Arg arg);
static void updategeom();
/* Backend */
static void cleanup();
static int getptrcoords(int* x, int* y);
static void grabbuttons(client* c, int focused);
static void outputstats();
static void runconfig();
static void setrootstats();
static void setup();
static void start();
static int xerror(Display* dis, XErrorEvent* e);
static int xsendkill(Window w);
static void quit(const Arg arg);
#ifdef _SHAPE_H_
static void roundcorners(client* c);
static void unroundcorners(client* c);
#endif
/* sarasock interfacing */
static void handlemsg(char* msg);
static void reloadgeom();
static void (*str2func(const char* str))(Arg);
/* X */
static void adopt();
static void buttonpress(XEvent* e);
static void configurenotify(XEvent* e);
static void configurerequest(XEvent* e);
static void destroynotify(XEvent* e);
static void enternotify(XEvent* e);
static void focusin(XEvent* e);
static void maprequest(XEvent* e);
static void motionnotify(XEvent* e);
static void unmapnotify(XEvent* e);

void (*events[LASTEvent])(XEvent* e) = {
	[ButtonPress] = buttonpress,
	[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[FocusIn] = focusin,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[UnmapNotify] = unmapnotify
};

/* Check assigned fields in arg when you use parser for safety.
 * If range is being used, SAFEPARG is suggested.
 */
void (*parser[NumTypes])(const char* s, Arg* arg) = {
	[WantInt] = estrtoi,
	[WantFloat] = estrtof
};


/* callable functions from outside */
struct {
	void (*func)(Arg);
	const char* str;
} conversions [] = {
	{changemsize,   "changemsize"},
	{focusmon,      "focusmon"},
	{killclient,    "killclient"},
	{moveclient,    "moveclient"},
	{movefocus,     "movefocus"},
	{quit,          "quit"},
	{todesktop,     "todesktop"},
	{toggledesktop, "toggledesktop"},
	{togglefloat,   "togglefloat"},
	{togglefs,      "togglefs"},
	{toggleview,    "toggleview"},
	{tomon,         "tomon"},
	{setlayout,     "setlayout"},
	{view,          "view"},
	{zoom,          "zoom"},
};


/* Make the above known */
#include "config.h"


/* ---------------------------------------
 * Globals
 * ---------------------------------------
 */

/* X */
static int screen;
static int sh;
static int sw;
static Cursor cursor;
static Display* dis;
static Window root;
/* for EACHCLIENT/EACHMON iterating */
client* ic;
monitor* im;
Arg parg; /* for parser */
/* Monitor Interfacing */
static monitor* curmon;
static monitor* mhead;
/* Backend */
static int restart;
static int running;
static const Arg dumbarg; /* passthrough for function calls like togglefs */
static long w_data[] = { WithdrawnState, None }; /* for unmanage and unmapnotify */
static Atom w_atom; /* for unmanage and unmapnotify */
static XEvent dumbev; /* for XCheckMasking */


/* ---------------------------------------
 * Clients
 * ---------------------------------------
 */

void
adjustcoords(client* c){
	monitor* m;

	if ( (m = coordstomon(c->x, c->y)) && m != c->mon ){
		c->x += (m->mx < c->mon->mx) ? c->mon->mx : -m->mx;
		c->y += (m->my < c->mon->my) ? c->mon->my : -m->my;
	}
}

void
applyrules(client* c){
	const char* class, * instance;
	int i;
	const rule* r;
	XTextProperty tp;
	XClassHint ch = { NULL, NULL };

	c->isfloat = c->desks = 0;

	XGetWMName(dis, c->win, &tp);
	XGetClassHint(dis, c->win, &ch);
	class = ch.res_class ? ch.res_class : "broken";
	instance = ch.res_name  ? ch.res_name  : "NULL";

	for (i=0;i < TABLENGTH(rules);i++){
		r = &rules[i];

		if ((!r->title || (tp.value && strstr(r->title, (const char*) tp.value)))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance))){
			c->isfloat = r->isfloat;
			c->isfull = r->isfull;
			c->desks |= r->desks;

			for EACHMON(mhead){
				if (im->num == r->monitor){
					c->mon = im;
					break;
				}
			}
		}
	}

	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);

	c->desks = c->desks ? c->desks : c->mon->seldesks;
}

void
attachaside(client* c){
	client* l;

	if (!c->mon->head){
		c->mon->head = c;

	} else {
		/* If not the first on this desktop */
		if (c->mon->current){
			c->next = c->mon->current->next;
			c->mon->current->next = c;

		} else {
			for (l=c->mon->head;l && l->next;l=l->next);
			if (l)
				l->next = c;
		}
	}
}

// TODO:
// if (m->prev && m->prev != c)
// 	m->current = m->prev
void
changecurrent(client* c, monitor* m, int desk, int refocused){
	client* vis;

	if (c){
		c->iscur ^= 1 << desk;
		grabbuttons(c, 1);
	}
	
	for EACHCLIENT(m->head){
		if (ic != c && (ic->iscur & 1 << desk)){
			ic->iscur ^= 1 << desk;
			grabbuttons(ic, 0);
		}
	}

	m->current = c;

	if (m->current && refocused){
		vis = (vis = findvisclient(c->next, WantFloating))
			? vis : findprevclient(c, OnlyVis, WantFloating);
		changecurrent(vis, m, m->curdesk, 0);
	}

	if (c)
		updatefocus(m);
}

void
configure(client* c){
	XConfigureEvent ce = {
		.type = ConfigureNotify,
		.display = dis,
		.event = c->win,
		.window = c->win,
		.x = c->x,
		.y = c->y,
		.width = c->w,
		.height = c->h,
		.above = None,
		.override_redirect = False
	};

	XSendEvent(dis, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
detach(client* c){
	client* p;

	/* refocus only as necessary */
	if (c == c->mon->current)
		changecurrent(c, c->mon, c->mon->curdesk, 1);

	/* For both, if NULL, then we're still okay */
	if ( (p = findprevclient(c, AnyVis, WantFloating)) )
		p->next = c->next;
	else
		c->mon->head = c->next;
}

client*
findclient(Window w){
	for EACHMON(mhead)
		for EACHCLIENT(im->head)
			if (ic->win == w)
				return ic;

	return NULL;
}

client*
findcurrent(monitor* m){
	for EACHCLIENT(m->head)
		if (ISVISIBLE(ic) && (ic->iscur & 1 << m->curdesk))
			return ic;

	return NULL;
}

client*
findprevclient(client* c, int onlyvis, int wantfloat){
	client* ret = NULL;

	for EACHCLIENT(c->mon->head){
		if (ic == c)
			break;

		if (onlyvis){
			if (ISVISIBLE(ic))
				ret = ic;

		} else if (!wantfloat){
			if (ISVISIBLE(ic) && !ic->isfloat)
				ret = ic;
		}

		if (ic->next == c)
			return (onlyvis || !wantfloat) ? ret : ic;
	}

	return NULL;
}

client*
findvisclient(client* c, int wantfloat){
	for EACHCLIENT(c){
		if (ISVISIBLE(ic)){
			if (!wantfloat && !ic->isfloat)
				return ic;
			else if (wantfloat)
				return ic;
		}
	}
	
	return NULL;
}

void
killclient(const Arg arg){
	if (!curmon->current)
		return;

	if (!xsendkill(curmon->current->win)){
		XGrabServer(dis);
		XSetCloseDownMode(dis, DestroyAll);
		XKillClient(dis, curmon->current->win);
		XSync(dis, False);
		XUngrabServer(dis);
	}
}

void
manage(Window parent, XWindowAttributes* wa){
	client* c, * t;
	Window trans = None;

	c = ecalloc(1, sizeof(client));

	if (curmon->current && curmon->current->isfull)
		togglefs(dumbarg);

	c->win = parent;
	c->isfloat = c->oldfloat = c->isfull = c->iscur = 0;
	c->x = wa->x;
	c->y = wa->y;
	c->w = wa->width;
	c->h = wa->height;

	if (XGetTransientForHint(dis, parent, &trans) && (t = findclient(trans))){
		c->desks = t->desks;
		c->mon = t->mon;

	} else {
		c->mon = curmon;
		applyrules(c);
	}
	if (!c->isfloat)
		c->isfloat = c->oldfloat = (trans != None);

	adjustcoords(c);
	c->y = (c->y < c->mon->wy) ? c->mon->wy : c->y;

	configure(c);
	XSelectInput(dis, c->win, EnterWindowMask|FocusChangeMask|PropertyChangeMask
			|StructureNotifyMask);
	grabbuttons(c, 0);

	attachaside(c);

	/* move out of the way until told otherwise */
	XMoveResizeWindow(dis, c->win, c->x + 2*sw, c->y, c->w, c->h);
	arrange(c->mon);
#ifdef _SHAPE_H_
	roundcorners(c);
#endif
	XMapWindow(dis, c->win);

	if (c->desks & c->mon->seldesks){
		changecurrent(c, c->mon, c->mon->curdesk, 0);
		restack(c->mon);

		/* applyrules */
		if (c->isfull){
			c->isfull = !c->isfull;
			togglefs(dumbarg);
		}
	}

	outputstats();
}

void
moveclient(const Arg arg){
	client* c;

	if (!curmon->current || curmon->current->isfull)
		return;
	
	parser[WantInt](arg.s, &parg);

	/* Up stack */
	if (parg.i > 0)
		moveclientup(curmon->current, NoZoom);
	/* Down stack - equivalent to moving next tiled client up */
	else if ( parg.i < 0 && (c = findvisclient(curmon->current->next, WantTiled)) )
		moveclientup(c, NoZoom);

	arrange(curmon);
}

void
moveclientup(client* c, int wantzoom){
	client* p, * target, * ptarget;

	if (!c)
		return;

	/* Go up only if not highest visible */
	target = wantzoom ? findvisclient(curmon->head, WantTiled) : findprevclient(c, 0, WantTiled);

	if (!target || target == c)
		return;

	p = findprevclient(c, AnyVis, WantFloating);
	ptarget = findprevclient(target, AnyVis, WantFloating);

	/* if p == target, then we're still okay */
	p->next = c->next;
	c->next = target;
	if (ptarget)
		ptarget->next = c;
	else
		curmon->head = c;
}

void
movefocus(const Arg arg){
	client* j, * c = NULL;

	if (!curmon->current || curmon->current->isfull)
		return;

	parser[WantInt](arg.s, &parg);

	/* up stack */
	if (parg.i > 0){
		for (j=curmon->head;j && j != curmon->current;j=j->next)
			if (ISVISIBLE(j))
				c = j;

		/* if curmon->current was highest, go to the bottom */
		if (!c)
			for (;j;j=j->next)
				if (ISVISIBLE(j))
					c = j;

	/* down stack, wrap around */
	} else if (parg.i < 0) {
		if ( !(c = findvisclient(curmon->current->next, WantFloating)) )
			c = findvisclient(curmon->head, WantFloating);
	}

	if (c && c != curmon->current){
		changecurrent(c, curmon, curmon->curdesk, 0);
		restack(curmon);
	}
}

void
manipulate(const Arg arg){
	int x, y, ocx, ocy, nx, ny, nw, nh;
	client* c;
	XEvent ev;
	int doresize = (arg.i == WantResize), trytoggle = 0;
	monitor* m = NULL;
	Time lasttime = 0;

	if ( !(c = curmon->current) || c->isfull )
		return;

	restack(curmon);

	if (XGrabPointer(dis, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor, CurrentTime) != GrabSuccess)
		return;

	if (!getptrcoords(&x, &y))
		return;

	if (doresize){
		if ( !(m = c->mon) )
			return;
		XWarpPointer(dis, None, c->win, 0, 0, 0, 0, c->w + 1, c->h + 1);
	}

	ocx = c->x;
	ocy = c->y;
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

			/* Only adjust if moving */
			nx = doresize ? c->x : ocx + (ev.xmotion.x - x);
			ny = doresize ? c->y : ocy + (ev.xmotion.y - y);
			/* Only adjust if resizing */
			nw = doresize ? MAX(ev.xmotion.x - ocx + 1, 1) : c->w;
			nh = doresize ? MAX(ev.xmotion.y - ocy + 1, 1) : c->h;

			/* if c extends beyond the boundaries of its monitor, make it a float */
			if (doresize && (m->mx + nw >= curmon->mx
						&& m->mx + nw <= curmon->mx + curmon->mw
						&& m->wy + nh >= curmon->wy
						&& m->wy + nh <= curmon->wy + curmon->wh)){
				trytoggle = 1;

			} else {
				/* if c is within snappx of the monitor borders,
				 * then snap and make it a float
				 */
				nx = (abs(curmon->mx - nx) < snappx) ? curmon->mx : nx;
				nx = (abs((curmon->mx + curmon->mw) - (nx + c->w)) < snappx)
					? curmon->mx + curmon->mw - c->w : nx;
				ny = (abs(curmon->wy - ny) < snappx) ? curmon->wy : ny;
				ny = (abs((curmon->wy + curmon->wh) - (ny + c->h)) < snappx)
					? curmon->wy + curmon->wh - c->h : ny;
				trytoggle = 1;
			}
			/* don't toggle if floating layout, do resize if floating */
			if (!c->isfloat && trytoggle && !(curmon->curlayout->arrange == &floaty))
				togglefloat(dumbarg);
			if (c->isfloat || (curmon->curlayout->arrange == &floaty))
				resizeclient(c, nx, ny, nw, nh);
			XFlush(dis);
			break;
		}
	} while (ev.type != ButtonRelease);

	if (doresize)
		XWarpPointer(dis, None, c->win, 0, 0, 0, 0, c->w + 1, c->h + 1);
	XUngrabPointer(dis, CurrentTime);

	if (doresize)
		while (XCheckMaskEvent(dis, EnterWindowMask, &ev));

	if ( (m = coordstomon(c->x, c->y)) && m != curmon){
		sendmon(c, m);
		changemon(m, YesFocus);
		outputstats();
	}
#ifdef _SHAPE_H_
	roundcorners(c);
#endif
}

void
resizeclient(client* c, int x, int y, int w, int h){
	XWindowChanges wc;

	c->x = wc.x = x;
	c->y = wc.y = y;
	c->w = wc.width = w;
	c->h = wc.height = h;
	XConfigureWindow(dis, c->win, CWX|CWY|CWWidth|CWHeight, &wc);
	XSync(dis, False);
#ifdef _SHAPE_H_
	roundcorners(c);
#endif
}

void
restack(monitor* m){
	XWindowChanges wc;

	if (!m->current)
		return;

	wc.stack_mode = Below;
	wc.sibling = m->current->win;

	for EACHCLIENT(m->head){
		//if (ic != m->current && !ic->isfloat && ISVISIBLE(ic)){
		// if not current, and visible, and both are floating or neither are floating
		if (ic != m->current && ISVISIBLE(ic) && (ic->isfloat == m->current->isfloat)){
			XConfigureWindow(dis, ic->win, CWSibling|CWStackMode, &wc);
			wc.sibling = ic->win;
		}
	}

	XSync(dis, False);
	while (XCheckMaskEvent(dis, EnterWindowMask, &dumbev));
}

void
showhide(monitor* m){
	for EACHCLIENT(m->head){
		if (ISVISIBLE(ic)){
			XMoveWindow(dis, ic->win, ic->x, ic->y);
			if (ic->isfloat && !ic->isfull){
				resizeclient(ic, ic->x, ic->y, ic->w, ic->h);
				XRaiseWindow(dis, ic->win);
			}

		} else {
			XMoveWindow(dis, ic->win, -2*ic->w, ic->y);
		}
	}
}

void
sendmon(client* c, monitor* m){
	if (c->mon == m || c->isfull)
		return;

	detach(c);
	c->mon = m;
	c->desks = m->seldesks;

	c->next = NULL;
	attachaside(c);
	c->iscur = 0;
	changecurrent(c, c->mon, c->mon->curdesk, 0);
	if (c->isfloat)
		adjustcoords(c);

	curmon->current = findcurrent(curmon);
	for EACHMON(mhead)
		arrange(im);
	changemon(c->mon, YesFocus);
}

void
todesktop(const Arg arg){
	if (!curmon->current)
		return;

	parser[WantInt](arg.s, &parg);

	if (!SAFEPARG(0,NUMTAGS))
		return;

	if (curmon->current->desks == (1 << parg.i))
		return;

	curmon->current->desks = 1 << parg.i;
	curmon->current->iscur = 0;
	changecurrent(curmon->current, curmon, 1 << parg.i, 1);

	arrange(curmon);
	outputstats();
}

void
toggledesktop(const Arg arg){
	unsigned int newdesks;

	if (!curmon->current)
		return;

	parser[WantInt](arg.s, &parg);

	if (!SAFEPARG(-1,NUMTAGS))
		return;

	if (parg.i < 0)
		newdesks = curmon->current->desks | ~(curmon->current->desks);
	else
		newdesks = curmon->current->desks ^ (1 << parg.i);

	if (newdesks){
		curmon->current->desks = newdesks;
		/* set current to be current on new desktop
		 * if it will no longer be visible, adjust current
		 */
		changecurrent(curmon->current, curmon, 1 << parg.i,
			(curmon->current->desks & curmon->seldesks) ? 0 : 1);

		arrange(curmon);
		outputstats();
	}
}

void
togglefloat(const Arg arg){
	if (!curmon->current || curmon->current->isfull)
		return;

	if (curmon->curlayout->arrange == &floaty)
		return;

	curmon->current->isfloat = !curmon->current->isfloat;
	arrange(curmon);
}

void
togglefs(const Arg arg){
	if (!curmon->current)
		return;

	if ( (curmon->current->isfull = !curmon->current->isfull) ){
		curmon->current->oldfloat = curmon->current->isfloat;
		curmon->current->isfloat = 0;

		resizeclient(curmon->current, curmon->mx, curmon->my,
				curmon->mw, curmon->mh);
#ifdef _SHAPE_H_
		unroundcorners(curmon->current);
#endif
		XRaiseWindow(dis, curmon->current->win);

	} else {
		curmon->current->isfloat = curmon->current->oldfloat;
		arrange(curmon);
	}
}

void
tomon(const Arg arg){
	if (!curmon->current || !mhead->next)
		return;
	
	parser[WantInt](arg.s, &parg);

	sendmon(curmon->current, dirtomon(parg.i));
	outputstats();
}

void
unmanage(client* c, int destroyed){
	monitor* m = c->mon;

	detach(c);
	if (!destroyed){
		XGrabServer(dis);
		XUngrabButton(dis, AnyButton, AnyModifier, c->win);
		XChangeProperty(dis, c->win, w_atom, w_atom, 32,
				PropModeReplace, (unsigned char*) w_data, 2);
		XSync(dis, False);
		XUngrabServer(dis);
	}
	free(c);
	arrange(m);
	outputstats();
}

void
updatefocus(monitor* m){
	if (!m)
		return;

	if (m->current)
		XSetInputFocus(dis, m->current->win, RevertToPointerRoot, CurrentTime);
	else
		XSetInputFocus(dis, root, RevertToPointerRoot, CurrentTime);

	XSync(dis, False);
}

void
zoom(const Arg arg){
	if (!curmon->current)
		return;

	moveclientup(curmon->current, YesZoom);
	arrange(curmon);
}


/* ---------------------------------------
 * Desktops
 * ---------------------------------------
 */

void
arrange(monitor* m){
	showhide(m);
	m->curlayout->arrange(m);
	restack(m);
};

void
changemsize(const Arg arg){
	parser[WantFloat](arg.s, &parg);
	curmon->msize += ( ((curmon->msize < 0.95 * curmon->mw) && (parg.f > 0))
			|| ((curmon->msize > 0.05 * curmon->mw) && (parg.f < 0)) )
		? parg.f * curmon->mw : 0;

	arrange(curmon);
}

/* don't toggle isfloat, so that everyone snaps back when you tile, etc. */
void
floaty(monitor* m){
	for EACHCLIENT(m->head){
		if (ISVISIBLE(ic)){
			resizeclient(ic, ic->x, ic->y, ic->w, ic->h);
		}
	}
}

void
loaddesktop(int i){
	curmon->desks[curmon->curdesk].msize = curmon->msize;
	curmon->desks[curmon->curdesk].curlayout = curmon->curlayout;

	curmon->msize = curmon->desks[i].msize;
	curmon->curlayout = curmon->desks[i].curlayout;
}

void
monocle(monitor* m){
	int x = m->mx + gappx, y = (bottombar ? (m->my + gappx) : (m->wy + barpx)), max_h = (bottombar ? (m->wh - barpx) : (m->mh - gappx));
	for EACHCLIENT(m->head)
		if (ISVISIBLE(ic) && !ic->isfloat && !ic->isfull)
			resizeclient(ic, x, y, m->mw - 2*gappx, max_h - y);
}

void
setlayout(const Arg arg){
	int i;
	for (i=0;i < TABLENGTH(layouts);i++)
		if (STREQ(arg.s, layouts[i].name))
			curmon->curlayout = (layout*) &layouts[i];

	arrange(curmon);
	outputstats();
}

void
tile(monitor* m){
	int n = 0, i = 0, h = 0, x = m->mx + gappx, y = (bottombar ? (m->my + gappx) : (m->wy + barpx)), max_h = (bottombar ? (m->wh - barpx) : (m->mh - gappx));
	client* nf = NULL;

	/* Find the first non-floating, visible window and tally non-floating, visible windows */
	for EACHCLIENT(m->head){
		if (!ic->isfloat && ISVISIBLE(ic)){
			nf = (!nf) ? ic : nf;
			n++;
		}
	}

	if (nf && n == 1){
		if (!nf->isfull)
			resizeclient(nf, x, y, m->mw - 2*gappx, max_h - y);

	} else if (nf){
		/* so having a master doesn't affect stack splitting */
		n--;

		/* Master window */
		if (!nf->isfull)
			resizeclient(nf, x, y, m->msize - gappx, max_h - y);

		/* Stack */
		for EACHCLIENT(nf->next){
			if (ISVISIBLE(ic) && !ic->isfloat && !ic->isfull){
				h = (max_h - y) / (n - i);

				resizeclient(ic, x + m->msize, y, m->mw - m->msize - 2*gappx, h);

				if (y + h < max_h)
					y += h + gappx;

				i++;
			}
		}
	}
}

void
toggleview(const Arg arg){
	int i;
	unsigned int tagmask;

	parser[WantInt](arg.s, &parg);

	if (!SAFEPARG(-1,NUMTAGS))
		return;

	if (parg.i < 0)
		tagmask = ~(curmon->seldesks);
	else
		tagmask = 1 << parg.i;

	/* if this would leave nothing visible */
	if ((curmon->seldesks ^ tagmask) == 0)
		return;

	if (curmon->current && curmon->current->isfull)
		togglefs(dumbarg);

	curmon->seldesks ^= tagmask;

	if (!(curmon->seldesks & 1 << curmon->curdesk)){
		for (i=0;i < NUMTAGS;i++){
			if (curmon->seldesks & 1 << i){
				loaddesktop(i);
				curmon->curdesk = i;
				break;
			}
		}
	}

	/* refocuses, toggles off curmon->current's currentness */
	if (curmon->current && !ISVISIBLE(curmon->current))
		changecurrent(curmon->current, curmon, curmon->curdesk, 1);

	arrange(curmon);
	outputstats();
}

void
view(const Arg arg){
	client* c;

	parser[WantInt](arg.s, &parg);

	if (!SAFEPARG(0,NUMTAGS))
		return;

	if (curmon->current && curmon->current->isfull)
		togglefs(dumbarg);

	loaddesktop(parg.i);
	curmon->seldesks = 1 << parg.i;
	curmon->curdesk = parg.i;

	if ( (c = findcurrent(curmon)) )
		/* rezero, so it can be set and everyone else unset */
		c->iscur ^= 1 << curmon->curdesk;
	else
		c = findvisclient(curmon->head, WantFloating);
		
	changecurrent(c, curmon, curmon->curdesk, 0);

	arrange(curmon);
	outputstats();
}


/* ---------------------------------------
 * Monitor Manipulation
 * ---------------------------------------
 */

void
changemon(monitor* m, int wantfocus){
	if (curmon && curmon->current)
		grabbuttons(curmon->current, 0);
	curmon = m;
	if (wantfocus)
		updatefocus(curmon);
}

void
cleanupmon(monitor* m){
	free(m->desks);
	free(m);
}

monitor*
coordstomon(int x, int y){
	for EACHMON(mhead)
		if (!ISOUTSIDE(x, y, im->mx, im->my, im->mw, im->mh))
			return im;

	return NULL;
}

monitor*
createmon(int num, int x, int y, int w, int h){
	int i;
	monitor* m = ecalloc(1, sizeof(monitor));

	m->num = num;
	m->mx = x;
	m->my = y;
	m->mw = w;
	m->mh = h;

	m->wy = m->my + (bottombar ? 0 : barpx );
	m->wh = m->mh - barpx;

	/* Default to first layout */
	m->curlayout = (layout*) &layouts[0];
	m->msize = m->mw * MASTER_SIZE;

	m->desks = ecalloc(NUMTAGS, sizeof(desktop));
	for (i=0;i < NUMTAGS;i++){
		m->desks[i].curlayout = m->curlayout;
		m->desks[i].msize = m->msize;
	}

	/* Default to first desktop */
	m->seldesks = 1 << 0;
	m->curdesk = 0;
	m->head = NULL;
	m->current = NULL;

	return m;
}

monitor*
dirtomon(int dir){
	monitor* m;

	if (dir > 0){
		if ( !(m = curmon->next) )
			m = mhead;

	} else if (curmon == mhead){
		for (m=mhead;m->next;m=m->next);

	} else {
		for (m=mhead;m->next != curmon;m=m->next);
	}

	return m;
}

monitor*
findmon(Window w){
	for EACHMON(mhead)
		for EACHCLIENT(im->head)
			if (ic->win == w)
				return im;

	return curmon;
}

void
focusmon(const Arg arg){
	monitor* m;

	if (!mhead->next)
		return;

	parser[WantInt](arg.s, &parg);

	if ( (m = dirtomon(parg.i)) && m == curmon )
		return;
	changemon(m, YesFocus);
}

#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo* unique, size_t n, XineramaScreenInfo* info){
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org)
			return 0;
	return 1;
}
#endif

// TODO: 6.2 approach to be less intrusive?
/* a la dwm 6.1 */
void
updategeom(){
	int x, y;
	client* c;
	monitor* m, * oldmhead = mhead;

#ifdef XINERAMA
	if (XineramaIsActive(dis)){
		int i, j, ns;
		XineramaScreenInfo* unique;
		XineramaScreenInfo* info = XineramaQueryScreens(dis, &ns);

      		/* only consider unique geometries as separate screens */
		unique = ecalloc(ns, sizeof(XineramaScreenInfo));
		for (i=0, j=0;i < ns;i++)
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

	} else
#endif
	{
		mhead = createmon(0, 0, 0, sw, sh);
	}

	/* if updating, reattach any old clients to the new mhead */
	while ( (m = oldmhead) ){
		while ( (c = m->head) ){
			m->head = c->next;
			detach(c);
			c->mon = mhead;
			c->next = NULL;
			attachaside(c);
		}
		mhead->seldesks |= m->seldesks;
		oldmhead = m->next;
		cleanupmon(m);
	}

	getptrcoords(&x, &y);
	if ( (m = coordstomon(x, y)) )
		changemon(m, YesFocus);
	if (!curmon)
		changemon(mhead, YesFocus);

	outputstats();
}


/* ---------------------------------------
 * Backend
 * ---------------------------------------
 */

/* Kill off any remaining clients
 * Free all the things
 */
void
cleanup(){
	monitor* m, * tm = mhead;
	const Arg arg = {.s = "-1"};

	/* for an unknown reason, using EACHMON segfaults this */
	for (m=mhead;m;m=m->next){
		changemon(m, NoFocus);
		/* make everything visible */
		toggleview(arg);
		while (curmon->current)
			unmanage(curmon->current, 0);
	}

	XUngrabKey(dis, AnyKey, AnyModifier, root);

	while ( (m = tm) ){
		tm = m->next;
		cleanupmon(m);
	}

	XFreeCursor(dis, cursor);

	XSync(dis, False);
	XSetInputFocus(dis, PointerRoot, RevertToPointerRoot, CurrentTime);
}

int
getptrcoords(int* x, int* y){
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dis, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

void
grabbuttons(client* c, int focused){
	int i, j;
	unsigned int modifiers[] = { 0, LockMask };

	XUngrabButton(dis, AnyButton, AnyModifier, c->win);
	if (!focused)
		XGrabButton(dis, AnyButton, AnyModifier, c->win, False,
			BUTTONMASK, GrabModeSync, GrabModeSync, None, None);

	/* "all" our clicks are ClkWin */
	for (i=0;i < TABLENGTH(buttons);i++)
		for (j=0;j < TABLENGTH(modifiers);j++)
			XGrabButton(dis, buttons[i].btn,
					buttons[i].mask | modifiers[j],
					c->win, False, BUTTONMASK,
					GrabModeAsync, GrabModeSync, None, None);
}

void
outputstats(){
	char* isdeskocc, * isdesksel, monstate[NUMTAGS+3];
	int i;
	unsigned int occ, sel;

	/* output:
	 * "0:SONNNNNNN:Y"
	 * im->num:SEL/OCC/EMPTY:curlayout->symbol
	 */
	for EACHMON(mhead){
		occ = sel = 0;
		isdeskocc = ecalloc(NUMTAGS, sizeof(char));
		isdesksel = ecalloc(NUMTAGS, sizeof(char));
		sel = im->seldesks;

		for EACHCLIENT(im->head)
			occ |= ic->desks;

		/* uis get reordered in the dest string so they are left-to-right */
		uitos(occ, NUMTAGS, isdeskocc);
		uitos(sel, NUMTAGS, isdesksel);

		for (i=0;i<NUMTAGS;i++){
			if (isdesksel[i] == '1'){
				monstate[i] = 'S';
			} else if (isdeskocc[i] == '1'){
				monstate[i] = 'O';
			} else {
				monstate[i] = 'N';
			}
		}
		monstate[NUMTAGS] = ':';
		monstate[NUMTAGS+1] = im->curlayout->letter;
		monstate[NUMTAGS+2] = '\0';

		setrootstats(monstate, im->num);

		free(isdeskocc);
		free(isdesksel);
	}

	fflush(stdout);
}

/* à la BSPWM */
void
runconfig(){
	int i;
	char shcmd[MAXLEN];

	for (i=0;i < TABLENGTH(progs);i++){
		snprintf(shcmd, slen(progs[i])+1, "%s", progs[i]);

		if (fork() == 0) {
			if (dis)
				close(ConnectionNumber(dis));
			setsid();
			execl("/bin/sh", "/bin/sh", "-c", shcmd, (char *) NULL);
			fprintf(stderr, "sara: failed to execute startup cmd\n");
			exit(EXIT_SUCCESS);
		}
	}
}


/* This man is a god
 * https://jonas-langlotz.de/2020/10/05/polybar-on-dwm
 */
void
setrootstats(char* monstate, int monnum){
	char* atom = malloc(sizeof(char) * 40);
	char* num = malloc(sizeof(char) * 12);
	sprintf(atom, "");
	sprintf(num, "%d", monnum);
	strcat(atom, "SARA_MONSTATE_");
	strcat(atom, num);
	strcat(atom, "\0");
	XChangeProperty(dis, root, XInternAtom(dis, atom, False),
			XA_STRING, 8, PropModeReplace, (unsigned char*) monstate,
			NUMTAGS+3);
}

void
setup(){
	XSetWindowAttributes wa;

	screen = DefaultScreen(dis);
	root = RootWindow(dis, screen);

	sw = XDisplayWidth(dis, screen);
	sh = XDisplayHeight(dis, screen);

	cursor = XCreateFontCursor(dis, 68);

	running = 1;

	mhead = NULL;
	curmon = NULL;

	updategeom();
	loaddesktop(0);
	outputstats();

	w_atom = XInternAtom(dis, "WM_STATE", False);

	wa.cursor = cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dis, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dis, root, wa.event_mask);
}

/* many thanks to bspwm, geeksforgeeks, Beej for sockets */
void
start(){
	int nbytes;
	char msg[MAXBUFF];
	fd_set desc;
	XEvent ev;
	int cfd, max_fd, sfd, xfd = ConnectionNumber(dis);
	struct sockaddr saddress = {AF_UNIX, INPUTSOCK};

	if ( (sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 )
		die("couldn't create socket!");

	unlink(INPUTSOCK);

	if (bind(sfd, &saddress, sizeof(saddress)) < 0)
		die("couldn't bind socket!");

	if (listen(sfd, SOMAXCONN) < 0)
		die("couldn't listen to socket!");

	runconfig();

	while (running){
		XFlush(dis);

		FD_ZERO(&desc);
		FD_SET(sfd, &desc);
		FD_SET(xfd, &desc);
		max_fd = MAX(sfd, xfd);

		if (select(max_fd + 1, &desc, NULL, NULL, NULL) > 0){
			/* Check for socket connections */
			if (FD_ISSET(sfd, &desc)){
				cfd = accept(sfd, NULL, NULL);
				if (cfd > 0 && (nbytes = recv(cfd, msg, sizeof(msg)-1, 0)) > 0){
					msg[nbytes] = '\0';
					handlemsg(msg);
					close(cfd);
				}
			}

			/* Check for an X event manually - XNextEvent blocks until an event occurs */
			if (FD_ISSET(xfd, &desc)){
				while (XCheckMaskEvent(dis, ~0, &ev))
					if (events[ev.type])
						events[ev.type](&ev);
			}
		}
	}

	close(sfd);
	unlink(INPUTSOCK);
}

int
xerror(Display* dis, XErrorEvent* e){
	return 0;
}

int
xsendkill(Window w){
	int n;
	Atom* protocols;
	XEvent ev;
	int exists = 0;
	Atom destproto = XInternAtom(dis, "WM_DELETE_WINDOW", False);

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

void
quit(const Arg arg){

	parser[WantInt](arg.s, &parg);

	if (!SAFEPARG(0,1))
		return;

	if (parg.i > 0)
		restart = 1;

	running = 0;
}

#ifdef _SHAPE_H_
void
roundcorners(client *c)
{
	int diam;
	Pixmap mask;
	GC shapegc;

	if (corner_radius < 0)
		return;

	if (!c || c->isfull)
		return;

	diam = 2 * corner_radius;
	if (c->w < diam || c->h < diam)
		return;

	if (!(mask = XCreatePixmap(dis, c->win, c->w, c->h, 1)))
		return;
	
	if (!(shapegc = XCreateGC(dis, mask, 0, NULL))){
	    XFreePixmap(dis, mask);
	    free(shapegc);
	    return;
	}

	XFillRectangle(dis, mask, shapegc, 0, 0, c->w, c->h);
	XSetForeground(dis, shapegc, 1);

	/* topleft, topright, bottomleft, bottomright
	 * man XArc - positive is counterclockwise
	 */
	XFillArc(dis, mask, shapegc, 0, 		0, 			diam, diam, 90 * 64, 90 * 64);
	XFillArc(dis, mask, shapegc, c->w - diam - 1, 	0, 			diam, diam, 0 * 64, 90 * 64);
	XFillArc(dis, mask, shapegc, 0,			c->h - diam - 1,	diam, diam, -90 * 64, -90 * 64);
	XFillArc(dis, mask, shapegc, c->w - diam - 1,	c->h - diam - 1,	diam, diam, 0 * 64, -90 * 64);

	XFillRectangle(dis, mask, shapegc, corner_radius, 0, c->w - diam, c->h);
	XFillRectangle(dis, mask, shapegc, 0, corner_radius, c->w, c->h - diam);
	XShapeCombineMask(dis, c->win, ShapeBounding, 0, 0, mask, ShapeSet);
	XFreePixmap(dis, mask);
	XFreeGC(dis, shapegc);
}

void
unroundcorners(client *c)
{
	Pixmap mask;
	GC shapegc;

	if (corner_radius < 0 || !c)
		return;

	if (!(mask = XCreatePixmap(dis, c->win, c->w, c->h, 1)))
		return;
	
	if (!(shapegc = XCreateGC(dis, mask, 0, NULL))){
	    XFreePixmap(dis, mask);
	    free(shapegc);
	    return;
	}

	XSetForeground(dis, shapegc, 1);
	XFillRectangle(dis, mask, shapegc, 0, 0, c->w, c->h);
	XShapeCombineMask(dis, c->win, ShapeBounding, 0, 0, mask, ShapeSet);
	XFreePixmap(dis, mask);
	XFreeGC(dis, shapegc);
}
#endif


/* ---------------------------------------
 * sarasock interfacing
 * ---------------------------------------
 */

/* Thanks to bspwm's handle_message and process_message for some inspiration */
void
handlemsg(char* msg){
	char* funcstr, * argstr;
	void (*func)(Arg);
	Arg arg;

	funcstr = strtok(msg, " ");
	argstr = strtok(NULL, " ");

	if (argstr){
		arg.s = argstr;

		if ( (func = str2func(funcstr)) )
			func(arg);
	}
}

void
reloadgeom(){
	for EACHMON(mhead){
		im->wy = im->my + (bottombar ? 0 : barpx );
		im->wh = im->mh - barpx;
		arrange(im);
	}
}

/* thanks to StackOverflow's wallyk for analagous str2enum */
void (*str2func(const char* str))(Arg){
	int i;
	for (i=0;i < TABLENGTH(conversions);i++)
		if (STREQ(str, conversions[i].str))
			return conversions[i].func;

	return NULL;
}


/* ---------------------------------------
 * X
 * ---------------------------------------
 */


// TODO: good XCB guinea pig?
/* dwm */
void
adopt(){
	unsigned int i, num;
	Window d1, d2, * wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dis, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dis, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dis, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable)
				manage(wins[i], &wa);
		}

		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dis, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dis, wins[i], &d1)
			&& (wa.map_state == IsViewable))
				manage(wins[i], &wa);
		}

		if (wins)
			XFree(wins);
	}
}

void
buttonpress(XEvent* e){
	int i;
	client* c;
	monitor* m;
	XButtonPressedEvent* ev = &e->xbutton;

	if ( (m = findmon(ev->window)) && m != curmon )
		changemon(m, NoFocus);

	if ( (c = findclient(ev->window)) ){
		if (c != c->mon->current)
			changecurrent(c, c->mon, c->mon->curdesk, 0);

		restack(c->mon);
		XAllowEvents(dis, ReplayPointer, CurrentTime);

		/* "all" our button functions are performed on a client */
		for (i=0;i < TABLENGTH(buttons);i++)
			if (buttons[i].func && buttons[i].btn == ev->button
			&& buttons[i].mask == ev->state)
				buttons[i].func(buttons[i].arg);
	}
}

void
configurenotify(XEvent* e){
	XConfigureEvent* ev = &e->xconfigure;

	if (ev->window == root){
		sw = ev->width; sh = ev->height;
		updategeom();

		for EACHMON(mhead){
			for EACHCLIENT(im->head)
				if (ic->isfull)
					resizeclient(ic, im->mx, im->my, im->mw, im->mh);

			arrange(im);
		}
	}
}

void
configurerequest(XEvent* e){
	client* c;
	monitor* m;
	XWindowChanges wc;
	XConfigureRequestEvent* ev = &e->xconfigurerequest;

	if ( (c = findclient(ev->window)) ){
		if (c->isfloat){
			m = c->mon;
			if (ev->value_mask & CWX)
				c->x = m->mx + ev->x;
			if (ev->value_mask & CWY)
				c->y = (ev->y < barpx) ? barpx : ev->y;
			if (ev->value_mask & CWWidth)
				c->w = ev->width;
			if (ev->value_mask & CWHeight)
				c->h = ev->height;
			if ((c->x + c->w) > m->mx + m->mw)
				c->x = m->mx + (m->mw / 2 - c->w / 2);
			if ((c->y + c->h) > m->wy + m->wh)
				c->y = m->wy + (m->wh / 2 - c->h / 2);
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dis, c->win, c->x, c->y, c->w, c->h);

		} else {
			configure(c);
		}

	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
	}

	XSync(dis, False);
}

void
destroynotify(XEvent* e){
	client* c;
	XDestroyWindowEvent* ev = &e->xdestroywindow;

	if ( (c = findclient(ev->window)) )
		unmanage(c, 1);
}

void
enternotify(XEvent* e){
	client* c;
	monitor* m;
	XCrossingEvent* ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;

	if ( !(c = findclient(ev->window)) || c == curmon->current )
		return;

	if ( (m = c->mon) && m != curmon )
		changemon(m, YesFocus);

	changecurrent(c, c->mon, c->mon->curdesk, 0);
}

void
focusin(XEvent* e){
	XFocusChangeEvent* ev = &e->xfocus;

	if (curmon->current && ev->window != curmon->current->win)
		updatefocus(curmon);
}

void
maprequest(XEvent* e){
	XWindowAttributes wa;
	XMapRequestEvent* ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dis, ev->window, &wa) || wa.override_redirect)
		return;

	if (!findclient(ev->window))
		manage(ev->window, &wa);
}

void
motionnotify(XEvent* e){
	monitor* m;
	XMotionEvent* ev = &e->xmotion;

	if (ev->window != root)
		return;

	if ( (m = coordstomon(ev->x_root, ev->y_root)) && m != curmon)
		changemon(im, YesFocus);
}

void
unmapnotify(XEvent* e){
	client* c;
	XUnmapEvent* ev = &e->xunmap;

	if ( (c = findclient(ev->window)) )
		unmanage(c, 0);
}


int
main(){
	if ( !(dis = XOpenDisplay(NULL)) )
		die("Cannot open display!");
	XSetErrorHandler(xerror);
	setup();

	adopt();
	start();

	if (restart)
		execlp("sara", "sara");

	cleanup();
        XCloseDisplay(dis);

	return 0;
}
