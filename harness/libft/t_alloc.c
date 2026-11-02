#include "../core.h"
#include "proto.h"

/* ---------------------------------------------------------------- substr */
static void substr_case(const char *s0, unsigned start, size_t len, const char *exp)
{
	size_t	m = pt_gb_mark();
	char	*s = pt_gb_str(s0);
	char	*call = pt_fmt("ft_substr(\"%s\", %u, %s)", pt_esc(s0), start, len == (size_t)-1 ? "SIZE_MAX" : pt_fmt("%zu", len));

	CALLING("%s", call);
	char *r = ft_substr(s, start, len);
	EXPECT_TRUE(r != NULL, pt_fmt("\"%s\"", exp), "NULL", "%s", call);
	EXPECT_STR(exp, r, "%s", call);
	EXPECT_STR(s0, s, "%s (the source string was modified)", call);
	free(r);
	pt_gb_reset(m);
}

TEST("ft_substr", "alloc", "regular substrings, len clamped to the end of the string", T_SPEC)
{
	HINT("allocate min(len, strlen(s) - start) + 1 bytes and NUL-terminate");
	substr_case("hello world", 0, 5, "hello");
	substr_case("hello world", 6, 5, "world");
	substr_case("hello world", 6, 100, "world");
	substr_case("hello world", 0, 0, "");
	substr_case("hello", 4, 1, "o");
	substr_case("hello", 1, 3, "ell");
	substr_case("hello", 0, 5, "hello");
}

TEST("ft_substr", "alloc", "len = SIZE_MAX must not overflow len + 1", T_SPEC)
{
	substr_case("hello", 2, (size_t)-1, "llo");
	substr_case("hello", 0, (size_t)-1, "hello");
}

TEST("ft_substr", "alloc", "start at or beyond the end returns an allocated empty string", T_CONV)
{
	HINT("the subject only lists NULL for allocation failure; returning \"\" is the safe reading (some evaluators accept NULL)");
	substr_case("hello", 5, 3, "");
	substr_case("hello", 10, 3, "");
	substr_case("", 0, 5, "");
	substr_case("", 0, 0, "");
	substr_case("hello", 0xFFFFFFFFu, 5, "");
}

static void scen_substr(void)
{
	char *s = pt_gb_str("hello world");

	CALLING("ft_substr(\"hello world\", 6, 5)");
	ARM();
	char *r = ft_substr(s, 6, 5);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_substr with malloc failing");
		return ;
	}
	EXPECT_STR("world", r, "ft_substr(\"hello world\", 6, 5)");
	free(r);
}
TEST("ft_substr", "alloc", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_substr);
}

/* ---------------------------------------------------------------- strjoin */
static void join_case(const char *a0, const char *b0)
{
	size_t	m = pt_gb_mark();
	char	*a = pt_gb_str(a0), *b = pt_gb_str(b0);
	size_t	la = strlen(a0), lb = strlen(b0);
	char	*exp = malloc(la + lb + 1);
	char	*call = pt_fmt("ft_strjoin(\"%s\", \"%s\")", pt_esc(a0), pt_esc(b0));

	memcpy(exp, a0, la);
	memcpy(exp + la, b0, lb + 1);
	CALLING("%s", call);
	char *r = ft_strjoin(a, b);
	EXPECT_TRUE(r != NULL, "the concatenation", "NULL", "%s", call);
	EXPECT_STR(exp, r, "%s", call);
	EXPECT_STR(a0, a, "%s (s1 was modified)", call);
	EXPECT_STR(b0, b, "%s (s2 was modified)", call);
	free(r);
	free(exp);
	pt_gb_reset(m);
}

TEST("ft_strjoin", "alloc", "empty/non-empty combinations; result is always a NEW block", T_SPEC)
{
	HINT("always malloc a fresh string, even when one side is empty (returning s1 or s2 would be freed twice / invalidly)");
	join_case("", "");
	join_case("a", "");
	join_case("", "b");
	join_case("hello", " world");
	join_case("\xe9\xff", "\x80z");
}

TEST("ft_strjoin", "alloc", "long strings (5000 + 3000 chars)", T_SPEC)
{
	char *a = malloc(5001), *b = malloc(3001);

	memset(a, 'a', 5000); a[5000] = 0;
	memset(b, 'b', 3000); b[3000] = 0;
	join_case(a, b);
	free(a);
	free(b);
}

