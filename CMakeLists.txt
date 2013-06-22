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
  ${SRC_DIR}/LangList.cpp
  ${SRC_DIR}/LangList.h
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
  ${SRC_DIR}/LanguageName.h
  ${SRC_DIR}/RemoveDics.cpp
  ${SRC_DIR}/RemoveDics.h
  ${SRC_DIR}/SelectProxy.cpp
  ${SRC_DIR}/SelectProxy.h
  ${SRC_DIR}/DSpellCheck.rc
  ${SRC_DIR}/AbstractSpellerInterface.cpp
  ${SRC_DIR}/AbstractSpellerInterface.h
  ${SRC_DIR}/AspellInterface.cpp
  ${SRC_DIR}/AspellInterface.h
  ${SRC_DIR}/HunspellInterface.cpp
  ${SRC_DIR}/HunspellInterface.h
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
)

SET(LINK_LIBRARY shlwapi comctl32 version wininet UxTheme Ws2_32)
add_library (DSpellCheck SHARED ${list_of_files_DSpellCheck})

#if (MSVC80 OR MSVC71 OR MSVC70 OR MSVC60)
# SET_TARGET_PROPERTIES(DSpellCheck PROPERTIES LINK_FLAGS "/FORCE:MULTIPLE")
#endif (MSVC80 OR MSVC71 OR MSVC70 OR MSVC60)

if(MSVC)
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
endif()

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
