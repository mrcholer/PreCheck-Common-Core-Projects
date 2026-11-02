#include "../core.h"
#include "proto.h"

#define EXPECT_OFF(base, e, a, ...) do { const char *pt_oe = pt_off((base), (e)); const char *pt_oa = pt_off((base), (a)); \
	if (strcmp(pt_oe, pt_oa)) pt_fail3(pt_fmt(__VA_ARGS__), pt_oe, pt_oa); } while (0)

/* ---------------------------------------------------------------- strlen */
TEST("ft_strlen", "string", "lengths 0..300 (guard page right after the NUL)", T_SPEC)
{
	char	buf[400];

	for (size_t n = 0; n <= 300; n++)
	{
		size_t m = pt_gb_mark();
		for (size_t i = 0; i < n; i++)
			buf[i] = 'a' + (i % 26);
		buf[n] = 0;
		char *s = pt_gb_str(buf);
		CALLING("ft_strlen(<%zu-char string>)", n);
		EXPECT_INT(n, ft_strlen(s), "ft_strlen(\"%s\")", pt_esc(s));
		pt_gb_reset(m);
	}
}

TEST("ft_strlen", "string", "bytes >= 0x80 and interior start offsets", T_SPEC)
{
	char *s = pt_gb_str("\xe9\xff\x80 ab\x7f");

	for (size_t i = 0; i <= 7; i++)
	{
		CALLING("ft_strlen(s + %zu)", i);
		EXPECT_INT(7 - i, ft_strlen(s + i), "ft_strlen(\"%s\")", pt_esc(s + i));
	}
}

TEST("ft_strlen", "string", "1 MiB string", T_SPEC)
{
	size_t	n = 1 << 20;
	char	*s = pt_gb_alloc(n + 1);

	memset(s, 'x', n);
	s[n] = 0;
	CALLING("ft_strlen(<1 MiB string>)");
	EXPECT_INT(n, ft_strlen(s), "ft_strlen(<1 MiB string>)");
}

/* ---------------------------------------------------------------- strchr / strrchr */
static const char *g_strs[] = {"", "a", "hello world", "aaaa", "abcabcabc", "\xe9\xff x\x80",
	"the quick brown fox jumps over the lazy dog", NULL};

static void chr_suite(char *(*f)(const char *, int), char *(*ref)(const char *, int), const char *name, int mode)
{
	for (int i = 0; g_strs[i]; i++)
	{
		size_t m = pt_gb_mark();
		char *s = pt_gb_str(g_strs[i]);
		for (int c = 0; c < 256; c++)
		{
			int cc = mode == 1 ? c + 256 * ((c % 3) + 1) : mode == 2 ? c - 256 : c;
			CALLING("%s(\"%s\", %d)", name, pt_esc(s), cc);
			EXPECT_OFF(s, ref(s, cc), f(s, cc), "%s(\"%s\", %d%s)", name, pt_esc(s), cc,
				(cc >= 32 && cc < 127) ? pt_fmt(" '%c'", cc) : "");
		}
		pt_gb_reset(m);
	}
}

TEST("ft_strchr", "string", "every byte value on assorted strings (incl. NUL terminator)", T_SPEC)
{
	HINT("searching for '\\0' must return a pointer to the terminating NUL");
	chr_suite(ft_strchr, strchr, "ft_strchr", 0);
}
TEST("ft_strchr", "string", "c is converted to char (c+256, c-256)", T_SPEC)
{
	chr_suite(ft_strchr, strchr, "ft_strchr", 1);
	chr_suite(ft_strchr, strchr, "ft_strchr", 2);
}
TEST("ft_strchr", "string", "returns the FIRST occurrence", T_SPEC)
{
	char *s = pt_gb_str("a.b.c.d");

	EXPECT_OFF(s, s + 1, ft_strchr(s, '.'), "ft_strchr(\"a.b.c.d\", '.')");
}
TEST("ft_strrchr", "string", "every byte value on assorted strings (incl. NUL terminator)", T_SPEC)
{
	HINT("searching for '\\0' must return a pointer to the terminating NUL");
	chr_suite(ft_strrchr, strrchr, "ft_strrchr", 0);
}
TEST("ft_strrchr", "string", "c is converted to char (c+256, c-256)", T_SPEC)
{
	chr_suite(ft_strrchr, strrchr, "ft_strrchr", 1);
	chr_suite(ft_strrchr, strrchr, "ft_strrchr", 2);
}
TEST("ft_strrchr", "string", "returns the LAST occurrence", T_SPEC)
{
	char *s = pt_gb_str("a.b.c.d");

	EXPECT_OFF(s, s + 5, ft_strrchr(s, '.'), "ft_strrchr(\"a.b.c.d\", '.')");
}