static void scen_strjoin(void)
{
	char *a = pt_gb_str("foo"), *b = pt_gb_str("bar");

	CALLING("ft_strjoin(\"foo\", \"bar\")");
	ARM();
	char *r = ft_strjoin(a, b);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strjoin with malloc failing");
		return ;
	}
	EXPECT_STR("foobar", r, "ft_strjoin(\"foo\", \"bar\")");
	free(r);
}
TEST("ft_strjoin", "alloc", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_strjoin);
}

/* ---------------------------------------------------------------- strtrim */
static void trim_case(const char *s0, const char *set0, const char *exp)
{
	size_t	m = pt_gb_mark();
	char	*s = pt_gb_str(s0), *set = pt_gb_str(set0);
	char	*call = pt_fmt("ft_strtrim(\"%s\", \"%s\")", pt_esc(s0), pt_esc(set0));

	CALLING("%s", call);
	char *r = ft_strtrim(s, set);
	EXPECT_TRUE(r != NULL, pt_fmt("\"%s\"", exp), "NULL", "%s", call);
	EXPECT_STR(exp, r, "%s", call);
	EXPECT_STR(s0, s, "%s (s1 was modified)", call);
	free(r);
	pt_gb_reset(m);
}

TEST("ft_strtrim", "alloc", "front, back, both, inner occurrences preserved", T_SPEC)
{
	HINT("remove set characters only from the two ends; the result is a fresh allocation");
	trim_case("  hello  ", " ", "hello");
	trim_case("xxhixx", "x", "hi");
	trim_case("abcba", "ab", "c");
	trim_case("  a b  ", " ", "a b");
	trim_case("\t\n x \n\t", " \t\n", "x");
	trim_case("abcxyzcba", "abc", "xyz");
	trim_case("aXbXa", "a", "XbX");
	trim_case("xhello", "x", "hello");
	trim_case("hellox", "x", "hello");
}

TEST("ft_strtrim", "alloc", "nothing to trim, empty set, empty string, everything trimmed", T_SPEC)
{
	trim_case("hello", "", "hello");
	trim_case("hello", "xyz", "hello");
	trim_case("", "abc", "");
	trim_case("", "", "");
	trim_case("aaaa", "a", "");
	trim_case("hello", "helo", "");
}

static void scen_strtrim(void)
{
	char *s = pt_gb_str("  hi  "), *set = pt_gb_str(" ");

	CALLING("ft_strtrim(\"  hi  \", \" \")");
	ARM();
	char *r = ft_strtrim(s, set);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strtrim with malloc failing");
		return ;
	}
	EXPECT_STR("hi", r, "ft_strtrim(\"  hi  \", \" \")");
	free(r);
}
TEST("ft_strtrim", "alloc", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_strtrim);
}

/* ---------------------------------------------------------------- split */
typedef struct { const char *p; size_t n; } t_tok;

static size_t ref_tokens(const char *s, char c, t_tok *t, size_t max)
{
	size_t	k = 0, i = 0, st;

	while (s[i])
	{
		while (s[i] && s[i] == c)
			i++;
		if (!s[i])
			break ;
		st = i;
		while (s[i] && s[i] != c)
			i++;
		if (k < max)
			t[k] = (t_tok){s + st, i - st};
		k++;
	}
	return (k);
}

static void check_tokens(const char *s0, char c, char **r, const char *call)
{
	static t_tok	t[8192];
	size_t			cnt = ref_tokens(s0, c, t, 8192), i;

	EXPECT_TRUE(r != NULL, "a NULL-terminated array", "NULL", "%s", call);
	for (i = 0; i < cnt; i++)
	{
		EXPECT_TRUE(r[i] != NULL, pt_fmt("%zu strings", cnt), pt_fmt("array ends (NULL) after %zu string(s)", i), "%s", call);
		EXPECT_TRUE(strlen(r[i]) == t[i].n && !strncmp(r[i], t[i].p, t[i].n), pt_fmt("token %zu = \"%s\"", i, pt_escn(t[i].p, t[i].n)),
			pt_fmt("token %zu = \"%s\"", i, pt_esc(r[i])), "%s", call);
	}
	EXPECT_TRUE(r[cnt] == NULL, "NULL terminator after the last string", "extra non-NULL element", "%s", call);
	for (i = 0; i < cnt; i++)
		free(r[i]);
	free(r);
}

