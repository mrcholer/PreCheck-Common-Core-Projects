/* Extended suite: memory functions. */
#include "ref.h"

/* ================================================================ memset / bzero */
TEST("ft_memset", "memory", "fuzz: 6000 random (n up to 3000, alignment 0..15, any int value incl. negative / >255)", T_SPEC)
{
	static unsigned char e[3100];

	for (int t = 0; t < 6000; t++)
	{
		size_t n = pt_rand() % 3001, off = pt_rand() % 16, m = pt_gb_mark();
		int v = (t % 4 == 0) ? (int)pt_rand() : (int)(pt_rand() % 600) - 100;
		t_pt_cbuf d = pt_cb_new(n, off, 0xAA);
		memset(e, 0xAA, sizeof(e));
		memset(e, v, n);
		CALLING("ft_memset(buf+%zu, %d, %zu)", off, v, n);
		void *r = ft_memset(d.p, v, n);
		EXPECT_PTR(d.p, r, "ft_memset(buf+%zu, %d, %zu) return value", off, v, n);
		pt_cb_check(&d, pt_fmt("ft_memset(buf+%zu, %d, %zu)", off, v, n));
		EXPECT_MEM(e, d.p, n, "ft_memset(buf+%zu, %d, %zu)", off, v, n);
		pt_gb_reset(m);
	}
}

TEST("ft_memset", "memory", "every n 0..9000 on a buffer that ends exactly at a guard page", T_SPEC)
{
	for (size_t n = 0; n <= 9000; n += (n < 300 ? 1 : 7))
	{
		size_t m = pt_gb_mark();
		unsigned char *b = pt_gb_alloc(n);
		CALLING("ft_memset(buf, 0x5A, %zu)", n);
		ft_memset(b, 0x5A, n);
		for (size_t i = 0; i < n; i++)
			EXPECT_INT(0x5A, b[i], "ft_memset(buf, 0x5A, %zu) byte %zu", n, i);
		pt_gb_reset(m);
	}
}

TEST("ft_bzero", "memory", "fuzz: 6000 random (n up to 3000, alignment 0..15) over a non-zero buffer", T_SPEC)
{
	static unsigned char e[3100];

	for (int t = 0; t < 6000; t++)
	{
		size_t n = pt_rand() % 3001, off = pt_rand() % 16, m = pt_gb_mark();
		t_pt_cbuf d = pt_cb_new(n, off, 0xAA);
		memset(e, 0xAA, sizeof(e));
		memset(e, 0, n);
		CALLING("ft_bzero(buf+%zu, %zu)", off, n);
		ft_bzero(d.p, n);
		pt_cb_check(&d, pt_fmt("ft_bzero(buf+%zu, %zu)", off, n));
		EXPECT_MEM(e, d.p, n, "ft_bzero(buf+%zu, %zu)", off, n);
		pt_gb_reset(m);
	}
}

TEST("ft_bzero", "memory", "every n 0..9000 on a buffer that ends exactly at a guard page", T_SPEC)
{
	for (size_t n = 0; n <= 9000; n += (n < 300 ? 1 : 7))
	{
		size_t m = pt_gb_mark();
		unsigned char *b = pt_gb_alloc(n);
		memset(b, 0xFF, n);
		CALLING("ft_bzero(buf, %zu)", n);
		ft_bzero(b, n);
		for (size_t i = 0; i < n; i++)
			EXPECT_INT(0, b[i], "ft_bzero(buf, %zu) byte %zu", n, i);
		pt_gb_reset(m);
	}
}

/* ================================================================ memcpy */
TEST("ft_memcpy", "memory", "fuzz: 4000 random (n up to 8000, src/dst alignment 0..31, bytes incl. 0x00/0xFF)", T_SPEC)
{
	static unsigned char ref[8200];

	for (int t = 0; t < 4000; t++)
	{
		size_t n = pt_rand() % 8001, so = pt_rand() % 32, dof = pt_rand() % 32, m = pt_gb_mark();
		t_pt_cbuf s = pt_cb_new(n, so, 0), d = pt_cb_new(n, dof, 0xAA);
		for (size_t i = 0; i < n; i++)
		{
			unsigned r = pt_rand();
			s.p[i] = (r % 5 == 0) ? 0 : (r % 5 == 1) ? 0xFF : (unsigned char)(r >> 8);
		}
		memcpy(ref, s.p, n);
		CALLING("ft_memcpy(dst+%zu, src+%zu, %zu)", dof, so, n);
		void *r = ft_memcpy(d.p, s.p, n);
		EXPECT_PTR(d.p, r, "ft_memcpy(dst+%zu, src+%zu, %zu) return value", dof, so, n);
		pt_cb_check(&d, pt_fmt("ft_memcpy(dst+%zu, src+%zu, %zu)", dof, so, n));
		EXPECT_MEM(ref, d.p, n, "ft_memcpy(dst+%zu, src+%zu, %zu) (destination differs)", dof, so, n);
		EXPECT_MEM(ref, s.p, n, "ft_memcpy(dst+%zu, src+%zu, %zu) (source modified)", dof, so, n);
		pt_gb_reset(m);
	}
}

