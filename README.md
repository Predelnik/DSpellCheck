DSpellCheck
===========

Yet Another Notepad++ Spell-checking Plug-In

Well, the story is that I wasn't very happy with current Notepad++ Spell-Checking Plug-ins, so I've decided to write another one. Not very original, but at least I've done some features I desired for a long time in other Spell-Checkers (such as Multi-Language Spell-Checking for example) and also of course did a little bit of a coding training and expanding of my current knowledge base, so it's not totally useless try, I guess.

Well it still uses old Aspell as a Spell-Checking library. Though I guess I'll add Hunspell support in the near future, the one thing I'm not so sure about Hunspell is because it's never used in shared version so there is no common place for Hunspell dictionaries for example and each program uses it's own and so own. Also if it's static linked it updates only when wrapping program updates.

About choosing suggestions menu for misspellings: The way I made it, may look peculiar at the first glance, but honestly I just reproduced the style which I saw in Microsoft Visual Studio Spell-Checking extension. It used some kind of thing called smart-tag, which are only available in some Microsoft programs, but I just kinda emulated this kind of behaviour. Doing it in normal context-menu way would require a little bit of hard-stuff on my side, because Plug-ins in N++ for now couldn't do it any normal way, but still I guess I'll add an option to see suggestions also in context-menu way, with current way still available as another option.

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
