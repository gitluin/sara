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
 * v0.9.2.5	- moveclient works by visibility now - no more hitting MOD+Shift+{J,K} several times to get clients to move on-screen.
 * v0.9.2.4	- enternotify is snappy!
 * v0.9.2.3	- Nasty bug go bye-bye, and fixed resizing mistake with fullscreen windows.
 * v0.9.2.2	- No more global current, head, seldesks, current\_desktop. Tag drawing in bars duly fixed.
 * v0.9.2.1	- viewall desktops at once (MOD + XK\_0)
 * v0.9.2	- Multiple monitors! Stable, but buggy. Currently there are some weird focusing issues and tags in the bars are not per-monitor.
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
 * With programs like Krita, the popup window generates an unmap event upon completion of its task, so it stays around and takes up tiling space. _unmap support_
 * Function spawns don't play nice if they hang around and you try to kill them before they go poof.
 * Bar doesn't redraw on laptop lid reopen until maprequest, changedesktop, etc.
 * Need to XFree() in applyrules, I think? T'is unclear.
 * Sometimes, download dialogs from Firefox end up in 9th hellspace.
 * buttonpress doesn't draw focus to new monitors, and sometimes not to new windows, but unsure when.
 * bar doesn't always draw immediately on xinit.

To Do
----
 * Continuous:
   * No bugs (just use [XGH](https://gist.github.com/banaslee/4147370)) (required for v1.0)
   * Code cleanup (as a constant) - function naming conventions.
   * Does it leak memory?
 * Discrete:
   * Implement rules (0.5 points).
   * Implement mouse move, resize support for clients (0.3 points).
   * Limited EWMH (automatic togglefs)?
   * Drop all pretense and just call desktops tags.
   * My own art.
   * Tutorial.

If you have some particular request, don't send me an e-mail, I won't do it!
