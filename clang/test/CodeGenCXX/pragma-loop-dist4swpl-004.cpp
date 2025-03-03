// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -S -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s

#define P(a) _Pragma(a)
#define A_1 "clang loop vectorize(enable)"
#define A_2 "clang loop vectorize(disable)"
#define B_1 "clang loop unroll(enable)"
#define B_2 "clang loop unroll(disable)"
#define C_1 "clang loop distribute(enable)"
#define C_2 "clang loop distribute(disable)"
#define D_1 "clang loop pipeline(enable)"
#define D_2 "clang loop pipeline(disable)"
#define E_1 "clang loop distribute4swpl(enable)"
#define E_2 "clang loop distribute4swpl(disable)"
#define E_3 "clang loop distribute4swpl()"
#define E_4 "clang loop distribute4swpl(2)"


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
        // CHECK: br label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}


// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[MP:![0-9]+]], [[UR:![0-9]+]], [[D4SWPL:![0-9]+]], [[D4SWPL_FA:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[UR]] = !{!"llvm.loop.unroll.disable"}
// CHECK-NEXT: [[D4SWPL]] = !{!"llvm.loop.distribute4swpl.enable", i1 true}
// CHECK-NEXT: [[D4SWPL_FA]] = !{!"llvm.loop.distribute4swpl.followup_all", [[DIST_1:![0-9]+]]}
// CHECK-NEXT: [[DIST_1]] = distinct !{[[DIST_1]], [[MP:![0-9]+]], [[UR:![0-9]+]], [[VEC_1:![0-9]+]], [[VEC_FA:![0-9]+]]}
// CHECK-NEXT: [[VEC_1]] = !{!"llvm.loop.vectorize.enable", i1 true}
// CHECK-NEXT: [[VEC_FA]] = !{!"llvm.loop.vectorize.followup_all", [[DIST_2:![0-9]+]]}
// CHECK-NEXT: [[DIST_2]] = distinct !{[[DIST_2]], [[MP:![0-9]+]], [[UR:![0-9]+]], [[VEC_2:![0-9]+]], [[PIP:![0-9]+]]}
// CHECK-NEXT: [[VEC_2]] = !{!"llvm.loop.isvectorized"}
// CHECK-NEXT: [[PIP]] = !{!"llvm.loop.pipeline.enable"}
