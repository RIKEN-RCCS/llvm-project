// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -o - %s | FileCheck %s
// CHECK-NOT: [[A:![0-9]+]] = !{!"llvm.remainder.pipeline.disable"}
// CHECK: [[B:![0-9]+]] = !{!"llvm.loop.pipeline.disable", i1 true}

extern int printf(const char*,...);

int remainder_loop(int count, char **val) {
	int sum=0;
	int tmp=100;
#pragma clang loop unroll(enable)
#pragma clang loop pipeline(disable)
    for (int i=0; i<count; i++) {
        sum*=val[0][0];
        sum+=i;
    }
	printf("%d\n", sum);
	return 0;
}
