#include "../core.h"
#include "proto.h"

#define EXPECT_OFF(base, e, a, ...) do { const char *pt_oe = pt_off((base), (e)); const char *pt_oa = pt_off((base), (a)); \
	if (strcmp(pt_oe, pt_oa)) pt_fail3(pt_fmt(__VA_ARGS__), pt_oe, pt_oa); } while (0)

/* ---------------------------------------------------------------- memset / bzero */
TEST("ft_memset", "memory", "n = 0..70, several fill values, alignments, canaries, return value", T_SPEC)
{
	static const int vals[] = {0, 0x5A, 0xFF, 256 + 7, -1, 0x80, 1};
	unsigned char e[80];

	HINT("fill with (unsigned char)c, exactly n bytes, and return the original pointer");
	for (size_t v = 0; v < sizeof(vals) / sizeof(vals[0]); v++)
		for (size_t n = 0; n <= 70; n++)
			for (size_t off = 0; off < 4; off++)
			{
				size_t m = pt_gb_mark();
				t_pt_cbuf d = pt_cb_new(n, off, 0xAA);
				memset(e, 0xAA, sizeof(e));
				memset(e, vals[v], n);
				const char *call = pt_fmt("ft_memset(buf+%zu, %d, %zu)", off, vals[v], n);
				CALLING("%s", call);
				void *r = ft_memset(d.p, vals[v], n);
				EXPECT_PTR(d.p, r, "%s return value", call);
				pt_cb_check(&d, call);
				EXPECT_MEM(e, d.p, n, "%s", call);
				pt_gb_reset(m);
			}
}

TEST("ft_memset", "memory", "100000 bytes ending at a guard page", T_SPEC)
{
	unsigned char *b = pt_gb_alloc(100000);

	ft_memset(b, 0x7F, 100000);
	for (size_t i = 0; i < 100000; i++)
		EXPECT_INT(0x7F, b[i], "ft_memset(buf, 0x7F, 100000) byte %zu", i);
}

TEST("ft_bzero", "memory", "n = 0..70, alignments, canaries", T_SPEC)
{
	unsigned char e[80];

	for (size_t n = 0; n <= 70; n++)
		for (size_t off = 0; off < 4; off++)
		{
			size_t m = pt_gb_mark();
			t_pt_cbuf d = pt_cb_new(n, off, 0xAA);
			memset(e, 0xAA, sizeof(e));
			memset(e, 0, n);
			const char *call = pt_fmt("ft_bzero(buf+%zu, %zu)", off, n);
			CALLING("%s", call);
			ft_bzero(d.p, n);
			pt_cb_check(&d, call);
			EXPECT_MEM(e, d.p, n, "%s", call);
			pt_gb_reset(m);
		}
}

/* ---------------------------------------------------------------- memcpy */
TEST("ft_memcpy", "memory", "sizes 0..5000 x source/destination alignments, return value, canaries", T_SPEC)
{
	static const size_t ns[] = {0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 100, 255, 256, 1000, 4096, 5000};
	static unsigned char ref[5100];

	for (size_t k = 0; k < sizeof(ns) / sizeof(ns[0]); k++)
		for (size_t so = 0; so < 8; so++)
			for (size_t dof = 0; dof < 8; dof++)
			{
				size_t n = ns[k], m = pt_gb_mark();
				t_pt_cbuf s = pt_cb_new(n, so, 0), d = pt_cb_new(n, dof, 0xAA);
				for (size_t i = 0; i < n; i++)
				{
					unsigned r = pt_rand();
					s.p[i] = (r % 7 == 0) ? 0 : (r % 7 == 1) ? 0xFF : (unsigned char)r;
				}
				memcpy(ref, s.p, n);
				const char *call = pt_fmt("ft_memcpy(dst+%zu, src+%zu, %zu)", dof, so, n);
				CALLING("%s", call);
				void *r = ft_memcpy(d.p, s.p, n);
				EXPECT_PTR(d.p, r, "%s return value", call);
				pt_cb_check(&d, call);
				EXPECT_MEM(ref, d.p, n, "%s (destination differs from source)", call);
				EXPECT_MEM(ref, s.p, n, "%s (source was modified)", call);
				pt_gb_reset(m);
			}
}

TEST("ft_memcpy", "memory", "1 MiB copy between guarded buffers", T_SPEC)
{
	size_t n = 1 << 20;
	unsigned char *s = pt_gb_alloc(n), *d = pt_gb_alloc(n);

	for (size_t i = 0; i < n; i++)
		s[i] = (unsigned char)(i * 31 + 7);
	ft_memcpy(d, s, n);
	EXPECT_MEM(s, d, n, "ft_memcpy(dst, src, 1 MiB)");
}

