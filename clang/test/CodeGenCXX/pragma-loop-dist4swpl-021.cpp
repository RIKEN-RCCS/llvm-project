// RUN: not %clang_cc1 -triple aarch64-unknown-hurd-gnu -S -o /dev/null -target-cpu a64fx %s 2>&1 | FileCheck -check-prefix CHECK-INVALID %s
// CHECK-INVALID: error: missing argument; expected an integer value

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
#define F_1 "clang loop distribute4swpl_freg(1)"
#define F_2 "clang loop distribute4swpl_freg(16)"
#define F_3 "clang loop distribute4swpl_freg(999)"
#define F_4 "clang loop distribute4swpl_freg(0)"
#define F_5 "clang loop distribute4swpl_freg()"
#define F_6 "clang loop distribute4swpl_freg(enable)"
#define G_1 "clang loop distribute4swpl_ireg(1)"
#define G_2 "clang loop distribute4swpl_ireg(16)"
#define G_3 "clang loop distribute4swpl_ireg(999)"
#define G_4 "clang loop distribute4swpl_ireg(0)"
#define G_5 "clang loop distribute4swpl_ireg()"
#define G_6 "clang loop distribute4swpl_ireg(enable)"

#define N 1000
extern int printf(const char*,...);

void test_1(void) {
	int i;
    int x[N], y[N], z[N];
	int sum=0;
P(E_1)
P(G_5)
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
    }
    sum = z[0];
	printf("%d\n", sum);
}
