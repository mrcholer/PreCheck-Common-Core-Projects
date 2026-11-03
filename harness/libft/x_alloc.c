/* Extended suite: itoa + allocating string functions. */
#include "ref.h"

/* ================================================================ itoa */
static void itoa_check(int v)
{
	char exp[32];

	snprintf(exp, sizeof(exp), "%d", v);
	CALLING("ft_itoa(%d)", v);
	char *r = ft_itoa(v);
	EXPECT_TRUE(r != NULL, pt_fmt("\"%s\"", exp), "NULL", "ft_itoa(%d)", v);
	EXPECT_STR(exp, r, "ft_itoa(%d)", v);
	free(r);
}

TEST("ft_itoa", "convert", "exhaustive: every integer from -100000 to 100000", T_SPEC)
{
	HINT("handle 0, negatives, and sizes that change at powers of ten");
	for (int v = -100000; v <= 100000; v++)
		itoa_check(v);
}

TEST("ft_itoa", "convert", "neighbourhood of every power of ten (+-3), both signs, and INT_MIN..INT_MIN+2000 / INT_MAX-2000..INT_MAX", T_SPEC)
{
	long p = 1;

	for (int k = 0; k <= 9; k++, p *= 10)
		for (long d = -3; d <= 3; d++)
		{
			if (p + d <= INT_MAX && p + d >= INT_MIN)
				itoa_check((int)(p + d));
			if (-p + d >= INT_MIN && -p + d <= INT_MAX)
				itoa_check((int)(-p + d));
		}
	for (int d = 0; d <= 2000; d++)
	{
		itoa_check(INT_MIN + d);
		itoa_check(INT_MAX - d);
	}
}

TEST("ft_itoa", "convert", "fuzz: 100000 random integers of every magnitude (1 to 10 digits)", T_SPEC)
{
	for (int t = 0; t < 100000; t++)
		itoa_check((int)pt_rand() >> (pt_rand() % 32));
}

static int g_itoa_v;
static void scen_itoa_x(void)
{
	CALLING("ft_itoa(%d)", g_itoa_v);
	ARM();
	char *r = ft_itoa(g_itoa_v);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_itoa(%d) with malloc failing", g_itoa_v);
		return ;
	}
	char exp[32];
	snprintf(exp, sizeof(exp), "%d", g_itoa_v);
	EXPECT_STR(exp, r, "ft_itoa(%d)", g_itoa_v);
	free(r);
}
TEST("ft_itoa", "convert", "malloc failure sweep over 0, -1, 7, 10, -10, INT_MIN, INT_MAX [inject]", T_SPEC)
{
	static const int vs[] = {0, -1, 7, 10, -10, INT_MIN, INT_MAX};

	for (size_t k = 0; k < sizeof(vs) / sizeof(vs[0]); k++)
	{
		g_itoa_v = vs[k];
		pt_inject_sweep(scen_itoa_x);
	}
}

/* ================================================================ substr */
static void ref_substr(const char *s, unsigned start, size_t len, char *out)
{
	size_t sl = strlen(s), n;

	if (start >= sl)
	{
		out[0] = 0;
		return ;
	}
	n = sl - start < len ? sl - start : len;
	memcpy(out, s + start, n);
	out[n] = 0;
}

static void substr_chk(const char *s0, unsigned start, size_t len)
{
	static char exp[5200];
	size_t m = pt_gb_mark();
	char *s = pt_gb_str(s0);
	const char *lens = len == (size_t)-1 ? "SIZE_MAX" : pt_fmt("%zu", len);

	ref_substr(s0, start, len, exp);
	CALLING("ft_substr(<%zu-char s>, %u, %s)", strlen(s0), start, lens);
	char *r = ft_substr(s, start, len);
	EXPECT_TRUE(r != NULL, pt_fmt("\"%s\"", pt_esc(exp)), "NULL", "ft_substr(\"%s\", %u, %s)", pt_esc(s0), start, lens);
	EXPECT_TRUE(!strcmp(r, exp), pt_fmt("\"%s\"", pt_esc(exp)), pt_fmt("\"%s\"", pt_esc(r)), "ft_substr(\"%s\", %u, %s)", pt_esc(s0), start, lens);
	EXPECT_STR(s0, s, "ft_substr(\"%s\", %u, %s) (source modified)", pt_esc(s0), start, lens);
	free(r);
	pt_gb_reset(m);
}

