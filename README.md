SARA
=====

        .-. .-.
      ..:  |  ;,.
     (___`.|.'___)
     (   .'|`.   )
      `'/  |  \`'
        `-' `-'                                                                    hjw, ascii.co.uk/art

What Is, Why Do
-------
sara is a very simple and lightweight tiling window manager, with the goal of making catwm actually work and paring down dwm. I will try to stay under 1000 SLOC, though usability is more important.

Recent Statii
------
 * v0.8.7	- enternotify works (I think)!
 * v0.8.2	- Can now view multiple tags at once. I probably won't, but you can (not much extra code, so big whoop).
 * v0.8.1	- Can now send clients to just a specific desktop (removes it from others).
 * v0.8		- No drw and no alpha behavior for the bar. We are a single .c file once more. Rules still don't work.
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

* dwm-like: tags (except for viewing multiple tags at once)
* patch-like:
	* attachaside
	* emptyview
	* hide vacant tags
	* movestack
	* pertag-like: layouts, master_size
	* singular borders
* gaps for you zoomers
* worst

### Design Limitations

* No unmapping, just destroying. I don't like window decorations, so no minimizing, etc. possible.
* Window borders are 1 pixel, and only if gap\_px > 0
* No support for urgency coloring in the bar, because nothing I do is urgent.
* No EWMH support, because you're an adult and can fullscreen manually (see config.h), and it's too much work to deal with floating pop-up windows when they'll disappear anyway.
* One bar. I don't like to look to a particular monitor for the time, just the same spot on any monitor. No Xinerama yet but we'll get there.
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
You're on your own with the brightness and volume key situation. I had to do what I did for dwm as well, so comment out entries in config.h as necessary. If you're lucky, your keys Just Work (TM).

| Key				| Function 						|
| -----------------------------	| -----------------------------------------------------	|
| Mod+NUMKEY			| Switch to desktop NUMKEY				|
| Mod+Shift+NUMKEY		| Add focused client to desktop NUMKEY			|
| Mod+Control+NUMKEY		| Add desktop NUMKEY to current view			|
| Mod+Shift+Control+NUMKEY	| Put focused client on only desktop NUMKEY		|
| Mod+T				| Spawn st						|
| Mod+W				| Spawn firefox						|
| Mod+F				| Spawn vifm						|
| Mod+R				| Spawn R session					|
| Mod+C				| Spawn calender (broke)				|
| Mod+D				| Spawn dmenu						|
| Mod+K				| Move focus up						|
| Mod+J				| Move focus down					|
| Mod+H				| Decrease master area					|
| Mod+L				| Increase master area					|
| Mod+Return			| Move client to master					|
| Mod+Shift+K			| Move client up stack					|
| Mod+Shift+J			| Move client down stack				|
| Mod+Shift+Q			| Kill client ("force quit")				|
| Mod+Shift+E			| Quit sara (shuts down X)				|
| Mod+Shift+Space		| Toggle client to floating (not useful w/o movement)	|
| Mod+Shift+Return		| Toggle client to fullscreen (no bar)			|
| Mod+Control+T			| Change layout to tiling				|
| Mod+Control+M			| Change layout to monocle				|

Bugs
----
 * Adjusting window sizes won't update the border size beyond the original dimensions, and will sometimes erase the contents - cmatrix or neofetch are examples of this _configurenotify/request?_
 * With large numbers of clients in the stack, clients start to get pushed beyond the desired bottom boundary
 * With programs like Krita, the popup window generates an unmap event upon completion of its task, so it stays around and takes up tiling space _somehow link unmap to destroy, but only for specific clients_
 * Function spawns don't play nice if they hang around and you try to kill them before they go poof
 * When you spawn and exit dmenu without launching anything, input focus returns to where it was, but XRaise does not

To Do
----
 * No bugs (just use XGH)
 * Code cleanup (as a constant)
 * Does it leak memory?
 * quit() should probably be more responsibly implemented
 * Implement multiple monitors (0.5 points)
 * Implement rules (0.5 points)
 * Implement mouse move, resize support for clients (0.2 points)
 * My own art (0.1 points)
 * Tutorial

If you have some particular request, don't send me an e-mail, I won't do it!