/* ---------------------------------------------------------------- memmove */
TEST("ft_memmove", "memory", "every (src, dst, n) overlap combination in a 128-byte buffer", T_SPEC)
{
	HINT("copy backwards when dst > src and the regions overlap");
	t_pt_cbuf b = pt_cb_new(128, 0, 0);
	unsigned char e[128];

	for (size_t so = 0; so <= 40; so++)
		for (size_t dof = 0; dof <= 40; dof++)
			for (size_t n = 0; n <= 48; n++)
			{
				for (size_t i = 0; i < 128; i++)
					b.p[i] = (unsigned char)(i * 7 + 3);
				memcpy(e, b.p, 128);
				memmove(e + dof, e + so, n);
				CALLING("ft_memmove(buf+%zu, buf+%zu, %zu)", dof, so, n);
				void *r = ft_memmove(b.p + dof, b.p + so, n);
				EXPECT_PTR(b.p + dof, r, "ft_memmove(buf+%zu, buf+%zu, %zu) return value", dof, so, n);
				EXPECT_MEM(e, b.p, 128, "ft_memmove(buf+%zu, buf+%zu, %zu)", dof, so, n);
			}
	pt_cb_check(&b, "ft_memmove sweep");
}

TEST("ft_memmove", "memory", "non-overlapping copies and a 5000-byte shift by one in both directions", T_SPEC)
{
	static unsigned char e[5100];
	t_pt_cbuf b = pt_cb_new(5100, 0, 0);

	for (int dir = 0; dir < 2; dir++)
	{
		for (size_t i = 0; i < 5100; i++)
			b.p[i] = (unsigned char)(i * 13 + dir);
		memcpy(e, b.p, 5100);
		if (dir)
			memmove(e, e + 1, 5000);
		else
			memmove(e + 1, e, 5000);
		CALLING("ft_memmove(buf%s, buf%s, 5000)", dir ? "" : "+1", dir ? "+1" : "");
		if (dir)
			ft_memmove(b.p, b.p + 1, 5000);
		else
			ft_memmove(b.p + 1, b.p, 5000);
		EXPECT_MEM(e, b.p, 5100, "ft_memmove 5000-byte shift (%s)", dir ? "src > dst" : "dst > src");
	}
}

/* ---------------------------------------------------------------- memchr / memcmp */
TEST("ft_memchr", "memory", "embedded NULs, 0xFF, c converted to unsigned char, never reads past n", T_SPEC)
{
	static const unsigned char data[8] = {'a', 'b', 0, 'c', 'd', 0xFF, 'e', 'f'};
	static const int cs[] = {'a', 'b', 0, 'c', 'd', 0xFF, 'e', 'f', 'z', 256 + 'a', -1, 256, 'a' + 1024};

	for (size_t n = 0; n <= 8; n++)
		for (size_t k = 0; k < sizeof(cs) / sizeof(cs[0]); k++)
		{
			size_t m = pt_gb_mark();
			char *buf = pt_gb_mem(data, n);
			const void *e = memchr(data, cs[k], n);
			CALLING("ft_memchr(<%zu bytes>, %d, %zu)", n, cs[k], n);
			const void *a = ft_memchr(buf, cs[k], n);
			EXPECT_OFF(buf, e ? buf + ((const char *)e - (const char *)data) : NULL, a,
				"ft_memchr(\"%s\", %d, %zu)", pt_escn(data, n), cs[k], n);
			pt_gb_reset(m);
		}
}

TEST("ft_memcmp", "memory", "sign of result vs libc on random buffers (unsigned bytes, embedded NULs)", T_SPEC)
{
	static const unsigned char alpha[] = {0, 1, 127, 128, 255};
	unsigned char a[32], b[32];

	HINT("compare as unsigned char, stop after n bytes, do not stop at NUL");
	for (int t = 0; t < 4000; t++)
	{
		size_t n = pt_rand() % 24, m = pt_gb_mark();
		for (size_t i = 0; i < n; i++)
			a[i] = b[i] = alpha[pt_rand() % 5];
		if (n && (pt_rand() & 1))
			b[pt_rand() % n] = alpha[pt_rand() % 5];
		char *ga = pt_gb_mem(a, n), *gb = pt_gb_mem(b, n);
		CALLING("ft_memcmp(\"%s\", \"%s\", %zu)", pt_escn(a, n), pt_escn(b, n), n);
		EXPECT_SIGN(memcmp(a, b, n), ft_memcmp(ga, gb, n), "ft_memcmp(\"%s\", \"%s\", %zu)", pt_escn(a, n), pt_escn(b, n), n);
		pt_gb_reset(m);
	}
}

TEST("ft_memcmp", "memory", "n = 0 returns 0 even if the buffers differ", T_SPEC)
{
	EXPECT_INT(0, ft_memcmp("x", "y", 0), "ft_memcmp(\"x\", \"y\", 0)");
}

