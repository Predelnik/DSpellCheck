cmake_minimum_required (VERSION 3.15)
project (DSpellCheck)
include (CMake/PVS-Studio.cmake)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
    set(DSpellCheck_SANITIZE "none" CACHE STRING "Enable one of llvm sanitizers")
    set_property(CACHE DSpellCheck_SANITIZE PROPERTY STRINGS none address memory thread undefined)
    if ( NOT ${DSpellCheck_SANITIZE} STREQUAL none)
        set(DSpellCheck_CLANG_WINDOWS_LIB_PATH "c:/Program Files/LLVM/lib/clang/5.0.0/lib/windows" CACHE PATH "Path to sanitizer libraries on windows")
        link_directories (${DSpellCheck_CLANG_WINDOWS_LIB_PATH})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${DSpellCheck_SANITIZE}")
    endif ()
endif()

add_subdirectory (deps/Hunspell) #Thanks https://github.com/zdenop/hunspell-mingw
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
    set(DSpellCheck_USE_VLD "OFF" CACHE BOOL "Use Visual Leak Detector")
endif()

file (GLOB_RECURSE source_files src/**/*.cpp src/**/*.h)
list(REMOVE_ITEM source_files src/plugin/DllMain.cpp)

add_library (DSpellCheckStatic STATIC ${source_files})

target_compile_definitions (DSpellCheckStatic PUBLIC _CRT_NONSTDC_NO_DEPRECATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NON_CONFORMING_SWPRINTFS _CRT_SECURE_NO_DEPRECATE _WINSOCK_DEPRECATED_NO_WARNINGS)
if ( DSpellCheck_USE_VLD )
  target_compile_definitions (DSpellCheckStatic PUBLIC VLD_BUILD)
endif ()

target_include_directories (DSpellCheckStatic PUBLIC src deps/win-iconv)
target_include_directories (DSpellCheckStatic PRIVATE src/Controls)

target_link_libraries (DSpellCheckStatic hunspell minizip ftpclient iconv-static Controls npp_extra lsignal aspell json)
target_link_libraries (DSpellCheckStatic shlwapi comctl32 version wininet Ws2_32 dbghelp UxTheme)

if ( NOT ${DSpellCheck_SANITIZE} STREQUAL none)
  if ( ${DSpellCheck_SANITIZE} STREQUAL address)
    target_link_libraries (DSpellCheckStatic
      clang_rt.asan_dynamic-x86_64
      clang_rt.asan_dynamic_runtime_thunk-x86_64)
  endif ()
endif ()

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

enable_testing()
add_test(NAME DSpellCheckTest COMMAND DSpellCheckTest)

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options (DSpellCheckStatic PUBLIC /permissive- /Zc:twoPhase- /Zc:threadSafeInit-)
endif ()
target_compile_options (DSpellCheckStatic PUBLIC /std:c++latest /EHsc)
target_compile_options (DSpellCheckStatic PUBLIC /W4 /WX /w44062 /utf-8)

set(DSpellCheck_PVS_CHECK OFF CACHE BOOL "Enable checking by PVS studio")
if (DSpellCheck_PVS_CHECK)
pvs_studio_add_target(TARGET DSpellCheck.analyze ALL
                      OUTPUT FORMAT errorfile
                      ANALYZE DSpellCheck
                      LOG DSpellCheck.psv-err)
endif ()

set_property(TARGET DSpellCheck DSpellCheckStatic DSpellCheckTest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_precompile_headers (DSpellCheckStatic PRIVATE "src/common/PrecompiledHeader.h")
target_precompile_headers (DSpellCheckTest REUSE_FROM DSpellCheckStatic)
