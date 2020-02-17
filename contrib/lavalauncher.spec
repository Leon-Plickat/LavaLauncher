# (c) Bob Hepple, MIT License
Name:     lavalauncher
Version:  1.6
Release:  1%{?dist}.wef
Summary:  LavaLauncher is a simple launcher for Wayland
License:  GPL
URL:      https://git.sr.ht/~leon_plickat/lavalauncher

# use this to fetch the source: spectool -g lavalauncher.spec
Source0:  %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: cairo-devel
BuildRequires: meson
BuildRequires: scdoc
BuildRequires: wayland-devel
BuildRequires: wayland-protocols-devel

%description
LavaLauncher is a simple launcher for Wayland.

It serves a single purpose: Letting the user execute shell commands by
clicking on icons on a dynamically sized bar, placed at one of the
screen edges or in the center.

Unlike most popular launchers, LavaLauncher does not care about
.desktop files or icon themes. To create a button, you simply provide
the path to an image and a shell command. This makes LavaLauncher
considerably more flexible: You could have buttons not just for
launching applications, but also for ejecting your optical drive,
rotating your screen, sending your cat an email, playing a funny
sound, muting all audio, toggling your lamps, etc. You can turn
practically anything you could do in your shell into a button.

The configuration is done entirely via command flags. See the manpage
for details and an example.

LavaLauncher has been successfully tested with sway and wayfire.
%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install

%files
%{_bindir}/%{name}

%doc README.md
%{_mandir}/man1/%{name}.1.*

%license LICENSE

%changelog
* Mon Feb 17 2020 Bob Hepple <bob.hepple@gmail.com> - 1.6-1.fc31.wef
- Initial version of the package
