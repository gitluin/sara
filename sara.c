/*                                                                                
 * SARA Window Manager
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
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
//#ifdef XINERAMA
//#include <X11/extensions/Xinerama.h>
//#endif

#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))
#define NUMDESKTOPS	8
#define ISVISIBLE(C)	((C->desktops & seldesks))
#define TEXTW(X)	(gettextwidth(X, slen(X)) + lrpad)

enum { cur_norm, cur_move, cur_last };
enum { sch_norm, sch_sel };
enum { ColFg, ColBg };

typedef union {
	const char** com;
	const int i;
	const float f;
	const void* v;
} Arg;

typedef XftColor Clr;


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
	Clr* scheme;
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
	layout current_layout;
};

typedef struct {
	Cursor cursor;
} cur;

typedef struct {
	const char* title;
	int desktopmask;
	int is_float;
} rule;

//typedef struct monitor monitor;
//struct monitor {
//	int h, w;
//	int num;
//	float master_size;
//	unsigned int seldesks;
//	client* head;
//	client* current;
//	desktop desktops[NUMDESKTOPS];
//	layout* current_layout;
//	monitor* next;
//};


/* ---------------------------------------
 * Util Functions
 * ---------------------------------------
 */

static cur* cur_create(Display* dis, int shape);
static void cur_free(Display* dis, cur* cursor);
static void die(const char *fmt, ...);
static void* ecalloc(size_t nmemb, size_t size);
static int slen(const char* str);

/* dwm copypasta */
cur* cur_create(Display* dis, int shape){
	cur* cur;

	if (!dis || !(cur = ecalloc(1,sizeof(cur)))){
		return NULL;
	}

	cur->cursor = XCreateFontCursor(dis,shape);

	return cur;
}

/* dwm copypasta */
void cur_free(Display* dis, cur* cursor){
	if (!cursor){
		return;
	}

	XFreeCursor(dis,cursor->cursor);
	free(cursor);
}

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

	while (*c){
		c++;
		i++;
	}

	return i;
}


/* ---------------------------------------
 * Main Function Declarations
 * ---------------------------------------
 */

static void apply_rules(client* c);
static void attachaside(client* c);
static void change_current(client* c);
static void change_desktop(const Arg arg);
static void change_msize(const Arg arg);
static void cleanup();
static void configurenotify(XEvent* e);
static void configurerequest(XEvent* e);
static void destroynotify(XEvent* e);
static void detach(client* c);
static void draw_bar();
static int draw_bar_text(int x, int y, int w, int h, unsigned int lpad, const char* text);
static void expose(XEvent* e);
static client* find_client(Window w);
static client* find_current();
static client* find_vis_client(client* c);
static int find_occ_desktops();
static client* find_prev_client(client* c, int is_vis);
static unsigned long getcolor(const char* color);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static int gettextwidth(const char* str, int len);
static void grabkeys();
static void init_bar();
static void keypress(XEvent* e);
static void kill_client();
static void load_desktop(int i);
static void manage(Window parent, XWindowAttributes* wa);
static void maprequest(XEvent* e);
static void map_clients();
static void monocle();
static void move_focus(const Arg arg);
static void move_client(const Arg arg);
static void propertynotify(XEvent* e);
static void raise_floats();
static void save_desktop(int i);
static Clr* scheme_create(const char* clrnames[], size_t clrcount);
static void select_desktop(int i);
static void send_xkill_signal(Window w);
static void send_to_desktop(const Arg arg);
static void set_current(client* c,int desktop);
static void set_layout(const Arg arg);
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static void swap_master();
static void tile();
static void toggle_desktop(const Arg arg);
static void toggle_float();
static void toggle_fullscreen();
static void unmanage(client* c);
static void update_focus();
static void update_status();
static void view(const Arg arg);
static void youviolatedmymother();
static int xerror(Display* dis, XErrorEvent* ee);


/* ---------------------------------------
 * Make the above known
 * ---------------------------------------
 */

#include "config.h"


/* ---------------------------------------
 * "Globals"
 * ---------------------------------------
 */

static int running;
static int current_desktop;
static unsigned int seldesks;
static float master_size;
static int screen;
static int sh;
static int sw;
static unsigned long win_focus;
static unsigned long win_unfocus;

