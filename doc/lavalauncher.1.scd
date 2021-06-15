lavalauncher(1)

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
to find a configuration file at, in descending priority, "./lavalauncher.conf",
"$XDG_CONFIG_HOME/lavalauncher/lavalauncher.conf",
"$HOME/.config/lavalauncher/lavalauncher.conf"
"/usr/local/etc/lavalauncher/lavalauncher.conf" and
"/etc/lavalauncher/lavalauncher.conf"

You can find an example configuration at the end of this section.

## STRUCTURE
The configuration file has a simple structure. It consists of *contexts* and
*assignments*.

Assignments are always inside context and have the following form.

```
<variable> = <value>;
```

Note that the value may be quoted, causing LavaLauncher to use it as is,
including all whitespace that would otherwise be stripped from the final
string.

Contexts have the following form.

```
<context name>
{
	<context content>
}
```

Some Contexts must be nested as contents of other contexts.

Comments start with an octothorpe ('#').

## GLOBAL  SETTINGS
Global settings can be configured in the "global-settings" context. The
assignments which can be made in this context are as follows.

*watch-config-file*
	Automatically reload when a change in the configuration file is detected.
	Can be "true" or "false". The default is "false". Behold: If the
	configuration file contains an error upon reload, LavaLauncher will exit.

## BAR
Every "bar" context will add a bar. The configuration changes in this context
make up the default configuration set of the bar. The assignments possible in
this context are as follows.

*alignment*
	Alignment of the buttons / bar. Can be "start", "center" or "end". The
	default is "center".

*background-colour*
	The background colour of the bar. The default is "#000000".

*border-colour*
	The border colour of the bar. The default is "#ffffff".

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
	surface of LavaLauncher should not be moved to accommodate other Layer-Shell
	surfaces and that it can freely place other surfaces over or under
	the surface of LavaLauncher. The default is "true". The exact
	implementation is compositor dependant; A compositor may choose to
	ignore the requested exclusive zone of surfaces.

*hidden-mode*
	Hidden mode of the bar. Can be "never", "always" and "river-auto". Defaults
	to "never".

	In "never" mode, the bar is never hidden.

	In "always" mode, the bar is always hidden.

	The "river-auto" mode works only in the river Wayland compositor. In this
	mode, the bar is only hidden if the output is currently occupied.

	Behold: If you want a hidden bar which sits above windows on hover, you need
	to set the layer to "top" and the exclusive zone to "off" or "stationary".

*hidden-size*
	Size of the invisible area where hovering with a pointer will unhide the bar
	if it is hidden. Defaults to 10.

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
	"circle". The default is "rounded-rectangle". The corner radii for
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
	The display mode of the bar. Can be "default", "full" or "aggressive".

	In "default" mode, the bar is sized just large enough to house all the
	buttons. In this mode, compositors may not respect margins parallel to the
	bar or the exclusive zone, possibly even depending on the alignment. This
	is not a bug in LavaLauncher but a limitation of your compositor.

	In "full" mode, the bar spans the entire length of the edge it is docked
	to.

	The "aggressive" mode is visually identical to "normal" mode, however the
	surface of the bar is sized and anchored more aggressively. This mode is
	intended for compositors with faulty Layer-Shell implementations. If
	LavaLauncher is not placed at the desired location or the exclusive zone or
	margins parallel to the bar are ignored, this mode may help. Beware however
	that it allows the compositor a lot less leeway when geometrically fitting
	the bar onto the screen.

*namespace*
	The namespace of the Layer-Shell surface of the bar. Some compositors may
	treat Layer-Shell surfaces differently based on their namespace. The default
	is "lhp.LavaLauncher". Note that this option only takes effect once on bar
	creation.

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

## CONFIG
Every "config" context will add an additional configuration set to a bar. As
such, this context is a nested inside the "bar" context. It copies the values
of the default configuration set, but every setting can be overridden. This is
useful to change bar configuration based on conditionals, as LavaLauncher will
automatically choose the first configuration set with fitting conditions.
Different instances of the same bar on different outputs can use different
configuration sets, but a bar has at most one instance per output and uses
exactly one configuration set per instance. The assignments possible in this
context are the same as for the "bar" context.

## BUTTON
Every "button" context will add a button to a bar. As such, this context is a
nested inside the "bar" context. The assignments possible in this context are
as follows.

*command*
	Sets the shell command for left-, right-, middle-click and touch
	interactions.

*command[*_<bind>_*]*
	Bind a shell command to a specific interaction plus modifier combination.
	This combination is defined by _<bind>_, which is a string containing an
	interaction name as well as optionally one or multiple modifier names,
	separated by "+".

	The possible interaction types are the following.

	- *mouse-{left, right, middle}*
	- *mouse-{forward, backward}*
	- *mouse-{mouse, side, extra, task, misc}*
	- *mouse-{1 - 9}*
	- *scroll-{up, down}*
	- *touch*

	The possible modifiers are the following.

	- *alt*
	- *capslock*
	- *control*
	- *logo* (super)
	- *numlock*
	- *shift*

	Behold: Due to the way the Layer-Shells keyboard interactivity was
	designed, LavaLauncher may only get send an updated modifier state when it
	gains keyboard focus. In some compositors this requires you to click on it,
	meaning that touch and scroll interactions with modifiers may not work in
	some compositors. This is a bug in the Layer-Shell protocol, not in
	LavaLauncher.

*image-path*
	The path to an image file, which will be used as the icon of the
	button.

## SPACER
Every "spacer" context will add a spacer to a bar. As such, this context is a
nested inside the "bar" context. The assignments possible in this context are
as follows.

*length*
	Length of the spacer.

## COMMANDS
Commands are executed using *sh(1)*.

LavaLauncher understands typical backslash escape sequences.

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
Conditions are (re-)evaluated and bars created and destroyed and their active
configuration set chosen accordingly whenever an outputs parameters are updated.

## IMAGES
LavaLauncher support PNG images and, if enabled at compile time, SVG images. It
is recommended to use square images.

## EXAMPLE CONFIGURATION
This is a simple configuration example, demonstrating a bar with two buttons
and an additional configuration set. The second button uses advanced command
binds.

```
global-settings
{
	watch-config-file = true;
}

bar
{
	output            = eDP-1;
	position          = bottom;
	background-colour = "#202020";

	# Condition for the default configuration set.
	condition-resolution = wider-than-high;

	config
	{
		# Condition for this configuration set.
		conditions-resolution = higher-than-wide;

		# Additional configuration sets copy the default configuration set,
		# but settings can be overwritten.
		position = left;
	}

	button
	{
		image-path = /path/to/image.png;
		command    = useful-script.sh;
	}

	button
	{
		image-path = /path/to/another/image.png;

		command[mouse-left]  = another-useful-script.sh;
		command[mouse-right] = another-useful-script.sh;

		command[shift+mouse-left]     = another-useful-script.sh;
		command[shift+alt+mouse-left] = another-useful-script.sh;

		# Binding commands to scroll events is nice to configure
		# things like audio volume or screen brightness
		command[scroll-up]         = volume up;
		command[scroll-down]       = volume down;
		command[shift+scroll-up]   = volume microphone-up;
		command[shift+scroll-down] = volume microphone-down;
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