static const size_t g_lens[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, (size_t)-1, (size_t)-2, 1UL << 31, 1UL << 32, (size_t)INT_MAX};

TEST("ft_substr", "alloc", "exhaustive: s of length 1..8, start < strlen, len 0..10 / huge values", T_SPEC)
{
	char s[16];

	HINT("clamp len to strlen(s) - start BEFORE adding 1 (len + 1 overflows for SIZE_MAX)");
	for (size_t sl = 1; sl <= 8; sl++)
	{
		for (size_t i = 0; i < sl; i++)
			s[i] = 'a' + i;
		s[sl] = 0;
		for (unsigned st = 0; st < sl; st++)
			for (size_t k = 0; k < sizeof(g_lens) / sizeof(g_lens[0]); k++)
				substr_chk(s, st, g_lens[k]);
	}
}

TEST("ft_substr", "alloc", "exhaustive: start >= strlen(s) (incl. UINT_MAX, INT_MAX) returns an allocated empty string", T_CONV)
{
	static const unsigned sts[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, INT_MAX, 0xFFFFFFFEu, 0xFFFFFFFFu};
	char s[16];

	HINT("the subject only lists NULL for allocation failure; returning \"\" is the safe reading (some evaluators accept NULL)");
	for (size_t sl = 0; sl <= 5; sl++)
	{
		for (size_t i = 0; i < sl; i++)
			s[i] = 'a' + i;
		s[sl] = 0;
		for (size_t k = 0; k < sizeof(sts) / sizeof(sts[0]); k++)
			if (sts[k] >= sl)
				for (size_t j = 0; j < sizeof(g_lens) / sizeof(g_lens[0]); j++)
					substr_chk(s, sts[k], g_lens[j]);
	}
}

TEST("ft_substr", "alloc", "fuzz: 6000 random strings (len 0..2000) with random start/len biased to the edges", T_SPEC)
{
	static char s[2100];

	for (int t = 0; t < 6000; t++)
	{
		size_t sl = 1 + pt_rand() % 2000, len;
		unsigned st;
		rf_rbytes(s, sl);
		s[sl] = 0;
		st = pt_rand() % sl;
		if (t % 5 == 0) st = (unsigned)(sl - 1);
		switch (pt_rand() % 5)
		{
			case 0: len = 0; break ;
			case 1: len = (size_t)-1; break ;
			case 2: len = sl - st; break ;
			case 3: len = sl - st + 1 + pt_rand() % 5; break ;
			default: len = pt_rand() % (sl + 3);
		}
		substr_chk(s, st, len);
	}
}

static const char *g_sub_s;
static unsigned g_sub_st;
static size_t g_sub_len;
static void scen_substr_x(void)
{
	char *s = pt_gb_str(g_sub_s);
	static char exp[64];

	ref_substr(g_sub_s, g_sub_st, g_sub_len, exp);
	CALLING("ft_substr(\"%s\", %u, %zu)", g_sub_s, g_sub_st, g_sub_len);
	ARM();
	char *r = ft_substr(s, g_sub_st, g_sub_len);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_substr(\"%s\", %u, %zu) with malloc failing", g_sub_s, g_sub_st, g_sub_len);
		return ;
	}
	EXPECT_STR(exp, r, "ft_substr(\"%s\", %u, %zu)", g_sub_s, g_sub_st, g_sub_len);
	free(r);
}
TEST("ft_substr", "alloc", "malloc failure sweep over several (s, start, len) [inject]", T_SPEC)
{
	static const struct { const char *s; unsigned st; size_t len; } c[] = {{"hello world", 6, 5}, {"hello", 0, 100}, {"abc", 1, 0}, {"x", 0, 1}, {"hello", 2, (size_t)-1}};

	for (size_t k = 0; k < 5; k++)
	{
		g_sub_s = c[k].s; g_sub_st = c[k].st; g_sub_len = c[k].len;
		pt_inject_sweep(scen_substr_x);
	}
}

