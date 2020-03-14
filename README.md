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
sara is a simple and lightweight tiling window manager. Originally started with the goal of making catwm actually work, it's now a slimmed down descendant of dwm with a bit of bspwm (sockets) for good measure. I will try to keep the source code as small and readable as possible.

Recent Statii
------
 * v2.0		- Whole lotta UNIX philosophy up in here! Keybinds are controlled by sxhkd now! A little bit of bspwm in your dwm and sara sandwich! Pointer events (click into window, move and resize window) still handled by sara.
 * v1.0.{0-1} 	- Finished it enough to share with the class. Fixed a changecurrent bug.
 * v0.9.{2-9.5}	- Multiple monitor support, viewall, reduced redundant variables, solved many enternotify edge cases, moveclient works by visibility, updategeom a la dwm 6.1, implemented rules, transient popup support, proper client killing, proper float stacking, code cleanup, mouse move and resize, lots of time wasted on easy (but sometimes obscure) bugs.
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

### Behavior/Traits
* dwm-like:
	* tags
	* Multihead support through Xinerama
	* patch-like:
		* attachaside
		* emptyview
		* hide vacant tags
		* movestack
		* pertag-like: layouts, master_size
		* singular borders
* bspwm-like:
	* keyboard handling via external program (like sxhkd)
* worst

### Design Limitations
* No support for urgency coloring in the bar, because nothing I do is urgent.
* No [ICCCM](https://web.archive.org/web/20190617214524/https://raw.githubusercontent.com/kfish/xsel/1a1c5edf0dc129055f7764c666da2dd468df6016/rant.txt) (found thanks to [dylan](https://github.com/dylanaraps/sowm)). This is mostly felt in the lack of the applysizehints behavior that dwm has (ex. cmatrix won't redraw using larger window bounds if you give it more space via togglefs, changemsize, etc.)
* No EWMH support, because fullscreening manually isn't so bad (see config.h).
* One bar. I don't like to look to a particular monitor for the time, just the same spot on any monitor.
* No unicode support. If you provide unicode characters to the bar, behavior is undefined. I get junk for symbols. You might crash. Behave yourself.
* No support for doing things through CapsLock or NumLock.

### Why Use Sara Over Other Small WM, X?

A few examples I was able to find:

* [dminiwm](https://github.com/moetunes/dminiwm) No status bar. No XINERAMA support. Also, some of the code: compare add\_window() to the combination of manage(), attachaside(), etc.
* [berry](https://github.com/JLErvin/berry) CLI-config vs. config.h. Extremely standards-compliant. Codebase is much larger as a consequence (wm.c is 1912 SLOC).
* [monsterwm](https://github.com/c00kiemon5ter/monsterwm) No status bar (I appreciate the UNIX philosophy adherence, though). No XINERAMA support. Not a fan of the code formatting, but not a big deal unless you're in my position/writing patches.

Also:

* I wrote a [bar](https://github.com/gitluin/sbar.git) that goes along with it!

Help Me (Keybindings, Installation, Etc.)!
-------------------------------------------
For more information, see the [wiki](https://github.com/gitluin/sara/wiki)!

Bugs
----
 * N O N E T H A T I K N O W O F !

To Do
----
 * Is RandR > Xinerama?
 * More UNIX philosophy? own bar (write out layout to file, to socket, etc.)?
   * bar:
     * specify size of bar space.
     * specify bar at top or bottom of the screen.
 * My own art.
 * A wiki.
 * Extensive Valgrind analysis to try and reduce resource usage.

If you have some particular request, don't send me an e-mail, I won't do it! Bug reports are welcome, though!
