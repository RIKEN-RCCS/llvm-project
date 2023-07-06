// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s
// CHECK: !{!"llvm.loop.pipeline.disable", i1 true}

#define n 10000

void foo (int *A, int *B, int *C, int *D, int *E) {
    int i;
	int sum=0;
#pragma clang loop distribute(enable)
#pragma clang loop pipeline(disable)
    for (i = 0; i < n; i++) {
        A[i + 1] = A[i] * B[i];  // line 8
        C[i] = D[i] * E[i];      // line 9
    }
}
