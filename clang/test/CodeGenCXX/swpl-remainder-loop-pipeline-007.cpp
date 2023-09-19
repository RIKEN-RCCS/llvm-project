// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -o - %s | FileCheck %s
// CHECK-NOT: !{!"llvm.remainder.pipeline.disable"}

#define N 992

extern int printf(const char*,...);

void test(void) {
    int i;
    int a[N], b[N], c[N];
    int x[N], y[N], z[N];
	int sum=0;
#pragma clang loop vectorize_width(512, fixed)
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
        c[i] = a[i] - b[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}
