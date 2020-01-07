/*                                                                                
*  SARA Window Manager                                                            
*  ______________________________________________________________________________ 
*                                                                                 
*  Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch              
*  Copyright (c) 2020, This Fackin Guy, gitluin on github (no email for you!)     
*                                                                                 
*  Permission is hereby granted, free of charge, to any person obtaining a        
*  copy of this software and associated documentation files (the "Software"),     
*  to deal in the Software without restriction, including without limitation      
*  the rights to use, copy, modify, merge, publish, distribute, sublicense,       
*  and/or sell copies of the Software, and to permit persons to whom the          
*  Software is furnished to do so, subject to the following conditions:           
*                                                                                 
*  The above copyright notice and this permission notice shall be included in     
*  all copies or substantial portions of the Software.                            
*                                                                                 
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL       
*  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING        
*  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER            
*  DEALINGS IN THE SOFTWARE.                                                      
*                                                                                 
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
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";

/* Winter theme! */
/* dwm copypasta */
//static const char col_gray1[]       = "#DBDBDB";
//static const char col_gray2[]       = "#444444";
//static const char col_gray3[]       = "#383838";
//static const char col_gray4[]       = "#000000";
//static const char col_cyan[]        = "#0090C9";
static const char* colors[][3]      = {
	/*               fg         bg         border   */
	[sch_norm] = { col_gray3, col_gray1, col_gray2 },
	[sch_sel]  = { col_gray4, col_cyan,  col_cyan  },
};

/* dwm copypasta */
static const char* tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

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
static const layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile     },    /* first entry is default */
	{ "[M]",      monocle  },
};

/* Colors */
#define FOCUS           "rgb:bc/57/66"
#define UNFOCUS         "rgb:88/88/88"

/* change desktop, add to desktop, move to only desktop */
#define DESKTOPCHANGE(K,N) \
    {  MOD,             	    K,	change_desktop, 	{.i = N}}, \
    {  MOD|ShiftMask,   	    K,  toggle_desktop, 	{.i = N}}, \
    {  MOD|ControlMask,   	    K,  view,		 	{.i = N}}, \
    {  MOD|ControlMask|ShiftMask,   K,  send_to_desktop, 	{.i = N}},
/* My workarounds */
#define BRTDOWNKEY 0x1008ff03
#define BRTUPKEY 0x1008ff02
#define VMUTEKEY 0x1008ff12
#define VDOWNKEY 0x1008ff11
#define VUPKEY 0x1008ff13


/* ---------------------------------------
 * Appearance
 * ---------------------------------------
 */

//const int gap_px = 50;
const int gap_px = 0;

static const char dmenufont[]	= "Misc Termsyn:size=10";


/* ---------------------------------------
 * Commands
 * ---------------------------------------
 */

//static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */

//const char* dmenucmd[]	= { "dmenu_run", "-m", dmenumon, "-b", "-l", "4", "-p", "Run", "-fn", dmenufont, "-nb", "#282a36", "-nf", "#f8f8f2", "-sb", "#1E88E5", "-sf", "#202020" , NULL};
const char* dmenucmd[]		= { "dmenu_run", "-m", "0", "-b", "-l", "4", "-p", "Run", "-fn", dmenufont, "-nb", "#282a36", "-nf", "#f8f8f2", "-sb", "#1E88E5", "-sf", "#202020" , NULL};
const char* termcmd[] 		= { "st", NULL};
const char* browscmd[] 	 	= { "firefox", NULL };
const char* fbrowscmd[]		= { "st", "-e", "vifm", NULL };
const char* rcalccmd[]  	= { "st", "-t", "rcalc",    "-g", "80x24+625+325", "-e", "R", "--vanilla", "-q", NULL };
const char* calcmd[] 	 	= { "st", "-t", "cal",      "-g", "26x14+855", "-e", "check_cal", NULL };
//const char* calcmd[] 	 	= { "st", "-e", "sl", NULL };
// Function key commands
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
	{ MOD|ShiftMask,             XK_q,      	kill_client,    	{NULL} },
	{ MOD|ShiftMask,             XK_e,      	youviolatedmymother,	{NULL} },
	{ MOD,             	     XK_j,      	move_focus,    		{.i = -1} },
	{ MOD,             	     XK_k,      	move_focus,    		{.i = 1} },
	{ MOD|ShiftMask,             XK_j,      	move_client,      	{.i = -1} },
	{ MOD|ShiftMask,   	     XK_k,      	move_client,      	{.i = 1} },
	{ MOD|ShiftMask,             XK_space,  	toggle_float,   	{NULL} },
	{ MOD|ControlMask,	     XK_t,      	set_layout,      	{.v = &layouts[0]} },
	{ MOD|ControlMask,	     XK_m,      	set_layout,      	{.v = &layouts[1]} },
	{ MOD,             	     XK_Return, 	swap_master,   		{NULL} },
	/* Go fullscreen, my dude */
	{ MOD|ShiftMask,	     XK_Return,		toggle_fullscreen,	{NULL} },
	/* Custom commands */
	{ MOD,                       XK_d,      	spawn,         		{.com = dmenucmd } },
	{ MOD,	                     XK_t, 		spawn,         		{.com = termcmd } },
	{ MOD,	                     XK_f, 		spawn,         		{.com = fbrowscmd } },
	{ MOD,                       XK_w,      	spawn,         		{.com = browscmd } },
	{ MOD,                       XK_r,      	spawn,         		{.com = rcalccmd } },
	{ MOD,                       XK_c,      	spawn,         		{.com = calcmd } },
	/* Brightness keys */
	{ 0,                         BRTDOWNKEY,	spawn,         		{.com = brightdown } },
	{ 0,                         BRTUPKEY,  	spawn,         		{.com = brightup } },
	{ ShiftMask,                 BRTDOWNKEY,	spawn,         		{.com = brightdownsmall } },
	{ ShiftMask,                 BRTUPKEY,  	spawn,         		{.com = brightupsmall } },
	/* Volume keys */
	{ 0,                         VMUTEKEY,  	spawn,         		{.com = volmute } },
	{ 0,                         VDOWNKEY,  	spawn,         		{.com = voldown } },
	{ 0,                         VUPKEY,    	spawn,         		{.com = volup } },
	{ ShiftMask,                 VDOWNKEY,  	spawn,         		{.com = voldownsmall } },
	{ ShiftMask,                 VUPKEY,    	spawn,         		{.com = volupsmall } },
	{ 0,                         XK_Print,  	spawn,         		{.com = prtsc } },
	/* Control size of master area (by 5 percent increments) */
	{ MOD,             	     XK_h,      change_msize,  		{.f = -0.05 } },
	{ MOD,             	     XK_l,      change_msize,  		{.f = 0.05 } },

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
