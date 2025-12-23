// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp %s -c -o /dev/null -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc003.c:20:3: ldist(2/3): remark: vectorized loop (vectorization width: vscale x 4, interleaved count: 1) [-Rpass=loop-vectorize]
// RPASS: ldist-loc003.c:26:14: ldist(3/3): remark: Recipe with invalid costs prevented vectorization at VF=(vscale x 1): load [-Rpass-analysis=loop-vectorize]
// RPASS:    26 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RPASS: ldist-loc003.c:26:14: ldist(3/3): remark: Cannot SLP vectorize list: vectorization was impossible with available vectorization factors [-Rpass-missed=slp-vectorizer]
// RPASS:    26 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp -fno-unroll-loops %s -g -S -o - | FileCheck %s --check-prefix=ASM
// ASM:		.loc	{{[0-9]+}} 26 14 1                        //  clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc003.c:26:14:ldsit(3/3)
// ASM: 	ld1w	{ z0.s }, p1/z, [x16, x1, lsl #2]

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
