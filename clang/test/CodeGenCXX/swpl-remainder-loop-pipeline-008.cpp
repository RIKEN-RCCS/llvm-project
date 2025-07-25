// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -o - %s | FileCheck %s
// CHECK-NOT: !{!"llvm.remainder.pipeline.disable"}

#define N 992

extern int printf(const char*,...);

void test(void) {
    int i;
    int a[N], b[N][N];
	int sum=0;
    for (int j = 0; j < N; j++) {
        for (int i = j+1; i < N; i++) {
            a[i] -= b[j][i] * a[j];
        }
    }
    sum = a[0];
	printf("%d\n", sum);
}
