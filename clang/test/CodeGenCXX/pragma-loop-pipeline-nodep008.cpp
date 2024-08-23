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


extern int printf(const char*,...);

void ipeline_disable (int Length, char **Value) {
	int sum=0;
P(B_1)
P(C_2)
P(D_2)
/* expected-error {{incompatible directives 'pipeline(disable)' and 'pipeline_nodep(enable)'}} */ P(E_1)
    for (int i=0; i<Length; i++) {
        sum*=Value[0][0];
        sum+=i;
    }
	printf("%d\n", sum);
}