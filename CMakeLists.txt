cmake_minimum_required (VERSION 3.7)
project (DSpellCheck)
include (CMake/ucm.cmake)

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

file (GLOB_RECURSE source_files src/*.cpp src/*.h)
list(REMOVE_ITEM source_files src/DllMain.cpp)

add_library (DSpellCheckStatic STATIC ${source_files})
target_include_directories (DSpellCheckStatic PUBLIC src deps/win-iconv)

target_link_libraries(DSpellCheckStatic hunspell)
target_link_libraries(DSpellCheckStatic minizip)
target_link_libraries(DSpellCheckStatic ftpclient)
target_link_libraries(DSpellCheckStatic iconv-static)
target_link_libraries(DSpellCheckStatic Controls)
target_link_libraries(DSpellCheckStatic npp_extra)
target_link_libraries(DSpellCheckStatic lsignal)
target_link_libraries(DSpellCheckStatic aspell)
target_link_libraries(DSpellCheckStatic json)
target_link_libraries(DSpellCheckStatic shlwapi comctl32 version wininet Ws2_32 dbghelp)
target_include_directories (DSpellCheckStatic PRIVATE src/Controls)

if ( NOT ${DSpellCheck_SANITIZE} STREQUAL none)
  if ( ${DSpellCheck_SANITIZE} STREQUAL address)
    target_link_libraries (DSpellCheckStatic
      clang_rt.asan_dynamic-x86_64
      clang_rt.asan_dynamic_runtime_thunk-x86_64)
  endif ()
endif ()

# this is hack to support native spell checker while building with xp toolset
set(DSpellCheck_SUPPOSED_SDK_PATH "c:\\Program Files (x86)\\Windows Kits\\8.1\\Include\\")
if (EXISTS ${DSpellCheck_SUPPOSED_SDK_PATH})
  set (DSpellCheck_NEW_SDK_PATH_DEFAULT ${DSpellCheck_SUPPOSED_SDK_PATH})
endif ()
set(DSpellCheck_NEW_SDK_PATH ${DSpellCheck_NEW_SDK_PATH_DEFAULT} CACHE STRING "Newer(8.1) SDK path to support native windows spell check")

if ( NOT ${DSpellCheck_NEW_SDK_PATH} STREQUAL none)
    target_compile_definitions (DSpellCheckStatic PRIVATE DSPELLCHECK_NEW_SDK)
    target_include_directories (DSpellCheckStatic PRIVATE ${DSpellCheck_NEW_SDK_PATH}/um 
      ${DSpellCheck_NEW_SDK_PATH}/shared)
endif ()

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options (DSpellCheckStatic PUBLIC /permissive- /Zc:twoPhase-)
endif ()
target_compile_options (DSpellCheckStatic PUBLIC /std:c++latest /EHsc)

add_library (DSpellCheck SHARED src/DllMain.cpp src/DSpellCheck.rc)
target_link_libraries (DSpellCheck DSpellCheckStatic)

set(DSpellCheck_DLL_OUTPUT_PATH "" CACHE STRING "Output for debug library (Possibly notepad++ plugin folder)")
if (NOT ${DSpellCheck_DLL_OUTPUT_PATH} STREQUAL "")
    set_target_properties (DSpellCheck PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DSpellCheck_DLL_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DSpellCheck_DLL_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DSpellCheck_DLL_OUTPUT_PATH})
endif ()

file (GLOB_RECURSE test_source_files test/*.cpp test/*.h)
add_executable (DSpellCheckTest ${test_source_files})
target_link_libraries (DSpellCheckTest DSpellCheckStatic catch)

if (MSVC)
	target_compile_options (DSpellCheckStatic PRIVATE "/YuPrecompiledHeader.h" "/FIPrecompiledHeader.h")
	SET_SOURCE_FILES_PROPERTIES("src/PrecompiledHeader.cpp" PROPERTIES COMPILE_FLAGS "/Yc")
endif ()
