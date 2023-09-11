// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -o - %s | FileCheck %s
// CHECK: !{!"llvm.remainder.pipeline.disable"}

#define N 999

extern int printf(const char*,...);

void pipeline_initiation_interval(void) {
    int i;
    int a[N], b[N], c[N];
    int x[N], y[N], z[N];
	int sum=0;
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
        c[i] = a[i] - b[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}
