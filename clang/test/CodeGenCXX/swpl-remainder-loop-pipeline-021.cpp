// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -Ofast -vectorize-loops -mllvm -fswp -mllvm -swpl-enable-pipeline-remainder-vec -o - %s | FileCheck %s

#define iterations 100000
#define LEN_1D 32000
#define LEN_2D 256

float aa[LEN_2D][LEN_2D],bb[LEN_2D][LEN_2D],cc[LEN_2D][LEN_2D];

extern int dummy(const float,...);

void test()
{

    for (int nl = 0; nl < 100*(iterations/LEN_2D); nl++) {
        for (int j = 0; j < LEN_2D; j++) {
            for (int i = j; i < LEN_2D; i++) {
                // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP1:.*]]
                // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP2:.*]]
                // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP3:.*]]
                // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP4:.*]]
                // CHECK: br i1 %{{.+}}, label {{.*}}, !llvm.loop ![[LOOP5:.*]]
                aa[i][j] = bb[i][j] + cc[i][j];
            }
        }
        dummy(1., aa, bb, cc, 1.);
    }

    return;
}

// CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[MP:![0-9]+]], [[VEC:![0-9]+]], [[UNROLL:![0-9]+]]}
// CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
// CHECK-NEXT: [[VEC]] = !{!"llvm.loop.isvectorized", i32 1}
// CHECK-NEXT: [[UNROLL]] = !{!"llvm.loop.unroll.runtime.disable"}

// CHECK: ![[LOOP2]] = distinct !{![[LOOP2]], [[UD:![0-9]+]], [[REMAINDER:![0-9]+]]}
// CHECK-NEXT: [[UD]] = !{!"llvm.loop.unroll.disable"}
// CHECK-NEXT: [[REMAINDER]] = !{!"llvm.remainder.pipeline.disable"}

// CHECK: ![[LOOP3]] = distinct !{![[LOOP3]], [[MP]]}

// CHECK: ![[LOOP4]] = distinct !{![[LOOP4]], [[MP]]}

// CHECK: ![[LOOP5]] = distinct !{![[LOOP5]], [[MP]], [[VEC]]}
