cmake_minimum_required(VERSION 3.25)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(lexkey C)

fetch_package("github:holepunchto/libcompact")

add_library(lexkey INTERFACE)

target_sources(
  lexkey
  INTERFACE
    include/lexkey.h
)

target_include_directories(
  lexkey
  INTERFACE
    include
)

target_link_libraries(
  lexkey
  INTERFACE
    compact_static
)

install(FILES include/lexkey.h DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()
