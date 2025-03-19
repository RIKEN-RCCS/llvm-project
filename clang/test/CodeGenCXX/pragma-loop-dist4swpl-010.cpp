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
#define F_1 "clang loop distribute4swpl_freg(1)"
#define F_2 "clang loop distribute4swpl_freg(16)"
#define F_3 "clang loop distribute4swpl_freg(999)"
#define F_4 "clang loop distribute4swpl_freg(0)"
#define F_5 "clang loop distribute4swpl_freg()"
#define F_6 "clang loop distribute4swpl_freg(enable)"
#define G_1 "clang loop distribute4swpl_ireg(1)"
#define G_2 "clang loop distribute4swpl_ireg(16)"
#define G_3 "clang loop distribute4swpl_ireg(999)"
#define G_4 "clang loop distribute4swpl_ireg(0)"
#define G_5 "clang loop distribute4swpl_ireg()"
#define G_6 "clang loop distribute4swpl_ireg(enable)"

#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
P(A_1)
P(C_1)
P(D_2)
P(G_1)
P(F_2)
P(E_1)
    for (i = 0; i < N; i++) {
        // CHECK: br label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}

// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[MP:![0-9]+]], [[D4SWPL:![0-9]+]], [[D4SWPLF:![0-9]+]], [[D4SWPLI:![0-9]+]], [[D4SWPL_FA:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[D4SWPL]] = !{!"llvm.loop.distribute4swpl.enable", i1 true}
// CHECK-NEXT: [[D4SWPLF]] = !{!"llvm.loop.distribute4swpl.freg", i32 16}
// CHECK-NEXT: [[D4SWPLI]] = !{!"llvm.loop.distribute4swpl.ireg", i32 1}
// CHECK-NEXT: [[D4SWPL_FA]] = !{!"llvm.loop.distribute4swpl.followup_all", [[DIST_1:![0-9]+]]}
// CHECK-NEXT: [[DIST_1]] = distinct !{[[DIST_1]], [[MP]], [[DIST:![0-9]+]], [[DIST_FA:![0-9]+]]}
// CHECK-NEXT: [[DIST]] = !{!"llvm.loop.distribute.enable", i1 true}
// CHECK-NEXT: [[DIST_FA]] = !{!"llvm.loop.distribute.followup_all", [[DIST_2:![0-9]+]]}
// CHECK-NEXT: [[DIST_2]] = distinct !{[[DIST_2]], [[MP]], [[VEC:![0-9]+]], [[VEC_FA:![0-9]+]]}
// CHECK-NEXT: [[VEC]] = !{!"llvm.loop.vectorize.enable", i1 true}
// CHECK-NEXT: [[VEC_FA]] = !{!"llvm.loop.vectorize.followup_all", [[DIST_3:![0-9]+]]}
// CHECK-NEXT: [[DIST_3]] = distinct !{[[DIST_3]], [[MP]], [[VEC_2:![0-9]+]], [[PIP:![0-9]+]]}
// CHECK-NEXT: [[VEC_2]] = !{!"llvm.loop.isvectorized"}
// CHECK-NEXT: [[PIP]] = !{!"llvm.loop.pipeline.disable", i1 true}
