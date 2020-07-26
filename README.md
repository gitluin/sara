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
sara is a simple and lightweight tiling window manager. Originally started with the goal of making catwm actually work, it's now a slimmed down descendant of dwm with a bit of bspwm (sockets) for good measure, and some extra UNIX philosophy inspiration from monsterwm. Project goals are small, readable code, and flexibility/extensibility.

Features
-----
### Modes
* Tiling mode (master window with right-hand vertical stack).
* Monocle mode (fullscreen, but bar still visible).
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
* bspwm-like:
	* keyboard event handling via external program (like `sxhkd`).
* monsterwm-like:
	* output desktop information for external parsing (like with shell script + `lemonbar`).
* no window borders.
* no window titles.
* easily hackable.

### Design Limitations
* No support for urgency, because nothing I do is urgent.
* No [ICCCM](https://web.archive.org/web/20190617214524/https://raw.githubusercontent.com/kfish/xsel/1a1c5edf0dc129055f7764c666da2dd468df6016/rant.txt) (found thanks to [dylan](https://github.com/dylanaraps/sowm)). This is mostly felt in the lack of the applysizehints behavior that dwm has (ex. cmatrix won't redraw using larger window bounds if you give it more space via togglefs, changemsize, etc.).
* No EWMH support. Fullscreening manually isn't so bad (see examples/sxhkdrc).

### Why Use sara Over Other WMs?

* You're me and felt the need to build a tiling/dynamic window manager, even though there are plenty of them and will probably be 5 more by the time you finish this sentence. But I think the strengths are the unique combination of `dwm`'s tag system, `bspwm`'s socket approach to handling keyboard input, and a somewhat `monsterwm`-inspired information outputting setup that allows you to go real bar-crazy with things like `lemonbar`.
* Relatedly, I wrote a [set of bar scripts](https://github.com/gitluin/sbar-lemon) that go along with it! This includes a script for getting a [lemonbar with Xft support](https://github.com/krypt-n/bar) setup running!

Help Me (Keybindings, Installation, Etc.)!
-------------------------------------------
For more information, see the [wiki](https://github.com/gitluin/sara/wiki)!

Recent Statii
------
 * v3.0		- Some silly bugs have been squashed, but the major news is that the bar has been excised and you are now free to use whatever you wish, so long as you can make it work with the output of sara! See the example bar setup I have in [this repo](https://github.com/gitluin/sbar-lemon) if you want to copy-paste and tweak for your system, or start from a template. This setup will produce a bar that is identical to the one in previous versions of sara.
 * v2.0		- Whole lotta UNIX philosophy up in here! Keybinds are controlled by sxhkd now! A little bit of bspwm in your dwm and sara sandwich! Pointer events (click into window, move and resize window) still handled by sara.
 * v1.0.{0-1} 	- Finished it enough to share with the class. Fixed a changecurrent bug.
 * v0.9.{2-9.5}	- Multiple monitor support, viewall, reduced redundant variables, solved many enternotify edge cases, moveclient works by visibility, updategeom a la dwm 6.1, implemented rules, transient popup support, proper client killing, proper float stacking, code cleanup, mouse move and resize, lots of time wasted on easy (but sometimes obscure) bugs.
 * v0.8.{1-7.7} - No drw and no alpha behavior for the bar. Can send clients to just a desktop, can view multiple desktops at once, enternotify works (maybe), gaps, window borders, bar boxes removed. Code cleanup. Memory freeing fixed. buttonpress works!
 * v0.7		- The dwm way of clients has been implemented, bug-free (I think)!
 * v0.6.{0-5}	- There's a bar with alpha support! Tags in the bar are updated properly by client movement, floating windows always appear raised, manual fullscreen toggling, no more swindows (the buggy version).
 * v0.5.{0-5}	- Windows can be destroyed any damn way, name change, better master_size, modular layouts, gaps implemented, individual client floating behavior.
 * v0.2.{0-8}	- Tagging works (no asterisks), no phantom windows, revamp of data structures, variable gap sizes, improvements to C programming (non-Xlib specific stuff), swindows are the true owners of Windows.
 * v0.1.{0-5}	- It compiles, there's a cursor icon, quit()ing is functional (but at what cost?), moving clients to other desktops works, tagging to other desktops "works" (it compiles & no segfaults), there are phantom windows after removing clients from the current desktop.

Bugs
----
 * N O N E T H A T I K N O W O F !

To Do
----
 * My own art.
 * Quality documentation for getting started and customizing.
 * Extensive Valgrind analysis to try and reduce resource usage.

If you have some particular request, don't send me an e-mail, I won't do it! Bug/documentation reports are welcome, though!
