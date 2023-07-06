// RUN: %clang -### -S -o FOO --target=aarch64-unknown-gnu -mcpu=a64fx -fswp %s 2>&1 | FileCheck %s
// CHECK: "-cc1"
// CHECK: "-mllvm" "-fswp"