static void split_case(const char *s0, char c)
{
	size_t	m = pt_gb_mark();
	char	*s = pt_gb_str(s0);
	char	*call = pt_fmt("ft_split(\"%s\", '%c')", pt_esc(s0), c ? c : '0');

	if (!c)
		call = pt_fmt("ft_split(\"%s\", '\\0')", pt_esc(s0));
	CALLING("%s", call);
	char **r = ft_split(s, c);
	check_tokens(s0, c, r, call);
	EXPECT_STR(s0, s, "%s (the source string was modified)", call);
	pt_gb_reset(m);
}

TEST("ft_split", "alloc", "regular separators; every string is its own freeable allocation", T_SPEC)
{
	HINT("malloc each word separately (+ the array); terminate the array with NULL");
	split_case("hello world", ' ');
	split_case("one two three four", ' ');
	split_case("a,b,c", ',');
	split_case("abcabc", 'b');
	split_case("word", ' ');
}

TEST("ft_split", "alloc", "leading, trailing and consecutive delimiters", T_SPEC)
{
	split_case("  a  b  ", ' ');
	split_case(",,a,,", ',');
	split_case("a,b,,c", ',');
	split_case("   lorem   ipsum   ", ' ');
}

TEST("ft_split", "alloc", "empty string, delimiters only, string equal to the delimiter", T_SPEC)
{
	HINT("these must return an array holding just the NULL terminator, not NULL");
	split_case("", ' ');
	split_case("   ", ' ');
	split_case("a", 'a');
	split_case("xxxx", 'x');
}

TEST("ft_split", "alloc", "3000 tokens separated by 1-3 delimiters", T_SPEC)
{
	char	*big = pt_gb_alloc(40000 + 1);
	size_t	o = 0;

	for (int i = 0; i < 3000; i++)
	{
		o += snprintf(big + o, 64, "tok%d", i);
		for (unsigned k = 0, w = 1 + pt_rand() % 3; k < w; k++)
			big[o++] = ' ';
	}
	big[o] = 0;
	CALLING("ft_split(<%zu-char string with 3000 words>, ' ')", o);
	char **r = ft_split(big, ' ');
	check_tokens(big, ' ', r, "ft_split(<3000 words>, ' ')");
}

TEST("ft_split", "alloc", "delimiter '\\0' returns the whole string as one word", T_CONV)
{
	split_case("hello world", '\0');
	split_case("", '\0');
}

static void scen_split_a(void)
{
	char *s = pt_gb_str("a b  c d");

	CALLING("ft_split(\"a b  c d\", ' ')");
	ARM();
	char **r = ft_split(s, ' ');
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when an allocation fails", "a non-NULL array", "ft_split(\"a b  c d\", ' ') with malloc failing");
		return ;
	}
	check_tokens("a b  c d", ' ', r, "ft_split(\"a b  c d\", ' ')");
}
TEST("ft_split", "alloc", "failing at every malloc: NULL returned and everything already allocated is freed [inject]", T_SPEC)
{
	HINT("when a word allocation fails, free the words already created AND the array before returning NULL");
	pt_inject_sweep(scen_split_a);
}

static void scen_split_b(void)
{
	char *s = pt_gb_str(",x,yy,,zzz,");

	CALLING("ft_split(\",x,yy,,zzz,\", ',')");
	ARM();
	char **r = ft_split(s, ',');
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when an allocation fails", "a non-NULL array", "ft_split(\",x,yy,,zzz,\", ',') with malloc failing");
		return ;
	}
	check_tokens(",x,yy,,zzz,", ',', r, "ft_split(\",x,yy,,zzz,\", ',')");
}
TEST("ft_split", "alloc", "failing at every malloc, delimiters at both ends [inject]", T_SPEC)
{
	pt_inject_sweep(scen_split_b);
}

/* ---------------------------------------------------------------- strmapi / striteri */
static int		g_calls;
static unsigned	g_idx[2048];
static char		g_chr[2048];
static char		*g_ptr[2048];

static void rec(unsigned i, char c)
{
	if (g_calls < 2048)
	{
		g_idx[g_calls] = i;
		g_chr[g_calls] = c;
	}
	g_calls++;
}
static char f_shift(unsigned i, char c) { rec(i, c); return ((char)(c + 1)); }
static char f_idx(unsigned i, char c) { rec(i, c); return ((char)('0' + i % 10)); }

static void check_calls(const char *s0, const char *call)
{
	size_t n = strlen(s0);

	EXPECT_INT(n, g_calls, "%s number of calls to f", call);
	for (size_t i = 0; i < n; i++)
	{
		EXPECT_INT(i, g_idx[i], "%s index passed at call #%zu", call, i);
		EXPECT_INT((unsigned char)s0[i], (unsigned char)g_chr[i], "%s character passed at call #%zu", call, i);
	}
}