/* ---------------------------------------------------------------- strncmp */
static const char *g_cmp[][2] = {{"", ""}, {"a", ""}, {"", "a"}, {"abc", "abc"}, {"abc", "abd"}, {"abd", "abc"},
	{"abc", "abcd"}, {"abcd", "abc"}, {"\xff", "a"}, {"a", "\xff"}, {"\x80zz", "\x7fzz"}, {"test", "tesu"},
	{"hello world", "hello there"}, {NULL, NULL}};
static const size_t g_ns[] = {0, 1, 2, 3, 4, 5, 6, 11, 100, (size_t)-1};

TEST("ft_strncmp", "string", "sign of result vs libc (n = 0, 1, ..., SIZE_MAX; bytes >= 0x80 are unsigned)", T_SPEC)
{
	HINT("compare as unsigned char; stop at the first NUL or after n bytes");
	for (int i = 0; g_cmp[i][0]; i++)
		for (size_t k = 0; k < sizeof(g_ns) / sizeof(g_ns[0]); k++)
		{
			size_t m = pt_gb_mark();
			char *a = pt_gb_str(g_cmp[i][0]), *b = pt_gb_str(g_cmp[i][1]);
			CALLING("ft_strncmp(\"%s\", \"%s\", %zu)", pt_esc(a), pt_esc(b), g_ns[k]);
			EXPECT_SIGN(strncmp(a, b, g_ns[k]), ft_strncmp(a, b, g_ns[k]), "ft_strncmp(\"%s\", \"%s\", %zu)",
				pt_esc(a), pt_esc(b), g_ns[k]);
			pt_gb_reset(m);
		}
}

TEST("ft_strncmp", "string", "never reads past n (non-terminated buffers of exactly n bytes)", T_SPEC)
{
	for (int i = 0; g_cmp[i][0]; i++)
		for (size_t n = 1; n <= 5; n++)
		{
			size_t la = strlen(g_cmp[i][0]), lb = strlen(g_cmp[i][1]);
			if (n > la || n > lb)
				continue ;
			size_t m = pt_gb_mark();
			char *a = pt_gb_mem(g_cmp[i][0], n), *b = pt_gb_mem(g_cmp[i][1], n);
			CALLING("ft_strncmp(<%zu bytes>, <%zu bytes>, %zu)", n, n, n);
			EXPECT_SIGN(strncmp(g_cmp[i][0], g_cmp[i][1], n), ft_strncmp(a, b, n),
				"ft_strncmp(\"%s\", \"%s\", %zu) on non-terminated buffers", pt_escn(a, n), pt_escn(b, n), n);
			pt_gb_reset(m);
		}
}

TEST("ft_strncmp", "string", "stops at an embedded NUL even if n is larger", T_SPEC)
{
	char *a = pt_gb_mem("abc\0d", 5), *b = pt_gb_mem("abc\0e", 5);

	EXPECT_INT(0, ft_strncmp(a, b, 5), "ft_strncmp(\"abc\\0d\", \"abc\\0e\", 5)");
}

/* ---------------------------------------------------------------- strnstr */
static const char *ref_strnstr(const char *h, const char *nd, size_t len)
{
	size_t	nl = strlen(nd);

	if (!nl)
		return (h);
	for (size_t i = 0; i < len && h[i]; i++)
	{
		if (i + nl > len)
			break ;
		if (!strncmp(h + i, nd, nl))
			return (h + i);
	}
	return (NULL);
}

static const char *g_nstr[][2] = {{"hello world", "world"}, {"hello", ""}, {"", "a"}, {"", ""}, {"aaaaab", "aab"},
	{"abc", "abcd"}, {"abcabc", "abc"}, {"mississippi", "issip"}, {"mississippi", "pi"}, {"a", "a"},
	{"lorem ipsum dolor sit amet", "dolor"}, {"abcdef", "f"}, {"abcdef", "ef"}, {"foo bar baz", "baz"}, {NULL, NULL}};

