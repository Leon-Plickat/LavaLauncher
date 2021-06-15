lavalauncher(1)

# NAME
LavaLauncher - A simple launcher panel for Wayland desktops


# DESCRIPTION
LavaLauncher is a simple launcher panel for Wayland desktops.

It displays a dynamically sized bar with user defined buttons. These Buttons
consist of an image, which is displayed as the button icon on the bar, and at
least one shell command, which is executed when the user activates the button.

Buttons can be activated with pointer and touch events.

The Wayland compositor must implement the Layer-Shell and XDG-Output for
LavaLauncher to work.


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
to find a configuration file at the following paths, in descending priority.

- ./lavalauncher.conf
- $XDG_CONFIG_HOME/lavalauncher/lavalauncher.conf
- $HOME/.config/lavalauncher/lavalauncher.conf
- /usr/local/etc/lavalauncher/lavalauncher.conf
- /etc/lavalauncher/lavalauncher.conf

The configuration syntax is similar to the common INI format, consisiting of
*sections* and *assignments*. Assignments are always inside sections and on a
single line, unless the newline is escaped, and set a *variable* to a *value*.
Comments start with an octothorpe ('#'). If you want to use the octothorpe
symbol in an assignment, you have to escape it ('\#'). Backslashes in
assignments need to be escaped as well ('\\'). All trailing and leading
whitespace is ignored.

```
# Comment
[section]
variable = value
```

You can find explanation of all existing sections and variables following this
introduction as well as an example configuration at the end of this section.

Concepts, formats and other things will be explained only the first time they
appear, so it is recommended you read this document in its entirety.

## [global-settings]
In this section global settings can be configured.

*watch-config-file* = true|false
	Automatically reload when a change in the configuration file is detected.
	Defaults to _false_.

	Behold: If the configuration file contains an error upon reload,
	LavaLauncher will exit.

## [config]
Every instance of this section will add a configurattion set.

Configuration sets define the current configuration of the bar. Multiple
configuration sets can be defined. The first configuration set to be defined is
considered the default one and all following sets will implicitly copy the
defaults values unless overwritten.

For every output, LavaLauncher tries to find a valid configuration set, based
on the defined conditions. Only outputs for which a valid configuration set has
been found will have a bar. The sets are evaluated in the same order they have
been defined in. Whenever an outputs parameters are updated, configuration sets
are re-evaluated and the outputs bar modified, created or destroyed accordingly.

By default the condititions of a configuration set match every output. To make
use of multiple configuration sets, you have to explicitly set the conditions
of the default configuration set so it is not valid in the cases you want
LavaLauncher to use another configuration set for.

The following assignments can be made in this section.

*background-colour* = <colour>
	The background colour of the bar. Defaults to _0x000000_.

	LavaLauncher can parse hex code colours and read RGB values directly,
	supporting the following formats.

	- *0xRRGGBB*
	- *0xRRGGBBAA*
	- *rgb(RRR,GGG,BBB)*
	- *rgba(RRR,GGG,BBB,AAA)*

*border-colour* = <colour>
	The border colour of the bar. Defaults to _0xffffff_.

*border* = <size>
	Set the size of all four borders at once. Defaults to _1_.

*border* = <top size> <right size> <bottom size> <left size>
	Set the size of all four borders individually.

*condition-resolution* = all|wider-than-high|higher-than-wide
	The resolution proportions an output must have for this configuration
	set to be valid. Defaults to _all_.

*condition-scale* = all|<scale>
	The scale an output must have for this configuration set to be valid.
	Defaults to _all_.

*condition-transform* = all|1|2|3|4
	The rotation an output must have for this configuration set to be valid.
	Defaults to _all_.

	Behold: The numbers 1 to 4 represent rotations of 0, 90, 180 and 270
	degrees, with the exact correspondence depending on the compositor.

*cursor-default* = <string>
	Name of the default cursor. Defaults to _default_.

	Behold: What cursors are available depends on your active cursor theme.

*cursor-hover* = <string>
	Name of the default cursor displayed when hovering the pointer over the
	bar. Defaults to _pointer_.

*cursor-size* = <size>
	Set the size of the cursor. Must be at least 24. Defaults to the value
	of the *$XCURSOR_SIZE* environmental variable, or, if that is not set,
	to _24_.

*exclusive-zone* = true|false|stationary.
	Exclusive zone of the surface. Defaults to _true_.

	If *true*, the compositor is informed that it should not obstruct the
	surface of LavaLauncher with other surfaces.

	If *false*, the compositor is informed that it can place other surfaces
	over or under the surface of LavaLauncher.

	If *stationary*, the compositor is informed that the surface of
	LavaLauncher should not be moved to accommodate other Layer-Shell
	surfaces and that it can freely place other surfaces over or under
	the surface of LavaLauncher.

	Behold: A compositor may choose to ignore the requested exclusive zone
	of surfaces.

*hidden-mode* = never|always|river-auto
	Hidden mode of the bar. Defaults to _never_.

	In *never* mode, the bar is never hidden.

	In *always* mode, the bar is always hidden.

	The *river-auto* mode works only with the river Wayland compositor. In
	this mode, the bar is only hidden if the output is currently occupied.

	Behold: If you want the common use case of a hidden bar which sits above
	windows on hover, you need to set the *layer* to _top_ and the
	*exclusive zone* to _off_ or _stationary_.

*hidden-size* = <size>
	Size of the invisible area where hovering with a pointer will unhide the
	bar if it is hidden. Defaults to _10_.

*icon-padding* = <size>
	Padding around the icons. Defaults to _4_.

