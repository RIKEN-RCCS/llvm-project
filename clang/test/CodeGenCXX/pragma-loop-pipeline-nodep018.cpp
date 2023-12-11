// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -Ofast -emit-llvm -mllvm -fswp -target-cpu a64fx -o - %s | FileCheck %s

#define P(a) _Pragma(a)
#define A_1 "clang loop vectorize(enable)"
#define A_2 "clang loop vectorize(disable)"
#define B_1 "clang loop unroll(enable)"
#define B_2 "clang loop unroll(disable)"
#define C_1 "clang loop distribute(enable)"
#define C_2 "clang loop distribute(disable)"
#define D_1 "clang loop pipeline(enable)"
#define D_2 "clang loop pipeline(disable)"
#define E_1 "clang loop pipeline_nodep(enable)"


#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
P(E_1)
P(A_1)
P(B_2)
P(D_1)
    for (i = 0; i < N; i++) {
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_2:.*]]
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_3:.*]]
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}


// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[DIS_1:![0-9]+]], ![[URR:.*]]}
// CHECK-NEXT: [[DIS_1]] = distinct !{[[DIS_1]], [[MP:![0-9]+]], [[UR:![0-9]+]], [[VCT:![0-9]+]], [[PIPE:![0-9]+]], [[NDP:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[UR]] = !{!"llvm.loop.unroll.disable"}
// CHECK-NEXT: [[VCT]] = !{!"llvm.loop.isvectorized"}
// CHECK-NEXT: [[PIPE]] = !{!"llvm.loop.pipeline.enable"}
// CHECK-NEXT: [[NDP]] = !{!"llvm.loop.pipeline.nodep"}
// CHECK-NEXT: ![[URR]] = !{!"llvm.loop.unroll.runtime.disable"}


// CHECK: ![[LOOP1_2]] = distinct !{![[LOOP1_2]], [[DIS_1]], ![[URR]], [[RPIPE:![0-9]+]]}
// CHECK-NEXT: [[RPIPE]] = !{!"llvm.remainder.pipeline.disable"}
// CHECK: ![[LOOP1_3]] = distinct !{![[LOOP1_3]], [[DIS_1]]}
