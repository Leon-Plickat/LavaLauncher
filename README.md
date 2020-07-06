# LavaLauncher
<img src="https://git.sr.ht/~leon_plickat/lavalauncher/blob/master/.meta/example.jpg">

## Description

LavaLauncher is a simple launcher for Wayland.

It serves a single purpose: Letting the user execute shell commands by clicking
on icons on a dynamically sized bar, placed at one of the screen edges.

Unlike most popular launchers, LavaLauncher does not care about `.desktop`
files or icon themes and it does not track open applications; It is not a dock.
To create a button, you simply provide the path to an image and a shell command.
This makes LavaLauncher considerably more flexible: You could have buttons not
just for launching applications, but also for ejecting your optical drive,
rotating your screen, sending your cat an email, playing a funny sound, muting
all audio, toggling your lamps, etc. You can turn practically anything you could
do in your shell into a button.

LavaLauncher is opinionated, yet remains configurable. The configuration syntax
is documented in the man page. Modifying the configuration file will cause
LavaLauncher to automatically reload.

LavaLauncher has been successfully tested with [sway](https://github.com/swaywm/sway)
and [wayfire](https://github.com/WayfireWM/wayfire).


## Installation

### Packages

The following distributions have an official LavaLauncher package:

* Fedora
* Gentoo

### Building

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

**Contributions are welcome!** Read `CONTRIBUTING.md` to find out how you can
contribute.


## Licensing

LavaLauncher is licensed under the GPLv3.

The contents of the `protocol` directory are licensed differently.  See the
header of the files for more information.


## Authors

Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>

