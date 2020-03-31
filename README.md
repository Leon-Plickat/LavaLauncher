# LavaLauncher
<img src="https://git.sr.ht/~leon_plickat/lavalauncher/blob/master/.meta/example.jpg">

## Description

LavaLauncher is a simple launcher for Wayland.

It serves a single purpose: Letting the user execute shell commands by clicking
on icons on a dynamically sized bar, placed at one of the screen edges or in the
center.

Unlike most popular launchers, LavaLauncher does not care about `.desktop`
files or icon themes. To create a button, you simply provide the path to an
image and a shell command. This makes LavaLauncher considerably more flexible:
You could have buttons not just for launching applications, but also for
ejecting your optical drive, rotating your screen, sending your cat an email,
playing a funny sound, muting all audio, toggling your lamps, etc. You can turn
practically anything you could do in your shell into a button.

The configuration is done entirely via command flags. See the manpage for
details and an example.

LavaLauncher has been successfully tested with [sway](https://github.com/swaywm/sway)
and [wayfire](https://github.com/WayfireWM/wayfire).


## Building

LavaLauncher depends on Wayland, Wayland protocols and Cairo. To build
this program you will need a C compiler, the meson & ninja build system and
`scdoc` to generate the manpage.

    git clone https://git.sr.ht/~leon_plickat/lavalauncher
    cd lavalauncher
    meson build
    ninja -C build
    sudo ninja -C build install


## Mailinglist

The mailinglist is for bug reports, contributions, feedback and getting help.

[~leon_plickat/lavalauncher@lists.sr.ht](mailto:~leon_plickat/lavalauncher@lists.sr.ht)


## Contributing

**Contributions are welcome!** You can send your patches via email to the
mailing list. See [this](https://git-send-email.io/) helpful link to learn how
to send emails with git.

For larger contributions, especially for radical changes, I highly recommend you
to ask me whether I will include your commit *before* you start investing time
into it. Also please respect the coding style, even if you do not like it.

If you are looking for things to do, check out the section aptly called "TODO"
or simply grep for "TODO" in the source files.

You are strongly invited to add your name to the copyright header of the files
you changed and to the authors sections in the man page and README if you made
an important contribution. A Free Software project is proud of every contributor.


## Licensing

LavaLauncher is licensed under the GPLv3.

The contents of the `protocol` directory are licensed differently.  See the
header of the files for more information.


## TODO

* Additional 'return' command for buttons that causes LavaLauncher to exit with
  a given number or string, which might be interesting for scripting.
* Speed up drawing, either by using something other than cairo for the icons or
  by drawing icons to a subsurface.
* Different background for button the pointer hovers over. This likely requires
  speeding up of the drawing code.
* Test touch support.
* Add support for other input methods, like drawing tablets
* clone() instead of fork() ?


## Authors

Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
