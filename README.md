sara
=====
Description
-------
sara is a simple and lightweight dynamic window manager. What started out as an attempt to make catwm into a small and fully-functional window manager has turned into my primary project in C. At the moment, sara combines aspects of dwm and bspwm with some custom features. As my needs/wants change, so too will the program, and probably how much you like it.

If things like [dwm-ipc](https://github.com/mihirlad55/dwm-ipc) scare the hell out of you because they're so big, this window manager might be for you. You want a window manager that works like a souped-up dwm with sane defaults that is also opinionated like all the cool kids. I think the combination of the tag system, the flexibility of sxhkd and IPC interaction, and the ease of parsing the window manager's internal state are good selling points.

Features
-----
### Default Modes
* Tiling mode (master window with right-hand vertical stack).
* Monocle mode (fullscreen, but bar area still visible).
* Floating mode (clients freely set coordinates).
* Fullscreen mode (see Design Limitations).

### Behavior/Traits
* dwm-like:
	* tags.
	* Multihead support through Xinerama.
	* patch-like:
		* attachaside
		* movestack
		* pertag-like: layouts, master_size
		* restartsig
		* singularborders
		* vanitygaps
* bspwm-like:
	* IPC-based interaction via a client program, `sarasock`.
	* keyboard event handling via external program (like `sxhkd`).
* output desktop information to the X server for external parsing (like with shell script + `lemonbar`/`polybar`).
* no window borders.
* no window titles.

### Design Limitations
* No support for urgency, because nothing I do is urgent.
* No support for iconified (i.e. "minimized") clients.
* No [ICCCM](https://web.archive.org/web/20190617214524/https://raw.githubusercontent.com/kfish/xsel/1a1c5edf0dc129055f7764c666da2dd468df6016/rant.txt). This is mostly felt in the lack of the applysizehints behavior that dwm has (ex. cmatrix won't redraw using larger window bounds if you give it more space via togglefs, changemsize, etc.).
* No EWMH support. Fullscreening is done manually (see examples/sxhkdrc).


Help Me (Keybindings, Installation, Etc.)!
-------------------------------------------
`man sara`

If that doesn't answer your question, check out the [wiki](https://github.com/gitluin/sara/wiki)!

Version History
------
 * v4.2		- `config.h` now supports running anything via shell script on startup like a traditional `rc` file: just add new lines to the `const char* progs[]` array in `config.h`! `sara` also now supports reloading this config file on-the-fly, without restarting, thanks to bringing in the `dwm` approach to "adopting" unmanaged windows and much of the `restartsig` patch (anything specified in `progs[]` will be re-run, FYI!). And `sarasock` no longer requires quoting all its arguments, though you can still do so! Updated directory structure for the repository so it's less messy.
 * v4.1		- Now sources a config file, `$XDG_CONFIG_HOME/sara/sararc` or `$HOME/.config/sara/sararc`, which is a shell script that specifies programs to start à la `bspwm`. My plan is to update `sarasock` to configure some variables at runtime in a similar fashion. Also including a `polybar` script for tag information that has support for clickable areas.
 * v4.0		- A man page! Lots of """"bloat""""; `polybar` is now the suggested default! Huge shoutout to [Jonas](https://jonas-langlotz.de/2020/10/05/polybar-on-dwm) for the only post on his blog single-handedly making my `peachbar` problems obsolete.
 * v3.0		- Internal bar removed and bar scripts created. Floating layout. More Zoom-friendly client handling (nothing is sacred). Fixed longstanding math issues with `tile()`.
 * v2.0		- Keybinds are controlled by sxhkd now! A little bit of bspwm never hurt anyone. Pointer events (click into window, move and resize window) still handled internally.
 * v1.0 	- Finished it enough to share with the class. At this stage, very like dwm with some patches (see above) built-in, some cleaned-up code (and some not), some cut corners (not as much support for Atoms), but all is well and functional.

Bugs
----
 * Reinitializing monitors should update focus to a client and reset enternotify status.
 * Sometimes after using physlock + betterlockscreen, clients need a redraw and don't do it until toggled/moved around.
 * Can't round corners on some clients like `chromium`.

To Do
----
 * Fix bugs.
 * External pointer management: can `sxhkd` reasonably do this?
 * Partial standards compliance so things like `rofi -m -4` works.
 * Convert to XCB, starting with where it will improve performance.

Please submit bug reports! I've only tested this on my own system.