/* ---------------------------------------------------------------- calloc */
TEST("ft_calloc", "memory", "block is fully zeroed (heap is pre-filled with 0xA5)", T_SPEC)
{
	static const size_t p[][2] = {{1, 1}, {5, 1}, {1, 5}, {10, 10}, {100, 3}, {1000, 8}, {3, 7}, {64, 64}};

	for (size_t k = 0; k < sizeof(p) / sizeof(p[0]); k++)
	{
		CALLING("ft_calloc(%zu, %zu)", p[k][0], p[k][1]);
		unsigned char *r = ft_calloc(p[k][0], p[k][1]);
		EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(%zu, %zu)", p[k][0], p[k][1]);
		for (size_t i = 0; i < p[k][0] * p[k][1]; i++)
			EXPECT_INT(0, r[i], "ft_calloc(%zu, %zu) byte %zu", p[k][0], p[k][1], i);
		free(r);
	}
}

TEST("ft_calloc", "memory", "nmemb or size == 0 returns a unique pointer that can be freed", T_SPEC)
{
	HINT("subject: calloc(0, x) / calloc(x, 0) must return a unique pointer usable with free() (so malloc(0)-like, not NULL)");
	void *a = ft_calloc(0, 5), *b = ft_calloc(5, 0), *c = ft_calloc(0, 0);

	EXPECT_TRUE(a != NULL, "non-NULL pointer", "NULL", "ft_calloc(0, 5)");
	EXPECT_TRUE(b != NULL, "non-NULL pointer", "NULL", "ft_calloc(5, 0)");
	EXPECT_TRUE(c != NULL, "non-NULL pointer", "NULL", "ft_calloc(0, 0)");
	EXPECT_TRUE(a != b && b != c && a != c, "three distinct pointers", "duplicate pointers", "ft_calloc(0, 5), (5, 0), (0, 0)");
	free(a);
	free(b);
	free(c);
}

TEST("ft_calloc", "memory", "nmemb * size overflow returns NULL (man page)", T_SPEC)
{
	HINT("check nmemb != 0 && size > SIZE_MAX / nmemb before multiplying");
	CALLING("ft_calloc(SIZE_MAX, 2)");
	void *a = ft_calloc((size_t)-1, 2);
	EXPECT_TRUE(a == NULL, "NULL", "non-NULL pointer", "ft_calloc(SIZE_MAX, 2)");
	CALLING("ft_calloc(SIZE_MAX / 2 + 1, 2)");
	void *b = ft_calloc((size_t)-1 / 2 + 1, 2);
	EXPECT_TRUE(b == NULL, "NULL", "non-NULL pointer", "ft_calloc(SIZE_MAX / 2 + 1, 2)");
	CALLING("ft_calloc(2, SIZE_MAX / 2 + 1)");
	void *c = ft_calloc(2, (size_t)-1 / 2 + 1);
	EXPECT_TRUE(c == NULL, "NULL", "non-NULL pointer", "ft_calloc(2, SIZE_MAX / 2 + 1)");
}

TEST("ft_calloc", "memory", "1 MiB block is fully zeroed", T_SPEC)
{
	unsigned char *r = ft_calloc(1024, 1024);

	EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(1024, 1024)");
	for (size_t i = 0; i < (1 << 20); i++)
		EXPECT_INT(0, r[i], "ft_calloc(1024, 1024) byte %zu", i);
	free(r);
}

static void scen_calloc(void)
{
	CALLING("ft_calloc(10, 4)");
	ARM();
	unsigned char *r = ft_calloc(10, 4);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_calloc(10, 4) with malloc failing");
		return ;
	}
	EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(10, 4)");
	free(r);
}
TEST("ft_calloc", "memory", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_calloc);
}

/* ---------------------------------------------------------------- strdup */
TEST("ft_strdup", "memory", "lengths 0..64: equal content, distinct block, independent of the original", T_SPEC)
{
	char buf[80];

	for (size_t n = 0; n <= 64; n++)
	{
		size_t m = pt_gb_mark();
		for (size_t i = 0; i < n; i++)
			buf[i] = 'A' + (i % 26) + ((i % 5 == 4) ? 0x80 : 0);
		buf[n] = 0;
		char *s = pt_gb_str(buf);
		CALLING("ft_strdup(<%zu-char string>)", n);
		char *r = ft_strdup(s);
		EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_strdup(\"%s\")", pt_esc(buf));
		EXPECT_TRUE(r != s, "a new block", "the same pointer", "ft_strdup(\"%s\")", pt_esc(buf));
		EXPECT_STR(buf, r, "ft_strdup(\"%s\")", pt_esc(buf));
		if (n)
		{
			s[0] ^= 1;
			EXPECT_STR(buf, r, "ft_strdup(\"%s\") after modifying the original", pt_esc(buf));
		}
		free(r);
		pt_gb_reset(m);
	}
}

static void scen_strdup(void)
{
	char *s = pt_gb_str("hello world");

	CALLING("ft_strdup(\"hello world\")");
	ARM();
	char *r = ft_strdup(s);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strdup with malloc failing");
		return ;
	}
	EXPECT_STR("hello world", r, "ft_strdup(\"hello world\")");
	free(r);
}
TEST("ft_strdup", "memory", "malloc failure is reported with NULL and nothing leaks [inject]", T_SPEC)
{
	pt_inject_sweep(scen_strdup);
}
