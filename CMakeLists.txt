cmake_minimum_required (VERSION 2.6) 
project (DSpellCheck)
set (DSpellCheck_VERSION_MAJOR 1)
set (DSpellCheck_VERSION_MINOR 0)
set (SRC_DIR src)
set (BIN_DIR bin)
configure_file ("${SRC_DIR}/config.h.in" "${SRC_DIR}/config.h")
include_directories (${SRC_DIR}/Controls) 
include_directories (${SRC_DIR})

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})
set (DEBUG_OUTPUT_DIR, ${BIN_DIR}/debug)
set (RELEASE_OUTPUT_DIR, ${BIN_DIR}/release)
add_definitions ("-D _CRT_NONSTDC_NO_DEPRECATE;_UNICODE;UNICODE;_CRT_SECURE_NO_WARNINGS;_CRT_NON_CONFORMING_SWPRINTFS")

set(DEBUG_OUTPUT_PATH "" CACHE STRING "Output for debug library (Possibly notepad++ plugin folder)")

if ( MSVC )
    #set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GENERIC_OUTPUT_DIR} )
    if (${DEBUG_OUTPUT_PATH} STREQUAL "")
    	set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_DIR} )
   	else (${DEBUG_OUTPUT_PATH} STREQUAL "")
   		set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_PATH} )
   	endif (${DEBUG_OUTPUT_PATH} STREQUAL "")
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
	${SRC_DIR}/PluginDefinition.cpp 
	${SRC_DIR}/PluginDefinition.h 
	${SRC_DIR}/DSpellCheck.cpp 
	${SRC_DIR}/menuCmdID.h
	${SRC_DIR}/scintilla.h 
	${SRC_DIR}/PluginInterface.h
	${SRC_DIR}/Notepad_plus_msgs.h
	${SRC_DIR}/aspell.h
	${SRC_DIR}/aspell.cpp
	${SRC_DIR}/MainDef.h
	${SRC_DIR}/SettingsDlg.cpp
	${SRC_DIR}/SettingsDlg.h
	${SRC_DIR}/CommonFunctions.cpp
	${SRC_DIR}/CommonFunctions.h
	${SRC_DIR}/DSpellCheck.rc
	${SRC_DIR}/Controls/StaticDialog/StaticDialog.cpp
	${SRC_DIR}/Controls/StaticDialog/StaticDialog.h
	${SRC_DIR}/Controls/Window.h
	${SRC_DIR}/Controls/StaticDialog/StaticDialog.h
	${SRC_DIR}/Controls/TabBar/ControlsTab.cpp
	${SRC_DIR}/Controls/TabBar/ControlsTab.h
	${SRC_DIR}/Controls/TabBar/TabBar.cpp
	${SRC_DIR}/Controls/TabBar/TabBar.h	
	)



SET(LINK_LIBRARY shlwapi comctl32)
add_library (DSpellCheck SHARED ${list_of_files_DSpellCheck})

target_link_libraries(DSpellCheck ${LINK_LIBRARY})
