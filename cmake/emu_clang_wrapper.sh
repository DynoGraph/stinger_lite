#!/bin/bash

# The emu toolchain breaks compilation into several intermediate steps:
#
# clang:          .c -> .ll           # Compilation
# opt:            .ll -> .sexp        # Optimization {running some module called "Spiders"?}
# codegen:        .sexp -> .s         # Code generation
# ema.py:         .s -> .o            # Assembly
# gossamer64-ld:  .o -> .mwx          # Linking
#
# Build tools like CMake expect the compiler to be able to make a .o from a .c in one step
# This script wraps up all these steps into one that can be transparently used like a regular compiler

# Copied from common makefile in examples
# TODO determine how many of these flags belong in the makefile instead of the compiler
LLVM_CILK_HOME="/home1/tdysart/Emu1-Nightly/llvm-cilk"
MEMWEB_INSTALL="${LLVM_CILK_HOME}/memoryweb-libraries/memoryweb/install"
LIBC_INSTALL="${LLVM_CILK_HOME}/memoryweb-libraries/musl/install"

LLVM_DIR="${LLVM_CILK_HOME}/lc_install_noStats"
INCLUDE="-I${MEMWEB_INSTALL}/include -I${LLVM_DIR}/include/cilk -I${LIBC_INSTALL}/include -I${LLVM_CILK_HOME}"
LINKAGE="-L${MEMWEB_INSTALL}/lib -L${LIBC_INSTALL}/lib -lmemoryweb -lmuslc"
CLANG="${LLVM_DIR}/bin/clang"
OPT="${LLVM_DIR}/bin/opt"
ASSEMBLER="${LLVM_CILK_HOME}/ema/ema.py"
LD="${LLVM_CILK_HOME}/binutils-gdb-install/bin/gossamer64-ld"

CLANGFLAGS="-O -fno-builtin -nostdinc -fno-unroll-loops -ffreestanding -fno-vectorize -fno-slp-vectorize -S -emit-llvm -fcilkplus -target le64-nacl"
OPTFLAGS="-load Spiders.so -expandgep -O1 -lowerswitch -break-crit-edges -spiders -S"
CODEGEN="${LLVM_CILK_HOME}/code-generator/codegen"

# Extract the name of the source file and the output file from the command line arguments
i=0
capture_object=0
capture_source=0
for var in "$@"; do
    if [[ "$var" == "-o" ]]; then
        capture_object=1
    elif [[ "$var" == "-c" ]]; then
        # Can't always be sure that source file follows -c, but this should be true for cmake at least
        capture_source=1
    elif [[ "$capture_object" -eq 1 ]]; then
        OBJECT="$var"
        capture_object=0
    elif [[ "$capture_source" -eq 1 ]]; then
        SOURCE="$var"
        capture_source=0
    else
        # Pass everything else on to clang
        CLANGFLAGS+=" $var"
    fi

    #echo "$i >>>$var<<<"
    i=$((i+1))
done

NAME=${OBJECT}

# DEBUG
# echo "SOURCE=$SOURCE"
# echo "OBJECT=$OBJECT"
# echo "NAME=$NAME"

# Quit if any commands fail
set -e
# Echo commands as we go
set -x

${CLANG} ${INCLUDE} ${CLANGFLAGS} -o ${NAME}.ll ${SOURCE}
${OPT} ${OPTFLAGS} ${NAME}.ll > ${NAME}.sexp
${CODEGEN} ${CODEGENFLAGS} ${NAME}.sexp > ${NAME}.s
${ASSEMBLER} ${ASSEMBLERFLAGS} -o ${OBJECT} ${NAME}.s