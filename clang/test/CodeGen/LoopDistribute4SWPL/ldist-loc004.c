// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp -fno-vectorize -fno-unroll-loops %s -c -o /dev/null -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc004.c:17:3:ldist(3/3) remark: distributed loop. num of distributied is 3. [-Rpass=loop-distribute4swpl]
// RPASS: ldist-loc004.c:17:3:ldist(2/3) remark: loop not vectorized [-Rpass-missed=loop-vectorize]
// RPASS: ldist-loc004.c:17:3:ldist(1/3) remark: loop not vectorized [-Rpass-missed=loop-vectorize]
// RPASS: ldist-loc004.c:17:3:ldist(3/3) remark: loop not vectorized [-Rpass-missed=loop-vectorize]
// RPASS: ldist-loc004.c:17:3:ldist(3/3) remark: software pipelining (IPC: 2.66, ITR: 10, MVE: 4, II: 3, Stage: 7, (VReg Fp: 13/32, Int: 7/29, Pred: 1/8)), SRA(PReg Fp: 9/32, Int: 5/29, Pred: 0/8) [-Rpass=aarch64-swpipeliner]
// RPASS: ldist-loc004.c:17:3:ldist(1/3) remark: software pipelining (IPC: 2.66, ITR: 10, MVE: 4, II: 3, Stage: 7, (VReg Fp: 13/32, Int: 7/29, Pred: 1/8)), SRA(PReg Fp: 9/32, Int: 5/29, Pred: 0/8) [-Rpass=aarch64-swpipeliner]
// RPASS: ldist-loc004.c:17:3:ldist(2/3) remark: software pipelining (IPC: 3.00, ITR: 11, MVE: 4, II: 3, Stage: 8, (VReg Fp: 10/32, Int: 7/29, Pred: 1/8)), SRA(PReg Fp: 8/32, Int: 5/29, Pred: 0/8) [-Rpass=aarch64-swpipeliner]

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
