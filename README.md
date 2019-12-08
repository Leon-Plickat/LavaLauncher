# LavaLauncher
## Description

<img src="https://git.sr.ht/~leon_plickat/lavalauncher/blob/master/.meta/example.jpg">

A simple launcher for Wayland.

This program serves a single purpose: Letting the user execute shell commands
by clicking on icons on a dynamically sized bar, placed at one of the screen
edges or in the center.

LavaLauncher is configured entirely via command flags. See the manpage for
details and an example.


## Building

LavaLauncher depends on Wayland, Wayland protocols and Cairo. To build
this program you will need a C compiler and a GNU compatible implementation of
`make`.

    git clone https://git.sr.ht/~leon_plickat/lavalauncher
    cd lavalauncher
    make
    sudo make install


## Contributing

**Contributions are welcome!** You can send your patches via email to the
mailing list. See [this](https://git-send-email.io/) helpful link to learn how
to send emails with git.

For larger contributions, especially for radical changes, I highly recommend you
to ask me whether I will include your commit *before* you start investing time
into it.

You are strongly invited to add your name to the copyright header of the files
you changed and to the authors sections in the man page and README if you made
an important contribution.

Your changes will be licensed under the same license as the file(s) you modified.


## Mailinglist

The mailinglist is for bug reports, contributions, feedback and getting help.

[~leon_plickat/lavalauncher@lists.sr.ht](mailto:~leon_plickat/lavalauncher@lists.sr.ht)


## Licensing

LavaLauncher is licensed under the GPLv3.

The contents of the `lib/wayland-protocols` directory are licensed differently.
See the header of the files for more information.

The contents of the `lib/pool-buffer` directory are licensed under the MIT
license. The copyright of these files belongs to emersion. The files were copied
from the project "slurp" and slightly modified to work with LavaLauncher.


## Authors

Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
