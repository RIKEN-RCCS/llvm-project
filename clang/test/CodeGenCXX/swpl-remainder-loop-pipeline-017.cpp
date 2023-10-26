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
    // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP1:.*]]
    A[ i ] = C[ i ] + D[ i ];
    B[ i ] = C[ i ] * D[ i ];
  }
  printf("%d\n", B[0]);
}

// CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[MP:![0-9]+]], [[UD:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[UD]] = !{!"llvm.loop.unroll.disable"}
