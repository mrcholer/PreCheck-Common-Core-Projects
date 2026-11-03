/* Deep suite: hidden state, aliasing, read-only inputs, huge inputs, re-entrancy, error paths, over-allocation. */
#include "ref.h"
#include <fcntl.h>

#define NEED_ALLOC(p, need, ...) do { size_t pt_sz = pt_alloc_size(p); pt_sh->checks++; \
	if (pt_sz != (size_t)-1 && pt_sz > (size_t)(need)) pt_fail3(pt_fmt(__VA_ARGS__), pt_fmt("a block of %zu bytes", (size_t)(need)), \
		pt_fmt("a block of %zu bytes", pt_sz)); } while (0)

static char *bigstr(size_t n, int ch)
{
	char *s = pt_gb_alloc(n + 1);

	memset(s, ch, n);
	s[n] = 0;
	return (s);
}

/* ================================================================== strlen / strchr / strrchr */
TEST("ft_strlen", "string", "read-only input memory (any write faults), lengths 0..200", T_SPEC)
{
	char b[256];

	for (size_t n = 0; n <= 200; n++)
	{
		size_t m = pt_gb_mark();
		memset(b, 'x', n); b[n] = 0;
		char *s = pt_gb_ro_str(b);
		CALLING("ft_strlen(<read-only %zu-char string>)", n);
		EXPECT_INT(n, ft_strlen(s), "ft_strlen(<read-only string of %zu chars>)", n);
		pt_gb_reset(m);
	}
}
TEST("ft_strlen", "string", "8 MiB string (recursive or quadratic implementations die here; stack is limited to 1 MiB)", T_SPEC)
{
	char *s = bigstr(8 << 20, 'z');

	CALLING("ft_strlen(<8 MiB string>)");
	EXPECT_INT(8 << 20, ft_strlen(s), "ft_strlen(<8 MiB string>)");
}
TEST("ft_strchr", "string", "read-only input; 4 MiB string: char absent / first / last / NUL terminator", T_SPEC)
{
	size_t n = 4 << 20;
	char *s = bigstr(n, 'a');

	s[0] = 'F'; s[n - 1] = 'L';
	EXPECT_OFF(s, NULL, ft_strchr(s, 'Z'), "ft_strchr(<4 MiB string>, 'Z')");
	EXPECT_OFF(s, s, ft_strchr(s, 'F'), "ft_strchr(<4 MiB string>, 'F')");
	EXPECT_OFF(s, s + n - 1, ft_strchr(s, 'L'), "ft_strchr(<4 MiB string>, 'L')");
	EXPECT_OFF(s, s + n, ft_strchr(s, 0), "ft_strchr(<4 MiB string>, 0)");
	pt_gb_protect(s);
	EXPECT_OFF(s, s + 1, ft_strchr(s, 'a'), "ft_strchr(<read-only 4 MiB string>, 'a')");
}
TEST("ft_strrchr", "string", "read-only input; 4 MiB string: char absent / first / last / NUL terminator", T_SPEC)
{
	size_t n = 4 << 20;
	char *s = bigstr(n, 'a');

	s[0] = 'F'; s[n - 1] = 'L';
	EXPECT_OFF(s, NULL, ft_strrchr(s, 'Z'), "ft_strrchr(<4 MiB string>, 'Z')");
	EXPECT_OFF(s, s, ft_strrchr(s, 'F'), "ft_strrchr(<4 MiB string>, 'F')");
	EXPECT_OFF(s, s + n - 1, ft_strrchr(s, 'L'), "ft_strrchr(<4 MiB string>, 'L')");
	EXPECT_OFF(s, s + n, ft_strrchr(s, 0), "ft_strrchr(<4 MiB string>, 0)");
	pt_gb_protect(s);
	EXPECT_OFF(s, s + n - 2, ft_strrchr(s, 'a'), "ft_strrchr(<read-only 4 MiB string>, 'a')");
}
TEST("ft_strchr", "string", "read-only inputs: every byte value on assorted strings", T_SPEC)
{
	static const char *ss[] = {"", "a", "hello world", "\xe9\xff x\x80", "abcabcabc", NULL};

	for (int i = 0; ss[i]; i++)
	{
		size_t m = pt_gb_mark();
		char *s = pt_gb_ro_str(ss[i]);
		for (int c = 0; c < 256; c++)
			{ const char *e = strchr(ss[i], c); EXPECT_OFF(s, e ? s + (e - ss[i]) : NULL, ft_strchr(s, c), "ft_strchr(<read-only \"%s\">, %d)", pt_esc(ss[i]), c); }
		pt_gb_reset(m);
	}
}
TEST("ft_strrchr", "string", "read-only inputs: every byte value on assorted strings", T_SPEC)
{
	static const char *ss[] = {"", "a", "hello world", "\xe9\xff x\x80", "abcabcabc", NULL};

	for (int i = 0; ss[i]; i++)
	{
		size_t m = pt_gb_mark();
		char *s = pt_gb_ro_str(ss[i]);
		for (int c = 0; c < 256; c++)
			{ const char *e = strrchr(ss[i], c); EXPECT_OFF(s, e ? s + (e - ss[i]) : NULL, ft_strrchr(s, c), "ft_strrchr(<read-only \"%s\">, %d)", pt_esc(ss[i]), c); }
		pt_gb_reset(m);
	}
}