TEST("ft_memcpy", "memory", "every n 0..9000 between buffers that both end exactly at a guard page", T_SPEC)
{
	static unsigned char pat[9100];

	for (size_t i = 0; i < sizeof(pat); i++)
		pat[i] = (unsigned char)(i * 37 + 11);
	for (size_t n = 0; n <= 9000; n += (n < 200 ? 1 : 13))
	{
		size_t m = pt_gb_mark();
		unsigned char *s = pt_gb_mem(pat, n), *d = pt_gb_alloc(n);
		CALLING("ft_memcpy(dst, src, %zu)", n);
		ft_memcpy(d, s, n);
		EXPECT_MEM(pat, d, n, "ft_memcpy(dst, src, %zu)", n);
		pt_gb_reset(m);
	}
}

/* ================================================================ memmove */
TEST("ft_memmove", "memory", "exhaustive: every (src, dst, n) in a 24-byte buffer of distinct bytes", T_SPEC)
{
	t_pt_cbuf b = pt_cb_new(24, 0, 0);
	unsigned char e[24];

	for (size_t so = 0; so < 24; so++)
		for (size_t dof = 0; dof < 24; dof++)
			for (size_t n = 0; n <= 24 - (so > dof ? so : dof); n++)
			{
				for (size_t i = 0; i < 24; i++)
					b.p[i] = (unsigned char)(i + 1);
				memcpy(e, b.p, 24);
				memmove(e + dof, e + so, n);
				CALLING("ft_memmove(buf+%zu, buf+%zu, %zu)", dof, so, n);
				void *r = ft_memmove(b.p + dof, b.p + so, n);
				EXPECT_PTR(b.p + dof, r, "ft_memmove(buf+%zu, buf+%zu, %zu) return value", dof, so, n);
				EXPECT_MEM(e, b.p, 24, "ft_memmove(buf+%zu, buf+%zu, %zu)", dof, so, n);
			}
	pt_cb_check(&b, "ft_memmove exhaustive");
}

TEST("ft_memmove", "memory", "fuzz: 20000 random overlapping/non-overlapping moves inside a 4096-byte buffer", T_SPEC)
{
	static unsigned char sh[4096];
	t_pt_cbuf b = pt_cb_new(4096, 0, 0);

	HINT("copy backwards when dst > src and the regions overlap");
	for (int t = 0; t < 20000; t++)
	{
		size_t n = (t % 10 == 0) ? pt_rand() % 2000 : pt_rand() % 300;
		size_t so = pt_rand() % (4096 - n + 1), dof;
		if (t % 2)
		{
			long d = (long)so + (long)(pt_rand() % 65) - 32;
			dof = d < 0 ? 0 : (size_t)d > 4096 - n ? 4096 - n : (size_t)d;
		}
		else
			dof = pt_rand() % (4096 - n + 1);
		for (size_t i = so < dof ? so : dof; i < 4096 && i < (so > dof ? so : dof) + n; i++)
			b.p[i] = (unsigned char)(pt_rand());
		memcpy(sh, b.p, 4096);
		memmove(sh + dof, sh + so, n);
		CALLING("ft_memmove(buf+%zu, buf+%zu, %zu)", dof, so, n);
		void *r = ft_memmove(b.p + dof, b.p + so, n);
		EXPECT_PTR(b.p + dof, r, "ft_memmove(buf+%zu, buf+%zu, %zu) return value", dof, so, n);
		EXPECT_MEM(sh, b.p, 4096, "ft_memmove(buf+%zu, buf+%zu, %zu)", dof, so, n);
	}
	pt_cb_check(&b, "ft_memmove fuzz");
}

