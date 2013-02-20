cmake_minimum_required (VERSION 2.6) 
project (DSpellCheck)
set (DSpellCheck_VERSION_MAJOR 1)
set (DSpellCheck_VERSION_MINOR 0)
set (SRC_DIR src)
set (BIN_DIR bin)
configure_file ("${SRC_DIR}/config.h.in" "${SRC_DIR}/config.h")
include_directories (${SRC_DIR}/aspell) 

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})
set (DEBUG_OUTPUT_DIR, ${BIN_DIR}/debug)
set (RELEASE_OUTPUT_DIR, ${BIN_DIR}/release)

if ( MSVC )
    set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
    set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_DIR} )
    set ( CMAKE_PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${RELEASE_OUTPUT_DIR} )
    set (add_definitions, UNICODE)
else ( MSVC )
# For possible MinGW compilation
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} ) 
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
endif ( MSVC )

set (list_of_files_DSpellCheck 
	${SRC_DIR}/Plugin.cpp 
	${SRC_DIR}/Plugin.h 
	${SRC_DIR}/PluginDefintion.cpp 
	${SRC_DIR}/PluginDefintion.h 
	${SRC_DIR}/DSpellCheck.cpp 
	${SRC_DIR}/menuCmdID.h
	${SRC_DIR}/scintilla.h 
	${SRC_DIR}/PluginInterface.cpp
	${SRC_DIR}/Notepad_plus_msgs.h
	${SRC_DIR}/aspell.h
	${SRC_DIR}/aspell.cpp)

add_library (DSpellCheck.dll SHARED ${list_of_files_DSpellCheck})