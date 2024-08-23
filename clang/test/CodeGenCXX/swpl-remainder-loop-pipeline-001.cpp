// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -fno-unroll-loops -o - %s | FileCheck %s

#define N 999

extern int printf(const char*,...);

void pipeline_initiation_interval(void) {
    int i;
    int a[N], b[N], c[N];
    int x[N], y[N], z[N];
	int sum=0;
#pragma clang loop pipeline(enable)
#pragma clang loop pipeline_initiation_interval(10)
    for (i = 0; i < N; i++) {
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP1:.*]]
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP2:.*]]
        z[i] = x[i] + y[i];
        c[i] = a[i] - b[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}

// CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[MP:![0-9]+]], [[UD:![0-9]+]], [[PIPELINE:![0-9]+]], [[II:![0-9]+]], [[VEC:![0-9]+]], [[UR:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[UD]] = !{!"llvm.loop.unroll.disable"}
// CHECK-NEXT: [[PIPELINE]] = !{!"llvm.loop.pipeline.enable"}
// CHECK-NEXT: [[II]] = !{!"llvm.loop.pipeline.initiationinterval", i32 10}
// CHECK-NEXT: [[VEC]] = !{!"llvm.loop.isvectorized", i32 1}
// CHECK-NEXT: [[UR]] = !{!"llvm.loop.unroll.runtime.disable"}

// CHECK: ![[LOOP2]] = distinct !{![[LOOP2]], [[MP]], [[UD]], [[PIPELINE]], [[II]], [[VEC]], [[UR]], [[REMAINDER:![0-9]+]]}
// CHECK-NEXT: [[REMAINDER]] = !{!"llvm.remainder.pipeline.disable"}