/* ================================================================ strjoin */
static void join_chk(const char *a0, size_t la, const char *b0, size_t lb)
{
	static char exp[7000];
	size_t m = pt_gb_mark();
	char *a = pt_gb_mem(a0, la + 1), *b = pt_gb_mem(b0, lb + 1);

	memcpy(exp, a0, la);
	memcpy(exp + la, b0, lb + 1);
	CALLING("ft_strjoin(<%zu bytes>, <%zu bytes>)", la, lb);
	char *r = ft_strjoin(a, b);
	EXPECT_TRUE(r != NULL, "the concatenation", "NULL", "ft_strjoin(<%zu bytes>, <%zu bytes>)", la, lb);
	EXPECT_INT(la + lb, strlen(r), "strlen(ft_strjoin(<%zu bytes>, <%zu bytes>))", la, lb);
	EXPECT_MEM(exp, r, la + lb + 1, "ft_strjoin(\"%s\", \"%s\")", pt_esc(a0), pt_esc(b0));
	EXPECT_MEM(a0, a, la + 1, "ft_strjoin(<%zu bytes>, ..) (s1 modified)", la);
	EXPECT_MEM(b0, b, lb + 1, "ft_strjoin(.., <%zu bytes>) (s2 modified)", lb);
	free(r);
	pt_gb_reset(m);
}

TEST("ft_strjoin", "alloc", "exhaustive: every pair of lengths 0..8 x 0..8", T_SPEC)
{
	char a[16], b[16];

	HINT("always malloc a fresh string, even when one side is empty");
	for (size_t la = 0; la <= 8; la++)
		for (size_t lb = 0; lb <= 8; lb++)
		{
			for (size_t i = 0; i < la; i++) a[i] = 'a' + i;
			for (size_t i = 0; i < lb; i++) b[i] = 'A' + i;
			a[la] = b[lb] = 0;
			join_chk(a, la, b, lb);
		}
}

TEST("ft_strjoin", "alloc", "fuzz: 4000 random pairs (len 0..3000, bytes 1..255)", T_SPEC)
{
	static char a[3100], b[3100];

	for (int t = 0; t < 4000; t++)
	{
		size_t la = pt_rand() % 3001, lb = pt_rand() % 3001;
		rf_rbytes(a, la); rf_rbytes(b, lb);
		a[la] = b[lb] = 0;
		join_chk(a, la, b, lb);
	}
}

static const char *g_ja, *g_jb;
static void scen_join_x(void)
{
	char *a = pt_gb_str(g_ja), *b = pt_gb_str(g_jb);
	char exp[64];

	snprintf(exp, sizeof(exp), "%s%s", g_ja, g_jb);
	CALLING("ft_strjoin(\"%s\", \"%s\")", g_ja, g_jb);
	ARM();
	char *r = ft_strjoin(a, b);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strjoin(\"%s\", \"%s\") with malloc failing", g_ja, g_jb);
		return ;
	}
	EXPECT_STR(exp, r, "ft_strjoin(\"%s\", \"%s\")", g_ja, g_jb);
	free(r);
}
TEST("ft_strjoin", "alloc", "malloc failure sweep over (\"\",\"\") (\"a\",\"\") (\"\",\"b\") (\"foo\",\"bar\") [inject]", T_SPEC)
{
	static const char *c[][2] = {{"", ""}, {"a", ""}, {"", "b"}, {"foo", "bar"}};

	for (size_t k = 0; k < 4; k++)
	{
		g_ja = c[k][0]; g_jb = c[k][1];
		pt_inject_sweep(scen_join_x);
	}
}

