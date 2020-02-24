sara
=====

        .-. .-.
      ..:  |  ;,.
     (___`.|.'___)
     (   .'|`.   )
      `'/  |  \`'
        `-' `-'                                                                    hjw, ascii.co.uk/art 
What Is, Why Do
-------
sara is a very simple and lightweight tiling window manager, with the goal of making catwm actually work and paring down dwm. I will try to keep the source code as small and readable as possible.

Recent Statii
------
 * v0.9.7.6	- Code cleanup. No more xsendkill - don't be nice, just XKillClient. Some other nice dwmizations.
 * v0.9.7.5	- Monitor list will be reordered by x origin as necessary. Pls don't look at my insertion sort code.
 * v0.9.7.4	- More TODO finishers. No more multiple GC, Drawable, etc. across all bars. I have my own mini-drw struct now.
 * v0.9.7.3	- Floats are always on top. Not sure if it's the most efficient way (just call raisefloats), but it works and as long as you don't have 8 bajillion windows you should be okay.
 * v0.9.7.2	- Bug fixes. TODOs slowly being knocked out.
 * v0.9.7.1	- Transient popup windows (Libreoffice, Firefox download, etc.) float like they ought to, instead of being tiled. This solved the Krita bug too. Yay!
 * v0.9.7	- Implemented rules! Just need to test that sending to a monitor works correctly. focusmon bug fixed.
 * v0.9.2.{0-7} - Multiple monitors implemented, viewall added, reduction in global variable count, bar drawing fixes, enternotify improved, moveclient works by visibility, bugs with togglefs, send clients tomon.
 * v0.8.{1-7.7} - No drw and no alpha behavior for the bar. Can send clients to just a desktop, can view multiple desktops at once, enternotify works (maybe), gaps, window borders, bar boxes removed. Code cleanup. Memory freeing fixed. buttonpress works!
 * v0.7		- The dwm way of clients has been implemented, bug-free (I think)!
 * v0.6.{0-5}	- There's a bar with alpha support! Tags in the bar are updated properly by client movement, floating windows always appear raised, manual fullscreen toggling, no more swindows (the buggy version).
 * v0.5.{0-5}	- Windows can be destroyed any damn way, name change, better master_size, modular layouts, gaps implemented, individual client floating behavior.
 * v0.2.{0-8}	- Tagging works (no asterisks), no phantom windows, revamp of data structures, variable gap sizes, improvements to C programming (non-Xlib specific stuff), swindows are the true owners of Windows.
 * v0.1.{0-5}	- It compiles, there's a cursor icon, quit()ing is functional (but at what cost?), moving clients to other desktops works, tagging to other desktops "works" (it compiles & no segfaults), there are phantom windows after removing clients from the current desktop.

Features
-----

### Modes

* Tiling mode (master window with right-hand vertical stack)
* Monocle mode
* Fullscreen mode (see limitations)

### Behavior

* dwm-like: tags
* patch-like:
	* attachaside
	* emptyview
	* hide vacant tags
	* movestack
	* pertag-like: layouts, master_size
	* singular borders
* worst

### Design Limitations

* No support for urgency coloring in the bar, because nothing I do is urgent.
* No [ICCCM](https://web.archive.org/web/20190617214524/https://raw.githubusercontent.com/kfish/xsel/1a1c5edf0dc129055f7764c666da2dd468df6016/rant.txt) (found thanks to [dylan](https://github.com/dylanaraps/sowm)). This is mostly felt in the lack of the applysizehints behavior that dwm has (ex. cmatrix won't redraw using larger window bounds if you give it more space via togglefs, changemsize, etc.)
* No EWMH support, because fullscreening manually isn't so bad (see config.h).
* One bar. I don't like to look to a particular monitor for the time, just the same spot on any monitor.
* No unicode support. If you provide unicode characters to the bar, behavior is undefined. I get junk for symbols. You might crash. Behave yourself.

### Why Use Sara Over Other Small WM, X?

A few examples I was able to find:

* [dminiwm](https://github.com/moetunes/dminiwm) No status bar. Also, some of the code: compare add\_window() to the combination of manage(), attachaside(), etc.
* [berry](https://github.com/JLErvin/berry) CLI-config vs. config.h. Extremely standards-compliant. Codebase is much larger as a consequence (wm.c is 1912 SLOC).
* [monsterwm](https://github.com/c00kiemon5ter/monsterwm) No status bar (I appreciate the UNIX philosophy adherence, though). Not a fan of the code formatting, but not a big deal unless you're in my position/writing patches.

Installation
------------
Need Xlib, then sudo make install clean

Keys
------------

| Key				| Function 						|
| -----------------------------	| -----------------------------------------------------	|
| Mod+NUMKEY			| View desktop NUMKEY					|
| Mod+Shift+NUMKEY		| Toggle focused client to/from desktop NUMKEY		|
| Mod+Control+NUMKEY		| Toggle desktop NUMKEY to/from view			|
| Mod+Shift+Control+NUMKEY	| Put focused client on only desktop NUMKEY		|
| Mod+XK\_0			| View all desktops					|
| Mod+K				| Move focus up stack					|
| Mod+J				| Move focus down stack					|
| Mod+Shift+K			| Move focused client up stack				|
| Mod+Shift+J			| Move focused client down stack			|
| Mod+Return			| Move focused client to master				|
| Mod+Shift+Space		| Toggle focused client to floating			|
| Mod+Shift+Return		| Toggle focused client to fullscreen			|
| Mod+Comma			| Move focus to previous monitor			|
| Mod+Period			| Move focus to next monitor				|
| Mod+Shift+Comma		| Move focused client to previous monitor		|
| Mod+Shift+Period		| Move focused client to next monitor			|
| Mod+Shift+Q			| Kill focused client ("force quit")			|
| Mod+H				| Decrease master area					|
| Mod+L				| Increase master area					|
| Mod+Control+T			| Change layout to tiling				|
| Mod+Control+M			| Change layout to monocle				|
| Mod+Shift+E			| Quit sara (shuts down X)				|
| Mod+T				| Spawn st						|
| Mod+W				| Spawn firefox						|
| Mod+F				| Spawn vifm						|
| Mod+R				| Spawn R session					|
| Mod+C				| Spawn calendar					|
| Mod+D				| Spawn dmenu						|

Bugs
----
 * Bar doesn't redraw on laptop lid reopen until maprequest, changedesktop, etc. (also a problem with dwm)
 * Need to XFree() in applyrules, I think? T'is unclear.
 * buttonpress doesn't draw focus to new monitors, and sometimes not to new windows, but unsure when.
 * dmenu spawn does not work on monitor focusing. _is this because of the renumbering?_

To Do
----
 * Continuous:
   * No bugs (just use [XGH](https://gist.github.com/banaslee/4147370)) (required for v1.0)
   * Code cleanup (as a constant) - function naming conventions.
   * Does it leak memory?
 * Discrete:
   * Finish TODOs (required for v1.0)
   * Implement mouse move, resize support for clients (0.3 points)?
   * My own art.
   * Tutorial.

If you have some particular request, don't send me an e-mail, I won't do it!
