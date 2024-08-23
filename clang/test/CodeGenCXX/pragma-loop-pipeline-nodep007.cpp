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


extern int printf(const char*,...);

void ipeline_disable (int Length, char **Value) {
	int sum=0;
P(B_1)
P(C_2)
P(D_1)
P(E_1)
    for (int i=0; i<Length; i++) {
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_1:.*]]
        sum*=Value[0][0];
        // CHECK: br i1 {{.*}}, label {{.*}}, label {{.*}}, !llvm.loop ![[LOOP1_2:.*]]
        sum+=i;
    }
	printf("%d\n", sum);
}



// CHECK: ![[LOOP1_1]] = distinct !{![[LOOP1_1]], [[DIS:![0-9]+]], ![[RPIPE:.*]]}
// CHECK-NEXT: [[DIS]] = distinct !{[[DIS]], [[MP:![0-9]+]], [[DB:![0-9]+]], [[UR:![0-9]+]], [[PIPE:![0-9]+]], [[NDP:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[DB]] = !{!"llvm.loop.distribute.enable", i1 false}
// CHECK-NEXT: [[UR]] = !{!"llvm.loop.unroll.disable"}   
// CHECK-NEXT: [[PIPE]] = !{!"llvm.loop.pipeline.enable"}
// CHECK-NEXT: [[NDP]] = !{!"llvm.loop.pipeline.nodep"}
// CHECK-NEXT: ![[RPIPE]] = !{!"llvm.remainder.pipeline.disable"}

// CHECK: ![[LOOP1_2]] = distinct !{![[LOOP1_2]], [[DIS]]}
