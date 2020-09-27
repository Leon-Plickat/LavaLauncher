lavalauncher(1) ["Version 2.0.0" ["Version 2.0.0"]]

# NAME
LavaLauncher - A simple launcher panel for Wayland desktops


# DESCRIPTION
LavaLauncher is a simple launcher panel for Wayland desktops.

It displays a dynamically sized bar with user defined buttons. These Buttons
consist of an image, which is displayed as the button icon on the bar, and at
least one shell command, which is executed when the user activates the button.

Buttons can be activated with pointer and touch events.

A single LavaLauncher instance can provide multiple such bars, across multiple
outputs.

The Wayland compositor must implement the Layer-Shell and XDG-Output for
LavaLauncher to work.

Beware: Unlike applications launchers which are similar in visual design to
LavaLauncher, which are often called "docks", LavaLauncher does not care about
.desktop files or icon themes nor does it keep track running applications.
Instead, LavaLaunchers approach of manually defined buttons is considerably more
flexible: You could have buttons not just for launching applications, but for
practically anything you could do in your shell, like for ejecting your optical
drive, rotating your screen, sending your cat an email, playing a funny sound,
muting all audio, toggling your lamps and a lot more. Be creative!


# OPTIONS
*-c <path>*, *--config <path>*
	Path to the configuration file.

*-h*, *--help*
	Display a helpful help message and exit.

*-v*, *--verbose*
	Enable verbose output.

*-V*, *--version*
	Show version.


# CONFIGURATION
LavaLauncher is configured with a configuration file.

Unless the path to a configuration file has been specified, LavaLauncher tries
to find a configuration file at, in decending priority, "./lavalauncher.conf",
"$XDG_CONFIG_HOME/lavalauncher/lavalauncher.conf",
"$HOME/.config/lavalauncher/lavalauncher.conf"
"/usr/local/etc/lavalauncher/lavalauncher.conf" and
"/etc/lavalauncher/lavalauncher.conf"

You can find an example configuration at the end of this section.

## STRUCTURE
The configuration file has a simple structure. It consists of *contexts* and
*assignments*.

Assignments are always inside contexts and consist of three parts: The variable
to be changed, followed by the new value for the variable, and finally a
semicolon (';'). The value string must be separated from the variable name by
whitespace and may be quoted, causing LavaLauncher to use it as is, including
all whitespace that would otherwise be stripped from the final string.

A context consist of at least three parts: The context name, followed by an
opening bracket, optionally contents, and finally a closing bracket. All these
must be separated by whitespace. Some Contexts must be nested as contents of
other contexts.

Comments start with an octothorpe ('#').

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
	The border size. Expects either one integer, to set the size of all borders,
	or four integers, to set the individual sizes for the top, left, right and
	bottom border. The default border size is 1.

*condition-resolution*
	The resolution proportions an output must have for the bar to be displayed
	on it. Can be either "all", "wider-than-high" or "higher-than-wide". The
	default is "all".

*condition-scale*
	The scale an output must have for the bar to be displayed on it. Can be
	either an integer greater than zero or "all". The default is "all".

*condition-transform*
	The rotation an output must have for the bar to be displayed on it. Can be
	"all", 0, 1, 2 or 3. The numerical values represent rotations of 0, 90,
	180 and 270 degrees, with the exact correspondence depending on the
	compositor. The default is "all".

*cursor-name*
	Name of the cursor displayed when hovering the pointer over the bar. The
	default cursor is "pointer". Behold: What cursors are available depends
	on your cursor theme.

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

*icon-padding*
	Padding around the icons. The default 4.

*indicator-active-colour*
	The colour of the indicator for icons being activated. The default is
	"#606060".

*indicator-hover-colour*
	The colour of the indicator for icons over which the cursor hovers. The
	default is "#404040".

*indicator-padding*
	Padding around the icon indicator. The default 0.

*indicator-style*
	Style of the icon indicator. Can be "rectangle", "rounded-rectangle" and
	"circle". The default is "rounded-rectangle". The corner radia for
	"rounded-rectangle" are the same as the ones of the bar itself.

*layer*
	Layer of the bar surface. Can be "overlay", "top", "bottom" or
	"background". Typically, "bottom" and "background" will be underneath
	regular windows, while "top" and "overlay" will be above them, however
	the exact placement on the z axis depends on the implementation of the
	compositor. The default layer is "bottom".

*margin*
	The margin of the surface. Expects either one integer, to set the margin in
	all directions, or four integers, to set the individual top-, left-, right-
	and bottom-margins. The default margin is 0.

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

*namespace*
	The namespace of the layershell surface of the bar. Some compositors may
	treat layershell surfaces differently based on their namespace. The default
	is "LavaLauncher".

*output*
	Name of the output on which the bar should be displayed. Output names
	are compositor dependant. If set to "all" or "\*", LavaLauncher will
	display the bar on all outputs, which is the default behaviour.

*position*
	Position of the bar. Can be "top", "right", "bottom", "left". The bar
	will dock to that edge of the output(s). The default position is "bottom".

*radius*
	The radius of the corners. Expects either one integer, to set the radius of
	all corners, or four integers, to set the individual radii for the top-left,
	top-right, bottom-left and bottom-right corner. The default radius is 5. Set
	to 0 to disable corner roundness.

*size*
	Size of the bar. The default size is 60.

## BAR COPY
Every "bar-copy" context will add a bar. This new bar has the same configuration
and items as the previous bar. You can change the configuration and define
additional items as usual. The assignments possible in this context are the same
as for the "bar" context.

## BUTTON
Every "button" context will add a button to a bar. As such, this context is
a nested inside the "bar" or "bar-copy" context. The assignments possible in
this context are as follows.

*command*
	Sets the shell command for left-, right-, middle-click and touch
	interactions.

*image-path*
	The path to an image file, which will be used as the icon of the
	button.

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
Commands are executed using sh(1).

LavaLauncher understands typical backslash escape sequences.

If the command is "exit", LavaLauncher will exit, if it is "reload",
LavaLauncher will reload.

LavaLauncher sets some environmental variables for the processes it launches.
Since all commands are executed via a shell, they can be used as arguments for
the commands. These variables are as follows.

*$LAVALAUNCHER_OUTPUT_NAME*
	The name of the output the button has been clicked on.

*$LAVALAUNCHER_OUTPUT_SCALE*
	The scale of the output the button has been clicked on.

## COLOURS
LavaLauncher can parse hex code colours and read RGB values directly.

The supported formats are "#RRGGBB", "#RRGGBBAA", "0xRRGGBB" and "0xRRGGBBAA"
for hex code colours and "rgb(rrr,ggg,bbb)" and "rgba(rrr,ggg,bbb,aaa)" for RGB
colours.

## CONDITIONS
Conditions are (re-)evaluated and bars created and destroyed accordingly
whenever an outputs parameters are updated.

You can use conditions in combinations with the "bar-copy" feature to create
multiple versions of a bar for different conditions.

## IMAGES
LavaLauncher support PNG images and, if enabled at compile time, SVG images. It
is recommended to use square images.

## EXAMPLE CONFIGURATION
This is an example configuration which defines two bars.

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
		image-path /path/to/image.png;
		command    useful-script.sh;
	}

	button
	{
		image-path "/path/to/another/image.png";
		command    "another-useful-script.sh";
	}
}

bar # this is the second bar
{
	output all;

	button
	{
		image-path "/path/to/a/third/image.png";
		command    "do-something"
	}
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
