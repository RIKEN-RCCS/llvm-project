// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s
// CHECK: !{!"llvm.loop.pipeline.disable", i1 true}

void for_test(int *List, int Length) {
#pragma clang loop pipeline(disable)
#pragma clang loop interleave(disable)
  for (int i = 0; i < Length; i++) {
    List[i] = i * 2;
  }
}
