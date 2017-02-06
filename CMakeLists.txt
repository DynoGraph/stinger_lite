cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "cmake/emu-rules.cmake")
project(stinger_lite C)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emu1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x -Wall")
endif()

# Memory size configuration bits for stinger_core
set(STINGER_DEFAULT_VERTICES "(1L<<24)" CACHE STRING "Default number of vertices")
set(STINGER_DEFAULT_NUMETYPES "5" CACHE STRING "Default number of edge types")
set(STINGER_DEFAULT_NUMVTYPES "128" CACHE STRING "Default number of vertex types")
set(STINGER_DEFAULT_NEB_FACTOR "4" CACHE STRING "Default number of edge blocks per vertex")
set(STINGER_EDGEBLOCKSIZE "14" CACHE STRING "Number of edges per edge block")
set(STINGER_NAME_STR_MAX "255" CACHE STRING "Max string length in physmap")

set(STINGER_ALLOCATION_STRATEGY "CONTIGUOUS" CACHE STRING
  "Controls the way the stinger data structure is allocated. Options are:
  CONTIGUOUS: Use a single memory allocation for the entire stinger data structure. Must be enabled to use shared memory.
  DISTRIBUTED: Distribute the stinger data structure across memory in a platform-dependent manner.
  MULTIPLE: Allocate each sub-structure separately and store pointers to each one."
)
if(STINGER_ALLOCATION_STRATEGY STREQUAL "CONTIGUOUS")
  add_definitions(-DSTINGER_USE_CONTIGUOUS_ALLOCATION)
elseif(STINGER_ALLOCATION_STRATEGY STREQUAL "DISTRIBUTED")
  add_definitions(-DSTINGER_USE_DISTRIBUTED_ALLOCATION)
elseif(STINGER_ALLOCATION_STRATEGY STREQUAL "MULTIPLE")
  add_definitions(-DSTINGER_USE_MULTIPLE_ALLOCATION)
else()
  message(FATAL_ERROR "'${STINGER_ALLOCATION_STRATEGY}' is not a valid allocation strategy.")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/stinger_core/inc/stinger_defs.h.in ${CMAKE_BINARY_DIR}/include/stinger_core/stinger_defs.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/stinger_core/inc/stinger_names.h.in ${CMAKE_BINARY_DIR}/include/stinger_core/stinger_names.h @ONLY)

# Define a custom function for copying headers to the build directory
function(publish_headers header_list destination)
  set(published_headers "")
  foreach(header ${${header_list}})
    get_filename_component(name ${header} NAME)
    set(output "${destination}/${name}")
    list(APPEND published_headers ${output})
    add_custom_command(
      OUTPUT ${output}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${destination}
      COMMAND ${CMAKE_COMMAND} -E copy ${header} ${destination}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${header}
    )
  endforeach()
  # Overwrite the list of headers in the caller, so targets depend on the published version
  set(${header_list} ${published_headers} PARENT_SCOPE)
endfunction()

include_directories("${CMAKE_BINARY_DIR}/include")

if (CMAKE_SYSTEM_NAME STREQUAL "Emu1")
  link_directories("${LIBC_INSTALL}/lib" "${MEMWEB_INSTALL}/lib")
  link_libraries(memoryweb muslc)
endif()

add_subdirectory(lib)
add_subdirectory(src)
