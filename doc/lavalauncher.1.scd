lavalauncher(1) ["Version 1.1" ["19 December 2019"]]

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
*-b <path> <command>*
	Add a button.

	*Path* is the path to a PNG image file. It is recommended to use
	square images.

	*Command* is the shell command executed when the user clicks on the
	button. If the given command is 'exit', LavaLauncher will exit.
	LavaLauncher being able to close itself means that a user can launch
	instances that are meant to be temporary, which opens up many
	interesting possibilities, like having a main bar which launches smaller
	bars as sub-menus.

	The buttons on the bar will be shown in the order in which they are
	given by the user.

*-c <colour>*
	Colour of the bar in the format '#RRGGBBAA'.

	The default bar colour is '#000000FF'

*-C <colour>*
	Colour of the border in the format '#RRGGBBAA'.

	The default border colour is '#FFFFFFFF'
*-h*
	Display a helpful help message and exit.

*-l <overlay|top|bottom|background>*
	Layer of the bar surface.

	Typically, 'bottom' and 'background' will be underneath regular windows,
	while 'top' and 'overlay' will be above them, however the exact
	placement on the z axis depends on the implementation of the compositor.

	The default layer is 'bottom'.

*-m <default|aggressive|full|full-center>*
	The display mode of the bar.

	In 'default' mode, the surface is sized just large enough to house all
	the buttons and placed in the middle of the edge it is docked to, or in
	the outputs center, depending on the position. The surface will only be
	anchored to that edge. Choosing the position 'center' will automatically
	enable this mode.

	The 'aggressive' mode is needed, because some compositors do not respect
	the exclusive zone of surfaces that are anchored to less then three
	sides. To work around this, 'aggressive' mode will make the surface use
	the entire length of the edge it is docked to and also anchor it to the
	adjacent edges. Visually however, 'aggressive' mode should not deviate
	from 'default' mode, as the bar is only drawn in the center of the
	surface.

	In 'full' mode, the bar spans the entire length of the edge it is docked
	to. The surface is created similarly to 'aggressive' mode, however the
	bar is drawn to the full area of the surface. Icons will be drawn
	starting on the left or top side of the bar, depending on the position.

	The 'full-center' mode works just like the 'full' mode, with the
	difference that the icons will be centered.

*-M <size>*
	Margin from the screen edge the bar is docked to in pixels.

	The margin is ignored when the position is 'center'.

	The default is 0.

*-p <top|right|bottom|left|center>*
	Position of the bar.

	The bar will dock to that edge, or in the case of 'center' in the
	middle, of the output. No border will be drawn on the side where the bar
	is docked.

	Choosing 'center' will automatically set the display mode to 'default'.

	The default position is 'bottom'.

*-s <size>*
	Size of the icons in pixels.

	The default icon size is 80;

*-S <size>*
	Size of the border in pixels.

	The default border size is 2;

*-v*
	Verbose output.


# EXAMPLE
lavalauncher -b "/home/leon/.config/icons/firefox.png" firefox -b "/home/leon/.config/icons/glenda.png" acme -p bottom -s 80 -c "#20202088"


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
