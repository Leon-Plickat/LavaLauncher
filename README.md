# LavaLauncher

<img src="https://git.sr.ht/~leon_plickat/lavalauncher/blob/master/.meta/example.jpg">


## Description

LavaLauncher is a simple launcher panel for Wayland desktops.

It displays a dynamically sized bar with user defined buttons. Buttons consist
of an image, which is displayed as the button icon on the bar, and at least one
shell command, which is executed when the user activates the button.

Buttons can be activated with pointer and touch events.

A single LavaLauncher instance can provide multiple such bars, across multiple
outputs.

The Wayland compositor must implement the Layer-Shell and XDG-Output for
LavaLauncher to work.

Beware: Unlike applications launchers which are similar in visual design to
LavaLauncher, which are often called "docks", LavaLauncher does not care about
`.desktop` files or icon themes nor does it keep track running applications.
Instead, LavaLaunchers approach of manually defined buttons is considerably more
flexible: You could have buttons not just for launching applications, but for
practically anything you could do in your shell, like for ejecting your optical
drive, rotating your screen, sending your cat an email, playing a funny sound,
muting all audio, toggling your lamps and a lot more. Be creative!

LavaLauncher is opinionated, yet remains configurable. The configuration syntax
is documented in the man page.

LavaLauncher has been successfully tested on
[sway](https://github.com/swaywm/sway),
~~[wayfire](https://github.com/WayfireWM/wayfire)~~ (Wayfire currently does not
respect subsurfaces ordering used by LavaLauncher),
[river](https://github.com/ifreund/river) and
[hikari](https://hikari.acmelabs.space/).


## Installation

### Packages

LavaLauncher has been packaged into a few software repositories.

[![Packaging status](https://repology.org/badge/vertical-allrepos/lavalauncher.svg)](https://repology.org/project/lavalauncher/versions)

Many thanks to the maintainers of these packages!


### Building

LavaLauncher depends on Wayland, Wayland protocols, xkbcommon and Cairo. To
compile LavaLauncher with SVG image support, it additionally depends on librsvg.

To build this program you will need a C compiler, the meson & ninja build system
and `scdoc` to generate the manpage.

    git clone https://git.sr.ht/~leon_plickat/lavalauncher
    cd lavalauncher
    meson build
    ninja -C build
    sudo ninja -C build install


## Mailinglist

The mailinglist is for bug reports, contributions, feedback and getting help.

[~leon_plickat/lavalauncher@lists.sr.ht](mailto:~leon_plickat/lavalauncher@lists.sr.ht)

If you found this project on [GitHub](https://github.com/Leon-Plickat/LavaLauncher),
you can use that platforms contribution workflow and bug tracking system, but
beware that I may be slow to respond to anything but email and that development
officially takes place over at [sourcehut](https://sr.ht/~leon_plickat/LavaLauncher/),
the main hosting platform for this project.


## Contributing

**Contributions are welcome!** Read `CONTRIBUTING.md` to find out how you can
contribute. Do not be afraid, it is really not that complicated.


## Licensing

LavaLauncher is licensed under the GPLv3.

The contents of the `protocol` directory are licensed differently. Please see
the header of the files for more information.


## Authors

Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>

