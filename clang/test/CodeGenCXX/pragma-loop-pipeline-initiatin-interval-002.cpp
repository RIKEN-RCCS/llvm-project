// RUN: not %clang_cc1 -triple aarch64-unknown-hurd-gnu -S -o /dev/null -target-cpu a64fx -mllvm -fswp %s 2>&1 | FileCheck -check-prefix CHECK-INVALID %s
// CHECK-INVALID: error: invalid value '-1'; must be positive

void pipeline_initiation_interval(int *List, int Length, int Value) {

#pragma clang loop pipeline_initiation_interval(-1)
  for (int i = 0; i < Length; i++) {
    List[i] = Value;
  }
}
