lavalauncher(1) ["Version 1.7.0" ["Version 1.7.0"]]

# NAME
LavaLauncher - A simple launcher panel for Wayland desktops


# DESCRIPTION
LavaLauncher serves a simple purpose: Letting the user execute shell commands
by clicking on icons on a dynamically sized bar, placed at one of the screen
edges. A single LavaLauncher instance can provide multiple such bars, across
multiple outputs.

The Wayland compositor must implement the Layer-Shell and XDG-Output for
LavaLauncher to work. If LavaLauncher is displayed but wont respond to clicks,
chances are that the compositor you are using only displays Layer-Shell surfaces
but does not forward events to them.


# OPTIONS
*-c <path>*
	Path to the configuration file.

*-h*
	Display a helpful help message and exit.

*-v*
	Enable verbose output.


# CONFIGURATION
LavaLauncher is configured with a configuration file.

The default location of this file is "$XDG_CONFIG_HOME/lavalauncher/lavalauncher.conf".
If that environmental variable is not set, "$HOME/.config/lavalauncher/lavalauncher.conf"
is used instead. Alternatively, you can provide a path manually.

You can find an example configuration at the end of this section.


## STRUCTURE
The configuration file has a simple structure. It consists of *contexts* and
*assignments*.

Assignments are defined by a string (the variable to be changed) followed by
another string (the new value for the variable) and finally a semicolon (';').
Assignments are always inside contexts.

A context is defined by a string (the context name) followed by an opening
bracket. To leave a context, use a closing bracket. Contexts can be nested, but
unless specifically pointed out as otherwise, every context mentioned in this
document is a top-level context.

Comments start with an octothorpe ('#'). Any string can be quoted.

## GLOBAL  SETTINGS
Global settings can be configured int the "global-settings" context. The
assignments which can be made in this context are as follows.

*watch-config-file*
	Automatically reload when a change in the configuration file is detected.
	Can be "true" or "false". The default is "false". Behold: If the
	configuration file contains an error upon reload, LavaLauncher will exit.

## BAR
Every "bar" context will add a bar. The assignments possible in this context are
as follows.

*alignment*
	Alignment of the buttons / bar. Can be "start", "center" or "end". The
	default is "center".

*background-colour*
	The background colour of the bar. The default is "#000000".

*border*
	The border size. The Default is 2.

*border-{top, right, bottom, left}*
	The individual border sizes.

*condition-resolution*
	The resolution proportions an output must have for the bar to be displayed
	on it. Can be either "all", "wider-than-high" or "higher-than-wide". The
	default is "all".

*condition-scale*
	The scale an output must have for the bar to be displayed on it. Can be
	either an integer greater than zero or "all". The default is "all".

*cursor-name*
	Name of the cursor displayed when hovering the pointer over the bar. The
	default cursor is "pointer". Behold: What cursors are available depends
	on your cursor theme.

*effect*
	Drawing effect applied to the buttons. Can be "none", "box", "phone"
	or "circle". If "box", LavaLauncher will draw a rectangle underneath the
	button icons. If "phone", LavaLauncher will draw a rounded rectangle
	underneath the button icons, similar to how application icons look on
	phones. If "circle", LavaLauncher will draw a circle underneath the
	button icons. The default is "none".

*effect-colour*
	Colour of the drawing effect. The default is "#FFFFFF".

*effect-padding*
	Padding between drawing effect and bar outlines.

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
	is usually visually identical to "normal" mode, however the surface of the
	bar is sized and anchored less aggressively, which may (or may not,
	depending on the implementation of the compositor) improve how well
	LavaLauncher geometrically integrates and fits onto the desktop (and also
	will use less memory). It gives the compositor a lot more leeway regarding
	the exact placement of the bar. Behold: In "simple" mode, some compositors
	may not respect margins parallel to the bar or the exclusive zone, possibly
	even depending on the alignment. This is not a bug in LavaLauncher but a
	limitation of your compositor.

*output*
	Name of the output on which the bar should be displayed. Output names
	are compositor dependant. If set to "all" or "\*", LavaLauncher will
	display the bar on all outputs, which is the default behaviour.

*position*
	Position of the bar. Can be "top", "right", "bottom", "left". The bar
	will dock to that edge of the output(s). The default position is "bottom".

## BAR COPY
Every "bar-copy" context will add a bar. This new bar has the same configuration
and items as the previous bar. You can change the configuration and define
additional items as usual. The assignments possible in this context are the same
as for the "bar" context.

## BUTTON
Every "button" context will add a button to a bar. As such, this context is
a nested inside the "bar" or "bar-copy" context. The assignments possible in
this context are as follows.

*background-colour*
	The background colour of the individual button. This overrides the
	background colour of the bar.

*command*
	Sets the shell command for left-, right-, middle-click and touch
	interactions.

*image-path*
	The path to a PNG image file, which will be used as the icon of the
	button. It is recommended to use square images.

*left-click-command*
	The shell command executed when the user left-clicks on the button.

*right-click-command*
	The shell command executed when the user right-clicks on the button.

*middle-click-command*
	The shell command executed when the user middle-clicks on the button.

*scroll-down-command*
	The shell command executed when the user scrolls down on the button.

*scroll-up-command*
	The shell command executed when the user scrolls up on the button.

*touch-command*
	The shell command executed when the user touches the button.

## SPACER
Every "spacer" context will add a spacer to a bar. As such, this context is
a nested inside the "bar" or "bar-copy" context. The assignments possible in
this context are as follows.

*length*
	Length of the spacer.

## COMMANDS
Commands are executed using sh(1). If the command is "exit", LavaLauncher will
exit, if it is "reload", LavaLauncher will reload. LavaLauncher replaces certain
tokens in the command string with values when the command is executed. These
tokens are "%index%", "%buttons%", "%icon-size%",
"%border-{top,right,bottom,left}%", "%margin-{top,right,bottom,left}%",
"%output%" and "%scale%".

## COLOURS
LavaLauncher excepts colours in the formats "#RRGGBBAA" or "#RRGGBB". Therefore,
colour codes must be quoted to not be interpreted as comments. Alternatively,
"transparent", "white" and "black" are accepted as well.

## CONDITIONS
Conditions are (re-)evaluated and bars created and destroyed accordingly
whenever an outputs parameters are updated.

You can use conditions in combinations with the "bar-copy" feature to create
multiple versions of a bar for different conditions.

## EXAMPLE CONFIGURATION
Below you will find an example configuration, which demonstrates all available
contexts and some of the assignments by defining three bars and changing global
settings.

```
global-settings
{
	watch-config-file true;
}

bar # this is the first bar
{
	output eDP-1;
	position bottom;
	background-colour "#202020";

	button
	{
		image-path "/path/to/image.png";
		command    "useful-script.sh";
	}

	spacer
	{
		length 20;
	}

	button
	{
		image-path "/path/to/another/image.png";
		command    "another-useful-script.sh";
	}
}

bar # this is a second bar
{
	output    all;
	position  top;
	icon-size 50;
	border    0;
	alignment start;

	button
	{
		image-path "/path/to/a/third/image.png";
		command    "do-something"
	}
}

bar-copy # this copies the previous bar
{
	position left;
}
```


# BUGS
Probably.

Please report all bugs you find with an explanation how to reproduce them.


# MAILINGLIST
You can send bug reports, patches and feedback to the mailinglist.

*<~leon_plickat/lavalauncher@lists.sr.ht>*


# GIT
This project is developed using git. You can find the repository by visiting the
following URL.

*<https://git.sr.ht/~leon_plickat/lavalauncher>*


# AUTHORS
Leon Henrik Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
