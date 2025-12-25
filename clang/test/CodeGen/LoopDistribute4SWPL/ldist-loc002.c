// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp -fno-unroll-loops %s -c -o %t -fsave-optimization-record=yaml -foptimization-record-file=%t.opt.yaml | FileCheck %s --check-prefix=YAML < %t.opt.yaml
// YAML: Name:            Distribute
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML: Line: 19, Column: 3, ldist: '3/3' }
// YAML: Name:            Vectorized
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML: Line: 19, Column: 3, ldist: '1/3' }
// YAML: Name:            SoftwarePipelined
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML: Line: 19, Column: 3, ldist: '1/3' }

float target(int n,
            float *restrict A, float *restrict B, float *restrict C,
            float *restrict D, float *restrict E, float *restrict F,
            float *restrict G, float *restrict H, float *restrict I)
{
  float tmp1 = 0, tmp2 = 0;

  for (int i = 0; i < n; i++) {
    B[i] = A[i] * E[i];
    A[i+1] = B[i];
    tmp1   = A[i+1];
    C[i]   = F[i] * G[i];
    tmp2   = C[i];
    D[i]   = H[i] * I[i];
  }
  return tmp1 + tmp2;
}