TEST("ft_memmove", "memory", "between separate guarded buffers, n = 0..9000", T_SPEC)
{
	static unsigned char pat[9100];

	for (size_t i = 0; i < sizeof(pat); i++)
		pat[i] = (unsigned char)(i * 29 + 5);
	for (size_t n = 0; n <= 9000; n += (n < 200 ? 1 : 13))
	{
		size_t m = pt_gb_mark();
		unsigned char *s = pt_gb_mem(pat, n), *d = pt_gb_alloc(n);
		CALLING("ft_memmove(dst, src, %zu)", n);
		ft_memmove(d, s, n);
		EXPECT_MEM(pat, d, n, "ft_memmove(dst, src, %zu)", n);
		pt_gb_reset(m);
	}
}

/* ================================================================ memchr */
TEST("ft_memchr", "memory", "fuzz: 20000 random buffers (small alphabet => many matches) x random search values vs libc", T_SPEC)
{
	static unsigned char data[320];

	for (int t = 0; t < 20000; t++)
	{
		size_t n = pt_rand() % 300, m = pt_gb_mark();
		unsigned al = 2 + pt_rand() % 8;
		for (size_t i = 0; i < n; i++)
			data[i] = (t % 5 == 0) ? (unsigned char)pt_rand() : (unsigned char)(pt_rand() % al);
		unsigned char *g = pt_gb_mem(data, n);
		for (int k = 0; k < 3; k++)
		{
			int c = (k == 0) ? (int)(pt_rand() % al) : (k == 1) ? (int)(pt_rand() % 256) + 256 * (int)(pt_rand() % 3 + (int)(pt_rand() % 2)) - 256 * (int)(pt_rand() % 2) : -1 - (int)(pt_rand() % 3);
			const void *e = memchr(data, c, n);
			CALLING("ft_memchr(<%zu bytes>, %d, %zu)", n, c, n);
			const void *a = ft_memchr(g, c, n);
			EXPECT_OFF(g, e ? (const char *)g + ((const char *)e - (const char *)data) : NULL, a,
				"ft_memchr(\"%s\", %d, %zu)", pt_escn(data, n), c, n);
		}
		pt_gb_reset(m);
	}
}

TEST("ft_memchr", "memory", "n smaller than the position of the only match: must return NULL and not read further", T_SPEC)
{
	for (size_t len = 1; len <= 64; len++)
		for (size_t pos = 0; pos < len; pos++)
		{
			size_t m = pt_gb_mark();
			unsigned char *b = pt_gb_alloc(len);
			memset(b, 'x', len);
			b[pos] = 'Z';
			CALLING("ft_memchr(<%zu bytes>, 'Z', %zu)", len, pos);
			EXPECT_OFF((char *)b, NULL, ft_memchr(b, 'Z', pos), "ft_memchr(buf with 'Z' at index %zu, 'Z', n=%zu)", pos, pos);
			EXPECT_OFF((char *)b, (char *)b + pos, ft_memchr(b, 'Z', pos + 1), "ft_memchr(buf with 'Z' at index %zu, 'Z', n=%zu)", pos, pos + 1);
			pt_gb_reset(m);
		}
}

/* ================================================================ memcmp */
TEST("ft_memcmp", "memory", "exhaustive: all pairs of buffers of length 0..3 over {0x00, 0x01, 0x80, 0xFF}", T_SPEC)
{
	static const unsigned char al[4] = {0x00, 0x01, 0x80, 0xFF};
	unsigned char a[4], b[4];

	for (size_t n = 0; n <= 3; n++)
		for (size_t i = 0; i < rf_pow(4, n); i++)
			for (size_t j = 0; j < rf_pow(4, n); j++)
			{
				size_t m = pt_gb_mark(), x = i, y = j;
				for (size_t k = 0; k < n; k++, x /= 4, y /= 4)
				{
					a[k] = al[x % 4];
					b[k] = al[y % 4];
				}
				void *ga = pt_gb_mem(a, n), *gb = pt_gb_mem(b, n);
				CALLING("ft_memcmp(\"%s\", \"%s\", %zu)", pt_escn(a, n), pt_escn(b, n), n);
				EXPECT_SIGN(memcmp(a, b, n), ft_memcmp(ga, gb, n), "ft_memcmp(\"%s\", \"%s\", %zu)", pt_escn(a, n), pt_escn(b, n), n);
				pt_gb_reset(m);
			}
}

