# build:
# mkdir build
# cd build
# cmake .. -G "MinGW Makefiles"

cmake_minimum_required(VERSION 2.8)
project ( hunspell )
SET(HUNSPELL_VERSION 1.3.2)

if(WIN32)
    add_definitions( -DWIN32)
endif(WIN32)

INCLUDE_DIRECTORIES (./ src/hunspell src/parsers src/tools src/win_api)

# LIBS
SET(SRCS
    src/hunspell/affentry.cxx
    src/hunspell/affixmgr.cxx
    src/hunspell/csutil.cxx
    src/hunspell/dictmgr.cxx
    src/hunspell/hashmgr.cxx
    src/hunspell/suggestmgr.cxx
    src/hunspell/license.myspell
    src/hunspell/license.hunspell
    src/hunspell/phonet.cxx
    src/hunspell/filemgr.cxx
    src/hunspell/hunzip.cxx
    src/hunspell/hunspell.cxx
    src/hunspell/replist.cxx)

SET(HEADERS
    src/hunspell/affentry.hxx
    src/hunspell/htypes.hxx
    src/hunspell/affixmgr.hxx
    src/hunspell/csutil.hxx
    src/hunspell/hunspell.hxx
    src/hunspell/atypes.hxx
    src/hunspell/dictmgr.hxx
    src/hunspell/hunspell.h
    src/hunspell/suggestmgr.hxx
    src/hunspell/baseaffix.hxx
    src/hunspell/hashmgr.hxx
    src/hunspell/langnum.hxx
    src/hunspell/phonet.hxx
    src/hunspell/filemgr.hxx
    src/hunspell/hunzip.hxx
    src/hunspell/w_char.hxx
    src/hunspell/replist.hxx
    src/hunspell/hunvisapi.h)

ADD_LIBRARY( hunspell-static STATIC ${HEADERS} ${SRCS})
TARGET_LINK_LIBRARIES( hunspell-static  )
ADD_DEPENDENCIES( hunspell-static  ${HEADERS})

SET_TARGET_PROPERTIES(
    hunspell-static
  PROPERTIES
    VERSION ${HUNSPELL_VERSION}
    SOVERSION ${HUNSPELL_VERSION}
    PREFIX "lib"
    OUTPUT_NAME "hunspell")
	
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