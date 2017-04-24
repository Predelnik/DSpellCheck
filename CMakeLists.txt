cmake_minimum_required (VERSION 2.6)
project (DSpellCheck)
set (DSpellCheck_VERSION_MAJOR 1)
set (DSpellCheck_VERSION_MINOR 0)
set (SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set (BIN_DIR ${PROJECT_SOURCE_DIR})
configure_file ("${SRC_DIR}/config.h.in" "${SRC_DIR}/config.h")
include_directories (${SRC_DIR}/Controls)
include_directories (${SRC_DIR})
include_directories (Hunspell/src)
include_directories (minizip/)
include_directories (FtpClient/)
include_directories (libiconv/include)

add_subdirectory ("Hunspell") #Thanks https://github.com/zdenop/hunspell-mingw
add_subdirectory ("libiconv")
add_subdirectory ("minizip")
add_subdirectory ("FtpClient")

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})
set (DEBUG_OUTPUT_DIR, ${BIN_DIR}/debug)
set (RELEASE_OUTPUT_DIR, ${BIN_DIR}/release)
add_definitions ("-D_CRT_NONSTDC_NO_DEPRECATE -D_UNICODE -DUNICODE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_WIN32_WINNT=0x501 -DWINVER=0x501")

set(DEBUG_OUTPUT_PATH "" CACHE STRING "Output for debug library (Possibly notepad++ plugin folder)")
set(VLD_BUILD_ON "OFF" CACHE BOOL "Use Visual Leak Detector")

if ( VLD_BUILD_ON )
  add_definitions ("-DVLD_BUILD")
endif ( VLD_BUILD_ON )

if ( MSVC )
    #set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
    if (NOT DEFINED DEBUG_OUTPUT_PATH)
    	set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_DIR} )
   	else (NOT DEFINED DEBUG_OUTPUT_PATH)
   		set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_PATH} )
   	endif (NOT DEFINED DEBUG_OUTPUT_PATH)
    # set ( CMAKE_PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${RELEASE_OUTPUT_DIR} )
    set (add_definitions, UNICODE)
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG} )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG} )
else ( MSVC )
# For possible MinGW compilation
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
endif ( MSVC )

set (list_of_files_DSpellCheck
  ${SRC_DIR}/Plugin.cpp
  ${SRC_DIR}/Plugin.h
  ${SRC_DIR}/SpellChecker.cpp
  ${SRC_DIR}/SpellChecker.h
  ${SRC_DIR}/DllMain.cpp
  ${SRC_DIR}/menuCmdID.h
  ${SRC_DIR}/scintilla.h
  ${SRC_DIR}/SciLexer.h
  ${SRC_DIR}/PluginInterface.h
  ${SRC_DIR}/Notepad_plus_msgs.h
  ${SRC_DIR}/LangListDialog.cpp
  ${SRC_DIR}/LangListDialog.h
  ${SRC_DIR}/aspell.h
  ${SRC_DIR}/aspell.cpp
  ${SRC_DIR}/MainDef.h
  ${SRC_DIR}/Progress.cpp
  ${SRC_DIR}/Progress.h
  ${SRC_DIR}/SettingsDlg.cpp
  ${SRC_DIR}/SettingsDlg.h
  ${SRC_DIR}/CommonFunctions.cpp
  ${SRC_DIR}/CommonFunctions.h
  ${SRC_DIR}/Suggestions.cpp
  ${SRC_DIR}/Suggestions.h
  ${SRC_DIR}/PrecompiledHeader.cpp
  ${SRC_DIR}/PrecompiledHeader.h
  ${SRC_DIR}/AboutDlg.cpp
  ${SRC_DIR}/AboutDlg.h
  ${SRC_DIR}/DownloadDicsDlg.cpp
  ${SRC_DIR}/DownloadDicsDlg.h
  ${SRC_DIR}/LanguageName.cpp
  ${SRC_DIR}/LanguageName.h
  ${SRC_DIR}/RemoveDicsDialog.cpp
  ${SRC_DIR}/RemoveDicsDialog.h
  ${SRC_DIR}/SelectProxy.cpp
  ${SRC_DIR}/SelectProxy.h
  ${SRC_DIR}/DSpellCheck.rc
  ${SRC_DIR}/SpellerController.cpp
  ${SRC_DIR}/SpellerController.h
  ${SRC_DIR}/AspellController.cpp
  ${SRC_DIR}/AspellController.h
  ${SRC_DIR}/HunspellController.cpp
  ${SRC_DIR}/HunspellController.h
  ${SRC_DIR}/Settings.h
  ${SRC_DIR}/Settings.cpp
  ${SRC_DIR}/CustomMessage.h
  ${SRC_DIR}/utils/enum_vector.h
  ${SRC_DIR}/utils/enum_range.h
  ${SRC_DIR}/utils/Uncopiable.h
  ${SRC_DIR}/Controls/CheckedList/CheckedList.h
  ${SRC_DIR}/Controls/CheckedList/CheckedList.cpp
  ${SRC_DIR}/Controls/StaticDialog/StaticDialog.cpp
  ${SRC_DIR}/Controls/StaticDialog/StaticDialog.h
  ${SRC_DIR}/Controls/Window.h
  ${SRC_DIR}/Controls/StaticDialog/StaticDialog.h
  ${SRC_DIR}/Controls/TabBar/ControlsTab.cpp
  ${SRC_DIR}/Controls/TabBar/ControlsTab.h
  ${SRC_DIR}/Controls/TabBar/TabBar.cpp
  ${SRC_DIR}/Controls/TabBar/TabBar.h
  ${SRC_DIR}/StackWalker/StackWalker.h
  ${SRC_DIR}/StackWalker/StackWalker.cpp
)

SET(LINK_LIBRARY shlwapi comctl32 version wininet Ws2_32)
add_library (DSpellCheck SHARED ${list_of_files_DSpellCheck})

#if (MSVC80 OR MSVC71 OR MSVC70 OR MSVC60)
# SET_TARGET_PROPERTIES(DSpellCheck PROPERTIES LINK_FLAGS "/FORCE:MULTIPLE")
#endif (MSVC80 OR MSVC71 OR MSVC70 OR MSVC60)

SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,5.01")
SET (CMAKE_CXX_FLAGS "/WX /EHsc /MP /W4 /wd4127")
foreach(flag_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
endforeach(flag_var)
SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "MSVC C Debug MT flags " FORCE)
SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "MSVC CXX Debug MT flags " FORCE)
SET (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "MSVC C Release MT flags " FORCE)
SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "MSVC CXX Release MT flags " FORCE)
SET (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}" CACHE STRING "MSVC C Debug MT flags " FORCE)
SET (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}" CACHE STRING "MSVC C Release MT flags " FORCE)
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "MSVC CXX Debug MT flags " FORCE)
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "MSVC CXX Release MT flags " FORCE)

target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/Hunspell/Debug/libhunspell.lib)
target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/libiconv/DebugStatic_Win32/libiconv.lib)
target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/minizip/Debug/minizip.lib)
target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/ftpclient/Debug/ftpclient.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/Hunspell/Release/libhunspell.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/libiconv/ReleaseStatic_Win32/libiconv.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/minizip/Release/minizip.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/ftpclient/Release/ftpclient.lib)


include (PrecompiledHeader.cmake)
add_precompiled_header(DSpellCheck PrecompiledHeader.h FORCEINCLUDE)

target_link_libraries(DSpellCheck ${LINK_LIBRARY})
