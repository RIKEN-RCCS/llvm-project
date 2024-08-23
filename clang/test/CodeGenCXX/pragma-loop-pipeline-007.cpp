// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s
// CHECK: !{!"llvm.loop.pipeline.disable", i1 true}

extern int printf(const char*,...);

void ipeline_disable (int Length, char **Value) {
	int sum=0;
#pragma clang loop pipeline(disable)
#pragma clang loop unroll(enable)
    for (int i=0; i<Length; i++) {
        sum*=Value[0][0];
        sum+=i;
    }
	printf("%d\n", sum);
}

