// RUN: %clang_cc1 -verify -triple aarch64-unknown-hurd-gnu -Ofast -emit-llvm -target-cpu a64fx -o - %s 

#define P(a) _Pragma(a)
#define A_1 "clang loop vectorize(enable)"
#define A_2 "clang loop vectorize(disable)"
#define B_1 "clang loop unroll(enable)"
#define B_2 "clang loop unroll(disable)"
#define C_1 "clang loop distribute(enable)"
#define C_2 "clang loop distribute(disable)"
#define D_1 "clang loop pipeline(enable)"
#define D_2 "clang loop pipeline(disable)"
#define E_1 "clang loop pipeline_nodep(enable)"
#define E_2 "clang loop pipeline_nodep(disable)"
#define E_3 "clang loop pipeline_nodep()"


#define n 10000

void foo (float *A, float *B, float *C, float *D, float *E) {
    int i;
	int sum=0;
P(A_2)
P(C_1)
P(D_2)
/* expected-error {{incompatible directives 'pipeline(disable)' and 'pipeline_nodep(enable)'}} */ P(E_1)
    for (i = 0; i < n; i++) {
        A[i + 1] = A[i] * B[i];  // line 8
        C[i] = D[i] * E[i];      // line 9
    }
}