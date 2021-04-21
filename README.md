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
sara is a simple and lightweight dynamic window manager. What started out as an attempt to make catwm into a small and fully-functional window manager has turned into my primary project in C. At the moment, sara combines aspects of dwm, bspwm, and monsterwm. As my needs/wants change, so too will the program, and probably how much you like it.

Features
-----
### Modes
* Tiling mode (master window with right-hand vertical stack).
* Monocle mode (fullscreen, but bar still visible).
* Floating mode (clients freely set coordinates).
* Fullscreen mode (see limitations).

### Behavior/Traits
* dwm-like:
	* tags.
	* Multihead support through Xinerama.
	* patch-like:
		* attachaside
		* movestack
		* pertag-like: layouts, master_size
		* singularborders
		* vanitygaps
* bspwm-like:
	* IPC-based interaction via a client program, `sarasock`.
	* keyboard event handling via external program (like `sxhkd`).
* monsterwm-like:
	* output desktop information for external parsing (like with shell script + `lemonbar`/`polybar`).
* no window borders.
* no window titles.
* easily hackable.

### Design Limitations
* No support for urgency, because nothing I do is urgent.
* No [ICCCM](https://web.archive.org/web/20190617214524/https://raw.githubusercontent.com/kfish/xsel/1a1c5edf0dc129055f7764c666da2dd468df6016/rant.txt). This is mostly felt in the lack of the applysizehints behavior that dwm has (ex. cmatrix won't redraw using larger window bounds if you give it more space via togglefs, changemsize, etc.).
* No EWMH support. Fullscreening manually isn't so bad (see examples/sxhkdrc).

### Why Use sara Over Other WMs?

Things like [dwm-ipc](https://github.com/mihirlad55/dwm-ipc) scare the hell out of you because they're so big. You want a window manager that works like a souped-up dwm with sane defaults that is also opinionated like all the cool kids. I think the combination of the tag system, the flexibility of sxhkd and IPC interaction, and the ease of parsing the window manager's internal state are good selling points.

Help Me (Keybindings, Installation, Etc.)!
-------------------------------------------
`man sara`

If that doesn't answer your question, check out the [wiki](https://github.com/gitluin/sara/wiki)!

Recent Statii
------
 * v4.0		- A man page! Lots of """"bloat""""; polybar is now the suggested default! Huge shoutout to [Jonas](https://jonas-langlotz.de/2020/10/05/polybar-on-dwm) for the only post on his blog single-handedly making my `peachbar` problems obsolete.
 * v3.{0-3}	- Internal bar removed and bar scripts created. Floating layout. More Zoom-friendly client handling (nothing is sacred). Fixed longstanding math issues with `tile()`.
 * v2.0		- Whole lotta UNIX philosophy up in here! Keybinds are controlled by sxhkd now! A little bit of bspwm in your dwm and sara sandwich! Pointer events (click into window, move and resize window) still handled by sara.
 * v1.0.{0-1} 	- Finished it enough to share with the class. Fixed a `changecurrent()` bug.
 * v0.9.{2-9.5}	- Multiple monitor support, viewall, reduced redundant variables, solved many enternotify edge cases, moveclient works by visibility, updategeom a la dwm 6.1, implemented rules, transient popup support, proper client killing, proper float stacking, code cleanup, mouse move and resize, lots of time wasted on easy (but sometimes obscure) bugs.
 * v0.8.{1-7.7} - No drw and no alpha behavior for the bar. Can send clients to just a desktop, can view multiple desktops at once, enternotify works (maybe), gaps, window borders, bar boxes removed. Code cleanup. Memory freeing fixed. `buttonpress()` works!
 * v0.7		- The dwm way of clients has been implemented, bug-free (I think)!
 * v0.6.{0-5}	- There's a bar with alpha support! Tags in the bar are updated properly by client movement, floating windows always appear raised, manual fullscreen toggling, no more swindows (the buggy version).
 * v0.5.{0-5}	- Windows can be destroyed any damn way, name change, better master_size, modular layouts, gaps implemented, individual client floating behavior.
 * v0.2.{0-8}	- Tagging works (no asterisks), no phantom windows, revamp of data structures, variable gap sizes, improvements to C programming (non-Xlib specific stuff), swindows are the true owners of Windows.
 * v0.1.{0-5}	- It compiles, there's a cursor icon, `quit()`ing is functional (but at what cost?), moving clients to other desktops works, tagging to other desktops "works" (it compiles & no segfaults), there are phantom windows after removing clients from the current desktop.

Bugs
----
 * cursor in a window, move focus to other monitor using keyboard, click in same window, focus not drawn
 * Reinitializing monitors should update focus to a client.
 * After using physlock + betterlockscreen, clients are not told to redraw.

To Do
----
 * Fix bugs.
 * `bspc` style interfacing with `sara`: config setting, rule setting.
 * Partial standards compliance so things like `rofi -m -4` works.
 * Convert to XCB, probably.
 * Quality documentation for getting started and customizing.
 * Any way to separate pointer behavior management?
 * My own art.

Tiling WM Probs
----
 * No "minimize" feature: you have some tag you dump things into.
 * It's really only feasible to have 3 windows in the master/stack layout. Otherwise they're too small, or if you follow the dwm philosophy and only use master for active work, then you've got the equivalent of a dock taking up a lot of screen real estate.

If you have some particular request, don't send me an e-mail, I won't do it! Bug/documentation reports are welcome, though!
