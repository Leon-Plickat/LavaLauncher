lavalauncher(1) ["Version 1.7.0" ["Version 1.7.0"]]

# NAME
LavaLauncher - A simple launcher panel for Wayland desktops


# DESCRIPTION
LavaLauncher serves a single purpose: Letting the user execute shell commands
by clicking on icons on a dynamically sized bar, placed at one of the screen
edges.

The Wayland compositor must implement the Layer-Shell for LavaLauncher to work.
If LavaLauncher is displayed but wont respond to clicks, chances are that the
compositor you are using only displays Layer-Shell surfaces but does not forward
events to them.


# OPTIONS
*-c <path>*
	Path to the configuration file.

*-h*
	Display a helpful help message and exit.

*-v*
	Enable verbose output.

# CONFIGURATION
LavaLauncher is configured with a configuration file. There is no default
location, meaning manually providing the path to a file is mandatory.

## STRUCTURE
A LavaLauncher configuration file has a simple structure. It consists of
*contexts* and *assignments*.

In the configuration syntax, entering a context is defined by a string (the
context name) followed by an opening bracket. To leave a context, use a closing
bracket. Assignments are defined by a string (the variable to be changed)
followed by another string (the new value for the variable) and finally a
semicolon (';'). Comments start with an octothorpe ('#').

Contexts can be nested, but unless specifically pointed out as otherwise, every
context mentioned in this document is a top-level context. Assignments are
always inside contexts.

Any string can be quoted.

```
context_1
{
	variable1 value1;
	variable2 value2;
}

# I am a comment
context_2
{
	nested_context
	{
		"variable3" value3;
		variable4   "value4";
	}
}
```

## GENERAL SETTINGS
General settings can be configured int the "settings" context. The assignments
which can be made in this context are as follows.

*alignment*
	Alignment of the buttons / bar. Can be "start", "center" or "end". The
	default is "center". This option has no effect in "simple" mode.

*background-colour*
	The background colour of the bar. The default is "#000000".

*border-{top, right, bottom, left}*
	The individual border sizes. The default border size is 2.

*cursor-name*
	Name of the cursor displayed when hovering the pointer over the bar. The
	default cursor is "pointing_hand". Behold: What cursors are available
	depends on the cursor theme. If the cursor of the given name can not be
	loaded, LavaLauncher will display a warning and disable changing the
	cursor.

*effect*
	Drawing effect applied to the buttons. Can be "none", "box" or "phone".
	If "box", LavaLauncher will draw a rectangle underneath the button icons.
	If "phone", LavaLauncher will draw a rounded rectangle underneath the
	button icons, similar to how application icons look on phones. The
	default is "none".

*effect-colour*
	Colour of the drawing effect. The default is "#FFFFFF".

*exclusive-zone*
	Exclusive zone of the surface. Can be "true", "false" or "stationary".
	If "true", the compositor is informed that it should not obstruct the
	surface of LavaLauncher with other surfaces. If "false", the compositor
	is informed that it can place other surfaces over or under the surface
	of LavaLauncher. If "stationary", the compositor is informed that the
	surface of LavaLauncher should not be moved to accommodate other layer
	shell surfaces and that it can freely place other surfaces over or under
	the surface of LavaLauncher. The default is "true". The exact
	implementation is compositor dependant; A compositor may choose to
	ignore the requested exclusive zone of surfaces.

*icon-size*
	Size of the icons. The default size is 80.

*layer*
	Layer of the bar surface. Can be "overlay", "top", "bottom" or
	"background". Typically, "bottom" and "background" will be underneath
	regular windows, while "top" and "overlay" will be above them, however
	the exact placement on the z axis depends on the implementation of the
	compositor. The default layer is "bottom".

*margin-{top, right, bottom, left}*
	Directional margins of the surface. The default margins are 0.

*mode*
	The display mode of the bar. Can be "default", "full" or "simple". In
	"default" mode, the bar is sized just large enough to house all the
	buttons. Logically, the default mode is "default". In "full" mode, the
	bar spans the entire length of the edge it is docked to. "simple" mode
	is visually identical to "normal" mode, however the surface of the bar
	is sized and anchored less aggressively, which may (or may not;
	depending on the implementation of the compositor) improve how well
	LavaLauncher geometrically integrates and fits onto the desktop (and
	also will use less memory). It gives the compositor a lot more leeway
	regarding the exact placement of the bar, at the cost of not respecting
	the user defined alignment and margins parallel to the bar.

*output*
	Name of the output on which the bar should be displayed. Output names
	are compositor dependant. If set to "all" or "\*", LavaLauncher will
	display a bar on all outputs, which is the default behaviour.

*position*
	Position of the bar. Can be "top", "right", "bottom", "left". The bar
	will dock to that edge of the output(s). The default position is "bottom".

## ITEMS
The "items" context is used to configure the contents of the bar. It has no
assignments, only nested contexts. The items on the bar will be shown in the
order in which they are defined in the configuration file.

## BUTTON
Every "button" context will add a button to the bar. This context is nested
inside the "items" context. The assignments possible in
this context are as follows.

*command*
	The shell command executed when the user clicks on the button. The
	command is executed using sh(1). If the given command is "exit",
	LavaLauncher will exit. LavaLauncher replaces certain tokens in the
	command string with values when the command is executed. These tokens
	are "%index%", "%buttons%", "%icon-size%",
	"%border-{top,right,bottom,left}%", "%margin-{top,right,bottom,left}%",
	"%colour%", "%border-colour%", "%output%" and "%scale%".

*image-path*
	The path to a PNG image file, which will be used as the icon of the
	button. It is recommended to use square images.

*background-colour*
	The background colour of the individual button. This overrides the
	global background colour.

## SPACER
Every "spacer" context will add a spacer to the bar. This context is nested
inside the "items" context. The assignments possible in this context are as
follows.

*length*
	Length of the spacer.

## COLOURS
LavaLauncher excepts colours in the formats "#RRGGBBAA" or "#RRGGBB". Therefore,
colour codes must be quoted. Alternatively, "transparent", "white" and "black"
are accepted as well.


# BUGS
Probably.

Please report all bugs you find with a small explanation how to reproduce them.


# MAILINGLIST
You can send bug reports, patches and feedback to the mailinglist.

*<~leon_plickat/lavalauncher@lists.sr.ht>*

# GIT
This project is developed using git. You can find the repository by visiting the
following URL.

*<https://git.sr.ht/~leon_plickat/lavalauncher>*

# AUTHORS
Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