static bar* sbar;
static client* head;
static client* current;
static layout* current_layout;
static cur* cursor[cur_last];
static desktop desktops[NUMDESKTOPS];
static Display* dis;
static Window root;

static char xsetr_text[256];
static int lrpad;
static Clr** scheme;

/* Events array */
static void (*events[LASTEvent])(XEvent* e) = {
	[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[DestroyNotify] = destroynotify,
	[Expose] = expose,
	[KeyPress] = keypress,
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
/*	[UnmapNotify] = unmapnotify */
};


/* ---------------------------------------
 * Function Bodies
 * ---------------------------------------
 */

void apply_rules(client* c){
	int i;
	const rule* r;
	XTextProperty tp;
	XGetWMName(dis,c->win,&tp);

	for (i=0;i<TABLENGTH(rules);i++){
		r = &rules[i];
		if ( !r->title || (tp.value && strstr(r->title,(const char*)tp.value)) ){
			if (r->desktopmask > 0){
				c->desktops = 1 << r->desktopmask;
			}
			c->is_float = r->is_float;
		}
	}
}	

void attachaside(client* c){
	client* l;

	if (!head){
		head = c;

	} else {
		if (c->is_float){
			c->next = head->next;
			head->next = c;

		/* Else if not the first on this desktop */
		} else if (current){
			c->next = current->next;
			current->next = c;

		} else {
			for (l=head;l->next;l=l->next);
			l->next = c;
		}
	}

	XSelectInput(dis,c->win,EnterWindowMask|FocusChangeMask|StructureNotifyMask);
	change_current(c);
}

void change_current(client* c){
	/* because detach */
	if (c){
		c->is_current ^= 1 << current_desktop;
	}

	if (current){
		current->is_current ^= 1 << current_desktop;
	}
	current = c;
}

void change_desktop(const Arg arg){
	client* c, * j;

	if (arg.i == current_desktop){
		return;
	}
	seldesks = 1 << arg.i;
	load_desktop(arg.i);

	map_clients();

	current = find_current();
	if ( !current && (c = find_vis_client(head)) ){
		current = c;
	}

	current_layout->arrange();
	update_focus();
	update_status();
}

void change_msize(const Arg arg){
	master_size += ( ((master_size < 0.95*sw) && (arg.f > 0))
			|| ((master_size > 0.05*sw) && (arg.f < 0))  ) ? arg.f*sw : 0;

	current_layout->arrange();
}

/* Kill off any remaining clients
 * Free all the things
 */
void cleanup(){
	int i;
	Colormap map = DefaultColormap(dis,screen);

	while (current){
		kill_client();
	}

	/* This ain't pretty, but it gets the job done (Without memory leak? Not sure) */
	XUnmapWindow(dis,sbar->win);
	XDestroyWindow(dis,sbar->win);
	XUngrabKey(dis,AnyKey,AnyModifier,root);

	XftDrawDestroy(sbar->xd);
	XFreeGC(dis,sbar->gc);
	XftFontClose(dis,sbar->xfont);
	XFreePixmap(dis,sbar->d);
	free(sbar);

	free(current_layout);

	for (i=0;i<cur_last;i++){
		cur_free(dis,cursor[i]);
	}
	for (i=0;i<TABLENGTH(colors);i++){
		free(scheme[i]);
	}

	XFreeColors(dis,map,&win_focus,1,0);
	XFreeColors(dis,map,&win_unfocus,1,0);

	fprintf(stdout,"sara: Thanks for using!\n");
	XDestroySubwindows(dis,root);

	XSync(dis,False);
	XSetInputFocus(dis,PointerRoot,RevertToPointerRoot,CurrentTime);

        XCloseDisplay(dis);
}

/* dwm copypasta */
void configurenotify(XEvent* e){
	XConfigureEvent* ev = &e->xconfigure;

	if (ev->window == root) {
		sbar->width = ev->width;
		XFreePixmap(dis,sbar->d);
		sbar->d = XCreatePixmap(dis,root,sbar->width,sbar->height,DefaultDepth(dis,screen));

		XMoveResizeWindow(dis,sbar->win,0,0,sbar->width,sbar->height);
		draw_bar();
	}
}

/* dwm copypasta */
void configurerequest(XEvent* e){
	XConfigureRequestEvent* ev = &e->xconfigurerequest;
	XWindowChanges wc;
	wc.x = ev->x;
	wc.y = ev->y;
	wc.width = ev->width;
	wc.height = ev->height;
	wc.border_width = ev->border_width;
	wc.sibling = ev->above;
	wc.stack_mode = ev->detail;
	XConfigureWindow(dis,ev->window,ev->value_mask,&wc);
}

void destroynotify(XEvent* e){
	client* c;
	XDestroyWindowEvent* ev = &e->xdestroywindow;

	if ( (c = find_client(ev->window)) ){
		unmanage(c);
		draw_bar();
	}
}

void detach(client* c){
	client* p, * vis;
	/* Move the window out of the way first to hide it while it hangs around :) */
	XWindowAttributes wa;
	XGetWindowAttributes(dis,c->win,&wa);
	XMoveWindow(dis,c->win,-2*wa.width,wa.y);

	/* focus moves down if possible, else up */
	vis = ( (vis = find_vis_client(c->next)) ) ? vis : find_prev_client(c,1);

	/* For both, if NULL, then we're still okay */
	if ( (p = find_prev_client(c,0)) ){
		p->next = c->next;

	} else {
		head = c->next;
	}

	change_current(vis);
}

/* part dwm copypasta */
void draw_bar(){
	int x = 0, w, xsetr_text_w = 0;
	unsigned int i, occ = 0;

	/* draw status */
	sbar->scheme = scheme[sch_norm];
	xsetr_text_w = TEXTW(xsetr_text) - lrpad + 2; /* 2px right padding */
	draw_bar_text(sbar->width - xsetr_text_w,0,xsetr_text_w,sbar->height,0,xsetr_text);

	occ = find_occ_desktops();

	/* draw tags */
	for (i=0;i<TABLENGTH(tags);i++){
		/* do not draw vacant tags, but do draw selected tags regardless */
		if ( !(occ & 1 << i) && !(seldesks & 1 << i) ){
			continue;
		}

		w = TEXTW(tags[i]);
		sbar->scheme = scheme[seldesks & 1 << i ? sch_sel : sch_norm];
		draw_bar_text(x,0,w,sbar->height,lrpad / 2,tags[i]);
		x += w;
	}
	w = TEXTW(current_layout->symbol);
	sbar->scheme = scheme[sch_norm];
	x = draw_bar_text(x,0,w,sbar->height,lrpad / 2,current_layout->symbol);

	/* fill any blank space in the middle with background */
	if ( (w = sbar->width - xsetr_text_w - x) > sbar->height ){
			sbar->scheme = scheme[sch_norm];
			XSetForeground(dis,sbar->gc,sbar->scheme[ColBg].pixel);
			XFillRectangle(dis,sbar->d,sbar->gc,x,0,w,sbar->height);
	}

	XCopyArea(dis,sbar->d,sbar->win,sbar->gc,0,0,sbar->width,sbar->height,0,0);
	XSync(dis,False);
}

int draw_bar_text(int x, int y, int w, int h, unsigned int lpad, const char* text){
	int ty;

	XSetForeground(dis,sbar->gc,sbar->scheme[ColBg].pixel);
	XFillRectangle(dis,sbar->d,sbar->gc,x,y,w,h);

	ty = y + (h - (sbar->xfont->ascent + sbar->xfont->descent)) / 2 + sbar->xfont->ascent;
	XftDrawString8(sbar->xd,&sbar->scheme[ColFg],sbar->xfont,x + lpad,ty,(XftChar8*)text,slen(text));

	return x + w;
}

/* dwm copypasta */
void expose(XEvent* e){
	XExposeEvent* ev = &e->xexpose;

	if (ev->count == 0 && find_client(ev->window)){
		draw_bar();
	}
}

client* find_client(Window w){
	client* i;
	for (i=head;i;i=i->next){
		if (i->win == w){
			return i;
		}
	}

	return NULL;
}

client* find_current(){
	client* i;
	for (i=head;i;i=i->next){
		if (ISVISIBLE(i) && ((i->is_current >> current_desktop) & 1)){
			return i;
		}
	}

	return NULL;
}

int find_occ_desktops(){
	client* i;
	int occ = 0;

	for (i=head;i;i=i->next){
		occ |= i->desktops;
	}

	return occ;
}

client* find_prev_client(client* c, int is_vis){
	client* i, * ret = NULL;
	for (i=head;i && i != c;i=i->next){
		if (is_vis){
			if (ISVISIBLE(i)){
				ret = i;
			}
		}

		if (i->next == c){
			if (is_vis){
				return ISVISIBLE(i) ? i : ret;

			} else {
				return i;
			}
		}
	}

	return NULL;
}

client* find_vis_client(client* c){
	client* i;
	for (i=c;i;i=i->next){
		if (ISVISIBLE(i)){
			return i;
		}
	}
	
	return NULL;
}

unsigned long getcolor(const char* color){
	XColor c;
	Colormap map = DefaultColormap(dis,screen);

	if ( !XAllocNamedColor(dis,map,color,&c,&c) ){
		die("Error parsing color!");
	}

	return c.pixel;
}

/* dwm copypasta */
int gettextprop(Window w, Atom atom, char *text, unsigned int size){
	char** list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0){
		return 0;
	}

	text[0] = '\0';

	if (!XGetTextProperty(dis,w,&name,atom) || !name.nitems){
		return 0;
	}

	if (name.encoding == XA_STRING){
		strncpy(text,(char *)name.value,size - 1);

	} else {
		if (XmbTextPropertyToTextList(dis,&name,&list,&n) >= Success && n > 0 && *list){
			strncpy(text,*list,size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);

	return 1;
}

int gettextwidth(const char* str, int len){
	XGlyphInfo xgi;
	XftTextExtents8(dis,sbar->xfont,(XftChar8*)str,len,&xgi);

	return xgi.width;
}

void grabkeys(){
	int i;
	KeyCode code;

	for (i=0;i<TABLENGTH(keys);i++){
		if ( (code = XKeysymToKeycode(dis,keys[i].keysym)) ){
			XGrabKey(dis,code,keys[i].mod,root,True,GrabModeAsync,GrabModeAsync);
		}
	}
}

void init_bar(){
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ExposureMask
	};

	if ( !(sbar->xfont = XftFontOpenName(dis,screen,fontname)) ){
		die("The font you tried to use was not found. Check the name.");
	}

	sbar->height = sbar->xfont->ascent + sbar->xfont->descent + 2;
	sbar->width = sw;
	sbar->d = XCreatePixmap(dis,root,sw,sh,DefaultDepth(dis,screen));
	sbar->gc = XCreateGC(dis,sbar->d,0,NULL);
	sbar->xd = XftDrawCreate(dis,sbar->d,DefaultVisual(dis,screen),DefaultColormap(dis,screen));

	sbar->win = XCreateWindow(dis,root,0,0,sbar->width,sbar->height,0,
			DefaultDepth(dis,screen),InputOutput,DefaultVisual(dis,screen),
			CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask,&wa);


	XDefineCursor(dis,sbar->win,cursor[cur_norm]->cursor);
	XSelectInput(dis,sbar->win,StructureNotifyMask);
	XMapRaised(dis,sbar->win);
}


void keypress(XEvent* e){
	int i;
	XKeyEvent ke = e->xkey;
	KeySym keysym = XKeycodeToKeysym(dis,ke.keycode,0);

	for (i=0;i<TABLENGTH(keys);i++){
		if (keys[i].keysym == keysym && keys[i].mod == ke.state){
			keys[i].function(keys[i].arg);
		}
	}
}

void kill_client(){
	Window w;

	if (current){
		w = current->win;
		unmanage(current);

		send_xkill_signal(w);
	}

	update_status();
}

void load_desktop(int i){
	save_desktop(current_desktop);
	select_desktop(i);
}

void manage(Window parent, XWindowAttributes* wa){
	client* c;

	if ( !(c = ecalloc(1, sizeof(client))) ){
		die("Error while callocing new client!");
	}

	c->win = parent;
	c->is_float = 0;
	c->is_full = 0;
	c->is_current = 0;
	c->desktops = 1 << current_desktop;

	c->x = wa->x;
	c->y = wa->y;
	c->w = wa->width;
	c->h = wa->height;

	apply_rules(c);
	attachaside(c);
	/* if (c->no_focus){
	 * 	hide_client(c);
	 *	vis = ( (vis = find_vis_client(current->next)) ) ? vis : find_prev_client(current,1);
	 *	change_current(vis);
	 * }
	 * current_layout->arrange();
	 */
	XMapWindow(dis,c->win);
	draw_bar();
}

void maprequest(XEvent* e){
	XWindowAttributes wa;
	XMapRequestEvent* ev = &e->xmaprequest;

	if ( XGetWindowAttributes(dis,ev->window,&wa) && !find_client(ev->window) ){
		manage(ev->window,&wa);
		current_layout->arrange();
		update_focus();
	}
}

void map_clients(){
	client* i;
	for (i=head;i;i=i->next){
		if (ISVISIBLE(i)){
			XMapWindow(dis,i->win);

		} else {
			XUnmapWindow(dis,i->win);
		}
	}
}

void raise_floats(){
	client* i;
	for (i=head;i;i=i->next){
		if (ISVISIBLE(i) && i->is_float){
			XMoveResizeWindow(dis,i->win,i->x,i->y,i->w,i->h);
			XRaiseWindow(dis,i->win);
		}
	}
}

void monocle(){
	client* i;
	int mh = sh - sbar->height;

	raise_floats();

	for (i=head;i;i=i->next){
		if (ISVISIBLE(i) && !i->is_float){
			i->x = 0 + gap_px; i->y = sbar->height + gap_px;
			i->w = sw - 2.0*(double)gap_px;
			i->h = mh - 2.0*(double)gap_px;
		}

		XMoveResizeWindow(dis,i->win,i->x,i->y,i->w,i->h);
		XRaiseWindow(dis,i->win);
	}
}

void move_client(const Arg arg){
	client* p, * mp, * n;

	if (!current || current->is_full){
		return;
	}

	p = find_prev_client(current,0);

	/* Up stack if not head */
	if (arg.i == 1 && current != head){
		if (p == head){
			swap_master();

		} else {
			mp = find_prev_client(p,0);

			mp->next = current;
			p->next = current->next;
			current->next = p;
		}

	/* Down stack if not tail */
	} else if (arg.i == -1 && current->next){
		n = current->next;

		current->next = n->next;
		n->next = current;
		if (current == head){
			head = n;

		} else {
			p->next = n;
		}
	}

	current_layout->arrange();
	update_focus();
	draw_bar();
}

void move_focus(const Arg arg){
	client* j, * c = NULL;

	if (current && head){
		if (!current->is_full){
			/* up in stack */
			if (arg.i == 1){
				if (current == head){
					/* Save the last, visible j */
					for (j=head;j;j=j->next){
						if (ISVISIBLE(j)){
							c = j;
						}
					}

				} else {
					c = find_prev_client(current,1);
				}

			/* down in stack */
			} else if (arg.i == -1){
				if (current->next){
					c = find_vis_client(current->next);
				}
				if (!c){
					for (j=head;j && !ISVISIBLE(j);j=j->next);
					c = j;
				}
			}

			change_current(c);
			update_focus();
			draw_bar();
		}
	}
}

/* dwm copypasta */
void propertynotify(XEvent* e){
	XPropertyEvent* ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME)){
		update_status();
	}
}

