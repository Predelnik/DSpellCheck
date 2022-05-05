cmake_minimum_required (VERSION 3.7)
project (DSpellCheck)
include (CMake/ucm.cmake)
include (CMake/PVS-Studio.cmake)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:threadSafeInit-")
endif()

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
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE  "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG:INCREMENTAL")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO  "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /LTCG:INCREMENTAL")
    ucm_set_runtime(STATIC)
endif()

add_definitions ("-D_CRT_NONSTDC_NO_DEPRECATE -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS")
add_subdirectory (deps/Hunspell) #Thanks https://github.com/zdenop/hunspell-mingw
# options for win-iconv, which sadly doesn't have prefixes
add_subdirectory (deps/win-iconv)
add_subdirectory (deps/minizip)
add_subdirectory (deps/FtpClient)
add_subdirectory (deps/Controls)
add_subdirectory (deps/npp_extra)
add_subdirectory (deps/lsignal)
add_subdirectory (deps/aspell)
add_subdirectory (deps/catch)
add_subdirectory (deps/json)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /w44062 /utf-8")
    set(DSpellCheck_USE_VLD "OFF" CACHE BOOL "Use Visual Leak Detector")
endif()

if ( DSpellCheck_USE_VLD )
    add_definitions ("-DVLD_BUILD")
endif ()

file (GLOB_RECURSE source_files src/**/*.cpp src/**/*.h)
list(REMOVE_ITEM source_files src/plugin/DllMain.cpp)

add_library (DSpellCheckStatic STATIC ${source_files})
target_include_directories (DSpellCheckStatic PUBLIC src deps/win-iconv)

target_link_libraries(DSpellCheckStatic hunspell minizip ftpclient iconv-static Controls npp_extra lsignal aspell json)
target_link_libraries(DSpellCheckStatic shlwapi comctl32 version wininet Ws2_32 dbghelp)
target_include_directories (DSpellCheckStatic PRIVATE src/Controls)

if ( NOT ${DSpellCheck_SANITIZE} STREQUAL none)
  if ( ${DSpellCheck_SANITIZE} STREQUAL address)
    target_link_libraries (DSpellCheckStatic
      clang_rt.asan_dynamic-x86_64
      clang_rt.asan_dynamic_runtime_thunk-x86_64)
  endif ()
endif ()

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options (DSpellCheckStatic PUBLIC /permissive- /Zc:twoPhase-)
endif ()
target_compile_options (DSpellCheckStatic PUBLIC /std:c++latest /EHsc)

add_library (DSpellCheck SHARED src/plugin/DllMain.cpp src/plugin/DSpellCheck.rc)
target_link_libraries (DSpellCheck DSpellCheckStatic)

set(DSpellCheck_DLL_OUTPUT_PATH "" CACHE PATH "Output for debug library (Possibly notepad++ plugin folder)")
if (NOT ${DSpellCheck_DLL_OUTPUT_PATH} STREQUAL "")
    set_target_properties (DSpellCheck PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DSpellCheck_DLL_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DSpellCheck_DLL_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DSpellCheck_DLL_OUTPUT_PATH})
endif ()

file (GLOB_RECURSE test_source_files test/*.cpp test/*.h)
list (APPEND test_source_files src/common/PrecompiledHeader.cpp)
add_executable (DSpellCheckTest ${test_source_files})
target_link_libraries (DSpellCheckTest DSpellCheckStatic catch)
set(DSpellCheck_PVS_CHECK OFF CACHE BOOL "Enable checking by PVS studio")
if (DSpellCheck_PVS_CHECK)
pvs_studio_add_target(TARGET DSpellCheck.analyze ALL
                      OUTPUT FORMAT errorfile
                      ANALYZE DSpellCheck
                      LOG DSpellCheck.psv-err)
endif ()

target_precompile_headers (DSpellCheckStatic PRIVATE "src/common/PrecompiledHeader.h")
target_precompile_headers (DSpellCheckTest REUSE_FROM DSpellCheckStatic)
