lavalauncher(1) ["Version 1.6" ["Version 1.6"]]

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
	Alignment of the icons / bar.

	Can be 'start', 'center' or 'end'. The default is 'center'. This option
	has no effect in 'simple' mode.

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
	defined by the user.

*-c <colour>*
	Colour of the bar.

	The default bar colour is 'black'

*-C <colour>*
	Colour of the border.

	The default border colour is 'white'

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

	Can be 'default', 'full' or 'simple'. In 'default' mode, the bar is
	sized just large enough to house all the buttons. Logically, the default
	mode is 'default'. In 'full' mode, the bar spans the entire length of the
	edge it is docked to. 'simple' mode is visually identical to 'normal'
	mode, however the surface of the bar is sized and anchored less
	aggressively, which may (or may not; depending on the implementation of
	the compositor) improve how well LavaLauncher geometrically integrates
	and fits onto the desktop. It gives the compositor a lot more leeway
	regarding the exact placement of the bar, at the cost of not respecting
	the user defined alignment and margins parallel to the bar.

*-M <top> <right> <bottom> <left>*
	Directional margins of the surface,

	The default margins are 0 0 0 0.

*-o <name>*
	Name of the output on which the bar should be displayed.

	Output names are compositor dependant. If the argument 'all' is given,
	LavaLauncher will display a bar on all outputs, which is the default
	behaviour.

*-p <position>*
	Position of the bar.

	Can be 'top', 'right', 'bottom', 'left'. The bar will dock to that edge
	of the output(s). The default position is 'bottom'.

*-P <cursor>*
	Name of the cursor displayed when hovering over the bar.

	The default cursor is 'pointing_hand'. Behold: What cursors are
	available depends on the cursor theme.

	If the cursor of the given name can not be loaded, LavaLauncher will
	display a warning and disable changing the cursor.

*-s <size>*
	Size of the icons in pixel.

	The default icon size is 80;

*-S <size>*
	Size of all borders in pixel.

	The default border size is 2.

*-S <top> <right> <bottom> <left>*
	Size of each individual border in pixel.

*-v*
	Enable verbose output.


# COLOURS
LavaLauncher excepts colours in the formats '#RRGGBBAA' or '#RRGGBB'.
Alternatively, 'transparent', 'white' and 'black' are accepted as well.


# EXAMPLE
lavalauncher -b "~/icons/foo.png" "notify-send 'Output: %output%'" -b "~/icons/glenda.png" acme -p bottom -s 80 -S 2 2 0 2 -c "#20202088" -o eDP-1


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