*indicator-active-colour* = <colour>
	The colour of the indicator for icons being activated. Defaults to
	_0x606060_.

*indicator-hover-colour* = <colour>
	The colour of the indicator for icons over which the cursor hovers.
	Defaults to _0x404040_.

*indicator-padding* = <size>
	Padding around the icon indicator. Defaults to _0_.

*layer* = orverlay|top|bottom|background
	Layer of the bar surface. Defaults to _bottom_.

	Behold: Typically, *bottom* and *background* will be underneath regular
	windows, while *top* and *overlay* will be above them, however the exact
	placement on the z axis depends on the compositor.

*margin* = <size>
	Set the size of all four margins at once. Defaults to _0_.

	Behold: Some compositors may not respect margins parallel to the bars
	orientation if the *mode* is _default_. This is not a bug in
	LavaLauncher but a (possibly intentional) limitation of the compositor.

*margin* = <top size> <right size> <bottom size> <left size>
	Set the size of all four margins individually.

*mode* = default|full
	The display mode of the bar. Defaults to _default_.

	In *default* mode, the bar is sized just large enough to house all the
	buttons and placed in the center of the output edge it is docked to.

	In *full* mode, the bar spans the entire length of the edge it is docked
	to.

*namespace* = <string>
	The namespace of the Layer-Shell surface of the bar. Defaults to
	_lhp.LavaLauncher_.

	Some compositors may treat Layer-Shell surfaces differently based on
	their namespace. 

	Behold: This option only takes effect once on the initial bar creation.
	If the active configuration set changes, this value will stay the same.

*output* = all|\*|<string>
	Name of the output on which this configuration set is valid. Defaults to
	_all_.

	If set to *all* or *\**, this configuration set is valid on all outputs.

	Behold: Output names are compositor dependant.

*position* = top|right|bottom|left
	Position of the bar. Defaults to _bottom_.

*radius* = <size>
	Set the radius of all four corners at once. Defaults to _5_.

	If set to *0*, corner roundness is disabled.

*radius* = <top-left size> <top-right size> <bottom-left size> <bottom-right size>
	Set the radii of all four corners individually.

*size* = <size>
	Size of the bar (without borders). Defaults to _60_.

## [item:button]
Every instance of this section will add a button to the bar.

*command* = <string>
	Bind a command to left-, right-, middle-click and touch interactions.

	By default the command string is interpreted as a shell command and,
	uppon activation of the button, executed with *sh(1)*.

	Additionally, a set of meta commands is available, causing special
	behaviour. Meta commands can be used instead of or in combination with
	shell commands, but meta commands must always be the first part of the
	command string. The following meta commands are available.

	- *@toplevel-activate* If a toplevel matching the associated app-id of
	  the button is found, it is activated ("focused"). Otherwise if a shell
	  command has been provided it is executed.
	- *@toplevel-close* If a toplevel matching the associated app-id of
	  the button is found, it is closed. Otherwise if a shell command has
	  been provided it is executed.
	- *@exit* LavaLauncher will exit. If a shell command has been provided
	  it will be executed beforehand.
	- *@reload* LavaLauncher will reload. If a shell command has been
	  provided it will be executed beforehand.

	LavaLauncher sets the following environmental variables for the executed
	shell commands. As an example, they could be used as arguments for
	launched programs.
	
	- *$LAVALAUNCHER_OUTPUT_NAME* The name of the output the button has been
	  clicked on.
	- *$LAVALAUNCHER_OUTPUT_SCALE* The scale of the output the button has
	  been clicked on.

*command[*<bind>*]* = <string>
	Advanced bind, allows binding a command to a specific interaction plus
	modifier combination. This combination is defined by *<bind>*, which is
	a string containing an interaction name as well as optionally one or
	multiple modifier names, separated by *+*.

	The possible interaction types are the following.

	- *mouse-{left, right, middle}*
	- *mouse-{forward, backward}*
	- *mouse-{mouse, side, extra, task, misc}*
	- *mouse-{1-9}*
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
	designed, LavaLauncher may only get send an updated modifier state when
	it gains keyboard focus. In some compositors this requires you to click
	on it, meaning that touch and scroll interactions with modifiers may not
	work in some compositors. This is a bug in the Layer-Shell protocol, not
	in LavaLauncher.

*image-path* = <path>
	The path to an image file, which will be used as the icon of the
	button.

	LavaLauncher support PNG images and, if enabled at compile time,
	SVG images. It is recommended to use square images.

*toplevel-app-id* = none|<string>
	Toplevel app-id associated with this button. If set, this button will
	have toplevel indicators. Defaults to _none_.

## [item:spacer]
Every instance of this section will add a spacer to the bar.

*length* = <size>
	Length of the spacer.

## EXAMPLE CONFIGURATION
This is a simple configuration example, demonstrating a bar with three buttons
and an additional configuration set. The second button uses advanced command
binds. The third button implements the common "focus or spawn" functionality
for firefox.

```
[global-settings]
watch-config-file = true

[config]
output = eDP-1
position = bottom
background-colour = 0x202020
condition-resolution = wider-than-high
border = 1 1 0 1

[config]
conditions-resolution = higher-than-wide
position = left
border = 5

[item:button]
image-path = /path/to/image.png
command = useful-script.sh

[item:button]
image-path           = /path/to/another/image.png
command[mouse-left]  = another-useful-script.sh
command[mouse-right] = another-useful-script.sh
command[scroll-up]   = volume up;
command[scroll-down] = volume down;

[item:button]
toplevel-app-id = firefox
image-path = firefox.png
command = @toplevel-activate firefox
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

