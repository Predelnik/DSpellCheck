cmake_minimum_required (VERSION 3.15)
project (npp_extra)
file (GLOB_RECURSE source_files *.cpp *.h)
add_library (npp_extra STATIC ${source_files})
target_include_directories(npp_extra PUBLIC .)
set_property(TARGET npp_extra PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
