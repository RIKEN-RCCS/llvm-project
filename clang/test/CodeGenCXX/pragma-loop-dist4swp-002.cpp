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
#define E_1 "clang loop distribute4swp(enable)"
#define E_2 "clang loop distribute4swp(disable)"
#define E_3 "clang loop distribute4swp()"
#define E_4 "clang loop distribute4swp(2)"


#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
P(A_2)
P(C_1)
P(D_2)
P(E_2)
    for (i = 0; i < N; i++) {
        // CHECK: br label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}


// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[MP:![0-9]+]], [[D4SWP:![0-9]+]], [[DISTRIBUTE:![0-9]+]], [[DISTRIBUTE_FA:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[D4SWP]] = !{!"llvm.loop.distribute4swp.enable", i1 false}
// CHECK-NEXT: [[DISTRIBUTE]] = !{!"llvm.loop.distribute.enable", i1 true}
// CHECK-NEXT: [[DISTRIBUTE_FA]] = !{!"llvm.loop.distribute.followup_all", [[DIST_1:![0-9]+]]}
// CHECK-NEXT: [[DIST_1]] = distinct !{[[DIST_1]], [[MP:![0-9]+]], [[D4SWP:![0-9]+]], [[VEC_WIDTH:![0-9]+]], [[VEC_FA:![0-9]+]]}
// CHECK-NEXT: [[VEC_WIDTH]] = !{!"llvm.loop.vectorize.width", i32 1}
// CHECK-NEXT: [[VEC_FA]] = !{!"llvm.loop.vectorize.followup_all", [[DIST_2:![0-9]+]]}
// CHECK-NEXT: [[DIST_2]] = distinct !{[[DIST_2]], [[MP:![0-9]+]], [[D4SWP:![0-9]+]], [[ISVECTORIZED:![0-9]+]], [[PIP:![0-9]+]]}
// CHECK-NEXT: [[ISVECTORIZED]] = !{!"llvm.loop.isvectorized"}
// CHECK-NEXT: [[PIP]] = !{!"llvm.loop.pipeline.disable", i1 true}
