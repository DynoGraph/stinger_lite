# Pass this file to the first invocation of cmake using -DCMAKE_TOOLCHAIN_FILE=
set(CMAKE_SYSTEM_NAME Emu1)

set(LLVM_CILK_HOME "/home1/tdysart/Emu1-Nightly/llvm-cilk")
set(CMAKE_C_COMPILER "${LLVM_CILK_HOME}/emu-cc.sh")
set(MEMWEB_INSTALL "${LLVM_CILK_HOME}/memoryweb-libraries/memoryweb/install")
set(LIBC_INSTALL "${LLVM_CILK_HOME}/memoryweb-libraries/musl/install")