// RUN: not %clang_cc1 -triple aarch64-unknown-hurd-gnu -S -o /dev/null -target-cpu a64fx %s 2>&1 | FileCheck -check-prefix CHECK-INVALID %s
// CHECK-INVALID: error: missing argument; expected 'enable' or 'disable'

#define P(a) _Pragma(a)
#define A_1 "clang loop vectorize(enable)"
#define A_2 "clang loop vectorize(disable)"
#define B_1 "clang loop unroll(enable)"
#define B_2 "clang loop unroll(disable)"
#define C_1 "clang loop distribute(enable)"
#define C_2 "clang loop distribute(disable)"
#define D_1 "clang loop pipeline(enable)"
#define D_2 "clang loop pipeline(disable)"
#define E_1 "clang loop distribute4swpl(enable)"
#define E_2 "clang loop distribute4swpl(disable)"
#define E_3 "clang loop distribute4swpl()"
#define E_4 "clang loop distribute4swpl(2)"


#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
P(E_3)
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}
