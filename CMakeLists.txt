cmake_minimum_required (VERSION 3.7)
project (DSpellCheck)

include_directories (src/Controls)
include_directories (src)
include_directories (Hunspell/src)
include_directories (minizip/)
include_directories (FtpClient/)
include_directories (libiconv/include)

add_subdirectory ("Hunspell") #Thanks https://github.com/zdenop/hunspell-mingw
add_subdirectory ("libiconv")
add_subdirectory ("minizip")
add_subdirectory ("FtpClient")

add_definitions ("-D_CRT_NONSTDC_NO_DEPRECATE -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS")

set(DEBUG_OUTPUT_PATH "" CACHE STRING "Output for debug library (Possibly notepad++ plugin folder)")
if (DEFINED DEBUG_OUTPUT_PATH)
	set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_PATH} )
endif ()

set(VLD_BUILD_ON "OFF" CACHE BOOL "Use Visual Leak Detector")

if ( VLD_BUILD_ON )
  add_definitions ("-DVLD_BUILD")
endif ()

file (GLOB_RECURSE source_files src/*.cpp src/*.h)

SET(LINK_LIBRARY shlwapi comctl32 version wininet Ws2_32)
add_library (DSpellCheck SHARED ${source_files})

foreach(CONFIG DEBUG MINSIZEREL RELEASE RELWITHDEBINFO)
  string(REPLACE "/MD" "/MT"
    "CMAKE_CXX_FLAGS_${CONFIG}"
    "${CMAKE_CXX_FLAGS_${CONFIG}}"
  )
endforeach()

target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/Hunspell/Debug/libhunspell.lib)
target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/libiconv/DebugStatic_Win32/libiconv.lib)
target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/minizip/Debug/minizip.lib)
target_link_libraries (DSpellCheck debug ${PROJECT_BINARY_DIR}/ftpclient/Debug/ftpclient.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/Hunspell/Release/libhunspell.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/libiconv/ReleaseStatic_Win32/libiconv.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/minizip/Release/minizip.lib)
target_link_libraries (DSpellCheck optimized ${PROJECT_BINARY_DIR}/ftpclient/Release/ftpclient.lib)

target_link_libraries(DSpellCheck ${LINK_LIBRARY})

include (cotire.cmake)
set_target_properties(DSpellCheck PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "src/PrecompiledHeader.h")
cotire (DSpellCheck)