/* ================================================================== strncmp */
TEST("ft_strncmp", "string", "read-only inputs and identical pointers (s, s)", T_SPEC)
{
	char *a = pt_gb_ro_str("hello"), *b = pt_gb_ro_str("help");

	EXPECT_SIGN(-1, ft_strncmp(a, b, 4), "ft_strncmp(\"hello\", \"help\", 4) read-only");
	EXPECT_INT(0, ft_strncmp(a, b, 3), "ft_strncmp(\"hello\", \"help\", 3) read-only");
	EXPECT_INT(0, ft_strncmp(a, a, 100), "ft_strncmp(s, s, 100) same pointer twice");
	EXPECT_INT(0, ft_strncmp(a, a, (size_t)-1), "ft_strncmp(s, s, SIZE_MAX) same pointer twice");
}
TEST("ft_strncmp", "string", "returns the difference of the first differing bytes (man page), not just -1/1", T_CONV)
{
	static const char *c[][2] = {{"a", "b"}, {"b", "a"}, {"abc", "abz"}, {"\x80", "a"}, {"a", "\xff"}, {"", "x"}, {"x", ""}, {"hello", "help"}, {NULL, NULL}};

	HINT("man strncmp: the result is the difference between the first differing bytes, taken as unsigned char");
	for (int i = 0; c[i][0]; i++)
	{
		size_t k = 0;
		while (c[i][0][k] && c[i][0][k] == c[i][1][k])
			k++;
		int exp = (unsigned char)c[i][0][k] - (unsigned char)c[i][1][k];
		char *a = pt_gb_str(c[i][0]), *b = pt_gb_str(c[i][1]);
		EXPECT_INT(exp, ft_strncmp(a, b, 100), "ft_strncmp(\"%s\", \"%s\", 100)", pt_esc(c[i][0]), pt_esc(c[i][1]));
	}
}
TEST("ft_strncmp", "string", "two 4 MiB strings: equal, and differing only in the last byte", T_SPEC)
{
	size_t n = 4 << 20;
	char *a = bigstr(n, 'q'), *b = bigstr(n, 'q');

	EXPECT_INT(0, ft_strncmp(a, b, (size_t)-1), "ft_strncmp(<4 MiB>, <same 4 MiB>, SIZE_MAX)");
	b[n - 1] = 'r';
	EXPECT_SIGN(-1, ft_strncmp(a, b, (size_t)-1), "ft_strncmp(<4 MiB>, <4 MiB differing in the last byte>, SIZE_MAX)");
	EXPECT_INT(0, ft_strncmp(a, b, n - 1), "ft_strncmp(.., .., n = index of the difference)");
}

/* ================================================================== strnstr */
TEST("ft_strnstr", "string", "read-only inputs; haystack == needle (same pointer); needle == haystack prefix", T_SPEC)
{
	char *h = pt_gb_ro_str("hello world");

	EXPECT_OFF(h, h, ft_strnstr(h, h, 11), "ft_strnstr(s, s, 11) same pointer twice");
	EXPECT_OFF(h, NULL, ft_strnstr(h, h, 10), "ft_strnstr(s, s, 10) (needle needs 11 bytes)");
	EXPECT_OFF(h, h, ft_strnstr(h, h, (size_t)-1), "ft_strnstr(s, s, SIZE_MAX)");
	EXPECT_OFF(h, h + 6, ft_strnstr(h, "world", 11), "ft_strnstr(\"hello world\", \"world\", 11) read-only haystack");
}
TEST("ft_strnstr", "string", "1 MiB of 'a' with needles of 'a'x100 + 'b' (match only at the very end) and absent", T_SPEC)
{
	size_t n = 1 << 20;
	char *h = bigstr(n, 'a'), nd[128];

	memset(nd, 'a', 100);
	nd[100] = 'b'; nd[101] = 0;
	EXPECT_OFF(h, NULL, ft_strnstr(h, nd, n), "ft_strnstr(<1 MiB of a>, \"a\"x100+\"b\", 1 MiB)");
	h[n - 1] = 'b';
	EXPECT_OFF(h, h + n - 101, ft_strnstr(h, nd, n), "ft_strnstr(<1 MiB of a ending in b>, \"a\"x100+\"b\", 1 MiB)");
	EXPECT_OFF(h, NULL, ft_strnstr(h, nd, n - 1), "ft_strnstr(<1 MiB of a ending in b>, .., 1 MiB - 1)");
}

/* ================================================================== strlcpy / strlcat */
TEST("ft_strlcpy", "string", "read-only source; 8 MiB source into an exactly sized destination", T_SPEC)
{
	char *s = pt_gb_ro_str("read only source");
	t_pt_cbuf d = pt_cb_new(8, 0, 'Z');

	EXPECT_INT(16, ft_strlcpy((char *)d.p, s, 8), "ft_strlcpy(dst[8], <read-only 16-char string>, 8)");
	EXPECT_STR("read on", (char *)d.p, "ft_strlcpy(dst[8], \"read only source\", 8)");
	pt_cb_check(&d, "ft_strlcpy(dst[8], src, 8)");
	size_t n = 8 << 20;
	char *big = bigstr(n, 'k');
	char *dst = pt_gb_alloc(n + 1);
	EXPECT_INT(n, ft_strlcpy(dst, big, n + 1), "ft_strlcpy(<8 MiB+1 dst>, <8 MiB src>, 8 MiB+1)");
	EXPECT_TRUE(!memcmp(dst, big, n + 1), "identical copy", "differs", "ft_strlcpy(<8 MiB>)");
}
TEST("ft_strlcat", "string", "read-only source; 4 MiB + 4 MiB appended into an exactly sized destination", T_SPEC)
{
	char *s = pt_gb_ro_str("-suffix");
	t_pt_cbuf d = pt_cb_new(12, 0, 'Z');

	memcpy(d.p, "prefix", 7);
	EXPECT_INT(13, ft_strlcat((char *)d.p, s, 12), "ft_strlcat(\"prefix\", <read-only \"-suffix\">, 12)");
	EXPECT_STR("prefix-suff", (char *)d.p, "ft_strlcat(\"prefix\", \"-suffix\", 12)");
	pt_cb_check(&d, "ft_strlcat(\"prefix\", \"-suffix\", 12)");
	size_t n = 4 << 20;
	char *a = bigstr(n, 'a'), *dst = pt_gb_alloc(2 * n + 1);
	memcpy(dst, a, n + 1);
	char *b = bigstr(n, 'b');
	EXPECT_INT(2 * n, ft_strlcat(dst, b, 2 * n + 1), "ft_strlcat(<4 MiB>, <4 MiB>, 8 MiB+1)");
	EXPECT_TRUE(dst[n - 1] == 'a' && dst[n] == 'b' && dst[2 * n - 1] == 'b' && dst[2 * n] == 0, "a..ab..b\\0", "wrong content", "ft_strlcat(<4 MiB>, <4 MiB>, 8 MiB+1)");
}

