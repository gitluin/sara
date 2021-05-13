/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#ifndef CONFIG_H
#define CONFIG_H

/* ---------------------------------------
 * Defines
 * ---------------------------------------
 */

#define NUMTAGS		9
#define MOUSEMOD	Mod4Mask
#define MASTER_SIZE     0.55


/* vertical space allotted for your bar of choice */
static int barpx			= 20;
static int bottombar			= 0;
static int gappx			= 10;
static int corner_radius		= 10;
/* once within snappx of a monitor edge, snap to the edge */
static unsigned int snappx		= 32;


//static const rule rules[] = {
//	/* WM_CLASS(STRING) = instance, class
//	 * WM_NAME(STRING) = title
//	 * tags mask 0 means it takes whatever the current tags are
//	 * else, go to that tag
//	 * monitor of -1 means spawn on focused monitor
//	 */
//
//	/* class      instance    title       tags mask     isfloat   isfull	monitor */
//	/* script for taking quick notes */
//	{ "st",       NULL,	  "scratch",  0,            1,	      1,	-1 },
//};

/* Layouts */
static const layout layouts[] = {
	/* letter	function	name	   */
	{ 'T',		tile,		"tile"     },    /* first entry is default */
	{ 'M',		monocle,	"monocle"  },
	{ 'F',		floaty,		"floaty"   },
};

static button buttons[] = {
	/* event mask     button          function        argument */
	{ MOUSEMOD,       Button1,        manipulate,     {.i = WantMove} },
	{ MOUSEMOD,       Button3,        manipulate,     {.i = WantResize} },
};

#endif
