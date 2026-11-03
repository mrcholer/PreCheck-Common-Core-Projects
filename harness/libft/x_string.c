/* Extended suite: ctype + string. Exhaustive small domains and large randomized differential runs. */
#include "ref.h"

/* ================================================================ ctype */
#define XC(fn, ref) \
TEST(#fn, "ctype", "extreme ints: INT_MIN/INT_MAX + 20000 random 32-bit values", T_CONV) \
{ \
	static const int sp[] = {INT_MIN, INT_MIN + 1, -65536, -257, -256, -129, 256, 257, 511, 512, 65535, 65536, INT_MAX - 1, INT_MAX}; \
	for (size_t i = 0; i < sizeof(sp) / sizeof(sp[0]); i++) \
	{ \
		CALLING(#fn "(%d)", sp[i]); \
		EXPECT_INT(ref(sp[i]), fn(sp[i]), #fn "(%d)", sp[i]); \
	} \
	for (int i = 0; i < 20000; i++) \
	{ \
		int v = (int)pt_rand(); \
		CALLING(#fn "(%d)", v); \
		EXPECT_INT(ref(v), fn(v), #fn "(%d)", v); \
	} \
}
XC(ft_isalpha, rf_alpha)
XC(ft_isdigit, rf_digit)
XC(ft_isalnum, rf_alnum)
XC(ft_isascii, rf_ascii)
XC(ft_isprint, rf_print)
XC(ft_toupper, rf_up)
XC(ft_tolower, rf_low)

TEST("ft_toupper", "ctype", "is idempotent and only ever changes a-z (0..255)", T_SPEC)
{
	for (int c = 0; c < 256; c++)
	{
		int u = ft_toupper(c);
		EXPECT_INT(u, ft_toupper(u), "ft_toupper(ft_toupper(%d))", c);
		EXPECT_TRUE(u == c || (c >= 'a' && c <= 'z'), "unchanged outside a-z", pt_fmt("changed to %d", u), "ft_toupper(%d)", c);
	}
}
TEST("ft_tolower", "ctype", "is idempotent and only ever changes A-Z (0..255)", T_SPEC)
{
	for (int c = 0; c < 256; c++)
	{
		int u = ft_tolower(c);
		EXPECT_INT(u, ft_tolower(u), "ft_tolower(ft_tolower(%d))", c);
		EXPECT_TRUE(u == c || (c >= 'A' && c <= 'Z'), "unchanged outside A-Z", pt_fmt("changed to %d", u), "ft_tolower(%d)", c);
	}
}

/* ================================================================ strlen */
TEST("ft_strlen", "string", "every length 0..9000 (crosses page boundaries, NUL flush with guard page)", T_SPEC)
{
	static char big[9100];

	memset(big, 'q', sizeof(big));
	for (size_t n = 0; n <= 9000; n++)
	{
		size_t m = pt_gb_mark();
		char *s = pt_gb_alloc(n + 1);
		memcpy(s, big, n);
		s[n] = 0;
		CALLING("ft_strlen(<%zu-char string>)", n);
		EXPECT_INT(n, ft_strlen(s), "ft_strlen(<string of %zu chars>)", n);
		pt_gb_reset(m);
	}
}

TEST("ft_strlen", "string", "fuzz: 4000 random strings (len 0..5000, bytes 1..255) at random interior offsets", T_SPEC)
{
	static char buf[5100];

	for (int t = 0; t < 4000; t++)
	{
		size_t n = pt_rand() % 5001, k = n ? pt_rand() % (n + 1) : 0, m = pt_gb_mark();
		rf_rbytes(buf, n);
		buf[n] = 0;
		char *s = pt_gb_str(buf);
		CALLING("ft_strlen(<%zu-char random string> + %zu)", n, k);
		EXPECT_INT(n - k, ft_strlen(s + k), "ft_strlen(<random %zu-char string> + %zu)", n, k);
		pt_gb_reset(m);
	}
}

/* ================================================================ strchr / strrchr */
static void chr_exh(char *(*f)(const char *, int), char *(*ref)(const char *, int), const char *name)
{
	static const int cs[] = {'a', 'b', 'c', 0, 'a' + 256, 'b' - 256, -1, 255, 'a' + 1024, 'a' - 1024};
	char buf[16];

	for (size_t len = 0; len <= 9; len++)
		for (size_t idx = 0; idx < rf_pow(2, len); idx++)
		{
			size_t m = pt_gb_mark();
			rf_gen(buf, len, idx, "ab");
			char *s = pt_gb_str(buf);
			for (size_t k = 0; k < sizeof(cs) / sizeof(cs[0]); k++)
			{
				CALLING("%s(\"%s\", %d)", name, buf, cs[k]);
				EXPECT_OFF(s, ref(s, cs[k]), f(s, cs[k]), "%s(\"%s\", %d)", name, buf, cs[k]);
			}
			pt_gb_reset(m);
		}
}

static void chr_fuzz(char *(*f)(const char *, int), char *(*ref)(const char *, int), const char *name)
{
	static char buf[300];

	for (int t = 0; t < 4000; t++)
	{
		size_t n = pt_rand() % 200, m = pt_gb_mark();
		rf_rbytes(buf, n);
		if (t % 3 == 0)
			for (size_t i = 0; i < n; i++)
				buf[i] = "abc"[pt_rand() % 3];
		buf[n] = 0;
		char *s = pt_gb_str(buf);
		for (int k = 0; k < 12; k++)
		{
			int c = (k < 4 && n) ? (unsigned char)buf[pt_rand() % n] : (int)(pt_rand() % 700) - 200;
			if (k == 4)
				c = 0;
			CALLING("%s(<%zu-byte random string>, %d)", name, n, c);
			EXPECT_OFF(s, ref(s, c), f(s, c), "%s(\"%s\", %d)", name, pt_escn(buf, n), c);
		}
		pt_gb_reset(m);
	}
}

TEST("ft_strchr", "string", "exhaustive: all strings over {a,b} up to length 9 x 10 search values", T_SPEC)
{
	chr_exh(ft_strchr, strchr, "ft_strchr");
}
TEST("ft_strchr", "string", "fuzz: 4000 random strings x 12 search values (incl. NUL, >255, negative)", T_SPEC)
{
	chr_fuzz(ft_strchr, strchr, "ft_strchr");
}
TEST("ft_strrchr", "string", "exhaustive: all strings over {a,b} up to length 9 x 10 search values", T_SPEC)
{
	chr_exh(ft_strrchr, strrchr, "ft_strrchr");
}
TEST("ft_strrchr", "string", "fuzz: 4000 random strings x 12 search values (incl. NUL, >255, negative)", T_SPEC)
{
	chr_fuzz(ft_strrchr, strrchr, "ft_strrchr");
}

/* ================================================================ strncmp */
TEST("ft_strncmp", "string", "exhaustive: all pairs of strings (len 0..3) over {a, b, 0x80, 0xFF} x n in 0..5 and SIZE_MAX", T_SPEC)
{
	static char strs[85][4];
	static char *gs[85];
	static const size_t ns[] = {0, 1, 2, 3, 4, 5, (size_t)-1};
	size_t cnt = 0;

	for (size_t len = 0; len <= 3; len++)
		for (size_t idx = 0; idx < rf_pow(4, len); idx++)
		{
			size_t x = idx;
			for (size_t i = 0; i < len; i++, x /= 4)
				strs[cnt][i] = "ab\x80\xff"[x % 4];
			strs[cnt][len] = 0;
			gs[cnt] = pt_gb_str(strs[cnt]);
			cnt++;
		}
	HINT("compare as unsigned char; stop at the first NUL or after n bytes");
	for (size_t i = 0; i < cnt; i++)
		for (size_t j = 0; j < cnt; j++)
			for (size_t k = 0; k < sizeof(ns) / sizeof(ns[0]); k++)
			{
				CALLING("ft_strncmp(\"%s\", \"%s\", %zu)", pt_esc(strs[i]), pt_esc(strs[j]), ns[k]);
				EXPECT_SIGN(strncmp(strs[i], strs[j], ns[k]), ft_strncmp(gs[i], gs[j], ns[k]),
					"ft_strncmp(\"%s\", \"%s\", %zu)", pt_esc(strs[i]), pt_esc(strs[j]), ns[k]);
			}
}

TEST("ft_strncmp", "string", "fuzz: 20000 random pairs with shared prefixes; also non-terminated slices of exactly n bytes", T_SPEC)
{
	static char a[260], b[260];

	for (int t = 0; t < 20000; t++)
	{
		size_t la = pt_rand() % 120, lb, pre, n, m = pt_gb_mark();
		rf_rbytes(a, la);
		if (t % 2)
			for (size_t i = 0; i < la; i++)
				a[i] = "ab\x80\xff"[pt_rand() % 4];
		a[la] = 0;
		pre = la ? pt_rand() % (la + 1) : 0;
		lb = pre + pt_rand() % 8;
		memcpy(b, a, pre);
		for (size_t i = pre; i < lb; i++)
			b[i] = (char)(1 + pt_rand() % 255);
		b[lb] = 0;
		n = (t % 7 == 0) ? (size_t)-1 : pt_rand() % (la + lb + 3);
		int sliced = n != (size_t)-1 && n <= la && n <= lb && (t % 3 == 0);
		char *ga = sliced ? pt_gb_mem(a, n) : pt_gb_str(a), *gb = sliced ? pt_gb_mem(b, n) : pt_gb_str(b);
		CALLING("ft_strncmp(<%zu-byte str>, <%zu-byte str>, %zu)", la, lb, n);
		EXPECT_SIGN(strncmp(a, b, n), ft_strncmp(ga, gb, n), "ft_strncmp(\"%s\", \"%s\", %zu)%s", pt_esc(a), pt_esc(b), n,
			sliced ? " [buffers are exactly n bytes, no NUL]" : "");
		pt_gb_reset(m);
	}
}

/* ================================================================ strnstr */
TEST("ft_strnstr", "string", "exhaustive: haystacks over {a,b} (len 0..7) x needles over {a,b} (len 0..4) x len 0..strlen+2", T_SPEC)
{
	static char nds[31][8];
	static char *gn[31];
	size_t nn = 0;
	char h[16];

	for (size_t len = 0; len <= 4; len++)
		for (size_t idx = 0; idx < rf_pow(2, len); idx++)
		{
			rf_gen(nds[nn], len, idx, "ab");
			gn[nn] = pt_gb_str(nds[nn]);
			nn++;
		}
	HINT("the match must lie entirely inside the first len bytes; never read h[len]");
	for (size_t hl = 0; hl <= 7; hl++)
		for (size_t idx = 0; idx < rf_pow(2, hl); idx++)
		{
			rf_gen(h, hl, idx, "ab");
			for (size_t len = 0; len <= hl + 2; len++)
			{
				size_t m = pt_gb_mark();
				char *g = len <= hl ? pt_gb_mem(h, len) : pt_gb_str(h);
				for (size_t k = 0; k < nn; k++)
				{
					const char *e = rf_strnstr(h, nds[k], len);
					CALLING("ft_strnstr(\"%s\", \"%s\", %zu)", pt_escn(h, hl), nds[k], len);
					EXPECT_OFF(g, e ? g + (e - h) : NULL, ft_strnstr(g, gn[k], len), "ft_strnstr(\"%s\", \"%s\", %zu)", h, nds[k], len);
				}
				pt_gb_reset(m + 0 * 0);
				/* needles were allocated before the mark, so resetting to m keeps them */
			}
		}
}

TEST("ft_strnstr", "string", "fuzz: 20000 random haystacks/needles (needle often a real substring), random len", T_SPEC)
{
	static char h[100], nd[16];

	for (int t = 0; t < 20000; t++)
	{
		size_t hl = pt_rand() % 60, nl, len, m = pt_gb_mark();
		for (size_t i = 0; i < hl; i++)
			h[i] = "abc"[pt_rand() % 3];
		h[hl] = 0;
		if (hl && t % 2)
		{
			size_t st = pt_rand() % hl;
			nl = 1 + pt_rand() % (hl - st < 8 ? hl - st : 8);
			memcpy(nd, h + st, nl);
		}
		else
		{
			nl = pt_rand() % 7;
			for (size_t i = 0; i < nl; i++)
				nd[i] = "abc"[pt_rand() % 3];
		}
		nd[nl] = 0;
		len = pt_rand() % (hl + 4);
		char *g = len <= hl ? pt_gb_mem(h, len) : pt_gb_str(h), *gn = pt_gb_str(nd);
		const char *e = rf_strnstr(h, nd, len);
		CALLING("ft_strnstr(\"%s\", \"%s\", %zu)", h, nd, len);
		EXPECT_OFF(g, e ? g + (e - h) : NULL, ft_strnstr(g, gn, len), "ft_strnstr(\"%s\", \"%s\", %zu)", h, nd, len);
		pt_gb_reset(m);
	}
}

/* ================================================================ strlcpy */
TEST("ft_strlcpy", "string", "exhaustive: src length 0..24 x size 0..28 (no padding, canaries, content, return)", T_SPEC)
{
	char src[40];
	unsigned char e[40];

	HINT("return strlen(src); copy at most size-1 bytes then NUL (only when size > 0); never pad or write past size");
	for (size_t sl = 0; sl <= 24; sl++)
	{
		for (size_t i = 0; i < sl; i++)
			src[i] = 'a' + (i % 26);
		src[sl] = 0;
		for (size_t size = 0; size <= 28; size++)
			for (size_t off = 0; off < 3; off++)
			{
				size_t m = pt_gb_mark();
				char *gs = pt_gb_str(src);
				t_pt_cbuf d = pt_cb_new(size, off, 'Z');
				memset(e, 'Z', sizeof(e));
				rf_strlcpy((char *)e, src, size);
				CALLING("ft_strlcpy(dst[%zu], <%zu-char src>, %zu)", size, sl, size);
				size_t r = ft_strlcpy((char *)d.p, gs, size);
				EXPECT_INT(sl, r, "ft_strlcpy(dst, \"%s\", %zu) return value", src, size);
				pt_cb_check(&d, pt_fmt("ft_strlcpy(dst, \"%s\", %zu)", src, size));
				EXPECT_MEM(e, d.p, size, "ft_strlcpy(dst, \"%s\", %zu) resulting dst = \"%s\"", src, size, pt_escn(d.p, size));
				pt_gb_reset(m);
			}
	}
}

TEST("ft_strlcpy", "string", "fuzz: 4000 random sources/sizes incl. size larger than the source; untouched bytes stay untouched", T_SPEC)
{
	static char src[400];
	static unsigned char e[420];

	for (int t = 0; t < 4000; t++)
	{
		size_t sl = pt_rand() % 300, size = pt_rand() % (sl + 20), m = pt_gb_mark();
		rf_rbytes(src, sl);
		src[sl] = 0;
		char *gs = pt_gb_str(src);
		t_pt_cbuf d = pt_cb_new(size, pt_rand() % 8, 0xA7);
		memset(e, 0xA7, sizeof(e));
		rf_strlcpy((char *)e, src, size);
		CALLING("ft_strlcpy(dst[%zu], <%zu-byte src>, %zu)", size, sl, size);
		size_t r = ft_strlcpy((char *)d.p, gs, size);
		EXPECT_INT(sl, r, "ft_strlcpy(dst, <%zu-byte random src>, %zu) return value", sl, size);
		pt_cb_check(&d, pt_fmt("ft_strlcpy(dst, <%zu-byte src>, %zu)", sl, size));
		EXPECT_MEM(e, d.p, size, "ft_strlcpy(dst, <%zu-byte random src>, %zu) dst", sl, size);
		pt_gb_reset(m);
	}
}

TEST("ft_strlcpy", "string", "size = SIZE_MAX with a big enough destination copies the whole string", T_CONV)
{
	char *src = pt_gb_str("hello world");
	t_pt_cbuf d = pt_cb_new(12, 0, 'Z');

	CALLING("ft_strlcpy(dst[12], \"hello world\", SIZE_MAX)");
	EXPECT_INT(11, ft_strlcpy((char *)d.p, src, (size_t)-1), "ft_strlcpy(dst, \"hello world\", SIZE_MAX) return value");
	EXPECT_STR("hello world", (char *)d.p, "ft_strlcpy(dst, \"hello world\", SIZE_MAX)");
	pt_cb_check(&d, "ft_strlcpy(dst, \"hello world\", SIZE_MAX)");
}

/* ================================================================ strlcat */
TEST("ft_strlcat", "string", "exhaustive: dst length 0..12 x src length 0..12 x size 0..30 (incl. unterminated dst when size <= strlen(dst))", T_SPEC)
{
	char dsts[16], srcs[16];
	unsigned char e[48];

	HINT("return min(size, strlen(dst)) + strlen(src); when size <= strlen(dst) leave dst untouched; never read past size");
	for (size_t dl = 0; dl <= 12; dl++)
		for (size_t sl = 0; sl <= 12; sl++)
		{
			for (size_t i = 0; i < dl; i++)
				dsts[i] = 'A' + i;
			dsts[dl] = 0;
			for (size_t i = 0; i < sl; i++)
				srcs[i] = 'a' + i;
			srcs[sl] = 0;
			for (size_t size = 0; size <= 30; size++)
			{
				size_t m = pt_gb_mark();
				char *gs = pt_gb_str(srcs);
				t_pt_cbuf d = pt_cb_new(size, size % 3, 'Z');
				memcpy(d.p, dsts, size > dl ? dl + 1 : size);
				memset(e, 'Z', sizeof(e));
				memcpy(e, d.p, size);
				size_t er = rf_strlcat((char *)e, srcs, size);
				CALLING("ft_strlcat(dst=\"%s\" in %zu-byte buffer, \"%s\", %zu)", dsts, size, srcs, size);
				size_t r = ft_strlcat((char *)d.p, gs, size);
				EXPECT_INT(er, r, "ft_strlcat(dst=\"%s\" in a %zu-byte buffer, \"%s\", %zu) return value", dsts, size, srcs, size);
				pt_cb_check(&d, pt_fmt("ft_strlcat(dst=\"%s\", \"%s\", %zu)", dsts, srcs, size));
				EXPECT_MEM(e, d.p, size, "ft_strlcat(dst=\"%s\" in a %zu-byte buffer, \"%s\", %zu) resulting buffer", dsts, size, srcs, size);
				pt_gb_reset(m);
			}
		}
}

TEST("ft_strlcat", "string", "fuzz: 4000 random dst/src/size combinations (long strings, page-sized buffers)", T_SPEC)
{
	static char ds[600], ss[600];
	static unsigned char e[1300];

	for (int t = 0; t < 4000; t++)
	{
		size_t dl = pt_rand() % 400, sl = pt_rand() % 400, size = pt_rand() % (dl + sl + 12), m = pt_gb_mark();
		rf_rbytes(ds, dl);
		rf_rbytes(ss, sl);
		ds[dl] = ss[sl] = 0;
		char *gs = pt_gb_str(ss);
		t_pt_cbuf d = pt_cb_new(size, pt_rand() % 8, 0xA7);
		memcpy(d.p, ds, size > dl ? dl + 1 : size);
		memset(e, 0xA7, sizeof(e));
		memcpy(e, d.p, size);
		size_t er = rf_strlcat((char *)e, ss, size);
		CALLING("ft_strlcat(dst[%zu] (len %zu), <%zu-byte src>, %zu)", size, dl, sl, size);
		size_t r = ft_strlcat((char *)d.p, gs, size);
		EXPECT_INT(er, r, "ft_strlcat(<dst len %zu in %zu bytes>, <src len %zu>, %zu) return value", dl, size, sl, size);
		pt_cb_check(&d, pt_fmt("ft_strlcat(dst len %zu, src len %zu, %zu)", dl, sl, size));
		EXPECT_MEM(e, d.p, size, "ft_strlcat(<dst len %zu in %zu bytes>, <src len %zu>, %zu) resulting buffer", dl, size, sl, size);
		pt_gb_reset(m);
	}
}

TEST("ft_strlcat", "string", "size = SIZE_MAX with a big enough destination appends the whole string", T_CONV)
{
	char *src = pt_gb_str("world");
	t_pt_cbuf d = pt_cb_new(12, 0, 'Z');

	memcpy(d.p, "hello ", 7);
	CALLING("ft_strlcat(\"hello \", \"world\", SIZE_MAX)");
	EXPECT_INT(11, ft_strlcat((char *)d.p, src, (size_t)-1), "ft_strlcat(\"hello \", \"world\", SIZE_MAX) return value");
	EXPECT_STR("hello world", (char *)d.p, "ft_strlcat(\"hello \", \"world\", SIZE_MAX)");
	pt_cb_check(&d, "ft_strlcat(\"hello \", \"world\", SIZE_MAX)");
}

/* ================================================================ atoi */
TEST("ft_atoi", "convert", "exhaustive: all 9331 strings of length 0..5 over {' ', '+', '-', '0', '5', 'a'} vs libc", T_SPEC)
{
	char buf[8];

	HINT("skip isspace chars, accept ONE optional sign, then digits; stop at the first non-digit");
	for (size_t len = 0; len <= 5; len++)
		for (size_t idx = 0; idx < rf_pow(6, len); idx++)
		{
			size_t m = pt_gb_mark();
			rf_gen(buf, len, idx, " +-05a");
			char *s = pt_gb_str(buf);
			CALLING("ft_atoi(\"%s\")", buf);
			EXPECT_INT(atoi(buf), ft_atoi(s), "ft_atoi(\"%s\")", buf);
			pt_gb_reset(m);
		}
}

TEST("ft_atoi", "convert", "every single byte value as prefix/separator: \"<c>7\", \"7<c>8\", \"<c>-7\" (whitespace set must be exactly isspace)", T_SPEC)
{
	char buf[8];

	for (int c = 1; c < 256; c++)
	{
		const char *fm[3] = {"%c7", "7%c8", "%c-7"};
		for (int k = 0; k < 3; k++)
		{
			size_t m = pt_gb_mark();
			snprintf(buf, sizeof(buf), fm[k], c);
			char *s = pt_gb_str(buf);
			CALLING("ft_atoi(\"%s\")", pt_esc(buf));
			EXPECT_INT(atoi(buf), ft_atoi(s), "ft_atoi(\"%s\") [byte 0x%02x]", pt_esc(buf), c);
			pt_gb_reset(m);
		}
	}
}

TEST("ft_atoi", "convert", "fuzz: 30000 random numbers (<= 9 digits) with whitespace, signs, leading zeros and trailing junk", T_SPEC)
{
	static const char ws[] = " \t\n\v\f\r";
	char buf[64];

	for (int t = 0; t < 30000; t++)
	{
		size_t o = 0, w = pt_rand() % 4, nd = 1 + pt_rand() % 9;
		while (w--)
			buf[o++] = ws[pt_rand() % 6];
		unsigned sg = pt_rand() % 5;
		if (sg == 1) buf[o++] = '+';
		else if (sg == 2) buf[o++] = '-';
		else if (sg == 3) { buf[o++] = '-'; buf[o++] = '+'; }
		else if (sg == 4) { buf[o++] = '+'; buf[o++] = '+'; }
		for (size_t i = 0; i < nd; i++)
			buf[o++] = '0' + pt_rand() % 10;
		if (pt_rand() % 3 == 0)
			for (size_t i = 0, j = pt_rand() % 4; i < j; i++)
				buf[o++] = " +-a9\t."[pt_rand() % 7];
		buf[o] = 0;
		size_t m = pt_gb_mark();
		char *s = pt_gb_str(buf);
		CALLING("ft_atoi(\"%s\")", pt_esc(buf));
		EXPECT_INT(atoi(buf), ft_atoi(s), "ft_atoi(\"%s\")", pt_esc(buf));
		pt_gb_reset(m);
	}
}

TEST("ft_atoi", "convert", "INT_MAX / INT_MIN with whitespace, signs and many leading zeros", T_SPEC)
{
	static const char *cs[] = {"2147483647", "+2147483647", "-2147483648", "   2147483647", "\t\n-2147483648abc",
		"0000000000000000002147483647", "-0000000000000000002147483648", "2147483647 99", "-2147483647", "  +0000", NULL};

	for (int i = 0; cs[i]; i++)
	{
		char *s = pt_gb_str(cs[i]);
		CALLING("ft_atoi(\"%s\")", cs[i]);
		EXPECT_INT(atoi(cs[i]), ft_atoi(s), "ft_atoi(\"%s\")", pt_esc(cs[i]));
	}
}
