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
#define E_2 "clang loop pipeline_nodep(disable)"
#define E_3 "clang loop pipeline_nodep()"


#define n 10000

void foo (float *A, float *B, float *C, float *D, float *E) {
    int i;
	int sum=0;
P(A_2)
P(C_1)
P(E_1)
    for (i = 0; i < n; i++) {
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        A[i + 1] = A[i] * B[i];  // line 8
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_2:.*]]
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_3:.*]]
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_4:.*]]
        C[i] = D[i] * E[i];      // line 9
    }
}


// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[MP:![0-9]+]], [[DIS:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[DIS]] = distinct !{[[DIS]], [[MP]], [[VW:![0-9]+]], [[VF:![0-9]+]]}
// CHECK-NEXT: [[VW]] = !{!"llvm.loop.vectorize.width", i32 1}
// CHECK-NEXT: [[VF]] = !{!"llvm.loop.vectorize.followup_all", [[DIS_1:![0-9]+]]}
// CHECK-NEXT: [[DIS_1]] = distinct !{[[DIS_1]], [[MP]], [[VCT:![0-9]+]], [[NDP:![0-9]+]]}
// CHECK-NEXT: [[VCT]] = !{!"llvm.loop.isvectorized"}
// CHECK-NEXT: [[NDP]] = !{!"llvm.loop.pipeline.nodep"}

// CHECK: ![[LOOP1_3]] = distinct !{![[LOOP1_3]], [[DIS]]}
// CHECK: ![[LOOP1_4]] = distinct !{![[LOOP1_4]], [[DIS]]}