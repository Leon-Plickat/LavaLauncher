# LavaLauncher
## Description

<img src="https://git.sr.ht/~leon_plickat/lavalauncher/blob/master/.meta/example.jpg">

A simple launcher for Wayland.

This program serves a single purpose: Letting the user execute shell commands
by clicking on icons on a dynamically sized bar, placed at one of the screen
edges.

LavaLauncher is configured entirely via command flags.


## Building

LavaLauncher depends on Wayland, the Wayland Layer Shell and Cairo. To build
this program you will need a C compiler and an implementation of `make`.

    git clone https://git.sr.ht/~leon_plickat/lavalauncher
    cd lavalauncher
    make
    sudo make install


## Contributing

**Contributions are welcome!** You can send your patches via email to the
[mailing list](mailto:~leon_plickat/lavalauncher@lists.sr.ht). See
[this](https://git-send-email.io/) link to learn how to set up git for sending
emails.

For larger contributions, especially for radical changes, I highly recommend
that you ask me if I will actually include your commit *before* working on it.

You are strongly invited to add your name to the copyright header of the files
you changed and to the authors sections in the man page and README if you made
an important contribution.

Your changes will be licensed under the same license as the file(s) you modified.


## Licensing

LavaLauncher is licensed under the GPLv3.

The contents of the `lib/wayland-protocols` directory are licensed differently.
See the header of the files for more information.

The contents of the `lib/pool-buffer` directory are licensed under the MIT
license. The copyright of these files belongs to emersion. The files were copied
from the project "slurp" and slightly modified to work with LavaLauncher.


## Authors

Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
