project(win_iconv)

cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

set(CPACK_GENERATOR "TBZ2")
include(CPack)

option(WINICONV_BUILD_TEST "build test executable" OFF)

if(WINICONV_BUILD_TEST)
    enable_testing()
endif(WINICONV_BUILD_TEST)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(BUILD_EXECUTABLE)
    add_executable(win_iconv win_iconv.c)
    set_target_properties(win_iconv PROPERTIES COMPILE_FLAGS "-DMAKE_EXE")
    if(WINCE)
        target_link_libraries(win_iconv ${WCECOMPAT_LIBRARIES})
    endif(WINCE)
    install(TARGETS win_iconv RUNTIME DESTINATION bin
                              LIBRARY DESTINATION lib
                              ARCHIVE DESTINATION lib)
endif(BUILD_EXECUTABLE)

add_library(iconv-static STATIC win_iconv.c)
set_target_properties(iconv-static PROPERTIES OUTPUT_NAME "iconv")
if(WINCE)
    target_link_libraries(iconv-static ${WCECOMPAT_LIBRARIES})
endif(WINCE)
install(TARGETS iconv-static RUNTIME DESTINATION bin
                             LIBRARY DESTINATION lib
                             ARCHIVE DESTINATION lib)


install(FILES iconv.h DESTINATION include)

if(WINICONV_BUILD_TEST)
    # tests:
    add_executable(win_iconv_test win_iconv_test.c)
    add_test(win_iconv_test win_iconv_test)
    if(WINCE)
        target_link_libraries(win_iconv_test ${WCECOMPAT_LIBRARIES})
    endif(WINCE)
endif(WINICONV_BUILD_TEST)
