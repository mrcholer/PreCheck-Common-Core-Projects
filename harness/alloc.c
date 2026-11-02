/* Instrumented allocator (linked with --wrap), guarded buffers, failure-injection sweep. */
#include "core.h"
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>

void	*__real_malloc(size_t n);
void	__real_free(void *p);
void	*__wrap_malloc(size_t n);
void	__wrap_free(void *p);
void	*__wrap_calloc(size_t n, size_t m);

#define PG 4096UL
#define TAB_BITS 20
#define TAB_SIZE (1UL << TAB_BITS)
#define GUARD_LIMIT 8000
#define K_GUARD 0
#define K_CANARY 1
#define K_PLAIN 2

int			pt_fail_at;
int			pt_cnt;
static int	g_armed, g_cb, g_fired;

typedef struct
{
	void			*ptr;
	size_t			size;
	void			*base;
	size_t			maplen;
	unsigned		gen;
	unsigned char	state;  /* 0 empty, 1 live, 2 freed */
	unsigned char	kind;
}	t_blk;

static t_blk		*g_tab;
static unsigned		g_gen = 1;
static size_t		g_live, g_guard_total;

void pt_mem_init(void)
{
	g_tab = mmap(NULL, TAB_SIZE * sizeof(t_blk), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

void pt_mem_reset(void)
{
	g_gen++;
	g_live = 0;
	g_guard_total = 0;
	g_armed = g_cb = g_fired = 0;
}

void pt_arm(void) { g_armed = 1; }
void pt_disarm(void) { g_armed = 0; }
int pt_fired(void) { return (g_fired); }
void pt_cb_enter(void) { g_cb++; }
void pt_cb_leave(void) { g_cb--; }

static t_blk *slot(void *p, int create)
{
	size_t	i, n;
	t_blk	*e;

	i = (size_t)(((uint64_t)(uintptr_t)p >> 4) * 11400714819323198485ULL >> (64 - TAB_BITS));
	for (n = 0; n < TAB_SIZE; n++, i = (i + 1) & (TAB_SIZE - 1))
	{
		e = &g_tab[i];
		if (e->gen != g_gen || !e->state)
		{
			if (!create)
				return (NULL);
			memset(e, 0, sizeof(*e));
			e->gen = g_gen;
			return (e);
		}
		if (e->ptr == p)
			return (e);
	}
	return (NULL);
}

static void mem_fail(const char *msg)
{
	pt_finish(PT_RC_MEMORY, pt_sh->call, "valid memory usage", msg, pt_sh->hint);
}

/* A block of n bytes whose last byte touches a PROT_NONE page; a PROT_NONE page precedes it too. */
static void *guard_map(size_t n, void **base, size_t *maplen)
{
	size_t	data, total;
	char	*m;

	data = (n + PG - 1) / PG * PG;
	if (data == 0)
		data = PG;
	total = data + 2 * PG;
	m = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (m == MAP_FAILED)
		return (NULL);
	mprotect(m, PG, PROT_NONE);
	mprotect(m + PG + data, PG, PROT_NONE);
	*base = m;
	*maplen = total;
	return (m + PG + data - n);
}

void *__wrap_malloc(size_t n)
{
	t_blk	*e;
	void	*p, *base = NULL;
	size_t	maplen = 0, pad;
	int		kind;

	if (g_armed && !g_cb)
	{
		pt_cnt++;
		pt_sh->total_allocs = pt_cnt;
		if (pt_fail_at && pt_cnt == pt_fail_at)
		{
			g_fired = 1;
			return (NULL);
		}
	}
	if (n > (1UL << 30))
		return (NULL);
	kind = pt_passthru ? K_PLAIN : K_GUARD;
	p = NULL;
	if (kind == K_GUARD && g_guard_total < GUARD_LIMIT)
	{
		p = guard_map(n, &base, &maplen);
		g_guard_total++;
		if (p)
		{
			pad = (size_t)((char *)p - ((char *)base + PG));
			if (pad > 16)
				pad = 16;
			memset((char *)p - pad, 0x5C, pad);
			memset(p, 0xA5, n);
		}
	}
	if (!p && kind != K_PLAIN)
	{
		kind = K_CANARY;
		base = __real_malloc(n + 48);
		if (!base)
			return (NULL);
		memset(base, 0x5C, 16);
		p = (char *)base + 16;
		memset(p, 0xA5, n);
		memset((char *)p + n, 0x5C, 16);
	}
	if (!p)
	{
		kind = K_PLAIN;
		p = __real_malloc(n ? n : 1);
		if (!p)
			return (NULL);
	}
	e = slot(p, 1);
	if (!e)
		return (p);
	e->ptr = p; e->size = n; e->base = base; e->maplen = maplen; e->state = 1; e->kind = kind;
	g_live++;
	return (p);
}

static void check_block(t_blk *e)
{
	size_t			i, pad;
	unsigned char	*u = e->ptr;

	if (e->kind == K_GUARD)
	{
		pad = (size_t)((char *)e->ptr - ((char *)e->base + PG));
		if (pad > 16)
			pad = 16;
		for (i = 1; i <= pad; i++)
			if (u[-(long)i] != 0x5C)
				mem_fail(pt_fmt("write %zu byte(s) before the start of a %zu-byte block (buffer underflow)", i, e->size));
	}
	else if (e->kind == K_CANARY)
	{
		for (i = 1; i <= 16; i++)
			if (u[-(long)i] != 0x5C)
				mem_fail(pt_fmt("write %zu byte(s) before the start of a %zu-byte block (buffer underflow)", i, e->size));
		for (i = 0; i < 16; i++)
			if (u[e->size + i] != 0x5C)
				mem_fail(pt_fmt("write %zu byte(s) past the end of a %zu-byte block (buffer overflow)", i + 1, e->size));
	}
}

void __wrap_free(void *p)
{
	t_blk	*e;

	if (!p)
		return ;
	e = slot(p, 0);
	if (!e)
		mem_fail("free() of a pointer malloc never returned (string literal? stack buffer? pointer moved off the start of its block?)");
	if (e->state == 2)
		mem_fail("double free (block already freed)");
	check_block(e);
	e->state = 2;
	g_live--;
	if (e->kind == K_GUARD)
		mprotect(e->base, e->maplen, PROT_NONE);
	else if (e->kind == K_CANARY)
	{
		memset(p, 0xDD, e->size);
		__real_free(e->base);
	}
	else
		__real_free(p);
}

void *__wrap_calloc(size_t n, size_t m)
{
	void	*p;

	if (m && n > SIZE_MAX / m)
		return (NULL);
	p = __wrap_malloc(n * m);
	if (p)
		memset(p, 0, n * m);
	return (p);
}

int pt_mem_leaks(char *buf, size_t n)
{
	size_t	i, cnt = 0, bytes = 0, o = 0;
	t_blk	*e;
	char	sample[3][64];

	if (!g_live)
		return (0);
	for (i = 0; i < TAB_SIZE; i++)
	{
		e = &g_tab[i];
		if (e->gen != g_gen || e->state != 1)
			continue ;
		if (cnt < 3)
			snprintf(sample[cnt], 64, "%zu B", e->size);
		cnt++;
		bytes += e->size;
	}
	o = snprintf(buf, n, "%zu block(s) never freed, %zu byte(s) total", cnt, bytes);
	for (i = 0; i < cnt && i < 3 && o < n; i++)
		o += snprintf(buf + o, n - o, "%s%s", i ? ", " : " (", sample[i]);
	if (cnt && o < n)
		snprintf(buf + o, n - o, "%s)", cnt > 3 ? ", ..." : "");
	return (1);
}

/* ------------------------------------------------------------ guarded test buffers */
typedef struct { void *base; size_t len; } t_gb;
static t_gb		g_gb[4096];
static size_t	g_ngb;

size_t pt_gb_mark(void) { return (g_ngb); }

void pt_gb_reset(size_t mark)
{
	while (g_ngb > mark)
	{
		g_ngb--;
		munmap(g_gb[g_ngb].base, g_gb[g_ngb].len);
	}
}

void *pt_gb_alloc(size_t n)
{
	void	*base, *p;
	size_t	len;

	if (g_ngb >= 4096)
		pt_finish(PT_RC_FAIL, "(harness)", "", "internal error: too many guarded buffers", "");
	p = guard_map(n, &base, &len);
	if (!p)
		pt_finish(PT_RC_FAIL, "(harness)", "", "internal error: cannot map guarded buffer", "");
	g_gb[g_ngb].base = base;
	g_gb[g_ngb].len = len;
	g_ngb++;
	memset(p, 0xCD, n);
	return (p);
}

char *pt_gb_str(const char *s)
{
	size_t	n = strlen(s);
	char	*p = pt_gb_alloc(n + 1);

	memcpy(p, s, n + 1);
	return (p);
}

void *pt_gb_mem(const void *src, size_t n)
{
	void	*p = pt_gb_alloc(n);

	if (n)
		memcpy(p, src, n);
	return (p);
}

t_pt_cbuf pt_cb_new(size_t n, size_t off, int fill)
{
	t_pt_cbuf	c;
	size_t		total = n + off + 64;

	c.raw = pt_gb_alloc(total);
	memset(c.raw, 0x5C, total);
	c.p = c.raw + 32 + off;
	memset(c.p, fill, n);
	c.n = n;
	c.off = off;
	return (c);
}

void pt_cb_check(t_pt_cbuf *c, const char *call)
{
	size_t	i, front = 32 + c->off, total = c->n + c->off + 64;

	for (i = 0; i < front; i++)
		if (c->raw[i] != 0x5C)
			pt_fail3(call, "no write before the start of the buffer", pt_fmt("byte at offset -%zu was modified", front - i));
	for (i = front + c->n; i < total; i++)
		if (c->raw[i] != 0x5C)
			pt_fail3(call, "no write past the end of the buffer", pt_fmt("byte at offset +%zu past the end was modified", i - front - c->n));
}

/* ------------------------------------------------------------ allocation-failure sweep */
static void relay(int s, char *msg, int k, int n)
{
	char	*f[4] = {msg, (char *)"", (char *)"", (char *)""};
	char	*q;
	int		rc, i;

	rc = s == S_LEAK ? PT_RC_LEAK : s == S_MEMORY ? PT_RC_MEMORY : s == S_CRASH ? PT_RC_CRASH
		: s == S_TIMEOUT ? PT_RC_TIMEOUT : s == S_WARN ? PT_RC_STRAY : PT_RC_FAIL;
	q = msg;
	for (i = 0; i < 3; i++)
	{
		q = strchr(q, '\x1f');
		if (!q)
			break ;
		*q++ = 0;
		f[i + 1] = q;
	}
	if (k > 0)
		pt_finish(rc, pt_fmt("%s  [malloc #%d of %d made to fail]", f[0], k, n), f[1], f[2], f[3]);
	pt_finish(rc, f[0], f[1], f[2], f[3]);
}

void pt_inject_sweep(void (*scen)(void))
{
	char	msg[4096];
	int		s, n, k;

	pt_sh->total_allocs = 0;
	s = pt_run_child(scen, 0, msg, sizeof(msg));
	if (s != S_PASS)
		relay(s, msg, 0, 0);
	n = pt_sh->total_allocs;
	for (k = 1; k <= n; k++)
	{
		s = pt_run_child(scen, k, msg, sizeof(msg));
		if (s != S_PASS)
			relay(s, msg, k, n);
	}
}