/* ================================================================== atoi */
TEST("ft_atoi", "convert", "read-only inputs; a million leading zeros / spaces before the number", T_SPEC)
{
	size_t n = 1 << 20;
	char *s = pt_gb_alloc(n + 8);

	memset(s, '0', n);
	memcpy(s + n, "42", 3);
	EXPECT_INT(42, ft_atoi(s), "ft_atoi(\"0\" x 1048576 + \"42\")");
	memset(s, ' ', n);
	memcpy(s + n, "-7", 3);
	EXPECT_INT(-7, ft_atoi(s), "ft_atoi(\" \" x 1048576 + \"-7\")");
	char *r = pt_gb_ro_str("  -12345xyz");
	EXPECT_INT(-12345, ft_atoi(r), "ft_atoi(<read-only \"  -12345xyz\">)");
}

/* ================================================================== memset / bzero / memcpy / memmove */
TEST("ft_memset", "memory", "32 MiB block, all bytes checked", T_SPEC)
{
	size_t n = 32 << 20;
	unsigned char *b = pt_gb_alloc(n);

	ft_memset(b, 0x6B, n);
	for (size_t i = 0; i < n; i++)
		if (b[i] != 0x6B)
			EXPECT_INT(0x6B, b[i], "ft_memset(<32 MiB>, 0x6B, 32 MiB) byte %zu", i);
}
TEST("ft_bzero", "memory", "32 MiB block, all bytes checked", T_SPEC)
{
	size_t n = 32 << 20;
	unsigned char *b = pt_gb_alloc(n);

	memset(b, 0xFF, n);
	ft_bzero(b, n);
	for (size_t i = 0; i < n; i++)
		if (b[i])
			EXPECT_INT(0, b[i], "ft_bzero(<32 MiB>, 32 MiB) byte %zu", i);
}
TEST("ft_memcpy", "memory", "32 MiB copy; read-only source", T_SPEC)
{
	size_t n = 32 << 20;
	unsigned char *s = pt_gb_alloc(n), *d = pt_gb_alloc(n);

	for (size_t i = 0; i < n; i++)
		s[i] = (unsigned char)(i * 7 + (i >> 8));
	pt_gb_protect(s);
	ft_memcpy(d, s, n);
	EXPECT_MEM(s, d, n, "ft_memcpy(<32 MiB>, <read-only 32 MiB>, 32 MiB)");
}
TEST("ft_memmove", "memory", "16 MiB overlapping shifts by 1, 7 and 4097 bytes in both directions; identical pointers", T_SPEC)
{
	size_t n = 16 << 20, sh[3] = {1, 7, 4097};
	unsigned char *b = pt_gb_alloc(n + 8200), *e = pt_gb_alloc(n + 8200);

	for (int dir = 0; dir < 2; dir++)
		for (int k = 0; k < 3; k++)
		{
			for (size_t i = 0; i < n + 8200; i++)
				b[i] = e[i] = (unsigned char)(i * 31 + i / 251);
			if (dir)
				memmove(e, e + sh[k], n);
			else
				memmove(e + sh[k], e, n);
			CALLING("ft_memmove(<16 MiB>, shift %zu, %s)", sh[k], dir ? "down" : "up");
			if (dir)
				ft_memmove(b, b + sh[k], n);
			else
				ft_memmove(b + sh[k], b, n);
			EXPECT_MEM(e, b, n + 8200, "ft_memmove 16 MiB shifted %s by %zu", dir ? "down" : "up", sh[k]);
		}
	ft_memmove(b, b, n);
	EXPECT_MEM(e, b, n + 8200, "ft_memmove(p, p, 16 MiB) must leave the data unchanged");
}

/* ================================================================== memchr / memcmp */
TEST("ft_memchr", "memory", "stops at the first match even when n is huge (SIZE_MAX); read-only buffer", T_SPEC)
{
	HINT("C11 7.24.5.1: memchr behaves as if it reads sequentially and stops at the first match, so n larger than the object is fine");
	for (size_t len = 1; len <= 40; len++)
		for (size_t pos = 0; pos < len; pos++)
		{
			size_t m = pt_gb_mark();
			unsigned char *b = pt_gb_alloc(len);
			memset(b, 'x', len);
			b[pos] = 'Z';
			pt_gb_protect(b);
			CALLING("ft_memchr(<%zu bytes>, 'Z', SIZE_MAX)", len);
			EXPECT_OFF((char *)b, (char *)b + pos, ft_memchr(b, 'Z', (size_t)-1), "ft_memchr(<%zu bytes, 'Z' at %zu>, 'Z', SIZE_MAX)", len, pos);
			pt_gb_reset(m);
		}
}
TEST("ft_memchr", "memory", "16 MiB buffer: absent / first / last byte", T_SPEC)
{
	size_t n = 16 << 20;
	char *b = pt_gb_alloc(n);

	memset(b, 'a', n);
	EXPECT_OFF(b, NULL, ft_memchr(b, 'Z', n), "ft_memchr(<16 MiB>, 'Z', 16 MiB)");
	b[n - 1] = 'Z';
	EXPECT_OFF(b, b + n - 1, ft_memchr(b, 'Z', n), "ft_memchr(<16 MiB with Z last>, 'Z', 16 MiB)");
	b[0] = 'Z';
	EXPECT_OFF(b, b, ft_memchr(b, 'Z', n), "ft_memchr(<16 MiB with Z first>, 'Z', 16 MiB)");
}
TEST("ft_memcmp", "memory", "identical pointers, read-only buffers, 16 MiB equal / last-byte difference", T_SPEC)
{
	size_t n = 16 << 20;
	unsigned char *a = pt_gb_alloc(n), *b = pt_gb_alloc(n);

	for (size_t i = 0; i < n; i++)
		a[i] = b[i] = (unsigned char)(i * 13);
	EXPECT_INT(0, ft_memcmp(a, a, n), "ft_memcmp(p, p, 16 MiB) same pointer twice");
	EXPECT_INT(0, ft_memcmp(a, b, n), "ft_memcmp(<16 MiB>, <equal 16 MiB>, 16 MiB)");
	b[n - 1] ^= 0x80;
	pt_gb_protect(a);
	pt_gb_protect(b);
	EXPECT_TRUE(ft_memcmp(a, b, n) != 0, "non-zero", "0", "ft_memcmp(<16 MiB>, <differing in the last byte>, 16 MiB) read-only");
	EXPECT_INT(0, ft_memcmp(a, b, n - 1), "ft_memcmp(.., .., n = index of the difference)");
}
TEST("ft_memcmp", "memory", "returns the difference of the first differing bytes (man page), not just -1/1", T_CONV)
{
	static const unsigned char x[][2] = {{1, 2}, {2, 1}, {0x80, 1}, {1, 0xFF}, {0, 255}, {200, 100}};

	HINT("man memcmp: the result is the difference between the first differing bytes, taken as unsigned char");
	for (int i = 0; i < 6; i++)
	{
		unsigned char *a = pt_gb_mem(&x[i][0], 1), *b = pt_gb_mem(&x[i][1], 1);
		EXPECT_INT((int)x[i][0] - (int)x[i][1], ft_memcmp(a, b, 1), "ft_memcmp(0x%02x, 0x%02x, 1)", x[i][0], x[i][1]);
	}
}
TEST("ft_memcmp", "memory", "huge n with an early difference must not read past it (n = SIZE_MAX)", T_CONV)
{
	unsigned char *a = pt_gb_mem("abcX", 4), *b = pt_gb_mem("abcY", 4);

	EXPECT_SIGN(-1, ft_memcmp(a, b, (size_t)-1), "ft_memcmp(\"abcX\", \"abcY\", SIZE_MAX)");
}
TEST("ft_memcpy", "memory", "n = 0 with identical / valid pointers does nothing and returns dst", T_SPEC)
{
	char *a = pt_gb_str("abc");

	EXPECT_PTR(a, ft_memcpy(a, a + 1, 0), "ft_memcpy(dst, src, 0)");
	EXPECT_STR("abc", a, "ft_memcpy(dst, src, 0) must not write");
}