/* ================================================================ strtrim */
static void ref_trim(const char *s, const char *set, char *out)
{
	size_t st = 0, en = strlen(s);

	while (s[st] && strchr(set, s[st]))
		st++;
	while (en > st && strchr(set, s[en - 1]))
		en--;
	memcpy(out, s + st, en - st);
	out[en - st] = 0;
}

static void trim_chk(const char *s0, const char *set0)
{
	static char exp[2200];
	size_t m = pt_gb_mark();
	char *s = pt_gb_str(s0), *set = pt_gb_str(set0);

	ref_trim(s0, set0, exp);
	CALLING("ft_strtrim(\"%s\", \"%s\")", pt_esc(s0), pt_esc(set0));
	char *r = ft_strtrim(s, set);
	EXPECT_TRUE(r != NULL, pt_fmt("\"%s\"", pt_esc(exp)), "NULL", "ft_strtrim(\"%s\", \"%s\")", pt_esc(s0), pt_esc(set0));
	EXPECT_TRUE(!strcmp(r, exp), pt_fmt("\"%s\"", pt_esc(exp)), pt_fmt("\"%s\"", pt_esc(r)), "ft_strtrim(\"%s\", \"%s\")", pt_esc(s0), pt_esc(set0));
	EXPECT_STR(s0, s, "ft_strtrim(\"%s\", \"%s\") (s1 modified)", pt_esc(s0), pt_esc(set0));
	EXPECT_STR(set0, set, "ft_strtrim(\"%s\", \"%s\") (set modified)", pt_esc(s0), pt_esc(set0));
	free(r);
	pt_gb_reset(m);
}

TEST("ft_strtrim", "alloc", "exhaustive: all s1 over {a,b,c} (len 0..6) x all sets over {a,b,c} (len 0..3, duplicates included)", T_SPEC)
{
	char s[16], set[8];

	HINT("remove set characters only from the two ends; the result is a fresh allocation");
	for (size_t sl = 0; sl <= 6; sl++)
		for (size_t i = 0; i < rf_pow(3, sl); i++)
		{
			rf_gen(s, sl, i, "abc");
			for (size_t tl = 0; tl <= 3; tl++)
				for (size_t j = 0; j < rf_pow(3, tl); j++)
				{
					rf_gen(set, tl, j, "abc");
					trim_chk(s, set);
				}
		}
}

TEST("ft_strtrim", "alloc", "fuzz: 6000 random strings/sets (bytes >= 0x80, spaces, newlines), long runs of trimmed chars", T_SPEC)
{
	static char s[2100], set[16];
	static const char al[] = " \t\n\x80\xff" "xyz";

	for (int t = 0; t < 6000; t++)
	{
		size_t sl = pt_rand() % 2000, tl = pt_rand() % 6, lead = pt_rand() % 50, tail = pt_rand() % 50;
		for (size_t i = 0; i < tl; i++)
			set[i] = al[pt_rand() % 7];
		set[tl] = 0;
		for (size_t i = 0; i < sl; i++)
			s[i] = al[pt_rand() % 7];
		for (size_t i = 0; i < lead && i < sl; i++)
			s[i] = tl ? set[pt_rand() % tl] : 'x';
		for (size_t i = 0; i < tail && i < sl; i++)
			s[sl - 1 - i] = tl ? set[pt_rand() % tl] : 'x';
		s[sl] = 0;
		trim_chk(s, set);
	}
}

