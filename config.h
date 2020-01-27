/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch              
 * Copyright (c) 2020, This Fackin Guy, gitluin on github (no email for you!)     
 *                                                                                
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 


/* ---------------------------------------
 * Defines
 * ---------------------------------------
 */

#ifndef CONFIG_H
#define CONFIG_H

#define MOD             Mod4Mask
#define MASTER_SIZE     0.55

static const char* fontname	    = "Noto Sans:size=10";
/* Normal theme! */
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#bbbbbb";
static const char col_gray3[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";

/* Winter theme! */
//static const char col_gray1[]       = "#DBDBDB";
//static const char col_gray2[]       = "#383838";
//static const char col_gray3[]       = "#000000";
//static const char col_cyan[]        = "#0090C9";
/* dwm copypasta */
static const char* colors[][3]      = {
	/*               fg         bg		*/
	[SchNorm] = { col_gray2, col_gray1 },
	[SchSel]  = { col_gray3, col_cyan  },
};

/* dwm copypasta */
static const char* tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
/* indicators for selected tags */
static const char* syms[] = { "<", ">" };

/* dwm copypasta */
static const rule rules[] = {
	/* WM_CLASS(STRING) = instance, class
	 * WM_NAME(STRING) = title
	 * If tag is -1, then the assigned tag is whatever the current tag is
	 * Else send spawnee to that desktop
	 */

	/* title      desktopmask     is_float    */
	{ "rcalc",    -1,		1, },
	{ "cal",      -1,		1, },
	// Any jerry-rigged shortcuts go to the 9th space
	{ "funkey",   8,		1, },
};

/* Layouts */
/* dwm copypasta */
static const layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile     },    /* first entry is default */
	{ "[M]",      monocle  },
};

/* change desktop, add to desktop, move to only desktop */
/* dwm copypasta */
#define DESKTOPCHANGE(K,N) \
    {  MOD,             	    K,	changedesktop, 		{.i = N}}, \
    {  MOD|ShiftMask,   	    K,  toggledesktop, 		{.i = N}}, \
    {  MOD|ControlMask,   	    K,  view,		 	{.i = N}}, \
    {  MOD|ControlMask|ShiftMask,   K,  todesktop, 		{.i = N}},

#define XK_BDOWN 	XF86XK_MonBrightnessDown
#define XK_BUP 		XF86XK_MonBrightnessUp
#define XK_AUDIOMUTE 	XF86XK_AudioMute
#define XK_AUDIODOWN 	XF86XK_AudioLowerVolume
#define XK_AUDIOUP 	XF86XK_AudioRaiseVolume


/* ---------------------------------------
 * Appearance
 * ---------------------------------------
 */

static const char dmenufont[]	= "Misc Termsyn:size=10";


/* ---------------------------------------
 * Commands
 * ---------------------------------------
 */

//static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
//const char* dmenucmd[]	= { "dmenu_run", "-m", dmenumon, "-b", "-l", "4", "-p", "Run", "-fn", dmenufont, "-nb", "#282a36", "-nf", "#f8f8f2", "-sb", "#1E88E5", "-sf", "#202020" , NULL};

/* Program Spawning Keys */
/* dwm copypasta */
const char* dmenucmd[]		= { "dmenu_run", "-m", "0", "-b", "-l", "4", "-p", "Run", "-fn", dmenufont, "-nb", "#282a36", "-nf", "#f8f8f2", "-sb", "#1E88E5", "-sf", "#202020" , NULL};
const char* termcmd[] 		= { "st", NULL};
const char* browscmd[] 	 	= { "firefox", NULL };
const char* fbrowscmd[]		= { "st", "-e", "vifm", NULL };
const char* rcalccmd[]  	= { "st", "-t", "rcalc",    "-g", "80x24+625+325", "-e", "R", "--vanilla", "-q", NULL };
const char* calcmd[] 	 	= { "st", "-t", "cal",      "-g", "26x14+855", "-e", "check_cal", NULL };

/* Function Keys */
const char* brightdown[]  	= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_bright.sh", "-U", "5", NULL };
const char* brightup[]	  	= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_bright.sh", "-A", "5", NULL };
const char* brightdownsmall[]  	= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_bright.sh", "-U", "1", NULL };
const char* brightupsmall[]  	= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_bright.sh", "-A", "1", NULL };
const char* volmute[] 		= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_audio.sh", "mute", "NULL", NULL };
const char* voldown[] 		= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_audio.sh", "-", "5", NULL };
const char* volup[]  		= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_audio.sh", "+", "5", NULL };
const char* voldownsmall[]  	= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_audio.sh", "-", "1", NULL };
const char* volupsmall[]  	= { "st", "-t", "funkey", "-e", "/bin/sbar/sbar_audio.sh", "+", "1", NULL };
const char* prtsc[]	  	= { "st", "-t", "funkey", "-e", "scrot", "-q", "100", NULL };