/* ================================================================== calloc / strdup: big, state, allocation size */
TEST("ft_calloc", "memory", "64 MiB block fully zeroed", T_SPEC)
{
	unsigned char *r = ft_calloc(1 << 16, 1 << 10);

	EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(65536, 1024)");
	for (size_t i = 0; i < (size_t)1 << 26; i++)
		if (r[i])
			EXPECT_INT(0, r[i], "ft_calloc(65536, 1024) byte %zu", i);
	free(r);
}
TEST("ft_calloc", "memory", "allocates exactly nmemb * size bytes (no waste)", T_CONV)
{
	static const size_t p[][2] = {{1, 1}, {10, 4}, {3, 7}, {100, 100}, {1, 1000}};

	for (int i = 0; i < 5; i++)
	{
		void *r = ft_calloc(p[i][0], p[i][1]);
		NEED_ALLOC(r, p[i][0] * p[i][1], "ft_calloc(%zu, %zu)", p[i][0], p[i][1]);
		free(r);
	}
}
TEST("ft_calloc", "memory", "300 blocks alive at once keep their own zeroed contents (no shared/static buffer)", T_SPEC)
{
	unsigned char *r[300];

	for (int i = 0; i < 300; i++)
	{
		r[i] = ft_calloc(1 + i % 17, 8);
		EXPECT_TRUE(r[i] != NULL, "non-NULL", "NULL", "ft_calloc #%d", i);
		memset(r[i], i, (1 + i % 17) * 8);
	}
	for (int i = 0; i < 300; i++)
	{
		for (int j = 0; j < i; j++)
			EXPECT_TRUE(r[i] != r[j], "distinct blocks", "the same block twice", "ft_calloc #%d vs #%d", i, j);
		EXPECT_INT((unsigned char)i, r[i][0], "block #%d content after later calls", i);
		free(r[i]);
	}
}
TEST("ft_strdup", "memory", "read-only source; 8 MiB source; 300 copies alive at once, independent of later input changes", T_SPEC)
{
	char *ro = pt_gb_ro_str("read only"), *c[300], b[40];

	char *r = ft_strdup(ro);
	EXPECT_STR("read only", r, "ft_strdup(<read-only \"read only\">)");
	NEED_ALLOC(r, 10, "ft_strdup(\"read only\")");
	free(r);
	size_t n = 8 << 20;
	char *big = bigstr(n, 'x');
	r = ft_strdup(big);
	EXPECT_TRUE(r && !memcmp(r, big, n + 1), "identical copy", "differs", "ft_strdup(<8 MiB string>)");
	free(r);
	for (int i = 0; i < 300; i++)
	{
		snprintf(b, sizeof(b), "string number %d", i);
		char *s = pt_gb_str(b);
		c[i] = ft_strdup(s);
		memset(s, 'Z', strlen(s));
	}
	for (int i = 0; i < 300; i++)
	{
		snprintf(b, sizeof(b), "string number %d", i);
		EXPECT_STR(b, c[i], "ft_strdup copy #%d after later calls / input overwritten", i);
		free(c[i]);
	}
}

/* ================================================================== itoa */
TEST("ft_itoa", "convert", "400 results alive at once stay correct and distinct (no static buffer)", T_SPEC)
{
	char *r[400], exp[16];

	HINT("each call must return its own malloc'd block, never a shared/static buffer");
	for (int i = 0; i < 400; i++)
		r[i] = ft_itoa(i * 7919 - 1000000);
	for (int i = 0; i < 400; i++)
	{
		snprintf(exp, sizeof(exp), "%d", i * 7919 - 1000000);
		EXPECT_STR(exp, r[i], "ft_itoa(%d) result after 399 other calls", i * 7919 - 1000000);
		for (int j = 0; j < i; j++)
			EXPECT_TRUE(r[i] != r[j], "distinct blocks", "the same pointer twice", "ft_itoa #%d vs #%d", i, j);
		free(r[i]);
	}
}
TEST("ft_itoa", "convert", "allocates only sign + digits + NUL", T_CONV)
{
	static const int vs[] = {0, 5, -5, 10, 99, 100, -100, 12345, INT_MAX, INT_MIN};
	char exp[16];

	for (int i = 0; i < 10; i++)
	{
		snprintf(exp, sizeof(exp), "%d", vs[i]);
		char *r = ft_itoa(vs[i]);
		NEED_ALLOC(r, strlen(exp) + 1, "ft_itoa(%d)", vs[i]);
		free(r);
	}
}

