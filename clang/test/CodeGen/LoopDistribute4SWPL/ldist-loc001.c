// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp %s -c -o /dev/null -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 2>&1 | FileCheck %s --check-prefix=RPASS
// RPASS: ldist-loc001.c:26:3:ldist(3/3) remark: distributed loop. num of distributied is 3. [-Rpass=loop-distribute4swpl]
// RPASS: ldist-loc001.c:26:3:ldist(2/3) remark: loop not vectorized [-Rpass-missed=loop-vectorize]
// RPASS:    26 |   for (int i = 0; i < n; i++) {
// RPASS:       |   ^
// RPASS: ldist-loc001.c:26:3:ldist(3/3) remark: vectorized loop (vectorization width: vscale x {{.*}}, interleaved count: {{.*}}) [-Rpass=loop-vectorize]
// RPASS:    26 |   for (int i = 0; i < n; i++) {
// RPASS:       |   ^
// RPASS: ldist-loc001.c:26:3: remark: List vectorization was possible but not beneficial with cost 0 >= 0 [-Rpass-missed=slp-vectorizer]
// RPASS:    26 |   for (int i = 0; i < n; i++) {
// RPASS:       |   ^
// RPASS: ldist-loc001.c:26:3:ldist(2/3) remark: unrolled loop by a factor of {{.*}} with run-time trip count [-Rpass=loop-unroll]
// RPASS:    26 |   for (int i = 0; i < n; i++) {
// RPASS:       |   ^
// RPASS: ldist-loc001.c:26:3:ldist(2/3) remark: The number of COPY instructions in the kernel loop for software pipelining is 4. [-Rpass-analysis=aarch64-swpipeliner]
// RPASS: ldist-loc001.c:26:3:ldist(2/3) remark: software pipelining


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