void save_desktop(int i){
	desktops[i].master_size = master_size;
	desktops[i].current_layout = *current_layout;
}

/* y'all need to free() this bad boi when you cleanup */
Clr* scheme_create(const char* clrnames[], size_t clrcount){
	int i;
	Clr* sch;

	if ( !(sch = ecalloc(clrcount, sizeof(XftColor))) ){
		die("Error while trying to ecalloc for scheme_create");
	}

	for (i=0;i < clrcount;i++){
		if (!XftColorAllocName(dis,DefaultVisual(dis,screen),DefaultColormap(dis,screen),clrnames[i],&sch[i])){
			die("Error while trying to allocate color '%s'", clrnames[i]);
		}
	}

	return sch;
}

void select_desktop(int i){
	master_size = desktops[i].master_size;
	current_layout = &(desktops[i].current_layout);
	current_desktop = i;
}

void send_xkill_signal(Window w){
	XEvent ev;
	ev.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = XInternAtom(dis,"WM_PROTOCOLS",True);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XInternAtom(dis,"WM_DELETE_WINDOW",True);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(dis,w,False,NoEventMask,&ev);
}

void setup(){
	int i;
	sigchld(0);

	XSetWindowAttributes wa;

	screen = DefaultScreen(dis);
	root = RootWindow(dis,screen);

	sw = XDisplayWidth(dis,screen);
	sh = XDisplayHeight(dis,screen);
	master_size = sw*MASTER_SIZE;

	grabkeys();

	/* Vertical stack by default */
	current_layout = ecalloc(1,sizeof(layout));
	current_layout->arrange = layouts[0].arrange;
	current_layout->symbol = layouts[0].symbol;

	cursor[cur_norm] = cur_create(dis,XC_left_ptr);
	cursor[cur_move] = cur_create(dis,XC_sizing);

	win_focus = getcolor(FOCUS);
	win_unfocus = getcolor(UNFOCUS);
	scheme = ecalloc(TABLENGTH(colors),sizeof(Clr*));
	for (i=0;i < TABLENGTH(colors);i++){
		scheme[i] = scheme_create(colors[i],3);
	}

	running = 1;

	/* set up bar */
	sbar = ecalloc(1,sizeof(bar));
	init_bar();
	lrpad = sbar->xfont->ascent + sbar->xfont->descent;

	head = NULL;
	current = NULL;

	wa.cursor = cursor[cur_norm]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dis,root,CWEventMask|CWCursor,&wa);

	/* Set up all desktops, default to 0 */
	for (i=0;i<NUMDESKTOPS;i++){
		save_desktop(i);
	}
	const Arg arg = {.i = 0};
	seldesks = 1 << arg.i;
	current_desktop = arg.i;
	change_desktop(arg);

	/* Catch requests */
	XSelectInput(dis,root,wa.event_mask);
}

