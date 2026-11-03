/* Test-only reference models shared by the extended (x_*.c) suites. */
#ifndef PT_REF_H
# define PT_REF_H
# include "../core.h"
# include "proto.h"

# define EXPECT_OFF(base, e, a, ...) do { const char *pt_oe = pt_off((base), (e)); const char *pt_oa = pt_off((base), (a)); \
	if (strcmp(pt_oe, pt_oa)) pt_fail3(pt_fmt(__VA_ARGS__), pt_oe, pt_oa); } while (0)

static inline int rf_alpha(int c) { return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); }
static inline int rf_digit(int c) { return (c >= '0' && c <= '9'); }
static inline int rf_alnum(int c) { return (rf_alpha(c) || rf_digit(c)); }
static inline int rf_ascii(int c) { return (c >= 0 && c <= 127); }
static inline int rf_print(int c) { return (c >= 32 && c <= 126); }
static inline int rf_up(int c) { return ((c >= 'a' && c <= 'z') ? c - 32 : c); }
static inline int rf_low(int c) { return ((c >= 'A' && c <= 'Z') ? c + 32 : c); }

static inline const char *rf_strnstr(const char *h, const char *nd, size_t len)
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

static inline size_t rf_strlcpy(char *dst, const char *src, size_t size)
{
	size_t	sl = strlen(src), n;

	if (size)
	{
		n = sl < size - 1 ? sl : size - 1;
		memcpy(dst, src, n);
		dst[n] = 0;
	}
	return (sl);
}

static inline size_t rf_strlcat(char *dst, const char *src, size_t size)
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

/* fills buf[0..n) with characters of alpha picked by the base-|alpha| digits of idx */
static inline void rf_gen(char *buf, size_t n, size_t idx, const char *alpha)
{
	size_t	a = strlen(alpha);

	for (size_t i = 0; i < n; i++, idx /= a)
		buf[i] = alpha[idx % a];
	buf[n] = 0;
}
static inline size_t rf_pow(size_t a, size_t n) { size_t r = 1; while (n--) r *= a; return (r); }

typedef struct { const char *p; size_t n; } t_tok;

static inline size_t rf_tokens(const char *s, char c, t_tok *t, size_t max)
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

/* random bytes 1..255 */
static inline void rf_rbytes(char *b, size_t n)
{
	for (size_t i = 0; i < n; i++)
		b[i] = (char)(1 + pt_rand() % 255);
}
#endif