/* ================================================================== substr */
TEST("ft_substr", "alloc", "300 results alive at once; read-only source; result independent of the source afterwards", T_SPEC)
{
	char *r[300], exp[32], src[40];

	for (int i = 0; i < 300; i++)
	{
		snprintf(src, sizeof(src), "abcdefghij%03d", i);
		char *s = pt_gb_str(src);
		r[i] = ft_substr(s, 3, 9);
		memset(s, 'Z', strlen(s));
	}
	for (int i = 0; i < 300; i++)
	{
		snprintf(src, sizeof(src), "abcdefghij%03d", i);
		memcpy(exp, src + 3, 9);
		exp[9] = 0;
		EXPECT_STR(exp, r[i], "ft_substr(\"%s\", 3, 9) after the source was overwritten", src);
		free(r[i]);
	}
	char *ro = pt_gb_ro_str("read only text");
	char *x = ft_substr(ro, 5, 4);
	EXPECT_STR("only", x, "ft_substr(<read-only \"read only text\">, 5, 4)");
	free(x);
}
TEST("ft_substr", "alloc", "allocates only what the result needs, even when len is huge", T_CONV)
{
	HINT("clamp len to strlen(s) - start first; do not malloc(len + 1) for len = 1000000");
	char *s = pt_gb_str("hello");
	char *r = ft_substr(s, 1, 1000000);
	EXPECT_STR("ello", r, "ft_substr(\"hello\", 1, 1000000)");
	NEED_ALLOC(r, 5, "ft_substr(\"hello\", 1, 1000000)");
	free(r);
	r = ft_substr(s, 1, 2);
	NEED_ALLOC(r, 3, "ft_substr(\"hello\", 1, 2)");
	free(r);
	r = ft_substr(s, 99, 10);
	if (r)
		NEED_ALLOC(r, 1, "ft_substr(\"hello\", 99, 10)");
	free(r);
}
TEST("ft_substr", "alloc", "8 MiB source: middle slice and whole string", T_SPEC)
{
	size_t n = 8 << 20;
	char *s = bigstr(n, 'm');

	s[n / 2] = 'X';
	char *r = ft_substr(s, (unsigned)(n / 2 - 1), 3);
	EXPECT_STR("mXm", r, "ft_substr(<8 MiB>, 4194303, 3)");
	free(r);
	r = ft_substr(s, 0, (size_t)-1);
	EXPECT_TRUE(r && !memcmp(r, s, n + 1), "identical copy", "differs", "ft_substr(<8 MiB>, 0, SIZE_MAX)");
	free(r);
}

/* ================================================================== strjoin */
TEST("ft_strjoin", "alloc", "same pointer for s1 and s2; read-only inputs", T_SPEC)
{
	char *s = pt_gb_ro_str("abc"), *e = pt_gb_ro_str("");
	char *r = ft_strjoin(s, s);

	EXPECT_STR("abcabc", r, "ft_strjoin(s, s) same pointer twice");
	free(r);
	r = ft_strjoin(e, e);
	EXPECT_STR("", r, "ft_strjoin(\"\", \"\") same pointer twice, read-only");
	NEED_ALLOC(r, 1, "ft_strjoin(\"\", \"\")");
	free(r);
}
TEST("ft_strjoin", "alloc", "300 results alive at once, independent of later input changes; exact allocation size", T_SPEC)
{
	char *r[300], a[24], b[24], exp[64];

	for (int i = 0; i < 300; i++)
	{
		snprintf(a, sizeof(a), "left%d-", i);
		snprintf(b, sizeof(b), "right%d", i * 3);
		char *ga = pt_gb_str(a), *gb = pt_gb_str(b);
		r[i] = ft_strjoin(ga, gb);
		NEED_ALLOC(r[i], strlen(a) + strlen(b) + 1, "ft_strjoin(\"%s\", \"%s\")", a, b);
		memset(ga, 'Z', strlen(ga));
		memset(gb, 'Z', strlen(gb));
	}
	for (int i = 0; i < 300; i++)
	{
		snprintf(exp, sizeof(exp), "left%d-right%d", i, i * 3);
		EXPECT_STR(exp, r[i], "ft_strjoin result #%d after later calls / inputs overwritten", i);
		free(r[i]);
	}
}
TEST("ft_strjoin", "alloc", "two 4 MiB strings", T_SPEC)
{
	size_t n = 4 << 20;
	char *a = bigstr(n, 'a'), *b = bigstr(n, 'b');
	char *r = ft_strjoin(a, b);

	EXPECT_TRUE(r != NULL && strlen(r) == 2 * n && r[0] == 'a' && r[n - 1] == 'a' && r[n] == 'b' && r[2 * n - 1] == 'b', "aaaa..bbbb", "wrong", "ft_strjoin(<4 MiB>, <4 MiB>)");
	free(r);
}

