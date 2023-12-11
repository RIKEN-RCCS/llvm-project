// RUN: %clang_cc1 -verify -triple aarch64-unknown-hurd-gnu -Ofast -emit-llvm -target-cpu a64fx -o - %s 

#define P(a) _Pragma(a)
#define A_1 "clang loop vectorize(enable)"
#define A_2 "clang loop vectorize(disable)"
#define B_1 "clang loop unroll(enable)"
#define B_2 "clang loop unroll(disable)"
#define C_1 "clang loop distribute(enable)"
#define C_2 "clang loop distribute(disable)"
#define D_1 "clang loop pipeline(enable)"
#define D_2 "clang loop pipeline(da)"
#define E_1 "clang loop pipeline_nodep(enable)"
#define E_2 "clang loop pipeline_nodep(disable)"
#define E_3 "clang loop pipeline_nodep()"


#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
/* expected-error {{invalid argument; expected 'enable'}} */P(E_2)
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}