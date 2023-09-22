// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -o - %s | FileCheck %s
// CHECK: [[A:![0-9]+]] = !{!"llvm.remainder.pipeline.disable"}
// CHECK: [[B:![0-9]+]] = !{!"llvm.loop.pipeline.enable"}
// CHECK-NEXT: [[C:![0-9]+]] = !{!"llvm.loop.pipeline.initiationinterval", i32 30}

extern int printf(const char*,...);

int remainder_loop(int count, char **val) {
	int sum=0;
	int tmp=100;
#pragma clang loop unroll(enable)
#pragma clang loop pipeline(enable)
#pragma clang loop pipeline_initiation_interval(30)
    for (int i=0; i<count; i++) {
        sum*=val[0][0];
        sum+=i;
    }
	printf("%d\n", sum);
	return 0;
}