void send_to_desktop(const Arg arg){
	client* vis;
	if (arg.i == current_desktop){
		return;
	}

	current->desktops = 1 << arg.i;
	current->is_current = 1 << arg.i;
	set_current(current,arg.i);

	/* focus moves down if possible, else up */
	vis = ( (vis = find_vis_client(current->next)) ) ? vis : find_prev_client(current,1);

	XUnmapWindow(dis,current->win);
	XSync(dis,False);

	change_current(vis);
	current_layout->arrange();
	update_focus();
	update_status();
}

void set_current(client* c,int desktop){
	client* i;
	for (i=head;i;i=i->next){
		if (i != c && ((i->is_current >> desktop) & 1)){
			i->is_current ^= 1 << desktop;
		}
	}
}


void set_layout(const Arg arg){
	current_layout = (layout *)arg.v;
	current_layout->arrange();
	draw_bar();
}

/* dwm copypasta */
void sigchld(int unused){
	if (signal(SIGCHLD,sigchld) == SIG_ERR){
		die("Can't install SIGCHLD handler");
	}
	while (0 < waitpid(-1,NULL,WNOHANG));
}

void spawn(const Arg arg){
	if (fork() == 0){
		if (fork() == 0){
			if (dis){
				close(ConnectionNumber(dis));
			}
			setsid();
			execvp((char*)arg.com[0],(char**)arg.com);
		}

		exit(0);
	}
}

