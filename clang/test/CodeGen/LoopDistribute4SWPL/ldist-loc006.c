// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -fswp -fno-unroll-loops %s -c -o /dev/null -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc006.c:17:3: remark: loop not vectorized [-Rpass-missed=loop-vectorize]
// RPASS:    17 |   for (int i = 0; i < n; i++) {
// RPASS:       |   ^
// RPASS: ldist-loc006.c:23:14: remark: Cannot SLP vectorize list: vectorization was impossible with available vectorization factors [-Rpass-missed=slp-vectorizer]
// RPASS:    23 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RPASS: ldist-loc006.c:17:3: remark: software pipelining (IPC: 2.14, ITR: 7, MVE: 4, II: 7, Stage: 4, (VReg Fp: 15/32, Int: 19/29, Pred: 1/8)), SRA(PReg Fp: 14/32, Int: 14/29, Pred: 0/8) [-Rpass=aarch64-swpipeliner]

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
