set(sources
	src/core_util.c
	src/stinger.c
	src/stinger_deprecated.c
	src/stinger_names.c
	# codegen doesn't like empty files
	#src/stinger_names_sqlite.c
	src/stinger_physmap.c
	src/stinger_shared.c
	src/stinger_vertex.c
	src/xmalloc.c
	src/emu_xmalloc.c
	src/x86_full_empty.c
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
	inc/emu_xmalloc.h
	# Only file that requires C++
	#inc/stinger_batch_insert.h
)

set(config
	${CMAKE_BINARY_DIR}/include/stinger_names.h
	${CMAKE_BINARY_DIR}/include/stinger_defs.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/stinger_core")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")

set_source_files_properties(${config} PROPERTIES GENERATED TRUE)
add_library(stinger_core ${sources} ${headers} ${config})
target_link_libraries(stinger_core compat)
