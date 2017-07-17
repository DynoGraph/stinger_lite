
include_directories(${CMAKE_SOURCE_DIR}/lib/dynograph_util)
link_libraries(stinger_core stinger_alg dynograph_util)

add_executable(dynograph main.c layout.c)

# Tests
add_executable(emu_array_test emu_array_test.c)
