// RUN: %clang --target=aarch64-linux-gnu -mcpu=a64fx -O3 -mllvm -vectorize-slp -mllvm -enable-loop-distribute4swpl -mllvm -distribute4swpl-limit-freg=1 -mllvm -distribute4swpl-limit-ireg=1 -fswp -fno-unroll-loops %s -c -o %t -fsave-optimization-record=yaml -foptimization-record-file=%t.opt.yaml | FileCheck %s --check-prefix=YAML < %t.opt.yaml
// YAML: --- !Passed
// YAML: Pass:            loop-distribute4swpl
// YAML: Name:            Distribute
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML-NEXT: Line: 59, Column: 3, ldist: 1 }
// YAML: Function:        target
// YAML: Args:
// YAML:   - String:          'distributed loop. num of distributied is '
// YAML:   - nDistributed:    '3'
// YAML:   - String:          .

// YAML: --- !Passed
// YAML: Pass:            loop-vectorize
// YAML: Name:            Vectorized
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML-NEXT: Line: 59, Column: 3, ldsit: 1 }
// YAML: Function:        target
// YAML: Args:
// YAML:   - String:          'vectorized '
// YAML:   - String:          ''
// YAML:   - String:          'loop (vectorization width: '
// YAML:   - VectorizationFactor: vscale x 4
// YAML:   - String:          ', interleaved count: '
// YAML:   - InterleaveCount: '1'
// YAML:   - String:          ')'

// YAML: --- !Missed
// YAML: Pass:            slp-vectorizer
// YAML: Name:            NotPossible
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML=NEXT; Line: 63, Column: 14 }
// YAML: Function:        target
// YAML: Function:        target
// YAML: Args:
// YAML:   - String:          'Cannot SLP vectorize list: vectorization was impossible'
// YAML:   - String:          ' with available vectorization factors'

// YAML: --- !Passed
// YAML: Pass:            aarch64-swpipeliner
// YAML: Name:            SoftwarePipelined
// YAML: DebugLoc:        { File: 'clang/test/CodeGen/LoopDistribute4SWPL/ldist-loc002.c',
// YAML-NEXT: Line: 59, Column: 3, ldist 1 }
// YAML: Function:        target
// YAML: Args:
// YAML:   - String:          'distributed loop: '
// YAML:   - NoOfDistributed: '1'
// YAML:   - String:          ' of '
// YAML:   - NumOfDistributed: '3'
// YAML:   - String:          ' '

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
