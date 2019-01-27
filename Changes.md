v1.4.10
* Fix string literals in some languages not being checked properly. (#166)
* Add the action to replace word with the first suggestion. (#165)
* Improve decoding of dictionary names for Windows Vista+. (#164)
* Fix problem with display of "special button". (#162)
* Add action to possibly call context menu with suggestions by hotkey. (#160)
* Add action to replace the same words in whole document with a selected suggestion. (#155)
* Fix occasional error messages appearing when Notepad++ was launched from git bash.
* Make dictionaries loading asynchronous. (#134)
* Fix CamelCase splitting for words with all-capitals.

v1.4.9
* Fix hang when spell-checking file containing invalid utf-8 characters.

v1.4.8
* Fix crash when spell-checking multiple languages with Aspell.

v1.4.7
* Remove hotkeys enabled by default because they collide with useful actions in some localizations (#150). Please configure them yourself through Settings->Shortcut Mapper if you need them.
* Fix exception triggered by horizontal scrolling when some lines are collapsed (#151)
* Separate Aspell Options to different dialog. Add option to configure personal (additional) dictionaries directory. (#152)
* Add third option to split words: by non-alpha/numeric characters from ANSI (current non-unicode locale in windows system) + all non ANSI characters with possible exclusions. (#149)
* Add option to remove all misspelled words to Additional Actions. (#147)
* Correct name for HIL_PH language abbreviation. (#145)

v1.4.6
* Support downloading of dictionaries from GitHub repositories. (#143, #61)
* Add LibreOffice repository as a default server for dictionaries.
* Make user dictionary files updated immediately on adding new word to them (#142)
* Make more explicit message about aspell DLL targeting incorrect architecture.

v1.4.5
* Exceptions on single line file which does not fit to the window (#139)
* Erroneous underline positioning with ANSI-encoded files (#140)

v1.4.4
* Fix hang on exit if plugin NppMenuSearch is installed (#135)
* Limit spell-checking to visible area in case if extremely long lines are present in file (#136)
* Remove FTP server which stopped hosting dictionaries from default selection (#137)

v1.4.3
* Update styles for rechecking .ini files (#54)
* Make words with non-arabic numerics also ignored if corresponding option is on (#58)
* Add check box for spell-checking/ignoring default style for UDL (turned on by default) (#132)
* Add info about compiler which was used for building plugin to about menu.

v1.4.2
* Prevent creating of plugin settings directory with wrong path. (#131)
* Restore spell-checking of YAML default style. (#129)
* Fix adding words to unified user dictionary for dictionaries with non-UTF8 encoding. (#130)

v1.4.1
* Restore spell-checking of text inside HTML.

v1.4.0
* Major internal code refactoring to make further changes and maintenance easier.
* Enable Spelling in Comments and Delimiters for UDL (#73)
* Updated Hunspell to 1.6.2
* Fix problems with dictionaries inheriting wrong permissions on download. (#83)
* Support "Allow Run-Together" option for Aspell.
* Exclude words having all capital letters from having non-first capital letter (#74)
* Support String/Comment detection for previously unsupported languages. (#71)
* Add option to split words by any non-alphabetic/numeric characters except specified exclusions. (New default)
* Support splitting of CamelCase words (disabled by default) (#89)
* Support native windows spell-checking as the third spell-checker on Windows 8+ (#65)
* Correctly handle period at the end of words like etc. (#48)
* Add possibility to check variable/function names (disabled by default) (#37)
* Add hidden option Word_Minimum_Length to disable checking of words with length less or equal to its value
* Supported using any custom Hunspell syntax in user dictionaries.
* Simplify external localization of plugin.
* Fix plugin icons and issues in high-dpi mode.
* Change usage libiconv to winiconv which should lower executable size.
* Enable more optimization options during compilation.

v1.3.5
* Fix disappearance of auto spell-checking icon.
* Fix work of system-wide dictionaries.

v1.3.4
* Fix exception on exit on some systems (#118)
* Fix recursive scanning for dictionaries (#113)

v1.3.3
* Fix non-ASCII delimiters saving/restoring (#108)
* Fix crash in case if settings file cannot be written (#110)
* Increase FTP timeout and disable passive mode which should help with its usage in some cases (#111)

v1.3.2
* Restore correct work of "Ignore for Current Session" functionality (#104)
* Restore downloading dictionaries on Windows XP (#107)

v1.3.1
* Restore correct work of plugin on systems missing Visual Studio redistributable package (#103)
* Fix creation of directory for dictionaries (#94)
* Restore work of unified user dictionary (#87)

v1.3.0
* Simplify plugin interaction with worker threads. Now most of the job is done in the main thread, it should increase plugin stability on exit.

v1.2.14.2
* Fix plugin's sudden demise after user executes "Find in Files"

v1.2.14.1
* Fix hanging on exit in newer version of Notepad++ in most common case.

v1.2.14
* Disable mistake underlining when printing is done.

v1.2.13
* Fix some issues regarding plugin exiting process.

v1.2.12
* Minor fixes with context menu on selection.
* Add marquee style to progress bar for dictionaries being downloaded through ftp web proxy.

v1.2.11
* Fix wrong context menu behaviour when selection isn't empty.

v1.2.10
* Prevent suggestions from appearing in totally wrong places.
* Make verbose Aspell reporting if error happened.

v1.2.9
* Fix bug which stopped suggestions from appearing on context menu call if the word was already selected .
* Make double quoted lines in php being checked.
* Make plugin less conflicting with Quick Color Picker plugin.

v1.2.8
* Make context menu way of suggestion selection to be default one.
* Update tool bar icon to be more commonly recognizable as spell-check icon.
* Rename name of tool bar button / check box to more understandable "Spell Check Document Automatically"
* Fix a bug of context menu showing additional items in it's sub-menu also.
* Fix a bug of scrolling being blocked while mouse cursor on suggestion button.

v1.2.7
* Fix cpu consuming bug when file doesn't have any line ends.
* Update default delimiters
* Make suggestion button never being drawn outside of editor window
* Make apostrophes at the beginning or in the end of words (if corresponding option is turned on) not being underlined and not being affected by replacing word through suggestions
* Make multi-line strings in python being checked.
* Make selection disappear after word is being added to dictionary or ignored.
* Fix that focus is now restored correctly to editor window after usage of suggestion button.

v1.2.6.1
* Fix crash on Notepad++ exit in some cases.
* Fix Aspell multiple languages not being correctly updated

v1.2.6
* Fix Doxygen line comments being not checked in c++/c files
* Fix Aspell working without English installed.
* Fix Aspell not updating it's status correctly sometimes.
* Restore plugin's work on old (prior to sp2) windows XP systems.
* Fix long lag of suggestion box on windows xp systems (at least it certainly appeared on virtual machines).
* Rise stability a little bit.

v1.2.5
* Fix some important issues connected with removing and downloading new dictionaries.
* Fix suggestions missing in multiple languages in some cases.
* Fix hotspot not being detected in some cases.
* Tool bar icon for Auto-check document menu item was added.
* Crash with usage of some dictionaries was fixed (i.e. Bulgarian)
* Separate network operations to different thread so timeouts will not interfere with plugin work.
* Fix one letter words ignoring for UTF-8 encoding in some cases.
* Initial ability to use proxy for ftp downloading.
* Option for ignoring apostrophe at the end of the word now is ignoring apostrophes at the beginning also. (thus renamed)
* Determination of current word for context menu suggestion mode is now based on caret position rather than mouse position
* Defaults delimiters updated a little.
* Fixed bug: focus wasn't being restored after using menu from suggestions button
* Fixed bug: in multiple language selection dialog - some languages could be corresponding to other languages actually.

v1.2.4
* Make option "ignore words starting or ending with apostrophe" to be turned off by default (still it's recommend for Aspell users)
* Add option to ignore apostrophe at the end of the words (turned on by default)
* Fix wrong message and behaviour when Hunspell directory for dictionaries doesn't exist.
* Restore work of multiple languages list for Aspell.
* Fixed bug: if usage of multiple languages was not chosen, all the dictionaries for it were still loading at plugin start.

v1.2.3
* Fix bug with Multiple Languages settings not being saved when chosen from "Change Current Language" menu.
* Fix bug with incorrect delimiters while checking ANSI files if system default encoding is Japanese.
* Add link to short online manual to plugin menu.

v1.2.2
* Rename Hunspell_Path setting in DSpellCheck.ini to User_Hunspell_Path to avoid some troubles.
* Make AutoCheck turned on by default.

v1.2.1
* Fix 1.2.0 specific bug that non-default encoding were causing some troubles (missing recheck) sometimes
* Make that if scrollbar was moved because of user input, immediate recheck isn't being done.
* Add support for using two dictionary Hunspell directories (by default one for all users, one for current only) and thus ability to remove and install dictionaries to/from these directories.
* Optimize plugin performance on scrolling a little.

v1.2.0
* Add possibility to download Hunspell dictionaries from common OpenOffice mirrors using plugin interface.
* Add possibility to remove Hunspell dictionaries using plugin interface.
* Add possibility to quickly change current language through Plugins->DSpellCheck->Change_Current_Language or corresponding hot key (initially Alt + D), also such items as "Download More Dictionaries", "Customize Multiple Dictionaries" and "Remove Unneeded Dictionaries" are also presented there.
* Add resolving common Hunspell dictionaries names to more user friendly language names.
* Now by Default Hunspell user dictionaries are stored separately, this could be turned to the way it was.
* No more checking of words contained in hotlinks
* Some minor bugs were fixed.

v1.1.7
* Fix plugin's crash in some system default encodings.

v1.1.6
* Fix working of "Add to dictionary" and "Ignore" for Aspell which were accidentally broken.

v1.1.5
* Fix segfault when Aspell is presented on system but have no dictionaries.
* Make Hunspell used by default even if Aspell library is installed.

v1.1.4
* Fix some encoding conversion problems, which were leading to very dramatic results on some systems.

v1.1.3
* Fix autospellcheck option not being saved correctly.
* Fix some minor bugs.

v1.1.2
* Fix crash on exit when aspell isn't presented.
* Fix some encoding problems.
* Fix words added to Hunspell to be being able to be suggested.

v1.1.1
* Fix adding words to user dictionary using Hunspell library.

v1.1
* Add Hunspell support, which is statically linked into Plug-in so you don't need to download any custom dlls to use it, dictionaries though (.aff and .dic files) should be placed by default into "***Notepad++_Dir***\plugins\Config\Hunspell\" (Location could be changed from settings).
* Add option to use native Notepad++ context menu for choosing suggestions and adding word to dictionary/ignoring word (Default option is the old way though).
