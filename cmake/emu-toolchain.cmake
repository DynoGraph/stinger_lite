# Pass this file to the first invocation of cmake using -DCMAKE_TOOLCHAIN_FILE=
set(CMAKE_SYSTEM_NAME Emu1)
set(CMAKE_C_COMPILER "${CMAKE_SOURCE_DIR}/cmake/emu_clang_wrapper.sh")

set(LLVM_CILK_HOME "/home1/tdysart/Emu1-Nightly/llvm-cilk")
set(MEMWEB_INSTALL "${LLVM_CILK_HOME}/memoryweb-libraries/memoryweb/install")
set(LIBC_INSTALL "${LLVM_CILK_HOME}/memoryweb-libraries/musl/install")
set(CMAKE_LINKER "${LLVM_CILK_HOME}/binutils-gdb-install/bin/gossamer64-ld")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
