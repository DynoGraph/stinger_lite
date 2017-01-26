# Pass this file to the first invocation of cmake using -DCMAKE_TOOLCHAIN_FILE=
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER "/home/ehein6/local/bin/emu_clang_wrapper.sh")

set(LLVM_CILK_HOME "/home1/tdysart/Emu1-Nightly/llvm-cilk")
set(CMAKE_LINKER "${LLVM_CILK_HOME}/binutils-gdb-install/bin/gossamer64-ld")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
