// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp -fno-unroll-loops %s -c -o /dev/null -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc005.c:30:3: ldist(1/3): remark: distributed loop. num of distributied is 3. [-Rpass=loop-distribute4swpl]
// RPASS: ldist-loc005.c:30:3: ldist(1/3): remark: vectorized loop (vectorization width: vscale x 4, interleaved count: 1) [-Rpass=loop-vectorize]
// RPASS: ldist-loc005.c:36:14: ldist(1/3): remark: Recipe with invalid costs prevented vectorization at VF=(vscale x 1): load [-Rpass-analysis=loop-vectorize]
// RPASS:    36 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RPASS: ldist-loc005.c:36:14: ldist(1/3): remark: Cannot SLP vectorize list: vectorization was impossible with available vectorization factors [-Rpass-missed=slp-vectorizer]
// RPASS:    36 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RPASS: ldist-loc005.c:30:3: ldist(1/3): remark: distributed loop: software pipelining (IPC: 2.00, ITR: 15, MVE: 8, II: 3, Stage: 8, (VReg Fp: 13/32, Int: 22/29, Pred: 2/8)), SRA(PReg Fp: 11/32, Int: 13/29, Pred: 1/8) [-Rpass=aarch64-swpipeliner]

// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -mllvm -disable-distribute4swpl-loc -fswp -fno-unroll-loops %s -c -o /dev/null -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc005.c:30:3: remark: distributed loop. num of distributied is 3. [-Rpass=loop-distribute4swpl]
// RPASS: ldist-loc005.c:30:3: remark: vectorized loop (vectorization width: vscale x 4, interleaved count: 1) [-Rpass=loop-vectorize]
// RPASS: ldist-loc005.c:36:14: remark: Recipe with invalid costs prevented vectorization at VF=(vscale x 1): load [-Rpass-analysis=loop-vectorize]
// RPASS:    36 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RPASS: ldist-loc005.c:36:14: remark: Cannot SLP vectorize list: vectorization was impossible with available vectorization factors [-Rpass-missed=slp-vectorizer]
// RPASS:    36 |     D[i]   = H[i] * I[i];
// RPASS:       |              ^
// RPASS: ldist-loc005.c:30:3: remark: distributed loop: 1 of 3 software pipelining (IPC: 2.00, ITR: 15, MVE: 8, II: 3, Stage: 8, (VReg Fp: 13/32, Int: 22/29, Pred: 2/8)), SRA(PReg Fp: 11/32, Int: 13/29, Pred: 1/8) [-Rpass=aarch64-swpipeliner]

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
