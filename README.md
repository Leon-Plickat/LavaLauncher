# LavaLauncher
## Description

A simple launcher for Wayland.

This program serves a single purpose: Letting the user execute shell commands
by clicking on icons on a dynamically sized bar, placed at one of the screen
edges. That's it; Not more, not less.

LavaLauncher is configured entirely via command flags.


## Building

LavaLauncher depends on Wayland, the Wayland Layer Shell and Cairo. To build
this program you will need a C compiler and an implementation of `make`.

    git clone https://git.sr.ht/~leon_plickat/lavalauncher
    cd lavalauncher
    make
    sudo make install


## Contributions

**Contributions are welcome!** You can send your patches via email. See
[this](https://git-send-email.io/) link to learn how to set up git for sending
emails.

For larger contributions, especially for radical changes, I highly recommend you
to ask me if I will actually include your change(s), *before* working on your
patch.

You are strongly invited to add your name to the copyright header of the files
you changed and to the authors sections in the man page and README, if you feel
like you made an important contribution.


## Licensing

LavaLauncher is licensed under the GPLv3.

The contents of the `lib/` directory are licensed differently. See the header
of the files for more information.

This is a *Free Software* project, **not** an *Open Source* project.


## Authors

Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