static const char *g_ts, *g_tset;
static void scen_trim_x(void)
{
	char *s = pt_gb_str(g_ts), *set = pt_gb_str(g_tset), exp[64];

	ref_trim(g_ts, g_tset, exp);
	CALLING("ft_strtrim(\"%s\", \"%s\")", g_ts, g_tset);
	ARM();
	char *r = ft_strtrim(s, set);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strtrim(\"%s\", \"%s\") with malloc failing", g_ts, g_tset);
		return ;
	}
	EXPECT_STR(exp, r, "ft_strtrim(\"%s\", \"%s\")", g_ts, g_tset);
	free(r);
}
TEST("ft_strtrim", "alloc", "malloc failure sweep over several (s1, set) [inject]", T_SPEC)
{
	static const char *c[][2] = {{"  hi  ", " "}, {"hello", ""}, {"", "a"}, {"aaaa", "a"}, {"xxhixx", "x"}};

	for (size_t k = 0; k < 5; k++)
	{
		g_ts = c[k][0]; g_tset = c[k][1];
		pt_inject_sweep(scen_trim_x);
	}
}

/* ================================================================ split */
static void tokens_chk(const char *s0, char c, char **r, const char *call)
{
	static t_tok t[65536];
	size_t cnt = rf_tokens(s0, c, t, 65536), i;

	EXPECT_TRUE(r != NULL, "a NULL-terminated array", "NULL", "%s", call);
	for (i = 0; i < cnt; i++)
	{
		EXPECT_TRUE(r[i] != NULL, pt_fmt("%zu strings", cnt), pt_fmt("array ends (NULL) after %zu string(s)", i), "%s", call);
		EXPECT_TRUE(strlen(r[i]) == t[i].n && !memcmp(r[i], t[i].p, t[i].n), pt_fmt("token %zu = \"%s\"", i, pt_escn(t[i].p, t[i].n)),
			pt_fmt("token %zu = \"%s\"", i, pt_esc(r[i])), "%s", call);
	}
	EXPECT_TRUE(r[cnt] == NULL, "NULL terminator after the last string", "extra non-NULL element", "%s", call);
	for (i = 0; i < cnt; i++)
		free(r[i]);
	free(r);
}

static void split_chk(const char *s0, char c)
{
	size_t m = pt_gb_mark();
	char *s = pt_gb_str(s0);
	const char *call = pt_fmt("ft_split(\"%s\", %s)", pt_esc(s0), pt_fmt(c >= 32 && c < 127 ? "'%c'" : "0x%02x", (unsigned char)c));

	CALLING("%s", call);
	char **r = ft_split(s, c);
	tokens_chk(s0, c, r, call);
	EXPECT_STR(s0, s, "%s (source modified)", call);
	pt_gb_reset(m);
}

TEST("ft_split", "alloc", "exhaustive: all strings over {',','a','b'} up to length 8 split on ','", T_SPEC)
{
	char s[16];

	HINT("leading/trailing/consecutive delimiters never produce empty strings; empty input gives just the NULL terminator");
	for (size_t len = 0; len <= 8; len++)
		for (size_t i = 0; i < rf_pow(3, len); i++)
		{
			rf_gen(s, len, i, ",ab");
			split_chk(s, ',');
		}
}

TEST("ft_split", "alloc", "exhaustive: all strings over {' ','x'} up to length 12 split on ' ' and on 'x'", T_SPEC)
{
	char s[16];

	for (size_t len = 0; len <= 12; len++)
		for (size_t i = 0; i < rf_pow(2, len); i++)
		{
			rf_gen(s, len, i, " x");
			split_chk(s, ' ');
			split_chk(s, 'x');
		}
}

TEST("ft_split", "alloc", "fuzz: 1500 random strings (len up to 400) with delimiters of every kind (0x80, 0xFF, tab, newline, letters)", T_SPEC)
{
	static char s[500];
	static const char dl[] = {' ', '\t', '\n', ',', 'a', (char)0x80, (char)0xFF, '#'};

	for (int t = 0; t < 1500; t++)
	{
		size_t n = pt_rand() % 400;
		char c = dl[pt_rand() % 8];
		for (size_t i = 0; i < n; i++)
			s[i] = (pt_rand() % 3 == 0) ? c : (char)(1 + pt_rand() % 255);
		s[n] = 0;
		split_chk(s, c);
	}
}