TEST("ft_strmapi", "alloc", "f gets (index, char) once per character in order; result is a new string", T_SPEC)
{
	static const char *cs[] = {"abcdef", "a", "hello world", "\x01\x7f\x80\xfe", NULL};

	for (int k = 0; cs[k]; k++)
	{
		size_t m = pt_gb_mark(), n = strlen(cs[k]);
		char *s = pt_gb_str(cs[k]), *exp = malloc(n + 1), *exp2 = malloc(n + 1);
		const char *call = pt_fmt("ft_strmapi(\"%s\", f)", pt_esc(cs[k]));
		for (size_t i = 0; i <= n; i++)
		{
			exp[i] = i < n ? (char)(cs[k][i] + 1) : 0;
			exp2[i] = i < n ? (char)('0' + i % 10) : 0;
		}
		CALLING("%s", call);
		g_calls = 0;
		char *r = ft_strmapi(s, f_shift);
		EXPECT_TRUE(r != NULL, "the mapped string", "NULL", "%s", call);
		check_calls(cs[k], call);
		EXPECT_STR(exp, r, "%s with f(i, c) = c + 1", call);
		EXPECT_STR(cs[k], s, "%s (the source string was modified)", call);
		free(r);
		g_calls = 0;
		r = ft_strmapi(s, f_idx);
		EXPECT_STR(exp2, r, "%s with f(i, c) = '0' + i %% 10", call);
		free(r);
		free(exp);
		free(exp2);
		pt_gb_reset(m);
	}
}

TEST("ft_strmapi", "alloc", "empty string: f is never called and an empty string is returned", T_SPEC)
{
	char *s = pt_gb_str("");

	g_calls = 0;
	char *r = ft_strmapi(s, f_shift);
	EXPECT_TRUE(r != NULL, "an allocated empty string", "NULL", "ft_strmapi(\"\", f)");
	EXPECT_INT(0, g_calls, "ft_strmapi(\"\", f) number of calls to f");
	EXPECT_STR("", r, "ft_strmapi(\"\", f)");
	free(r);
}

static void scen_strmapi(void)
{
	char *s = pt_gb_str("abc");

	CALLING("ft_strmapi(\"abc\", f)");
	ARM();
	char *r = ft_strmapi(s, f_shift);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strmapi with malloc failing");
		return ;
	}
	EXPECT_STR("bcd", r, "ft_strmapi(\"abc\", f)");
	free(r);
}
TEST("ft_strmapi", "alloc", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_strmapi);
}

static void f_up(unsigned i, char *c)
{
	if (g_calls < 2048)
	{
		g_idx[g_calls] = i;
		g_ptr[g_calls] = c;
	}
	g_calls++;
	if (*c >= 'a' && *c <= 'z')
		*c -= 32;
}

TEST("ft_striteri", "alloc", "f gets (index, &s[index]) in order and can modify the string in place", T_SPEC)
{
	static const char *cs[] = {"hello", "a", "mixed Case 123", NULL};

	for (int k = 0; cs[k]; k++)
	{
		size_t n = strlen(cs[k]);
		t_pt_cbuf b = pt_cb_new(n + 1, 0, 0);
		char exp[32];
		const char *call = pt_fmt("ft_striteri(\"%s\", f)", cs[k]);

		memcpy(b.p, cs[k], n + 1);
		for (size_t i = 0; i <= n; i++)
			exp[i] = (cs[k][i] >= 'a' && cs[k][i] <= 'z') ? cs[k][i] - 32 : cs[k][i];
		g_calls = 0;
		CALLING("%s", call);
		ft_striteri((char *)b.p, f_up);
		EXPECT_INT(n, g_calls, "%s number of calls to f", call);
		for (size_t i = 0; i < n; i++)
		{
			EXPECT_INT(i, g_idx[i], "%s index passed at call #%zu", call, i);
			EXPECT_PTR(b.p + i, g_ptr[i], "%s pointer passed at call #%zu (must be &s[%zu])", call, i, i);
		}
		EXPECT_STR(exp, (char *)b.p, "%s resulting string", call);
		pt_cb_check(&b, call);
	}
}

TEST("ft_striteri", "alloc", "empty string: f is never called", T_SPEC)
{
	char *s = pt_gb_str("");

	g_calls = 0;
	ft_striteri(s, f_up);
	EXPECT_INT(0, g_calls, "ft_striteri(\"\", f) number of calls to f");
}
