// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -mllvm -fswp -o - %s | FileCheck %s

#define N 999

extern int printf(const char*,...);

void pipeline_initiation_interval(void) {
    int i;
    int a[N], b[N], c[N];
    int x[N], y[N], z[N];
	int sum=0;
    for (i = 0; i < N; i++) {
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP1:.*]]
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP2:.*]]
        // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP3:.*]]
        z[i] = x[i] + y[i];
        c[i] = a[i] - b[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}

// CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[MP:![0-9]+]], [[VEC:![0-9]+]], [[UNROLL:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[VEC]] = !{!"llvm.loop.isvectorized", i32 1}
// CHECK-NEXT: [[UNROLL]] = !{!"llvm.loop.unroll.runtime.disable"}

// CHECK: ![[LOOP2]] = distinct !{![[LOOP2]], [[REMAINDER:![0-9]+]]}
// CHECK-NEXT: [[REMAINDER]] = !{!"llvm.remainder.pipeline.disable"}

// CHECK: ![[LOOP3]] = distinct !{![[LOOP3]], [[REMAINDER:![0-9]+]], [[VEC]]}
