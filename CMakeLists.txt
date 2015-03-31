project (magic_system)

cmake_minimum_required(VERSION 2.8)

set(SRC_DIR "src")
set(TST_DIR "tst")
set(EXECUTABLE_OUTPUT_PATH build)

file(GLOB_RECURSE magic_system_SOURCES "${SRC_DIR}/*.c")
file(GLOB_RECURSE magic_system_HEADERS "${SRC_DIR}/*.h")

file(GLOB_RECURSE test_source "${TST_DIR}/*.c")


#Pour si on veut faire des sous-dossiers dans src
set(magic_system_INCLUDE_DIRS "")
foreach(_headerFile ${magic_system_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND magic_system_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES magic_system_INCLUDE_DIRS)
#include_directories(${Plankton_INCLUDE_DIRS})

set(TST_SOURCE ${test_source} ${magic_system_SOURCES})
list(REMOVE_ITEM TST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)

add_executable (magic_system ${magic_system_SOURCES})
add_executable (magic_system_test ${TST_SOURCE})
