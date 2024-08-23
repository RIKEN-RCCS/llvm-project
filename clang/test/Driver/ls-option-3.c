// RUN: %clang -### -S -o FOO  -fls %s 2>&1 | FileCheck %s
// CHECK: clang: error: unsupported option '-fls' for target
