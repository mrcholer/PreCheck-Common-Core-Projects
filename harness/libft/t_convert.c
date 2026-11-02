#include "../core.h"
#include "proto.h"

TEST("ft_atoi", "convert", "signs, whitespace, junk, leading zeros, INT_MIN/INT_MAX (vs libc)", T_SPEC)
{
	static const char *cs[] = {"0", "42", "-42", "+42", " \t\n\v\f\r 42", "--42", "+-42", "-+42", "42abc", "abc", "",
		"-", "+", "  -  5", "0042", "-0", "2147483647", "-2147483648", "   +0", "\t-12 345", " 12", "+++1", "1-2",
		"\x1f""5", "00000000000000000000042", "-00000000000000000000042", "7\n8", NULL};

	HINT("skip isspace chars, accept ONE optional sign, then digits; stop at the first non-digit");
	for (int i = 0; cs[i]; i++)
	{
		size_t m = pt_gb_mark();
		char *s = pt_gb_str(cs[i]);
		CALLING("ft_atoi(\"%s\")", pt_esc(s));
		EXPECT_INT(atoi(s), ft_atoi(s), "ft_atoi(\"%s\")", pt_esc(s));
		pt_gb_reset(m);
	}
}

TEST("ft_atoi", "convert", "5000 random in-range integers with random leading whitespace", T_SPEC)
{
	static const char ws[] = " \t\n\v\f\r";
	char buf[64];

	for (int t = 0; t < 5000; t++)
	{
		int v = (int)pt_rand();
		if (t % 3 == 0)
			v %= 1000;
		size_t o = 0, w = pt_rand() % 4;
		while (w--)
			buf[o++] = ws[pt_rand() % 6];
		if (v >= 0 && (pt_rand() & 1))
			buf[o++] = '+';
		snprintf(buf + o, sizeof(buf) - o, "%d", v);
		size_t m = pt_gb_mark();
		char *s = pt_gb_str(buf);
		CALLING("ft_atoi(\"%s\")", pt_esc(s));
		EXPECT_INT(v, ft_atoi(s), "ft_atoi(\"%s\")", pt_esc(s));
		pt_gb_reset(m);
	}
}

TEST("ft_itoa", "convert", "boundary values incl. INT_MIN, INT_MAX, powers of ten", T_SPEC)
{
	static const int vs[] = {0, 1, -1, 9, -9, 10, -10, 11, 99, 100, -100, 101, 999, 1000, 12345, -12345, 2147483647,
		-2147483647, (int)-2147483647 - 1, 1000000000, -1000000000, 2000000000, 1000000001, -999999999};
	char exp[32];

	HINT("handle INT_MIN without negating it as an int; allocate room for sign + digits + NUL");
	for (size_t k = 0; k < sizeof(vs) / sizeof(vs[0]); k++)
	{
		snprintf(exp, sizeof(exp), "%d", vs[k]);
		CALLING("ft_itoa(%d)", vs[k]);
		char *r = ft_itoa(vs[k]);
		EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_itoa(%d)", vs[k]);
		EXPECT_STR(exp, r, "ft_itoa(%d)", vs[k]);
		free(r);
	}
}

TEST("ft_itoa", "convert", "4000 random integers vs snprintf (no '+', no leading zeros)", T_SPEC)
{
	char exp[32];

	for (int t = 0; t < 4000; t++)
	{
		int v = (int)pt_rand() >> (pt_rand() % 31);
		snprintf(exp, sizeof(exp), "%d", v);
		CALLING("ft_itoa(%d)", v);
		char *r = ft_itoa(v);
		EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_itoa(%d)", v);
		EXPECT_STR(exp, r, "ft_itoa(%d)", v);
		free(r);
	}
}

static void scen_itoa(void)
{
	CALLING("ft_itoa(-12345)");
	ARM();
	char *r = ft_itoa(-12345);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_itoa(-12345) with malloc failing");
		return ;
	}
	EXPECT_STR("-12345", r, "ft_itoa(-12345)");
	free(r);
}
TEST("ft_itoa", "convert", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_itoa);
}
