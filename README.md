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
sara is a very simple and lightweight tiling window manager. Originally started with the goal of making catwm actually work, it's now a slimmed down descendant of dwm. I will try to keep the source code as small and readable as possible.

Recent Statii
------
 * v0.9.9.5	- Some dwmization that was ultimately unnecessary to fix the pesky c->next = NULL bug, but I'll leave it there unless the SLOC is increased. Think all enternotify is actually, finally done.
 * v0.9.9.4	- Everything enternotify related is done. moving in the stack is based on tiling, not visibility. applyrules has fullscreen support, though without wm hints it can be goofy depending on your use case. Final code review.
 * v0.9.9.3	- Some enternotify bug fixing, but not everything is resolved yet.
 * v0.9.9.2	- updategeom works, motionnotify bug fixed, focusmon and tomon directions adjusted in config.h to fit updategeom.
 * v0.9.9.1	- updategeom analagous to the dwm 6.1 is in place! Just need to check that it works properly.
 * v0.9.9.0	- Mouse move and resize, in one mungo function! Cursor intentionally doesn't change shape. Just do bug fixing, code cleanup, TODOs, and I'll be done!
 * v0.9.7.{0-7}	- Implemented rules, transient popup window support, proper float stacking, proper client killing, removal of redundant variables, plenty of code cleanup.
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
* No support for doing things through CapsLock or NumLock.

### Why Use Sara Over Other Small WM, X?

A few examples I was able to find:

* [dminiwm](https://github.com/moetunes/dminiwm) No status bar. No XINERAMA support. Also, some of the code: compare add\_window() to the combination of manage(), attachaside(), etc.
* [berry](https://github.com/JLErvin/berry) CLI-config vs. config.h. Extremely standards-compliant. Codebase is much larger as a consequence (wm.c is 1912 SLOC).
* [monsterwm](https://github.com/c00kiemon5ter/monsterwm) No status bar (I appreciate the UNIX philosophy adherence, though). No XINERAMA support. Not a fan of the code formatting, but not a big deal unless you're in my position/writing patches.

Installation
------------
Need Xlib, then sudo make install clean

Help Me!
------------
For more information, see the [wiki](https://github.com/gitluin/sara/wiki)!

Bugs
----
 * N O N E T H A T I K N O W O F !

To Do
----
 * Continuous:
   * No bugs (just use [XGH](https://gist.github.com/banaslee/4147370)) (required for v1.0)
   * Code cleanup (as a constant)
 * Discrete:
   * My own art.
   * A wiki.

If you have some particular request, don't send me an e-mail, I won't do it!
