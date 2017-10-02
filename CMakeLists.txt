cmake_minimum_required (VERSION 3.7)
project (DSpellCheck)
include (CMake/ucm.cmake)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
    set(DSpellCheck_SANITIZE "none" CACHE STRING "Enable one of llvm sanitizers")
    set_property(CACHE DSpellCheck_SANITIZE PROPERTY STRINGS none address memory thread undefined)
    if ( NOT ${DSpellCheck_SANITIZE} STREQUAL none)
        set(DSpellCheck_CLANG_WINDOWS_LIB_PATH "c:/Program Files/LLVM/lib/clang/5.0.0/lib/windows" CACHE PATH "Path to sanitizer libraries on windows")
        link_directories (${DSpellCheck_CLANG_WINDOWS_LIB_PATH})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${DSpellCheck_SANITIZE}")
        if ( ${DSpellCheck_SANITIZE} STREQUAL address)
            ucm_gather_flags(0 flags_configs)
            foreach(flags ${flags_configs})
              string(REGEX REPLACE "/MDd" "/MD" ${flags} "${${flags}}")
            endforeach()
        endif ()
    endif ()
endif()

if(MSVC)
     ucm_set_runtime(STATIC)
endif()

add_definitions ("-D_CRT_NONSTDC_NO_DEPRECATE -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS")
add_subdirectory (Hunspell) #Thanks https://github.com/zdenop/hunspell-mingw
add_subdirectory (libiconv)
add_subdirectory (minizip)
add_subdirectory (FtpClient)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(DSpellCheck_USE_VLD "OFF" CACHE BOOL "Use Visual Leak Detector")
endif()

if ( DSpellCheck_USE_VLD )
    add_definitions ("-DVLD_BUILD")
endif ()
file (GLOB_RECURSE source_files src/*.cpp src/*.h src/*.rc)

include (CMake/PrecompiledHeader.cmake)
add_library (DSpellCheck SHARED ${source_files})
add_precompiled_header(DSpellCheck src/PrecompiledHeader.h FORCEINCLUDE)

set(DSpellCheck_DLL_OUTPUT_PATH "" CACHE STRING "Output for debug library (Possibly notepad++ plugin folder)")
if (NOT ${DSpellCheck_DLL_OUTPUT_PATH} STREQUAL "")
    set_target_properties (DSpellCheck PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DSpellCheck_DLL_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DSpellCheck_DLL_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DSpellCheck_DLL_OUTPUT_PATH})
endif ()

if ( NOT ${DSpellCheck_SANITIZE} STREQUAL none)
  if ( ${DSpellCheck_SANITIZE} STREQUAL address)
    target_link_libraries (DSpellCheck
      clang_rt.asan_dynamic-x86_64
      clang_rt.asan_dynamic_runtime_thunk-x86_64)
  endif ()
endif ()

target_link_libraries(DSpellCheck hunspell)
target_link_libraries(DSpellCheck minizip)
target_link_libraries(DSpellCheck ftpclient)
target_link_libraries(DSpellCheck libiconv)
target_link_libraries(DSpellCheck shlwapi comctl32 version wininet Ws2_32)
target_include_directories (DSpellCheck PRIVATE src/Controls)
target_include_directories (DSpellCheck PRIVATE src)
target_compile_options (DSpellCheck PRIVATE /std:c++latest)