TEST("ft_split", "alloc", "one huge token (100000 chars) and 50000 one-char tokens", T_SPEC)
{
	char *big = pt_gb_alloc(100001);
	char **r;

	memset(big, 'w', 100000);
	big[100000] = 0;
	CALLING("ft_split(<100000-char word>, ' ')");
	r = ft_split(big, ' ');
	tokens_chk(big, ' ', r, "ft_split(<100000-char word>, ' ')");
	char *many = pt_gb_alloc(100000);
	for (size_t i = 0; i < 99999; i++)
		many[i] = (i % 2) ? ' ' : 'z';
	many[99999] = 0;
	CALLING("ft_split(\"z z z ...\" (50000 words), ' ')");
	r = ft_split(many, ' ');
	tokens_chk(many, ' ', r, "ft_split(<50000 one-char words>, ' ')");
}

static const char *g_sp_s;
static char g_sp_c;
static void scen_split_x(void)
{
	char *s = pt_gb_str(g_sp_s);
	const char *call = pt_fmt("ft_split(\"%s\", '%c')", pt_esc(g_sp_s), g_sp_c);

	CALLING("%s", call);
	ARM();
	char **r = ft_split(s, g_sp_c);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when an allocation fails", "a non-NULL array", "%s with malloc failing", call);
		return ;
	}
	tokens_chk(g_sp_s, g_sp_c, r, call);
}
TEST("ft_split", "alloc", "malloc failure sweep over 14 strings (empty, delimiters only, edges, many words) [inject]", T_SPEC)
{
	static const char *c[] = {"", " ", "   ", "a", "a b", " a b ", "a  b  c", "abc def ghi jkl", "  x", "x  ", "one two three four five six",
		"a b c d e f g h i j", ",,,x,,y,,", "word", NULL};

	HINT("when a word allocation fails, free the words already created AND the array before returning NULL");
	for (int k = 0; c[k]; k++)
	{
		g_sp_s = c[k];
		g_sp_c = (k == 12) ? ',' : ' ';
		pt_inject_sweep(scen_split_x);
	}
}

/* ================================================================ strmapi / striteri */
static int		g_calls, g_mode;
static unsigned	g_idx[4096];
static char		g_chr[4096];
static char		*g_ptr[4096];

static char expc(int mode, unsigned i, char c)
{
	switch (mode)
	{
		case 0: return ((char)(c + 1));
		case 1: return ((char)('0' + i % 10));
		case 2: return ((i % 7 == 2) ? 0 : c);
		default: return (c);
	}
}
static char f_mode(unsigned i, char c)
{
	if (g_calls < 4096) { g_idx[g_calls] = i; g_chr[g_calls] = c; }
	g_calls++;
	return (expc(g_mode, i, c));
}

TEST("ft_strmapi", "alloc", "fuzz: 3000 random strings x 4 mapping functions (incl. one returning '\\0' mid-string): calls, order, bytes", T_SPEC)
{
	static char s[2100], exp[2100];

	HINT("map every character even if f returns '\\0'; f gets the ORIGINAL character and its index, in order, exactly once");
	for (int t = 0; t < 3000; t++)
	{
		size_t n = pt_rand() % 2000, m = pt_gb_mark();
		g_mode = t % 4;
		rf_rbytes(s, n);
		s[n] = 0;
		for (size_t i = 0; i < n; i++)
			exp[i] = expc(g_mode, (unsigned)i, s[i]);
		exp[n] = 0;
		char *gs = pt_gb_str(s);
		g_calls = 0;
		CALLING("ft_strmapi(<%zu-char string>, f[mode %d])", n, g_mode);
		char *r = ft_strmapi(gs, f_mode);
		EXPECT_TRUE(r != NULL, "the mapped string", "NULL", "ft_strmapi(<%zu-char string>, f[mode %d])", n, g_mode);
		EXPECT_INT(n, g_calls, "ft_strmapi(<%zu-char string>, f[mode %d]) number of calls to f", n, g_mode);
		for (size_t i = 0; i < n; i++)
		{
			EXPECT_INT(i, g_idx[i], "ft_strmapi mode %d: index passed at call #%zu", g_mode, i);
			EXPECT_INT((unsigned char)s[i], (unsigned char)g_chr[i], "ft_strmapi mode %d: character passed at call #%zu", g_mode, i);
		}
		EXPECT_MEM(exp, r, n + 1, "ft_strmapi(<%zu-char string>, f[mode %d]) resulting bytes (incl. the final NUL at index %zu)", n, g_mode, n);
		EXPECT_MEM(s, gs, n + 1, "ft_strmapi(<%zu-char string>, ..) (source modified)", n);
		free(r);
		pt_gb_reset(m);
	}
}