/* ================================================================== strtrim */
TEST("ft_strtrim", "alloc", "set == s1 (same pointer) and read-only inputs", T_SPEC)
{
	char *s = pt_gb_ro_str("abcba");
	char *r = ft_strtrim(s, s);

	EXPECT_STR("", r, "ft_strtrim(s, s) same pointer twice");
	free(r);
	char *a = pt_gb_ro_str("  hi  "), *set = pt_gb_ro_str(" ");
	r = ft_strtrim(a, set);
	EXPECT_STR("hi", r, "ft_strtrim(<read-only \"  hi  \">, <read-only \" \">)");
	NEED_ALLOC(r, 3, "ft_strtrim(\"  hi  \", \" \")");
	free(r);
}
TEST("ft_strtrim", "alloc", "2 MiB of spaces on both sides of one letter; 4 MiB with nothing to trim; empty strings", T_SPEC)
{
	size_t n = 2 << 20;
	char *s = pt_gb_alloc(2 * n + 2);

	memset(s, ' ', 2 * n + 1);
	s[n] = 'x';
	s[2 * n + 1] = 0;
	char *r = ft_strtrim(s, " ");
	EXPECT_STR("x", r, "ft_strtrim(<2 MiB spaces> x <2 MiB spaces>, \" \")");
	free(r);
	char *t = bigstr(4 << 20, 'k');
	r = ft_strtrim(t, " ");
	EXPECT_TRUE(r && !memcmp(r, t, (4 << 20) + 1), "identical copy", "differs", "ft_strtrim(<4 MiB>, \" \")");
	free(r);
	r = ft_strtrim(pt_gb_str(""), pt_gb_str(""));
	EXPECT_STR("", r, "ft_strtrim(\"\", \"\")");
	free(r);
}
TEST("ft_strtrim", "alloc", "300 results alive at once, independent of later input changes", T_SPEC)
{
	char *r[300], a[32];

	for (int i = 0; i < 300; i++)
	{
		snprintf(a, sizeof(a), "xx<%d>xx", i);
		char *s = pt_gb_str(a);
		r[i] = ft_strtrim(s, "x");
		memset(s, 'Z', strlen(s));
	}
	for (int i = 0; i < 300; i++)
	{
		snprintf(a, sizeof(a), "<%d>", i);
		EXPECT_STR(a, r[i], "ft_strtrim result #%d after later calls / input overwritten", i);
		free(r[i]);
	}
}

/* ================================================================== split */
static void free_split(char **r)
{
	for (size_t i = 0; r[i]; i++)
		free(r[i]);
	free(r);
}

TEST("ft_split", "alloc", "100 results alive at once; independent of the source string being overwritten afterwards", T_SPEC)
{
	char **r[100], s[64], exp[16];

	HINT("every token must be copied (malloc) from s; never point into s or reuse a static array");
	for (int i = 0; i < 100; i++)
	{
		snprintf(s, sizeof(s), " w%d  v%d x%d ", i, i * 2, i * 3);
		char *g = pt_gb_str(s);
		r[i] = ft_split(g, ' ');
		memset(g, 'Z', strlen(g));
	}
	for (int i = 0; i < 100; i++)
	{
		EXPECT_TRUE(r[i] != NULL, "an array", "NULL", "ft_split #%d", i);
		snprintf(exp, sizeof(exp), "w%d", i);
		EXPECT_STR(exp, r[i][0], "ft_split #%d token 0 after later calls / source overwritten", i);
		snprintf(exp, sizeof(exp), "v%d", i * 2);
		EXPECT_STR(exp, r[i][1], "ft_split #%d token 1", i);
		snprintf(exp, sizeof(exp), "x%d", i * 3);
		EXPECT_STR(exp, r[i][2], "ft_split #%d token 2", i);
		EXPECT_PTR(NULL, r[i][3], "ft_split #%d terminator", i);
		free_split(r[i]);
	}
}
TEST("ft_split", "alloc", "read-only source; 2 MiB string with a single delimiter in the middle; allocation sizes exact", T_SPEC)
{
	char *ro = pt_gb_ro_str("a,bb,,ccc");
	char **r = ft_split(ro, ',');

	EXPECT_TRUE(r && r[0] && r[1] && r[2] && !r[3], "3 tokens", "wrong token count", "ft_split(<read-only \"a,bb,,ccc\">, ',')");
	EXPECT_STR("a", r[0], "token 0");
	EXPECT_STR("bb", r[1], "token 1");
	EXPECT_STR("ccc", r[2], "token 2");
	free_split(r);
	size_t n = 2 << 20;
	char *s = bigstr(n, 'w');
	s[n / 2] = ' ';
	r = ft_split(s, ' ');
	EXPECT_TRUE(r && r[0] && r[1] && !r[2], "2 tokens", "wrong token count", "ft_split(<2 MiB with one space>, ' ')");
	EXPECT_INT(n / 2, strlen(r[0]), "length of token 0");
	EXPECT_INT(n / 2 - 1, strlen(r[1]), "length of token 1");
	free_split(r);
}
TEST("ft_split", "alloc", "allocates exactly (tokens + 1) pointers and len + 1 bytes per token", T_CONV)
{
	char *s = pt_gb_str("one two three");
	char **r = ft_split(s, ' ');

	NEED_ALLOC(r, 4 * sizeof(char *), "ft_split(\"one two three\", ' ') array");
	NEED_ALLOC(r[0], 4, "ft_split token \"one\"");
	NEED_ALLOC(r[1], 4, "ft_split token \"two\"");
	NEED_ALLOC(r[2], 6, "ft_split token \"three\"");
	free_split(r);
}

/* ================================================================== strmapi / striteri */
static unsigned g_next;
static int g_bad;
static char f_seq(unsigned i, char c)
{
	if (i != g_next++)
		g_bad++;
	return (c);
}
static void f_seq_it(unsigned i, char *c)
{
	if (i != g_next++)
		g_bad++;
	(void)c;
}

TEST("ft_strmapi", "alloc", "index is passed as a full unsigned int on a 300000-char string (no uchar/ushort truncation)", T_SPEC)
{
	size_t n = 300000;
	char *s = bigstr(n, 'q');

	g_next = 0; g_bad = 0;
	char *r = ft_strmapi(s, f_seq);
	EXPECT_TRUE(r != NULL, "mapped string", "NULL", "ft_strmapi(<300000 chars>, f)");
	EXPECT_INT(0, g_bad, "ft_strmapi(<300000 chars>, f): calls with a wrong index");
	EXPECT_INT(n, g_next, "ft_strmapi(<300000 chars>, f): number of calls");
	EXPECT_TRUE(!memcmp(r, s, n + 1), "identical copy", "differs", "ft_strmapi(<300000 chars>, identity)");
	free(r);
}
TEST("ft_striteri", "alloc", "index is passed as a full unsigned int on a 300000-char string", T_SPEC)
{
	size_t n = 300000;
	char *s = bigstr(n, 'q');

	g_next = 0; g_bad = 0;
	ft_striteri(s, f_seq_it);
	EXPECT_INT(0, g_bad, "ft_striteri(<300000 chars>, f): calls with a wrong index");
	EXPECT_INT(n, g_next, "ft_striteri(<300000 chars>, f): number of calls");
}