TEST("ft_memcmp", "memory", "fuzz: 3000 long buffers (n up to 6000), one differing byte at a random/last position; bytes past n ignored", T_SPEC)
{
	static unsigned char a[6100], b[6100];

	for (int t = 0; t < 3000; t++)
	{
		size_t n = 1 + pt_rand() % 6000, m = pt_gb_mark();
		for (size_t i = 0; i < n + 1; i++)
			a[i] = b[i] = (unsigned char)pt_rand();
		size_t pos = (t % 3 == 0) ? n - 1 : pt_rand() % n;
		unsigned char alt = (unsigned char)pt_rand();
		b[pos] = alt;
		b[n] ^= 0xFF;
		char *ga = pt_gb_mem(a, n), *gb = pt_gb_mem(b, n);
		CALLING("ft_memcmp(<%zu bytes>, <%zu bytes>, %zu)", n, n, n);
		EXPECT_SIGN(memcmp(a, b, n), ft_memcmp(ga, gb, n), "ft_memcmp(<%zu bytes>, <%zu bytes differing at %zu: 0x%02x vs 0x%02x>, %zu)", n, n, pos, a[pos], alt, n);
		EXPECT_INT(0, ft_memcmp(a, b, pos), "ft_memcmp(.., .., n = index of the first difference = %zu)", pos);
		pt_gb_reset(m);
	}
}

/* ================================================================ calloc */
TEST("ft_calloc", "memory", "exhaustive: every (nmemb, size) in 0..24 x 0..24: zeroed, non-NULL, unique when empty", T_SPEC)
{
	void *keep[3];

	HINT("zero-sized requests must return a unique, freeable, non-NULL pointer (subject rule)");
	for (size_t a = 0; a <= 24; a++)
		for (size_t b = 0; b <= 24; b++)
		{
			CALLING("ft_calloc(%zu, %zu)", a, b);
			unsigned char *r = ft_calloc(a, b);
			EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(%zu, %zu)", a, b);
			for (size_t i = 0; i < a * b; i++)
				EXPECT_INT(0, r[i], "ft_calloc(%zu, %zu) byte %zu", a, b, i);
			if (!a || !b)
			{
				keep[0] = ft_calloc(a, b);
				keep[1] = ft_calloc(a, b);
				keep[2] = r;
				EXPECT_TRUE(keep[0] && keep[1] && keep[0] != keep[1] && keep[0] != keep[2] && keep[1] != keep[2],
					"three distinct non-NULL pointers", "NULL or duplicate pointers", "ft_calloc(%zu, %zu) called 3 times", a, b);
				free(keep[0]);
				free(keep[1]);
			}
			free(r);
		}
}

TEST("ft_calloc", "memory", "overflow / impossible sizes return NULL: (SIZE_MAX,SIZE_MAX), (1,SIZE_MAX), (2^32,2^32), (2^16,2^48) ...", T_SPEC)
{
	static const size_t p[][2] = {{(size_t)-1, (size_t)-1}, {1, (size_t)-1}, {(size_t)-1, 1}, {1UL << 32, 1UL << 32},
		{1UL << 16, 1UL << 48}, {(size_t)-1 / 2, 3}, {3, (size_t)-1 / 2}, {(size_t)-1 / 3 + 1, 3}, {(size_t)-1, 2}};

	HINT("check nmemb != 0 && size > SIZE_MAX / nmemb before multiplying; handle malloc returning NULL");
	for (size_t k = 0; k < sizeof(p) / sizeof(p[0]); k++)
	{
		CALLING("ft_calloc(%zu, %zu)", p[k][0], p[k][1]);
		void *r = ft_calloc(p[k][0], p[k][1]);
		EXPECT_TRUE(r == NULL, "NULL", "a non-NULL pointer", "ft_calloc(%zu, %zu)", p[k][0], p[k][1]);
	}
}

TEST("ft_calloc", "memory", "fuzz: 20000 calloc/free cycles with random sizes; every byte zero (heap is poisoned), nothing leaks", T_SPEC)
{
	for (int t = 0; t < 20000; t++)
	{
		size_t a = pt_rand() % 40, b = pt_rand() % 40;
		CALLING("ft_calloc(%zu, %zu)", a, b);
		unsigned char *r = ft_calloc(a, b);
		EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(%zu, %zu)", a, b);
		for (size_t i = 0; i < a * b; i++)
			EXPECT_INT(0, r[i], "ft_calloc(%zu, %zu) byte %zu", a, b, i);
		memset(r, 0xEE, a * b);
		free(r);
	}
}

