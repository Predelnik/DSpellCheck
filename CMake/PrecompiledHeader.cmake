# Macro for setting up precompiled headers. Usage:
#
#   add_precompiled_header(target header.h [FORCEINCLUDE] [header_on_which_pch_depends header_on_which_pch_depends ...] )
#
# MSVC: A source file with the same name as the header must exist and
# be included in the target (E.g. header.cpp).
#
# MSVC: Add FORCEINCLUDE to automatically include the precompiled
# header file from every source file.
#
# GCC: The precompiled header is always automatically included from
# every header file.
#
# Notes:
#   ##   All dependencies of target must be defined at the point where
#        PCH is being added - this is required to correctly collect compile
#        options exported from dependencies.
#   ##   Unwrapping of generator expression is supported as long as there
#        are no multiple compile flags in single generator expression, i.e.
#        $<$<COFIG:DEBUG>:-flag> is supported, but $<$<COFIG:DEBUG>:-flag1 -flag2>
#        is not.

#
# Copyright (C) 2009-2013 Lars Christensen <larsch@belunktum.dk>
# Copyright (C) 2013-2016 Nenad Mikša <nenad.miksa@microblink.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the ‘Software’), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED ‘AS IS’, WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

macro(get_dependencies_compile_options target compileOptions compileDefinitions includeDirectories systemIncludeDirectories visited)
    get_target_property( dependencies ${target} INTERFACE_LINK_LIBRARIES )
    get_target_property( target_type ${target} TYPE )
    if( NOT ${target_type} STREQUAL INTERFACE_LIBRARY )
        get_target_property( private_dependencies ${target} LINK_LIBRARIES )
        list( APPEND dependencies ${private_dependencies} )
    endif()
    foreach(dep ${dependencies})
        if(TARGET ${dep} )
            # check if we already processed this dependency
            list(FIND ${visited} ${dep} pos)

            if(${pos} EQUAL -1)
                get_target_property(flags ${dep} INTERFACE_COMPILE_OPTIONS)
                if(flags)
                    list(APPEND ${compileOptions} ${flags})
                endif()

                get_target_property(def ${dep} INTERFACE_COMPILE_DEFINITIONS)
                if(def)
                    list( APPEND ${compileDefinitions} ${def} )
                endif()

                get_target_property(inc ${dep} INTERFACE_INCLUDE_DIRECTORIES)
                if(inc)
                    list( APPEND ${includeDirectories} ${inc} )
                endif()

                get_target_property(incSys ${dep} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
                if(incSys)
                    list( APPEND ${systemIncludeDirectories} )
                endif()

                list(APPEND ${visited} ${dep})

                get_dependencies_compile_options(${dep} ${compileOptions} ${compileDefinitions} ${includeDirectories} ${systemIncludeDirectories} ${visited})
            endif()
        endif()
    endforeach()
endmacro()

function( filter_item filtered_item item )
    set(append_item true)
    if( ${item} MATCHES ".*COMPILE_LANGUAGE.*" )
        if( ${item} MATCHES ".*\\\$<\\\$<COMPILE_LANGUAGE:CXX>:(.*)>$")
            set( item ${CMAKE_MATCH_1} )
        else()
            set(append_item false)
        endif()
    endif()
    if( ${item} MATCHES ".*(INSTALL_INTERFACE|BUILD_INTERFACE).*" )
        set(append_item false)
    endif()
    # Adding CONFIG:DEBUG flags in Release and vice versa produces empty quotes to compiler command line
    # which confuses GCC.
    string( TOUPPER ${item} upper_item )
    if(
        ( ( ${upper_item} MATCHES ".*CONFIG:RELEASE.*" OR ${upper_item} MATCHES ".*NOT:\\\$<CONFIG:DEBUG.*" ) AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug" ) OR
        ( ( ${upper_item} MATCHES ".*CONFIG:DEBUG.*" OR ${upper_item} MATCHES ".*NOT:\\\$<CONFIG:RELEASE.*" ) AND ${CMAKE_BUILD_TYPE} STREQUAL "Release" )
    )
        set(append_item false)
    endif()
    if( ${append_item} )
        set( ${filtered_item} ${item} PARENT_SCOPE )
#        message( STATUS "    -> filtered to ${item}" )
    else()
        unset( ${filtered_item} PARENT_SCOPE )
#        message( STATUS "    -> filtered out" )
    endif()
endfunction()

MACRO(ADD_PRECOMPILED_HEADER _targetName _input)
  GET_FILENAME_COMPONENT(_inputWe ${_input} NAME_WE)
  SET(pch_source ${_inputWe}.cpp)

  set( PCH_LOCAL_DEPENDS "")
  SET(FORCEINCLUDE OFF)

  FOREACH(arg ${ARGN})
    IF(arg MATCHES FORCEINCLUDE)
      SET(FORCEINCLUDE ON)
    ELSE()
      LIST(APPEND PCH_LOCAL_DEPENDS ${arg})
    ENDIF()
  ENDFOREACH(arg)

  IF(MSVC)
    GET_TARGET_PROPERTY(sources ${_targetName} SOURCES)
	GET_FILENAME_COMPONENT(PCH_NAME ${_input} NAME)
    SET(_sourceFound FALSE)
    FOREACH(_source ${sources})
      SET(PCH_COMPILE_FLAGS "")
      IF(_source MATCHES \\.\(cc|cxx|cpp\)$)
	GET_FILENAME_COMPONENT(_sourceWe ${_source} NAME_WE)
	IF(_sourceWe STREQUAL ${_inputWe})
	  SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yc${PCH_NAME} /FI${PCH_NAME}")
	  SET(_sourceFound TRUE)
	ELSE(_sourceWe STREQUAL ${_inputWe})
	  SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yu${PCH_NAME}")
	  IF(FORCEINCLUDE)
	    SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /FI${PCH_NAME}")
	  ENDIF(FORCEINCLUDE)
	ENDIF(_sourceWe STREQUAL ${_inputWe})
	SET_SOURCE_FILES_PROPERTIES(${_source} PROPERTIES COMPILE_FLAGS "${PCH_COMPILE_FLAGS}")
      ENDIF(_source MATCHES \\.\(cc|cxx|cpp\)$)
    ENDFOREACH()
    IF(NOT _sourceFound)
      MESSAGE(FATAL_ERROR "A source file for ${_input} was not found. Required for MSVC builds.")
    ENDIF(NOT _sourceFound)
  ENDIF(MSVC)

  IF (NOT MSVC)
      IF(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        if(${CMAKE_GENERATOR} STREQUAL "Xcode")
            set_target_properties(${_targetName} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${_input} XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "Yes")
        else()
            GET_FILENAME_COMPONENT(_name ${_input} NAME)
            SET(GCH_PATH "${CMAKE_CURRENT_BINARY_DIR}/${_targetName}-pch" )
            SET(_output "${GCH_PATH}/${_name}.gch")
        #    SET(_output "${CMAKE_BINARY_DIR}/${_name}.gch")

            string(REGEX REPLACE "[^a-zA-Z0-9]" "" OutputName ${_output})

            if(NOT DEFINED ${OutputName}_CREATED)
                # obtain optimization flags
                STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
                SET(_compiler_FLAGS ${${_flags_var_name}})
                # obtain all other flags
                list(APPEND _compiler_FLAGS ${CMAKE_CXX_FLAGS})

                # Obtain target dependencies flags (recursively)
                set( visited "" )
                set( depsCompileOptions "" )
                set( depsCompileDefinitions "" )
                set( depsIncludeDirectories "" )
                set( depsSystemIncludeDirectories "" )
                get_dependencies_compile_options( ${_targetName} depsCompileOptions depsCompileDefinitions depsIncludeDirectories depsSystemIncludeDirectories visited )
    #            message("Additional compile flags for ${_targetName}: ${targetCompileFlags}")

                get_directory_property( _directory_flags INCLUDE_DIRECTORIES )
                get_target_property( _target_include_dirs ${_targetName} INCLUDE_DIRECTORIES )
                get_target_property( _target_interface_include_dirs ${_targetName} INTERFACE_INCLUDE_DIRECTORIES )
                foreach( item ${_directory_flags} ${_target_include_dirs} ${_target_interface_include_dirs} ${depsIncludeDirectories} )
                    filter_item( filtered_item ${item} )
                    if( filtered_item )
                        list( APPEND _compiler_FLAGS "-I${filtered_item}" )
                    endif()
                endforeach()

                get_target_property( _target_system_include_dirs ${_targetName} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES )
                foreach( item ${_target_system_include_dirs} ${depsSystemIncludeDirectories} )
                    filter_item( filtered_item ${item} )
                    if( filtered_item )
                        list( APPEND _compiler_FLAGS "-isystem ${filtered_item}" )
                    endif()
                endforeach()

                get_directory_property( _directory_flags COMPILE_DEFINITIONS )
                get_target_property( _target_compile_definitions ${_targetName} COMPILE_DEFINITIONS )
                foreach( def ${_directory_flags} ${_target_compile_definitions} ${depsCompileDefinitions} )
                    filter_item( filtered_item ${def} )
                    if( filtered_item )
                        list( APPEND _compiler_FLAGS "-D${filtered_item}" )
                    endif()
                endforeach()

                get_directory_property( _compile_options COMPILE_OPTIONS )
                get_target_property( _target_compile_options ${_targetName} COMPILE_OPTIONS )
                foreach( option ${_compile_options} ${_target_compile_options} ${depsCompileOptions} )
                    filter_item( filtered_item ${option} )
                    if( filtered_item )
                        list(APPEND _compiler_FLAGS ${filtered_item})
                    endif()
                endforeach()

                if( ANDROID )
                    list( APPEND _compiler_FLAGS "--sysroot=${CMAKE_SYSROOT}" )
                    foreach( sid ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES} )
                        list( APPEND _compiler_FLAGS "-isystem ${sid}" )
                    endforeach()
                    list( APPEND _compiler_FLAGS "-target ${CMAKE_CXX_COMPILER_TARGET} " )
                endif()

                if( APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET )
                    list(APPEND _compiler_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
                endif()

                set(COMPILER ${CMAKE_CXX_COMPILER})
                if(${COMPILER} MATCHES "ccache")
        #            message("CCache will not give any boost when precompiled headers are used")
                    string(STRIP ${CMAKE_CXX_COMPILER_ARG1} CARG1)
                    set( COMPILER ${CMAKE_CXX_COMPILER} ${CARG1} )
                endif()
                if( CMAKE_CXX_COMPILER_LAUNCHER )
                    set( COMPILER ${CMAKE_CXX_COMPILER_LAUNCHER} ${COMPILER} )
                endif()

                list( REMOVE_DUPLICATES _compiler_FLAGS )

                SEPARATE_ARGUMENTS(_compiler_FLAGS)

                ADD_CUSTOM_COMMAND(
                  OUTPUT ${_output}
                  COMMAND ${CMAKE_COMMAND} -E remove ${_output}
                  COMMAND ${COMPILER} -x c++-header ${_compiler_FLAGS} -o ${_output} ${_input}
                  DEPENDS ${_input} ${PCH_LOCAL_DEPENDS}
                  COMMENT "Building precompiled header for ${_targetName}"
                  VERBATIM)
                set(${OutputName}_CREATED true)
                # set(${OutputName}_CREATED true PARENT_SCOPE)
            endif()
            add_custom_target(${_targetName}_PCH DEPENDS ${_output} SOURCES ${_input})

            ## Implementation note:
            ## We always build single PCH for target. However, some source files in target can have different compile flags.
            ## A right thing to do (TM) would be to generate PCH for each group of files with same flags. However, we do not
            ## do that. So, instead of including PCH globally on whole target, we will include PCH only on those sources that
            ## do not already have specific flags.
            ## Also, specific for C source files, we filter-out C sources with regex that matches only C++ files since
            ## generator expression $<COMPILE_LANGUAGE:CXX> works only for target_compile_options function.
            ##
            ##                                                                         Nenad Miksa (28.02.2017.)

            get_target_property( target_sources ${_targetName} SOURCES )
            foreach( source ${target_sources} )
                get_source_file_property( old_flags ${source} COMPILE_FLAGS )
                if( NOT old_flags AND ${source} MATCHES ".*\\.c.+" )
                    set_source_files_properties( "${source}" PROPERTIES COMPILE_FLAGS "-include ${GCH_PATH}/${PCH_NAME}.h -Winvalid-pch" )
                endif()
            endforeach()
    #        target_compile_options( ${_targetName} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-include ${GCH_PATH}/${PCH_NAME}.h -Winvalid-pch> )
            ADD_DEPENDENCIES(${_targetName} ${_targetName}_PCH)
        endif()
      ENDIF()
   ENDIF()
ENDMACRO()