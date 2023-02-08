// RUN: not %clang_cc1 -triple aarch64-unknown-hurd-gnu -S -o /dev/null -target-cpu a64fx %s 2>&1 | FileCheck -check-prefix CHECK-INVALID %s
// CHECK-INVALID: error: duplicate directives 'pipeline(enable)' and 'pipeline(disable)'

void pipeline_enable(int *List, int Length, int Value) {

#pragma clang loop pipeline(enable)
#pragma clang loop pipeline(disable)
  for (int i = 0; i < Length; i++) {
    List[i] = Value;
  }
}
