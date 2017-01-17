cmake_minimum_required(VERSION 2.8.11)
project(stinger_core)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")

# Memory size configuration bits for stinger_core
set(STINGER_DEFAULT_VERTICES "(1L<<24)" CACHE STRING "Default number of vertices")
set(STINGER_DEFAULT_NUMETYPES "5" CACHE STRING "Default number of edge types")
set(STINGER_DEFAULT_NUMVTYPES "128" CACHE STRING "Default number of vertex types")
set(STINGER_DEFAULT_NEB_FACTOR "4" CACHE STRING "Default number of edge blocks per vertex")
set(STINGER_EDGEBLOCKSIZE "14" CACHE STRING "Number of edges per edge block")
set(STINGER_NAME_STR_MAX "255" CACHE STRING "Max string length in physmap")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/inc/stinger_defs.h.in ${CMAKE_BINARY_DIR}/include/stinger_defs.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/inc/stinger_names.h.in ${CMAKE_BINARY_DIR}/include/stinger_names.h @ONLY)

set(sources
	src/core_util.c
	src/stinger.c
	src/stinger_deprecated.c
	src/stinger_names.c
	src/stinger_names_sqlite.c
	src/stinger_physmap.c
	src/stinger_shared.c
	src/stinger_vertex.c
	src/xmalloc.c
	src/x86_full_empty.c
	# HACK copied in from lib compat
	src/getMemorySize.c
)
set(headers
	inc/core_util.h
	inc/stinger.h
	inc/stinger_atomics.h
	inc/stinger_deprecated.h
	inc/stinger_error.h
	inc/stinger_internal.h
	inc/stinger_physmap.h
	inc/stinger_return.h
	inc/stinger_shared.h
	inc/stinger_traversal.h
	inc/stinger_vertex.h
	inc/x86_full_empty.h
	inc/xmalloc.h
	inc/stinger_batch_insert.h
	# HACK copied in from lib compat
	inc/getMemorySize.h
)

set(config
	${CMAKE_BINARY_DIR}/include/stinger_names.h
	${CMAKE_BINARY_DIR}/include/stinger_defs.h
)

include_directories(inc ${CMAKE_BINARY_DIR}/include )
set_source_files_properties(${config} PROPERTIES GENERATED TRUE)
add_library(stinger_core ${sources} ${headers} ${config})