/* dwm copypasta */
void start(){
	XEvent ev;

	while (running && !XNextEvent(dis,&ev)){
		if (events[ev.type]){
			events[ev.type](&ev);
		}
	}
}

void swap_master(){
	client* tmp;
	client* p;

	if (head && current && current != head){
		tmp = (head->next == current) ? head : head->next;
		p = find_prev_client(current,0);

		/* if p is head, this gets overwritten - saves an if statement */
		p->next = head;
		head->next = current->next;
		current->next = tmp;
		head = current;

		current_layout->arrange();
		update_focus();
	}
}

void tile(){
	client* i, * nf = NULL;
	int n = 0;
	int local_gap_px = gap_px;
	int available;
	int mh = sh - sbar->height;
	int y = sbar->height + gap_px;
	int x_off = 0.5*(double)gap_px;
	int y_off = 0;

	/* Find the first non-floating, visible window and tally non-floating, visible windows */
	for (i=head;i;i=i->next){
		if (!i->is_float && ISVISIBLE(i)){
			nf = (!nf) ? i : nf;
			n++;
		}
	}

	/* Raise any floating windows */
	raise_floats();

	if (nf && n == 1){
		nf->x = gap_px; nf->y = y;
		nf->w = sw - 2.0*(double)gap_px;
		nf->h = mh - 2.0*(double)gap_px;
		XMoveResizeWindow(dis,nf->win,nf->x,nf->y,nf->w,nf->h);

	} else if (nf){
		/* so having a master doesn't affect stack splitting */
		n--;

		/* Master window */
		nf->x = gap_px; nf->y = y;
		nf->w = master_size - 1.5*(double)gap_px;
		nf->h = mh - 2.0*(double)gap_px;
		XMoveResizeWindow(dis,nf->win,nf->x,nf->y,nf->w,nf->h);

		y_off = (mh - local_gap_px)/n;
		available = sh - gap_px - (sbar->height + gap_px);
		/* if average client height < 0.05*available, reduce gap until it won't be */
		while ( (y_off - local_gap_px) < (0.05 * available)){
			local_gap_px--;
		}
		y_off = (mh - local_gap_px)/n;

		/* Stack */
		/* if you draw a picture, this will make sense */
		for (i=nf->next;i;i=i->next){
			if (ISVISIBLE(i) && !i->is_float){
				i->x = master_size + x_off;
				i->y = y;
				i->w = sw - master_size - 1.5*(double)gap_px;
				/* make the bottom nice and flush */
				//i->h = (i->next) ? y_off - local_gap_px : sh - gap_px - y;
				i->h = y_off - local_gap_px;
				XMoveResizeWindow(dis,i->win,i->x,i->y,i->w,i->h);

				y += y_off;
			}
		}
	}
}

