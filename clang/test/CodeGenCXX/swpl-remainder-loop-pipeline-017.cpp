// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -o - %s | FileCheck %s
// CHECK-NOT: [[A:![0-9]+]] = !{!"llvm.remainder.pipeline.disable"}

// Non remainder loop 
extern int printf(const char*,...);

int A[1000];
int B[1000];
int C[1000];
int D[1000];

void sub(int N) {
#pragma clang loop unroll_count(50)
  for (int i = 0 ; i < 100; i++) {
    A[ i ] = C[ i ] + D[ i ];
    B[ i ] = C[ i ] * D[ i ];
  }
  printf("%d\n", B[0]);
}