static size_t g_ca, g_cb2;
static void scen_calloc_x(void)
{
	CALLING("ft_calloc(%zu, %zu)", g_ca, g_cb2);
	ARM();
	unsigned char *r = ft_calloc(g_ca, g_cb2);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_calloc(%zu, %zu) with malloc failing", g_ca, g_cb2);
		return ;
	}
	EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_calloc(%zu, %zu)", g_ca, g_cb2);
	for (size_t i = 0; i < g_ca * g_cb2; i++)
		EXPECT_INT(0, r[i], "ft_calloc(%zu, %zu) byte %zu", g_ca, g_cb2, i);
	free(r);
}
TEST("ft_calloc", "memory", "malloc failure sweep over (0,0) (1,1) (10,4) (100,100) (7,0) [inject]", T_SPEC)
{
	static const size_t p[][2] = {{0, 0}, {1, 1}, {10, 4}, {100, 100}, {7, 0}};

	for (size_t k = 0; k < 5; k++)
	{
		g_ca = p[k][0];
		g_cb2 = p[k][1];
		pt_inject_sweep(scen_calloc_x);
	}
}

/* ================================================================ strdup */
TEST("ft_strdup", "memory", "fuzz: 3000 random strings (len 0..5000, bytes 1..255): exact copy, own block, independent", T_SPEC)
{
	static char buf[5100];

	for (int t = 0; t < 3000; t++)
	{
		size_t n = pt_rand() % 5001, m = pt_gb_mark();
		rf_rbytes(buf, n);
		buf[n] = 0;
		char *s = pt_gb_str(buf);
		CALLING("ft_strdup(<%zu-byte random string>)", n);
		char *r = ft_strdup(s);
		EXPECT_TRUE(r != NULL, "non-NULL", "NULL", "ft_strdup(<%zu-byte string>)", n);
		EXPECT_TRUE(r != s, "a new block", "the same pointer", "ft_strdup(<%zu-byte string>)", n);
		EXPECT_INT(n, strlen(r), "strlen(ft_strdup(<%zu-byte string>))", n);
		EXPECT_MEM(buf, r, n + 1, "ft_strdup(<%zu-byte string>) (copy differs / not terminated)", n);
		if (n)
			memset(s, 'Q', n);
		EXPECT_MEM(buf, r, n + 1, "ft_strdup(<%zu-byte string>) after the original was overwritten", n);
		free(r);
		pt_gb_reset(m);
	}
}

TEST("ft_strdup", "memory", "1 MiB string and a string made of every byte value 1..255", T_SPEC)
{
	size_t n = 1 << 20;
	char *big = pt_gb_alloc(n + 1), all[256];

	memset(big, 'k', n);
	big[n] = 0;
	char *r = ft_strdup(big);
	EXPECT_TRUE(r != NULL && !memcmp(r, big, n + 1), "identical copy", "differs or NULL", "ft_strdup(<1 MiB string>)");
	free(r);
	for (int i = 1; i < 256; i++)
		all[i - 1] = (char)i;
	all[255] = 0;
	char *g = pt_gb_str(all);
	r = ft_strdup(g);
	EXPECT_TRUE(r != NULL && !memcmp(r, all, 256), "identical copy", "differs or NULL", "ft_strdup(<all byte values>)");
	free(r);
}

static const char *g_dup_s;
static void scen_strdup_x(void)
{
	char *s = pt_gb_str(g_dup_s);

	CALLING("ft_strdup(\"%s\")", pt_esc(g_dup_s));
	ARM();
	char *r = ft_strdup(s);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_strdup(\"%s\") with malloc failing", pt_esc(g_dup_s));
		return ;
	}
	EXPECT_STR(g_dup_s, r, "ft_strdup(\"%s\")", pt_esc(g_dup_s));
	free(r);
}
TEST("ft_strdup", "memory", "malloc failure sweep over an empty, a short and a 500-char string [inject]", T_SPEC)
{
	static char longs[501];

	memset(longs, 'L', 500);
	g_dup_s = "";
	pt_inject_sweep(scen_strdup_x);
	g_dup_s = "abc";
	pt_inject_sweep(scen_strdup_x);
	g_dup_s = longs;
	pt_inject_sweep(scen_strdup_x);
}
