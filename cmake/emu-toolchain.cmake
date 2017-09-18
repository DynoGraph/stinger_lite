# Pass this file to the first invocation of cmake using -DCMAKE_TOOLCHAIN_FILE=
set(CMAKE_SYSTEM_NAME Emu1)
set(LLVM_CILK_HOME "/usr/local/emu/toolchain")
set(CMAKE_C_COMPILER "${CMAKE_SOURCE_DIR}/cmake/emu_clang_wrapper.sh")

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH ${LLVM_CILK_HOME}/binutils-gdb-install/gossamer64)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
# search for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(MEMWEB_INSTALL "${LLVM_CILK_HOME}/memoryweb-libraries/memoryweb/install")
set(LIBC_INSTALL "${LLVM_CILK_HOME}/memoryweb-libraries/musl/install")

# In this version of the toolchain, we explicitly set the link command, and modify it to pass along the linked libaries
set(CMAKE_LINKER "${LLVM_CILK_HOME}/binutils-gdb-install/bin/gossamer64-ld")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# Main CMakeLists.txt should include the following lines:

# link_directories("${LIBC_INSTALL}/lib" "${MEMWEB_INSTALL}/lib")
# link_libraries(memoryweb muslc)