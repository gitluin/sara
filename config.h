/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch              
 * Copyright (c) 2020, This Fackin Guy, gitluin on github (no email for you!)     
 *                                                                                
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#ifndef CONFIG_H
#define CONFIG_H

/* ---------------------------------------
 * Defines
 * ---------------------------------------
 */

#define MOUSEMOD	Mod4Mask
#define MASTER_SIZE     0.55


/* ---------------------------------------
 * Appearance
 * ---------------------------------------
 */

/* vertical space allotted for your bar of choice */
static const int barpx			= 16;
static const char* fontname		= "Noto Sans:size=10";
/* Normal theme! */
static const char col_gray1[]		= "#222222";
static const char col_gray2[]		= "#bbbbbb";
static const char col_gray3[]		= "#eeeeee";
static const char col_cyan[]		= "#005577";

/* Winter theme! */
//static const char col_gray1[]		= "#DBDBDB";
//static const char col_gray2[]       	= "#383838";
//static const char col_gray3[]       	= "#000000";
//static const char col_cyan[]        	= "#0090C9";
static const char* colors[][2]		= {
	/*               fg         bg		*/
	[SchNorm] = { col_gray2, col_gray1 },
	[SchSel]  = { col_gray3, col_cyan  },
};

static const char* tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
/* indicators for selected tags */
static const char* syms[] = { "<", ">" };


/* ---------------------------------------
 * Behavior
 * ---------------------------------------
 */

/* once within 32 pixels of a monitor edge, snap to the edge */
static const unsigned int snap = 32;

static const rule rules[] = {
	/* WM_CLASS(STRING) = instance, class
	 * WM_NAME(STRING) = title
	 * If tag is -1, then the assigned tag is whatever the current tag is
	 * Else send spawnee to that desktop
	 */

	/* class      instance    title       tags mask     isfloat   isfull	monitor */
	{ "st",       NULL,       "rcalc",    0,            1,        0,	-1 },
	{ "st",       NULL,       "cal",      0,            1,        0,	-1 },
	{ "st",       NULL,	  "scratch",  0,            1,	      1,	-1 },
	// Any jerry-rigged shortcuts go to the 9th space
	{ "st",       NULL,       "funkey",   1 << 8,       1,        0,	-1 },
};

/* Layouts */
static const layout layouts[] = {
	/* symbol	function	name	   */
	{ "[]=",	tile,		"tile"     },    /* first entry is default */
	{ "[M]",	monocle,	"monocle"  },
};

/* dwm copypasta */
static button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkWin,         	MOUSEMOD,       Button1,        manipulate,     {WantMove} },
	{ ClkWin,         	MOUSEMOD,       Button3,        manipulate,     {WantResize} },
};

#endif
