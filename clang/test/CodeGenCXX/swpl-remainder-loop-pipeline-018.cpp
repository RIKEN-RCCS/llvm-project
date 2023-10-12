// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -o - %s | FileCheck %s
// CHECK-NOT: [[A:![0-9]+]] = !{!"llvm.remainder.pipeline.disable"}

// Unroll missed
extern int printf(const char*,...);

int A[1000];
int B[1000];
int C[1000];
int D[1000];

void sub(int N) {
#pragma clang loop unroll(enable)
  for (int i = 0 ; i < N; i++) {
    A[ i ] = C[ i ] + D[ i ];
    B[ i ] = C[ i ] * D[ i ];
    if (A[i] == 0)
      goto end;
  }
end:
  printf("%d\n", B[0]);
}