static char f_nested(unsigned i, char c)
{
	char *in, *x;

	(void)i;
	pt_cb_enter();
	x = pt_gb_str("xyz");
	in = ft_strmapi(x, f_seq);
	if (in)
		free(in);
	pt_cb_leave();
	return ((char)(c + 1));
}
TEST("ft_strmapi", "alloc", "re-entrancy: f itself calls ft_strmapi (no static/global state)", T_SPEC)
{
	char *s = pt_gb_str("abcdef");

	g_next = 0; g_bad = 0;
	char *r = ft_strmapi(s, f_nested);
	EXPECT_STR("bcdefg", r, "ft_strmapi(\"abcdef\", f) where f calls ft_strmapi");
	free(r);
}
static void it_nested(unsigned i, char *c)
{
	char tmp[8] = "abc";

	(void)i;
	pt_cb_enter();
	ft_striteri(tmp, f_seq_it);
	pt_cb_leave();
	*c += 1;
}
TEST("ft_striteri", "alloc", "re-entrancy: f itself calls ft_striteri (no static/global state)", T_SPEC)
{
	t_pt_cbuf b = pt_cb_new(8, 0, 0);

	memcpy(b.p, "abcdef", 7);
	g_next = 0; g_bad = 0;
	ft_striteri((char *)b.p, it_nested);
	EXPECT_STR("bcdefg", (char *)b.p, "ft_striteri(\"abcdef\", f) where f calls ft_striteri");
}
TEST("ft_strmapi", "alloc", "300 results alive at once; read-only source; allocation size exact", T_SPEC)
{
	char *r[300], exp[32], src[32];

	for (int i = 0; i < 300; i++)
	{
		snprintf(src, sizeof(src), "item-%d", i);
		char *s = pt_gb_ro_str(src);
		r[i] = ft_strmapi(s, f_seq);
		g_next = 0;
		NEED_ALLOC(r[i], strlen(src) + 1, "ft_strmapi(\"%s\", f)", src);
	}
	for (int i = 0; i < 300; i++)
	{
		snprintf(exp, sizeof(exp), "item-%d", i);
		EXPECT_STR(exp, r[i], "ft_strmapi result #%d after later calls", i);
		free(r[i]);
	}
}

