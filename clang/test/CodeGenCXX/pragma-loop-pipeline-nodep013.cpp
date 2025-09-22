// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -Ofast -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s

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


#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
P(D_1)
P(E_1)
    for (i = 0; i < N; i++) {
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}


// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[MP:![0-9]+]], [[PIPE:![0-9]+]], [[NDP:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[PIPE]] = !{!"llvm.loop.pipeline.enable"}
// CHECK-NEXT: [[NDP]] = !{!"llvm.loop.pipeline.nodep"}