/* ---------------------------------------
 * Keybindings
 * ---------------------------------------
 */

static struct key keys[] = {
	/* Commands from DESKTOPCHANGE function
	 * MOD + KEY 					-> go to desktop
	 * MOD|ShiftMask + KEY				-> toggle client on desktop
	 * MOD|ControlMask + KEY			-> add desktop to current view
	 * MOD|ControlMask|ShiftMask + KEY		-> send client to just desktop
	 * MOD              	     KEY        FUNCTION        	ARGS
	 */
	{ MOD|ShiftMask,             XK_q,      	killclient,    		{0} },
	{ MOD|ShiftMask,             XK_e,      	youviolatedmymother,	{0} },
	{ MOD,             	     XK_j,      	movefocus,    		{.i = -1} },
	{ MOD,             	     XK_k,      	movefocus,    		{.i = 1} },
	{ MOD|ShiftMask,             XK_j,      	moveclient,      	{.i = -1} },
	{ MOD|ShiftMask,   	     XK_k,      	moveclient,      	{.i = 1} },
	{ MOD|ShiftMask,             XK_space,  	togglefloat,   		{0} },
	{ MOD|ControlMask,	     XK_t,      	setlayout,      	{.v = &layouts[0]} },
	{ MOD|ControlMask,	     XK_m,      	setlayout,      	{.v = &layouts[1]} },
	{ MOD,             	     XK_Return, 	swapmaster,   		{0} },
	/* Go fullscreen, my dude */
	{ MOD|ShiftMask,	     XK_Return,		togglefs,		{0} },
	/* Custom commands */
	{ MOD,                       XK_d,      	spawn,         		{.v = dmenucmd } },
	{ MOD,	                     XK_t, 		spawn,         		{.v = termcmd } },
	{ MOD,	                     XK_f, 		spawn,         		{.v = fbrowscmd } },
	{ MOD,                       XK_w,      	spawn,         		{.v = browscmd } },
	{ MOD,                       XK_r,      	spawn,         		{.v = rcalccmd } },
	{ MOD,                       XK_c,      	spawn,         		{.v = calcmd } },
	/* Brightness keys */
	{ 0,                         XK_BDOWN,		spawn,         		{.v = brightdown } },
	{ 0,                         XK_BUP,  		spawn,         		{.v = brightup } },
	{ ShiftMask,                 XK_BDOWN,		spawn,         		{.v = brightdownsmall } },
	{ ShiftMask,                 XK_BUP,  		spawn,         		{.v = brightupsmall } },
	/* Volume keys */
	{ 0,                         XK_AUDIOMUTE,  	spawn,         		{.v = volmute } },
	{ 0,                         XK_AUDIODOWN,  	spawn,         		{.v = voldown } },
	{ 0,                         XK_AUDIOUP,    	spawn,         		{.v = volup } },
	{ ShiftMask,                 XK_AUDIODOWN,  	spawn,         		{.v = voldownsmall } },
	{ ShiftMask,                 XK_AUDIOUP,    	spawn,         		{.v = volupsmall } },
	{ 0,                         XK_Print,  	spawn,         		{.v = prtsc } },
	/* Control size of master area (by 5 percent increments) */
	{ MOD,             	     XK_h,      	changemsize,  		{.f = -0.05 } },
	{ MOD,             	     XK_l,      	changemsize,  		{.f = 0.05 } },

	/* To implement */
	// Send focused window to previous and next monitor
//	{ MOD|ShiftMask,             XK_comma,  tagmon,         	{.i = -1 } },
//	{ MOD|ShiftMask,             XK_period, tagmon,         	{.i = +1 } },
//	// Switch between monitors
//	{ MOD,                       XK_comma,  focusmon,       	{.i = -1 } },
//	{ MOD,                       XK_period, focusmon,       	{.i = +1 } },

	/* Movin aboot */
	DESKTOPCHANGE(	 	     XK_1,                      	0)
	DESKTOPCHANGE(   	     XK_2,                      	1)
	DESKTOPCHANGE(   	     XK_3,                      	2)
	DESKTOPCHANGE(   	     XK_4,                      	3)
	DESKTOPCHANGE(   	     XK_5,                      	4)
	DESKTOPCHANGE(   	     XK_6,                      	5)
	DESKTOPCHANGE(   	     XK_7,                      	6)
	DESKTOPCHANGE(   	     XK_8,                      	7)
	/* Banished for function keys */
/*	DESKTOPCHANGE(   	     XK_9,                      	8) */
};

#endif
