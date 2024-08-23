// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -mllvm -fswp -o - %s | FileCheck %s

extern int printf(const char*,...);

int remainder_loop(int count, char **val) {
	int sum=0;
	int tmp=100;
#pragma clang loop unroll(enable)
    for (int i=0; i<count; i++) {
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP1:.*]]
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP2:.*]]
        sum*=val[0][0];
        sum+=i;
    }
	printf("%d\n", sum);
	return 0;
}

// CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[UD:![0-9]+]], [[REMAINDER:![0-9]+]]}
// CHECK-NEXT: [[UD]] = !{!"llvm.loop.unroll.disable"}
// CHECK-NEXT: [[REMAINDER]] = !{!"llvm.remainder.pipeline.disable"}

// CHECK: ![[LOOP2]] = distinct !{![[LOOP2]], [[MP:![0-9]+]], [[UD]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
