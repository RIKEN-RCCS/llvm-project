// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp -fno-unroll-loops %s -g -S -o - | FileCheck %s --check-prefix=ASM
// ASM:		.loc	{{[0-9]+}} 13 19 is_stmt 0               // clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc003.c:13:19 ldist(2/3)
// ASM: 	ldur	s0, [x5]

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
