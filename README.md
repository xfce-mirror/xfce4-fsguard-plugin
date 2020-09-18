[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-fsguard-plugin/-/blob/master/COPYING)

# xfce4-fsguard-plugin

The FSGuard panel plugin checks free space on a chosen mount point frequently 
and displays a message when a limit is reached. 
There are two limits: a warning limit where only the icon changes, and an urgent limit
that advises the user with a message. 

The icon button can be clicked to open the chosen mount point. 
The amount of free space is visible in a tooltip.

----

### Homepage

[Xfce4-fsguard-plugin documentation](https://docs.xfce.org/panel-plugins/xfce4-fsguard-plugin)

### Changelog

See [NEWS](https://gitlab.xfce.org/panel-plugins/xfce4-fsguard-plugin/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[Xfce4-fsguard-plugin source code](https://gitlab.xfce.org/panel-plugins/xfce4-fsguard-plugin)

### Download a Release Tarball

[Xfce4-fsguard-plugin archive](https://archive.xfce.org/src/panel-plugins/xfce4-fsguard-plugin)
    or
[Xfce4-fsguard-plugin tags](https://gitlab.xfce.org/panel-plugins/xfce4-fsguard-plugin/-/tags)

### Required Packages

  * [libxfce4ui](https://gitlab.xfce.org/xfce/libxfce4ui)
  * [libxfce4util](https://gitlab.xfce.org/xfce/libxfce4util)
  * [xfce4-panel](https://gitlab.xfce.org/xfce/xfce4-panel)

### Installation

From source code repository: 

    % cd xfce4-fsguard-plugin
    % ./autogen.sh
    % make
    % make install

From release tarball:

    % tar xf xfce4-fsguard-plugin-<version>.tar.bz2
    % cd xfce4-fsguard-plugin-<version>
    % ./configure
    % make
    % make install

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/panel-plugins/xfce4-fsguard-plugin/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