void toggle_desktop(const Arg arg){
	unsigned int new_desktops;
	client* vis;

	if (!current){
		return;
	}

	new_desktops = current->desktops ^ (1 << arg.i);
	if (new_desktops){
		current->desktops = new_desktops;
		current->is_current ^= 1 << arg.i;

		set_current(current,arg.i);

		if ( !((current->desktops >> current_desktop) & 1) ){
			/* focus moves down if possible, else up */
			vis = ( (vis = find_vis_client(current->next)) ) ? vis : find_prev_client(current,1);

			XUnmapWindow(dis,current->win);
			XSync(dis,False);
			change_current(vis);
		}

		current_layout->arrange();
		update_focus();
		update_status();
	}
}

void toggle_float(){
	XWindowChanges wc;
	client* i;

	current->is_float = !current->is_float;
	current_layout->arrange();

	if (current->is_float){
		wc.sibling = current->win;
		wc.stack_mode = Below;

		for (i=head;i;i=i->next){
			XConfigureWindow(dis,i->win,CWSibling|CWStackMode,&wc);
		}
	}

	if (!current->is_float){
		wc.sibling = sbar->win;
		wc.stack_mode = Below;
		XConfigureWindow(dis,current->win,CWSibling|CWStackMode,&wc);
	}
}

