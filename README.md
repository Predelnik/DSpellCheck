DSpellCheck
===========

Yet Another Notepad++ Spell-checking Plug-In

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

v1.1 Changes:
* Add Hunspell support, which is statically linked into Plug-in so you don't need to download any custom dlls to use it, dictionaries though (.aff and .dic files) should be placed by default into "***Notepad++_Dir***\plugins\Config\Hunspell\" (Location could be changed from settings).
* Add option to use native Notepad++ context menu for choosing suggestions and adding word to dictionary/ignoring word (Default option is the old way though).

v1.1.1
* Fix adding words to user dictionary using Hunspell library.

v1.1.2
* Fix crash on exit when aspell isn't presented.
* Fix some encoding problems.
* Fix words added to Hunspell to be being able to be suggested.

v1.1.3
* Fix autospellcheck option not being saved correctly.
* Fix some minor bugs.

v1.1.4
* Fix some encoding conversion problems, which were leading to very dramatic results on some systems.

v1.1.5
* Fix segfault when Aspell is presented on system but have no dictionaries.
* Make Hunspell used by default even if Aspell library is installed.

v1.1.6
* Fix working of "Add to dictionary" and "Ignore" for Aspell which were accidentally broken.

v1.1.7
* Fix plugin's crash in some system default encodings.

v1.2.0
* Add possibility to download Hunspell dictionaries from common OpenOffice mirrors using plugin interface.
* Add possibility to remove Hunspell dictionaries using plugin interface.
* Add possibility to quickly change current language through Plugins->DSpellCheck->Change_Current_Language or corresponding hot key (initially Alt + D), also such items as "Download More Dictionaries", "Customize Multiple Dictionaries" and "Remove Unneeded Dictionaries" are also presented there.
* Add resolving common Hunspell dictionaries names to more user friendly language names.
* Now by Default Hunspell user dictionaries are stored separately, this could be turned to the way it was.
* No more checking of words contained in hotlinks
* Some minor bugs were fixed.

v1.2.1
* Fix 1.2.0 specific bug that non-default encoding were causing some troubles (missing recheck) sometimes
* Make that if scrollbar was moved because of user input, immediate recheck isn't being done.
* Add support for using two dictionary Hunspell directories (by default one for all users, one for current only) and thus ability to remove and install dictionaries to/from these directories.
* Optimize plugin performance on scrolling a little.

v1.2.2
* Rename Hunspell_Path setting in DSpellCheck.ini to User_Hunspell_Path to avoid some troubles.
* Make AutoCheck turned on by default.

v1.2.3
* Fix bug with Multiple Languages settings not being saved when chosen from "Change Current Language" menu.
* Fix bug with incorrect delimiters while checking ANSI files if system default encoding is Japanese.
* Add link to short online manual to plugin menu.
