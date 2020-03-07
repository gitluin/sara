#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif


#define TEXTW(M,X)			(gettextwidth(M, X, slen(X)) + lrpad)


enum { SchNorm, SchSel };
enum { ColFg, ColBg };


/* ---------------------------------------
 * Structs
 * ---------------------------------------
 */

struct bar {
	int w, h;
	Window win;
};


/* ---------------------------------------
 * "Globals"
 * ---------------------------------------
 */

/* X Interfacing */
static Display* dis;
static Window root;
static int screen;
static int sh;
static int sw;
/* Bar Business */
static int lrpad;
static int running;
static XftColor** scheme;
static drw* sdrw;
static char xsetr_text[256];


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
 * Main Functions
 * ---------------------------------------
 */

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

void updatestatus(){
	if (!gettextprop(root, XA_WM_NAME, xsetr_text, sizeof(xsetr_text)))
		strcpy(xsetr_text, "where's the leak, ma'am?");

	drawbars();
}

int main(){
	if ( !(dis = XOpenDisplay(NULL)) )
		die("Cannot open display!");

	screen = DefaultScreen(dis);
	root = RootWindow(dis, screen);

	sw = XDisplayWidth(dis, screen);
	sh = XDisplayHeight(dis, screen);

	scheme = ecalloc(TABLENGTH(colors), sizeof(XftColor*));
	for (i=0;i < TABLENGTH(colors);i++) scheme[i] = createscheme(colors[i], 2);

	running = 1;

	initdrw();

	// initialize monitors
	
	// initialize bars

	while (running){
		// 


	}

	return 0;
}