/* ================================================================== output: write errors must not hang */
#define EPIPE_TEST(fn, call) \
TEST(#fn, "output", "closed pipe (write returns EPIPE): returns instead of looping forever", T_CONV) \
{ \
	int p[2]; \
	if (pipe(p)) return ; \
	close(p[0]); \
	HINT("do not retry write() in a loop when it returns -1"); \
	CALLING(#fn " on a pipe with no reader"); \
	call; \
	close(p[1]); \
}
EPIPE_TEST(ft_putchar_fd, ft_putchar_fd('x', p[1]))
EPIPE_TEST(ft_putstr_fd, ft_putstr_fd("hello world", p[1]))
EPIPE_TEST(ft_putendl_fd, ft_putendl_fd("hello world", p[1]))
EPIPE_TEST(ft_putnbr_fd, ft_putnbr_fd(-123456, p[1]))

/* ================================================================== lists */
#define MAXD 8192
static int g_del_calls, g_seen[MAXD], g_f_calls;
static void del_free(void *p)
{
	pt_cb_enter();
	if (p && *(int *)p >= 0 && *(int *)p < MAXD)
		g_seen[*(int *)p]++;
	g_del_calls++;
	free(p);
	pt_cb_leave();
}
static t_list *mknode(int v)
{
	t_list *n = malloc(sizeof(t_list));

	n->content = malloc(sizeof(int));
	*(int *)n->content = v;
	n->next = NULL;
	return (n);
}
static t_list *build(int n)
{
	t_list *head = NULL, **tail = &head;

	for (int i = 0; i < n; i++)
	{
		*tail = mknode(i);
		tail = &(*tail)->next;
	}
	return (head);
}
static void free_list(t_list *l)
{
	t_list *nx;

	for (; l; l = nx)
	{
		nx = l->next;
		free(l->content);
		free(l);
	}
}
static t_list *ro_list(int n, int *vals)
{
	t_list *a = pt_gb_alloc(n * sizeof(t_list));

	for (int i = 0; i < n; i++)
	{
		a[i].content = &vals[i];
		a[i].next = i + 1 < n ? &a[i + 1] : NULL;
	}
	pt_gb_protect(a);
	return (a);
}

TEST("ft_lstsize", "list", "nodes in read-only memory (the function must not write to them)", T_SPEC)
{
	static int vals[1000];

	for (int n = 1; n <= 1000; n += (n < 50 ? 1 : 37))
	{
		size_t m = pt_gb_mark();
		t_list *l = ro_list(n, vals);
		CALLING("ft_lstsize(<read-only list of %d>)", n);
		EXPECT_INT(n, ft_lstsize(l), "ft_lstsize(<read-only list of %d nodes>)", n);
		pt_gb_reset(m);
	}
}
TEST("ft_lstlast", "list", "nodes in read-only memory (the function must not write to them)", T_SPEC)
{
	static int vals[1000];

	for (int n = 1; n <= 1000; n += (n < 50 ? 1 : 37))
	{
		size_t m = pt_gb_mark();
		t_list *l = ro_list(n, vals);
		CALLING("ft_lstlast(<read-only list of %d>)", n);
		EXPECT_PTR(&l[n - 1], ft_lstlast(l), "ft_lstlast(<read-only list of %d nodes>)", n);
		pt_gb_reset(m);
	}
}
static void f_inc(void *p) { *(int *)p += 1; g_f_calls++; }
static void *f_dup(void *p)
{
	int *r;

	pt_cb_enter();
	r = malloc(sizeof(int));
	pt_cb_leave();
	*r = *(int *)p + 1;
	g_f_calls++;
	return (r);
}
TEST("ft_lstiter", "list", "nodes in read-only memory: only the contents may change", T_SPEC)
{
	static int vals[100];
	t_list *l = ro_list(100, vals);

	g_f_calls = 0;
	ft_lstiter(l, f_inc);
	EXPECT_INT(100, g_f_calls, "ft_lstiter(<read-only list of 100>, f): calls to f");
	for (int i = 0; i < 100; i++)
		EXPECT_INT(1, vals[i], "content %d after ft_lstiter", i);
}
TEST("ft_lstmap", "list", "original list in read-only memory must not be written to; result is freed cleanly", T_SPEC)
{
	static int vals[100];
	t_list *l = ro_list(100, vals), *r, *p;

	for (int i = 0; i < 100; i++)
		vals[i] = i;
	memset(g_seen, 0, sizeof(g_seen));
	g_f_calls = g_del_calls = 0;
	r = ft_lstmap(l, f_dup, del_free);
	EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap(<read-only list of 100>, f, del)");
	p = r;
	for (int i = 0; i < 100; i++, p = p->next)
		EXPECT_INT(i + 1, *(int *)p->content, "ft_lstmap result content #%d", i);
	free_list(r);
}

TEST("ft_lstsize", "list", "100000-node list with a 1 MiB stack (recursive implementations crash)", T_SPEC)
{
	t_list *l = build(100000);

	EXPECT_INT(100000, ft_lstsize(l), "ft_lstsize(<list of 100000 nodes>)");
	free_list(l);
}
TEST("ft_lstlast", "list", "100000-node list with a 1 MiB stack (recursive implementations crash)", T_SPEC)
{
	t_list *l = build(100000), *last = l;

	while (last->next)
		last = last->next;
	EXPECT_PTR(last, ft_lstlast(l), "ft_lstlast(<list of 100000 nodes>)");
	free_list(l);
}
TEST("ft_lstiter", "list", "100000-node list with a 1 MiB stack (recursive implementations crash)", T_SPEC)
{
	t_list *l = build(100000);

	g_f_calls = 0;
	ft_lstiter(l, f_inc);
	EXPECT_INT(100000, g_f_calls, "ft_lstiter(<list of 100000>): calls to f");
	free_list(l);
}
TEST("ft_lstclear", "list", "100000-node list with a 1 MiB stack (recursive implementations crash)", T_SPEC)
{
	t_list *l = build(100000);

	memset(g_seen, 0, sizeof(g_seen));
	g_del_calls = 0;
	ft_lstclear(&l, del_free);
	EXPECT_INT(100000, g_del_calls, "ft_lstclear(<list of 100000>): calls to del");
	EXPECT_PTR(NULL, l, "*lst after ft_lstclear(<100000 nodes>)");
}
TEST("ft_lstmap", "list", "30000-node list with a 1 MiB stack (recursive implementations crash)", T_SPEC)
{
	t_list *l = build(30000), *r;

	g_f_calls = g_del_calls = 0;
	r = ft_lstmap(l, f_dup, del_free);
	EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap(<list of 30000>, f, del)");
	EXPECT_INT(30000, g_f_calls, "calls to f");
	free_list(l);
	free_list(r);
}

static void f_outer(void *p)
{
	t_list *inner = build(3);

	(void)p;
	pt_cb_enter();
	g_f_calls = g_f_calls;
	ft_lstiter(inner, f_inc);
	free_list(inner);
	pt_cb_leave();
}
TEST("ft_lstiter", "list", "re-entrancy: f itself iterates another list with ft_lstiter", T_SPEC)
{
	t_list *l = build(5);

	g_f_calls = 0;
	ft_lstiter(l, f_outer);
	EXPECT_INT(15, g_f_calls, "ft_lstiter(<5 nodes>, f) where f runs ft_lstiter on 3 nodes: total calls of the inner f");
	free_list(l);
}
static void del_nested(void *p)
{
	t_list *inner = p;

	pt_cb_enter();
	ft_lstclear(&inner, del_free);
	pt_cb_leave();
}
TEST("ft_lstclear", "list", "re-entrancy: del itself clears another list with ft_lstclear (list of lists)", T_SPEC)
{
	t_list *outer = NULL, **tail = &outer;

	memset(g_seen, 0, sizeof(g_seen));
	g_del_calls = 0;
	for (int i = 0; i < 6; i++)
	{
		*tail = malloc(sizeof(t_list));
		(*tail)->content = build(4);
		(*tail)->next = NULL;
		tail = &(*tail)->next;
	}
	ft_lstclear(&outer, del_nested);
	EXPECT_INT(24, g_del_calls, "ft_lstclear(<6 lists of 4 nodes>): inner contents deleted");
	EXPECT_PTR(NULL, outer, "*lst after the nested ft_lstclear");
}
static void *f_nested_map(void *p)
{
	t_list *in = build(2), *m;
	int *r;

	pt_cb_enter();
	m = ft_lstmap(in, f_dup, del_free);
	free_list(m);
	free_list(in);
	r = malloc(sizeof(int));
	pt_cb_leave();
	*r = *(int *)p + 100;
	return (r);
}
TEST("ft_lstmap", "list", "re-entrancy: f itself calls ft_lstmap", T_SPEC)
{
	t_list *l = build(6), *r, *p;

	r = ft_lstmap(l, f_nested_map, del_free);
	EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap with a nested ft_lstmap in f");
	int i = 0;
	for (p = r; p; p = p->next, i++)
		EXPECT_INT(i + 100, *(int *)p->content, "result content #%d", i);
	EXPECT_INT(6, i, "result length");
	free_list(l);
	free_list(r);
}

TEST("ft_lstadd_back", "list", "adding NULL as the new node leaves the list unchanged", T_CONV)
{
	t_list *l = build(3), *keep = l;

	ft_lstadd_back(&l, NULL);
	EXPECT_PTR(keep, l, "ft_lstadd_back(&lst, NULL): head");
	free_list(l);
}
TEST("ft_lstclear", "list", "NULL list pointer and NULL del do not crash", T_CONV)
{
	ft_lstclear(NULL, del_free);
}
TEST("ft_lstdelone", "list", "NULL node does not crash", T_CONV)
{
	ft_lstdelone(NULL, del_free);
}