static const char *g_ms;
static void scen_mapi_x(void)
{
	char *s = pt_gb_str(g_ms);

	CALLING("ft_strmapi(\"%s\", f)", g_ms);
	g_mode = 0;
	ARM();
	char *r = ft_strmapi(s, f_mode);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strmapi(\"%s\", f) with malloc failing", g_ms);
		return ;
	}
	EXPECT_TRUE(r != NULL && strlen(r) == strlen(g_ms), "mapped string", "NULL or wrong length", "ft_strmapi(\"%s\", f)", g_ms);
	free(r);
}
TEST("ft_strmapi", "alloc", "malloc failure sweep over \"\", \"a\", \"hello world\" [inject]", T_SPEC)
{
	static const char *c[] = {"", "a", "hello world"};

	for (int k = 0; k < 3; k++)
	{
		g_ms = c[k];
		pt_inject_sweep(scen_mapi_x);
	}
}

static void f_it(unsigned i, char *c)
{
	if (g_calls < 4096) { g_idx[g_calls] = i; g_ptr[g_calls] = c; }
	g_calls++;
	if (g_mode == 0)
		*c = (char)(*c ^ (char)(i + 1));
	else if (g_mode == 1)
		*c = (char)('A' + i % 26);
}

TEST("ft_striteri", "alloc", "fuzz: 2000 random strings x 3 modes: f gets (index, &s[index]) once each, in order; edits stick; canaries intact", T_SPEC)
{
	static char s[1100], exp[1100];

	for (int t = 0; t < 2000; t++)
	{
		size_t n = pt_rand() % 1000;
		g_mode = t % 3;
		rf_rbytes(s, n);
		s[n] = 0;
		t_pt_cbuf b = pt_cb_new(n + 1, pt_rand() % 8, 0);
		memcpy(b.p, s, n + 1);
		for (size_t i = 0; i <= n; i++)
			exp[i] = s[i];
		for (size_t i = 0; i < n; i++)
			exp[i] = (g_mode == 0) ? (char)(s[i] ^ (char)(i + 1)) : (g_mode == 1) ? (char)('A' + i % 26) : s[i];
		g_calls = 0;
		CALLING("ft_striteri(<%zu-char string>, f[mode %d])", n, g_mode);
		ft_striteri((char *)b.p, f_it);
		EXPECT_INT(n, g_calls, "ft_striteri(<%zu-char string>, f[mode %d]) number of calls to f", n, g_mode);
		for (size_t i = 0; i < n; i++)
		{
			EXPECT_INT(i, g_idx[i], "ft_striteri mode %d: index passed at call #%zu", g_mode, i);
			EXPECT_PTR(b.p + i, g_ptr[i], "ft_striteri mode %d: pointer passed at call #%zu (must be &s[%zu])", g_mode, i, i);
		}
		EXPECT_MEM(exp, b.p, n + 1, "ft_striteri(<%zu-char string>, f[mode %d]) resulting string", n, g_mode);
		pt_cb_check(&b, pt_fmt("ft_striteri(<%zu-char string>, f)", n));
	}
}
