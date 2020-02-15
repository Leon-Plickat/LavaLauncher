lavalauncher(1) ["Version 1.5" ["Version 1.5"]]

# NAME

LavaLauncher - A simple launcher panel for Wayland desktops


# SYNOPSIS
*lavalauncher* [options...]


# DESCRIPTION
LavaLauncher serves a single purpose: Letting the user execute shell commands
by clicking on icons on a dynamically sized bar, placed at one of the screen
edges.

The Wayland compositor must implement the Layer-Shell for LavaLauncher to work.
If LavaLauncher is displayed but wont respond to clicks, chances are that the
compositor you are using only displays Layer-Shell surfaces but does not forward
events to them.


# OPTIONS
*-a <alignment>*
	Alignment of the icons.

	Can be 'start', 'center' or 'end'.  The default is 'center'.

*-b <path> <command>*
	Add a button.

	*Path* is the path to a PNG image file. It is recommended to use square
	images. *Command* is the shell command executed when the user clicks on
	the button. The command is executed by the users system shell. If the
	given command is 'exit', LavaLauncher will exit.  LavaLauncher
	replaces '%buttons%' with the amount of buttons, '%icon-size%' with the
	icon size, '%border-{top, right, bottom, left}%' with the corresponding
	border size, '%colour%' with the colour of the bar, '%border-colour%'
	with the colour of the border, '%output%' with the name of the output
	and '%scale%' with the scale of the output.

	The buttons on the bar will be shown in the order in which they are
	given by the user.

*-c <colour>*
	Colour of the bar in the format '#RRGGBBAA' or '#RRGGBB'.

	The default bar colour is '#000000'

*-C <colour>*
	Colour of the border in the format '#RRGGBBAA' or '#RRGGBB'.

	The default border colour is '#FFFFFF'

*-e <mode>*
	Exclusive zone of the surface.

	Can be 'true', 'false' or 'stationary'. If 'true', the compositor is
	informed that it should not obstruct the surface of LavaLauncher with
	other surfaces. If 'false', the compositor is informed that it can place
	other surfaces over or under the surface of LavaLauncher.  If 'stationary',
	the compositor is informed that the surface of LavaLauncher should not
	be moved to accommodate other layer shell surfaces and that it can
	freely place other surfaces over or under the surface of LavaLauncher.
	The default is 'true'.

	The exact implementation is compositor dependant; A compositor may
	choose to ignore the requested exclusive zone of surfaces.

*-h*
	Display a helpful help message and exit.

*-l <layer>*
	Layer of the bar surface.

	Can be 'overlay', 'top', 'bottom' or 'background'. Typically, 'bottom'
	and 'background' will be underneath regular windows, while 'top'
	and 'overlay' will be above them, however the exact placement on the z
	axis depends on the implementation of the compositor. The default layer
	is 'bottom'.

*-m <mode>*
	The display mode of the bar.

	Can be 'default' or 'full'. In 'default' mode, the bar is sized just
	large enough to house all the buttons. Logically, the default mode
	is 'default'. In 'full' mode, the bar spans the entire length of the
	edge it is docked to.

*-M <top> <right> <bottom> <left>*
	Margin.

	The default margins are 0 0 0 0.

*-o <name>*
	Name of the output on which the bar should be displayed.

	If this option is omitted, the bar will be displayed on all outputs,
	which is the default behaviour.

*-p <position>*
	Position of the bar.

	Can be 'top', 'right', 'bottom' or 'left'. The bar will dock to that
	edge of the output.  The default position is 'bottom'.

*-s <size>*
	Size of the icons in pixel.

	The default icon size is 80;

*-S <top> <right> <bottom> <left>*
	Size of the borders in pixel.

	The default border sizes are 2 2 0 2.

*-v*
	Verbose output.


# EXAMPLE
lavalauncher -b "~/icons/foo.png" "notify-send 'Output: %output%'" -b "~/icons/glenda.png" acme -p bottom -a center -s 80 -S 2 2 0 2 -c "#20202088" -o eDP-1


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
