cmake_minimum_required (VERSION 3.7)
project (DSpellCheck)

add_definitions ("-D_CRT_NONSTDC_NO_DEPRECATE -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS")
add_subdirectory (Hunspell) #Thanks https://github.com/zdenop/hunspell-mingw
add_subdirectory (libiconv)
add_subdirectory (minizip)
add_subdirectory (FtpClient)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif()

set(DEBUG_OUTPUT_PATH "" CACHE STRING "Output for debug library (Possibly notepad++ plugin folder)")
if (DEFINED DEBUG_OUTPUT_PATH)
	set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${DEBUG_OUTPUT_PATH} )
endif ()

set(VLD_BUILD_ON "OFF" CACHE BOOL "Use Visual Leak Detector")

if ( VLD_BUILD_ON )
  add_definitions ("-DVLD_BUILD")
endif ()
file (GLOB_RECURSE source_files src/*.cpp src/*.h src/*.rc)

add_library (DSpellCheck SHARED ${source_files})

target_link_libraries(DSpellCheck hunspell)
target_link_libraries(DSpellCheck minizip)
target_link_libraries(DSpellCheck ftpclient)
target_link_libraries(DSpellCheck libiconv)
target_link_libraries(DSpellCheck shlwapi comctl32 version wininet Ws2_32)
target_include_directories (DSpellCheck PRIVATE src/Controls)
target_include_directories (DSpellCheck PRIVATE src)
target_compile_options (DSpellCheck PRIVATE /std:c++latest)

include (cotire.cmake)
set_target_properties(DSpellCheck PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "src/PrecompiledHeader.h")
cotire (DSpellCheck)
