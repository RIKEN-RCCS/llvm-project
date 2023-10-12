// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -mllvm -swpl-enable-pipeline-remainder-unroll -o - %s | FileCheck %s
// CHECK-NOT: [[A:![0-9]+]] = !{!"llvm.remainder.pipeline.disable"}

extern int printf(const char*,...);

int remainder_loop(int count, char **val) {
	int sum=0;
	int tmp=100;
#pragma clang loop unroll(enable)
    for (int i=0; i<count; i++) {
        sum*=val[0][0];
        sum+=i;
    }
	printf("%d\n", sum);
	return 0;
}