TEST("ft_strnstr", "string", "all haystack/needle pairs for len = 0..strlen+2", T_SPEC)
{
	HINT("empty needle returns the haystack; the match must lie entirely within the first len bytes");
	for (int i = 0; g_nstr[i][0]; i++)
	{
		size_t hl = strlen(g_nstr[i][0]);
		for (size_t len = 0; len <= hl + 2; len++)
		{
			size_t m = pt_gb_mark();
			char *h = len <= hl ? pt_gb_mem(g_nstr[i][0], len) : pt_gb_str(g_nstr[i][0]);
			char *nd = pt_gb_str(g_nstr[i][1]);
			const char *e = ref_strnstr(g_nstr[i][0], nd, len);
			CALLING("ft_strnstr(\"%s\", \"%s\", %zu)", pt_escn(h, len <= hl ? len : hl), nd, len);
			const char *a = ft_strnstr(h, nd, len);
			EXPECT_OFF(h, e ? h + (e - g_nstr[i][0]) : NULL, a, "ft_strnstr(\"%s\", \"%s\", %zu)",
				pt_escn(h, len <= hl ? len : hl), nd, len);
			pt_gb_reset(m);
		}
	}
}

TEST("ft_strnstr", "string", "needle longer than len and len = SIZE_MAX", T_SPEC)
{
	char *h = pt_gb_str("abcdef");

	EXPECT_OFF(h, NULL, ft_strnstr(h, "abcdefg", (size_t)-1), "ft_strnstr(\"abcdef\", \"abcdefg\", SIZE_MAX)");
	EXPECT_OFF(h, h + 2, ft_strnstr(h, "cd", (size_t)-1), "ft_strnstr(\"abcdef\", \"cd\", SIZE_MAX)");
}

/* ---------------------------------------------------------------- strlcpy / strlcat */
static const char *g_src[] = {"", "a", "hello", "hello world!", NULL};

TEST("ft_strlcpy", "string", "return value, truncation, NUL termination, no padding", T_SPEC)
{
	HINT("return strlen(src); copy at most size-1 bytes then NUL (only when size > 0); never pad or write past size");
	for (int i = 0; g_src[i]; i++)
		for (size_t size = 0; size <= strlen(g_src[i]) + 3; size++)
		{
			size_t m = pt_gb_mark();
			char *src = pt_gb_str(g_src[i]);
			t_pt_cbuf d = pt_cb_new(size, 0, 'Z');
			unsigned char e[64];
			memset(e, 'Z', sizeof(e));
			if (size)
			{
				size_t n = strlen(src) < size - 1 ? strlen(src) : size - 1;
				memcpy(e, src, n);
				e[n] = 0;
			}
			const char *call = pt_fmt("ft_strlcpy(dst[%zu], \"%s\", %zu)", size, src, size);
			CALLING("%s", call);
			size_t r = ft_strlcpy((char *)d.p, src, size);
			EXPECT_INT(strlen(src), r, "%s return value", call);
			pt_cb_check(&d, call);
			EXPECT_MEM(e, d.p, size, "%s resulting dst = \"%s\"", call, pt_escn(d.p, size));
			pt_gb_reset(m);
		}
}

static size_t ref_strlcat(char *dst, const char *src, size_t size)
{
	size_t	dl = 0, sl = strlen(src), i = 0;

	while (dl < size && dst[dl])
		dl++;
	if (dl == size)
		return (size + sl);
	while (src[i] && dl + i + 1 < size)
	{
		dst[dl + i] = src[i];
		i++;
	}
	dst[dl + i] = 0;
	return (dl + sl);
}

TEST("ft_strlcat", "string", "return value, truncation, NUL termination, dst longer than size", T_SPEC)
{
	static const char *dsts[] = {"", "abc", "hello", NULL};
	static const char *srcs[] = {"", "xy", "world", NULL};

	HINT("return min(size, strlen(dst)) + strlen(src); when size <= strlen(dst) leave dst untouched; never read past size");
	for (int i = 0; dsts[i]; i++)
		for (int j = 0; srcs[j]; j++)
			for (size_t size = 0; size <= strlen(dsts[i]) + strlen(srcs[j]) + 3; size++)
			{
				size_t m = pt_gb_mark(), dl = strlen(dsts[i]);
				char *src = pt_gb_str(srcs[j]);
				t_pt_cbuf d = pt_cb_new(size, 0, 'Z');
				unsigned char e[64];
				memset(e, 'Z', sizeof(e));
				if (size > dl)
					memcpy(d.p, dsts[i], dl + 1);
				else
					memcpy(d.p, dsts[i], size);
				memcpy(e, d.p, size);
				size_t er = ref_strlcat((char *)e, src, size);
				const char *call = pt_fmt("ft_strlcat(dst=\"%s\" in a %zu-byte buffer, \"%s\", %zu)", dsts[i], size, src, size);
				CALLING("%s", call);
				size_t r = ft_strlcat((char *)d.p, src, size);
				EXPECT_INT(er, r, "%s return value", call);
				pt_cb_check(&d, call);
				EXPECT_MEM(e, d.p, size, "%s resulting buffer", call);
				pt_gb_reset(m);
			}
}