void toggle_fullscreen(){
	current->is_full = !current->is_full;
	/* a pecularity of my implementation - will remain as such unless I decide to implement oldx, oldy, etc. for clients */
	current->is_float = 0;

	if (current->is_full){
		current->x = 0; current->y = 0;
		current->w = sw; current->h = sh;
		XMoveResizeWindow(dis,current->win,current->x,current->y,current->w,current->h);
		XRaiseWindow(dis,current->win);

		XUnmapWindow(dis,sbar->win);

	} else {
		current_layout->arrange();

		XMapRaised(dis,sbar->win);
		draw_bar();
	}
}

void unmanage(client* c){
	detach(c);
	free(c);
	current_layout->arrange();
	update_focus();
}

void update_focus(){
	client* i;

	for (i=head;i;i=i->next){
		if (i == current){
			if (gap_px > 0){
				XSetWindowBorderWidth(dis,i->win,1);
				XSetWindowBorder(dis,i->win,win_focus);
			}
			XSetInputFocus(dis,i->win,RevertToPointerRoot,CurrentTime);

		} else if (ISVISIBLE(i)){
			if (gap_px > 0){
				XSetWindowBorder(dis,i->win,win_unfocus);
			}
		}
	}
}

/* dwm copypasta */
void update_status(){
	if (!gettextprop(root,XA_WM_NAME,xsetr_text,sizeof(xsetr_text))){
		strcpy(xsetr_text,"sara-beta-leedle");
	}

	draw_bar();
}

void view(const Arg arg){
	client* j;

	if (arg.i == current_desktop){
		return;
	}

	seldesks ^= 1 << arg.i;

	map_clients();

	current_layout->arrange();
	update_focus();
	update_status();
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
	if ( !(dis = XOpenDisplay(NULL)) ){
		die("Cannot open display!");
	}

	XSetErrorHandler(xerror);
	setup();

	start();

	cleanup();

	return 0;
}
