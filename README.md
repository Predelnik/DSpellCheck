![Build status](https://github.com/predelnik/DSpellCheck/actions/workflows/build.yml/badge.svg)

DSpellCheck
===========

Notepad++ Spell-checking Plug-In

Following main features:
- Underlining spelling mistakes
- Iterating through all mistakes in document
- Finding mistakes only in comments and strings (For files with standard programming language syntax e.g. C++, Basic, Tex and so on)
- Possible usage of multiple languages (dictionaries) simultaneously to do spell-checking.
- Getting suggestions for words by either using default Notepad++ menu or separate context menu called by special button appearing under word.
- Able to add words to user dictionary or ignore them for current session of Notepad++
- Using either Aspell library (needs to be installed), either Hunspell library (Dictionaries by default should be placed to %Plugin Config Dir%\Hunspell)
- A lot of customizing available from Plugin settings (Ignoring/Allowing only specific files, Choosing delimiters for words, Maximum number of suggestions etc)
- Support for downloading and removing Hunspell dictionaries through user friendly GUI interface
- Ability to quickly change current language through context menu or DSpellCheck sub-menu.

[Changelog](Changes.md)
