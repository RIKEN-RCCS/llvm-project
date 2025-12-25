// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -mllvm -disable-distribute4swpl-loc -fswp -fno-unroll-loops %s -c -o /dev/null -Rpass="aarch64-sw|loop-dist|loop-vectorize|hardware-loops" -Rpass-missed="aarch64-sw|loop-dist|loop-vectorize|hardware-loops" -Rpass-analysis="loop-dist" 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 1 of 3 ireg=6, freg=3, numInst=12 [-Rpass-analysis=loop-distribute4swpl]
// RPASS:    29 |   for (int i = 0; i < n; i++) {
// RPASS:       |   ^
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 2 of 3 ireg=7, freg=3, numInst=11 [-Rpass-analysis=loop-distribute4swpl]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 3 of 3 ireg=7, freg=3, numInst=11 [-Rpass-analysis=loop-distribute4swpl]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop. num of distributied is 3. [-Rpass=loop-distribute4swpl]
// RPASS: ldist-loc007.c:29:3: remark: loop not vectorized [-Rpass-missed=loop-vectorize]
// RPASS: ldist-loc007.c:29:3: remark: vectorized loop (vectorization width: vscale x 4, interleaved count: 1) [-Rpass=loop-vectorize]
// RPASS: ldist-loc007.c:29:3: remark: vectorized loop (vectorization width: vscale x 4, interleaved count: 1) [-Rpass=loop-vectorize]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 3 of 3 hardware-loop not created: it's not profitable to create a hardware-loop [-Rpass-missed=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 3 of 3 hardware-loop not created: it's not profitable to create a hardware-loop [-Rpass-missed=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 3 of 3 hardware-loop created [-Rpass=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 1 of 3 hardware-loop not created: it's not profitable to create a hardware-loop [-Rpass-missed=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 1 of 3 hardware-loop not created: it's not profitable to create a hardware-loop [-Rpass-missed=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 1 of 3 hardware-loop created [-Rpass=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 2 of 3 hardware-loop created [-Rpass=hardware-loops]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 3 of 3 software pipelining (IPC: 2.00, ITR: 15, MVE: 8, II: 3, Stage: 8, (VReg Fp: 13/32, Int: 22/29, Pred: 2/8)), SRA(PReg Fp: 11/32, Int: 13/29, Pred: 1/8) [-Rpass=aarch64-swpipeliner]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 1 of 3 software pipelining (IPC: 2.00, ITR: 15, MVE: 8, II: 3, Stage: 8, (VReg Fp: 13/32, Int: 22/29, Pred: 2/8)), SRA(PReg Fp: 11/32, Int: 13/29, Pred: 1/8) [-Rpass=aarch64-swpipeliner]
// RPASS: ldist-loc007.c:29:3: remark: distributed loop: 2 of 3 software pipelining (IPC: 3.00, ITR: 11, MVE: 4, II: 3, Stage: 8, (VReg Fp: 10/32, Int: 7/29, Pred: 1/8)), SRA(PReg Fp: 8/32, Int: 5/29, Pred: 0/8) [-Rpass=aarch64-swpipeliner]

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
