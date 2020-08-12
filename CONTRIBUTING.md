# Contributing to LavaLauncher
This file explains how you can contribute to LavaLauncher.

For larger contributions, especially for radical changes, I highly recommend you
to ask me whether I will include your commit *before* you start investing time
into it. Also please respect the coding style, even if you do not like it.


## Communication
You can ask questions and start discussions on the [mailing list](mailto:~leon_plickat/lavalauncher@lists.sr.ht)
or if you find me on IRC (my nick is "leon-p" and you can often find me on
`#sway@freenode.net`).

If you found this project on GitHub or any other platform having some sort of
"issue" system, be aware that 1) I massively prefer the mailing list and 2) that
platform is only used as a mirror. Try your luck, but know that I do not
guarantee that I will notice your message.


## Sending Patches
You can send your patches via email to the mailing list. See
[this](https://git-send-email.io/) helpful link to learn how to correctly send
emails with git. Alternatively you can also manually attach a git patch file to
a mail, but beware that this is more work for both you and me.

All code review will happen on the mailing list as well.

Write good commit messages!

If you found this project on GitHub or any other platform having some sort of
"pull request" or "merge request" system, be aware that 1) I massively prefer
the email based git workflow and 2) that platform is only used as a mirror.
Try your luck, but know that I do not guarantee that I will notice your
contribution.


## Licensing and Copyright
LavaLauncher is licensed under the GPLv3, which applies to any contributions as
well. I will not accept contributions under a different license, even if you
create new files. The one single exception to this are Wayland protocol
extensions.

You are strongly invited to add your name to the copyright header of the files
you changed and if you made an important contribution also to the authors
sections in the man page and README. A Free Software project is proud of every
contributor.


## ToDo
This is a list of both rough spots in the code that need some improvement as
well as features I have planned for LavaLauncher. Feel free to pick up any of
these projects.

* Some widgets would be useful. With indicators for battery capacity, network
  status, time and workspace, LavaLauncher could completely replace a status
  bar. The sanest way to achieve this is probably to allow external applications
  to draw to a buffer which is attached to a wl_surface which is set as a
  subsurface of the bar surface (that way, we might not even need to do anything
  special in the event loop to communicate with those applications). This needs
  more investigation and thought.
* In LavaLaunchers code, scaling is done all over the place. It would be nice
  (and probably reduces complexity as well) if scaling was done all at once.
* LavaLauncher should handle drawing tablet events.
* LavaLauncher should handle touch screen swipes.
* LavaLauncher should handle touch pad swipes.
* A lot of code needs better documentation.
* A lot of code could probably be simplified.


## Style
This section describes the coding style of LavaLauncher. Try to follow it
closely.

Yes, it is not K&R. Cry me a river.

Indent with tabs exclusively (looking at you, Emacs users).

Lines should not exceed 80 characters. It is perfectly fine if a few lines are a
few characters longer, but generally you should break up long lines.

Braces go on the next line with struct declarations being the only exception.
If only a single operation follows a conditional or loop, braces should not be
used.

    static void function (void)
    {
        struct Some_struct some_struct = {
            .element_1 = 1,
            .element_2 = 2
        };

        if ( val == 1 )
        {
            function_2();
            function_3();
        }
        else
            function_4();
    }

For switches, indent the case labels once and the following code twice.

    switch (variable)
    {
        case 1:
            /* do stuff... */
            break;
    }

Conditionals are only separated by a space from their surrounding parenthesis
if they contain more than just a single variable.

    if (condition)
    {
        /* do stuff... */
    }
    else if ( variable == value )
    {
        /* do stuff... */
    }

An exception to this are `for` loops, where a space is only but always inserted
after each semicolon.

    for (int a = 0; a < 10; a--)
    {
        /* do stuff... */
    }

Logical operators in in-line conditionals or variable declarations / changes do
not necessarily require parenthesis.

    bool variable        = a == b;
    int another_variable = a > b ? 4 : 5;

Use `/* comment */` for permanent comments and `// comment` for temporary
comments.

Sometimes it makes sense to align variable declarations. But only sometimes.

    type       name_3      = value;
    type_1     name_acs    = value;
    type_scsdc name_23     = value;
    type_abc   name_advdfv = value;

You can combine multiple declarations and calculations with commas. Just be
careful that the code is still readable.

    int a, b;
    a = 1 + 2, b = a * 3;

Name your bloody variables! With very, very few exceptions, a variable name
should at least be three characters long. When someone sees your variable, they
should pretty much immediately know what information is stored in it without
needing to read the entire function. Variable names should generally be lower
case, but exceptions are perfectly fine if justified.

    DoNotUseCamelCase. Underscores_are_more_readable.

If the data stored in your variable is complex / does complex things /
influences complex things, document that.

Variables usually should have a lower case name, although exceptions are fine if
justified. The very first letter of a struct type or enum type name must be
capitalised. Members of an enum must be fully caps.

    enum My_fancy_enum
    {
        FANCY_1,
        FANCY_2,
        FANCY_3
    }

    struct Fancy_struct
    {
        int variable;
    }

    int another_variable;
    struct Fancy_struct fs;

`switch`, `while` and `for` may be in-lined after an `if` or `else` to
reduce indentation complexity, but only if readability is not compromised and
80 characters are not excessively exceeded. Under the same conditions, `case`
blocks may be in-lined, if they only contains a single operation.

    if (condition) switch (mode)
    {
        case MODE_1: do_stuff_1(); break;
        case MODE_2: do_stuff_2(); break;
        case MODE_3: do_stuff_3(); returned;
    }
    else while (stuff)
    {
        /* Do things... */
    }

`if` may be in-lined after `while`, `for` and `else` to reduce indentation
complexity, but only if readability is not compromised and 80 characters are not
excessively exceeded.

    while (variable) if (condition)
    {
        /* Do stuff... */
    }

    for (int a = b; a > c; a += b) if (condition)
    {
        /* Do stuff... */
    }

    if (condition)
    {
        /* Do stuff... */
    }
    else if (condition)
    {
        /* Do stuff... */
    }

Never in-line variable declaration or changes, function calls or `return`,
`goto`, `break` and friends.

