// Tests that the SLP vectorizer is correctly disabled by default.

// RUN: %clang --target=aarch64 -mcpu=a64fx -Rpass=slp-vectorize -c %s -o /dev/null 2>&1 | FileCheck --allow-empty %s
// RUN: %clang -O2 --target=aarch64 -mcpu=a64fx -Rpass=slp-vectorize -c %s -o /dev/null 2>&1 | FileCheck --allow-empty %s
// RUN: %clang -O2 -fslp-vectorize --target=aarch64 -mcpu=a64fx -Rpass=slp-vectorize -c %s -o /dev/null 2>&1 | FileCheck --allow-empty %s
// RUN: %clang -O2 -mllvm -vectorize-slp --target=aarch64 -mcpu=a64fx -Rpass=slp-vectorize -c %s -o /dev/null 2>&1 | FileCheck %s -check-prefix=CHECK-REMARK

alignas(64) double a[32000];

double foo() {
    for (int i = 0; i < 32000 - 3; i += 3) {
        a[i]     = a[i + 1] * a[i];
        a[i + 1] = a[i + 2] * a[i + 1];
        a[i + 2] = a[i + 3] * a[i + 2];
    }
    return a[0];
}

// CHECK-NOT: remark:
// CHECK-REMARK: {{.*}}:12:18: remark: Stores SLP vectorized with cost -2 and with tree size 4