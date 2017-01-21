# Pass this file to the first invocation of cmake using -DCMAKE_TOOLCHAIN_FILE=
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER "/home/ehein6/local/bin/emu_clang_wrapper.sh")
# Stable
#set(CMAKE_LINKER "/usr/local/Emu1/toolchain/binutils-gdb-install/bin/gossamer64-ld")
# Nightly build
set(CMAKE_LINKER "/home1/tdysart/Emu1-Nightly/llvm-cilk/binutils-gdb-install/bin/gossamer64-ld")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
