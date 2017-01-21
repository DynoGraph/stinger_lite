
include_directories(${CMAKE_SOURCE_DIR}/lib/dynograph_util)
link_libraries(stinger_core dynograph_util)

add_executable(dynograph main.c)