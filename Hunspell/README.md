# hunspell with cmake/qmake build for Windows mingw

## DESCRIPTION

This is a copy of [hunspell 1.3.2](http://sourceforge.net/projects/hunspell/) library that can be build with [cmake](http://cmake.org) or qmake from [QT4](http://qt.nokia.com/). Aim was to easily compile hunspell with [mingw](http://mingw.org/) on Windows. Most of unnecessary files (for generating windows library) where removed (gettext, autotools, tests...) I left project files for MS Visual Studio ;-)

Dictionaries for hunspell can be found on [OpenOffice.org wiki](http://wiki.services.openoffice.org/wiki/Dictionaries) or [extensions](http://extensions.services.openoffice.org/en).

`cmake` "version" is more complex and it should create the same output (including programs and 2 libraries) as autotool on linux.
`qmake` will produce only one library (hunspell) that includes also parsers library.

## Compiling 

### cmake

    $ mkdir build
    $ cd build
    $ cmake .. -G "MinGW Makefiles"
    $ mingw32-make
    $ mingw32-make install

### qmake

    $ qmake
    $ mingw32-make
    

## Download

hunspell library and programs compiled with cmake/mingw on Windowx XP can be found on [download page](https://github.com/zdenop/hunspell-mingw/downloads)