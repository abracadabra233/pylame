cmake_minimum_required(VERSION 3.12)

project(pylame C CXX)  

set(ARCH_FLAGS "")
set(HOST_PLATFORM "")

set(BUILT_FILE "pylame-1.0.0-cp34-cp34m-linux_x86_64.whl")
set(LAME_FLAGS "-fPIC")
set(LAME_CONFIGURE "<SOURCE_DIR>/configure" "CFLAGS=${LAME_FLAGS}" "LDFLAGS=${LAME_FLAGS}"
    "--prefix=<BINARY_DIR>"
    "--enable-expopt" "--enable-nasm" "--disable-frontend" "--disable-decoder"
    "--disable-analyzer-hooks" "--disable-debug" "--disable-dependency-tracking")
set(LAME_INSTALL $(MAKE) install)
set(LAME_MAKE "make")  # 明确设置 make 命令

message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

include(ExternalProject)
ExternalProject_Add(lame
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/lame"
    GIT_REPOSITORY https://gitee.com/abracadabra233/lame.git
    GIT_TAG main 
    PATCH_COMMAND "${CMAKE_COMMAND}" 
        -D "SOURCE_DIR=<SOURCE_DIR>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/patch-lame.cmake"
    CONFIGURE_COMMAND ${LAME_CONFIGURE}
    BUILD_COMMAND ${LAME_MAKE}
    INSTALL_COMMAND ${LAME_INSTALL}
    BUILD_IN_SOURCE 1
)

ExternalProject_Get_Property(lame BINARY_DIR)
ExternalProject_Get_Property(lame SOURCE_DIR)
message(STATUS "Lame binary directory: ${BINARY_DIR}")
message(STATUS "Lame source directory: ${SOURCE_DIR}")

include_directories("${BINARY_DIR}/include")

add_library(lame_lib STATIC IMPORTED)
set_target_properties(lame_lib PROPERTIES
    IMPORTED_LOCATION "${BINARY_DIR}/lib/libmp3lame.a"
)

add_subdirectory(pybind11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pybind11_add_module(pylame "${CMAKE_SOURCE_DIR}/pylame.cpp")

target_link_libraries(pylame PRIVATE lame_lib)
