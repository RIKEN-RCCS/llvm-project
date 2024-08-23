// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s
// CHECK: !{!"llvm.loop.pipeline.enable"}
// CHECK: !{!"llvm.loop.pipeline.initiationinterval", i32 30}

#define N 1000

extern int printf(const char*,...);

void pipeline_initiation_interval(void) {
    int i;
    int a[N], b[N], c[N];
    int x[N], y[N], z[N];
	int sum=0;
#pragma clang loop vectorize(enable)
#pragma clang loop pipeline(enable)
#pragma clang loop pipeline_initiation_interval(30)
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
        c[i] = a[i] - b[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}
