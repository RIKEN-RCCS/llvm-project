// RUN: %clang_cc1 -triple aarch64-unknown-hurd-gnu -emit-llvm -target-cpu a64fx -o - %s | FileCheck %s
// CHECK: !{!"llvm.loop.pipeline.enable"}
// CHECK: !{!"llvm.loop.pipeline.initiationinterval", i32 10}

void pipeline_enable(int *List, int Length, int Value) {

#pragma clang loop pipeline(enable)
#pragma clang loop pipeline_initiation_interval(10)
  for (int i = 0; i < Length; i++) {
    List[i] = Value;
  }
}
