/* prtest harness core: shared declarations. */
#ifndef PRTEST_CORE_H
# define PRTEST_CORE_H
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <limits.h>
# include <unistd.h>

/* Tiers: 1 = subject/man page (FAIL), 2 = convention (WARN), 3 = undefined (INFO only) */
# define T_SPEC 1
# define T_CONV 2
# define T_UB   3

# define PT_RC_PASS   0
# define PT_RC_FAIL   10
# define PT_RC_LEAK   11
# define PT_RC_MEMORY 12
# define PT_RC_STRAY  13
# define PT_RC_CRASH  14
# define PT_RC_TIMEOUT 15

typedef void (*pt_test_fn)(void);
void pt_register(const char *func, const char *cat, const char *name, int tier, pt_test_fn fn);

# define PT_CAT2(a, b) a##b
# define PT_CAT(a, b) PT_CAT2(a, b)
# define TEST(func, cat, name, tier) PT_TEST_I(__COUNTER__, func, cat, name, tier)
# define PT_TEST_I(id, func, cat, name, tier) \
	static void PT_CAT(pt_t_, id)(void); \
	__attribute__((constructor)) static void PT_CAT(pt_r_, id)(void) \
	{ pt_register(func, cat, name, tier, PT_CAT(pt_t_, id)); } \
	static void PT_CAT(pt_t_, id)(void)

/* shared state (lives in a MAP_SHARED page so parents can read it) */
typedef struct pt_shared
{
	int		total_allocs;
	char	call[512];
	char	hint[512];
	long	checks;
	int		used_gb;
}	t_pt_shared;

extern t_pt_shared	*pt_sh;
extern int			pt_stress;
extern unsigned		pt_seed;
extern int			pt_timeout;
extern int			pt_passthru;  /* 1: plain tracked malloc (ASan/valgrind mode) */
extern int			pt_cur_tier;
extern int			pt_front;   /* 1: guarded buffers sit at the START of their page (catches underflow) */
extern int			pt_sticky;  /* 1: once malloc #k fails, every later armed malloc fails too */
extern unsigned char	pt_poison;

/* messages */
char		*pt_fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
const char	*pt_esc(const char *s);
const char	*pt_escn(const void *s, size_t n);
void		pt_set_call(const char *f, ...) __attribute__((format(printf, 1, 2)));
void		pt_set_hint(const char *h);
void		pt_fail3(const char *call, const char *exp, const char *act) __attribute__((noreturn));
void		pt_finish(int rc, const char *call, const char *exp, const char *act, const char *hint) __attribute__((noreturn));

# define CALLING(...) pt_set_call(__VA_ARGS__)
# define HINT(s) pt_set_hint(s)

# define EXPECT_INT(e, a, ...) do { pt_sh->checks++; long long pt_e = (long long)(e); long long pt_a = (long long)(a); \
	if (pt_e != pt_a) pt_fail3(pt_fmt(__VA_ARGS__), pt_fmt("%lld", pt_e), pt_fmt("%lld", pt_a)); } while (0)
# define EXPECT_SIGN(e, a, ...) do { pt_sh->checks++; long long pt_e = (long long)(e); long long pt_a = (long long)(a); \
	int pt_se = (pt_e > 0) - (pt_e < 0); int pt_sa = (pt_a > 0) - (pt_a < 0); \
	if (pt_se != pt_sa) pt_fail3(pt_fmt(__VA_ARGS__), pt_fmt("%s", pt_se < 0 ? "negative" : pt_se ? "positive" : "zero"), \
		pt_fmt("%lld (%s)", pt_a, pt_sa < 0 ? "negative" : pt_sa ? "positive" : "zero")); } while (0)
# define EXPECT_STR(e, a, ...) do { pt_sh->checks++; const char *pt_e = (e); const char *pt_a = (a); \
	if ((pt_e == NULL) != (pt_a == NULL) || (pt_e && strcmp(pt_e, pt_a))) \
		pt_fail3(pt_fmt(__VA_ARGS__), pt_e ? pt_fmt("\"%s\"", pt_esc(pt_e)) : "NULL", pt_a ? pt_fmt("\"%s\"", pt_esc(pt_a)) : "NULL"); } while (0)
# define EXPECT_PTR(e, a, ...) do { pt_sh->checks++; const void *pt_e = (e); const void *pt_a = (a); \
	if (pt_e != pt_a) pt_fail3(pt_fmt(__VA_ARGS__), pt_fmt("%p", pt_e), pt_fmt("%p", pt_a)); } while (0)
# define EXPECT_MEM(e, a, n, ...) do { pt_sh->checks++; size_t pt_i = pt_memdiff((e), (a), (n)); \
	if (pt_i != (size_t)-1) pt_fail3(pt_fmt(__VA_ARGS__), \
		pt_fmt("byte %zu = 0x%02x", pt_i, ((const unsigned char *)(e))[pt_i]), \
		pt_fmt("byte %zu = 0x%02x", pt_i, ((const unsigned char *)(a))[pt_i])); } while (0)
# define EXPECT_TRUE(cond, exp, act, ...) do { pt_sh->checks++; if (!(cond)) pt_fail3(pt_fmt(__VA_ARGS__), (exp), (act)); } while (0)

const char	*pt_off(const void *base, const void *p);
size_t	pt_memdiff(const void *a, const void *b, size_t n);
unsigned pt_rand(void);

/* guarded buffers (not tracked as leaks) */
void	*pt_gb_alloc(size_t n);
char	*pt_gb_str(const char *s);
void	*pt_gb_mem(const void *p, size_t n);
char	*pt_gb_ro_str(const char *s);        /* read-only copy (writes fault) */
void	*pt_gb_ro_mem(const void *p, size_t n);
void	pt_gb_protect(void *p);               /* make an existing guarded buffer read-only */
size_t	pt_alloc_size(const void *p);          /* size malloc'd for p, or (size_t)-1 */
size_t	pt_gb_mark(void);             /* release every guarded buffer made after the mark */
void	pt_gb_reset(size_t mark);

typedef struct pt_cbuf { unsigned char *raw; unsigned char *p; size_t n; size_t off; } t_pt_cbuf;
t_pt_cbuf	pt_cb_new(size_t n, size_t off, int fill);
void		pt_cb_check(t_pt_cbuf *c, const char *call);

/* allocator control */
void	pt_arm(void);
void	pt_disarm(void);
int		pt_fired(void);
void	pt_cb_enter(void);
void	pt_cb_leave(void);
void	pt_inject_sweep(void (*scen)(void));
void	pt_mem_init(void);
void	pt_mem_reset(void);
int		pt_mem_leaks(char *buf, size_t n);

# define ARM() pt_arm()
# define DISARM() pt_disarm()

/* process isolation */
enum { S_PASS, S_FAIL, S_WARN, S_CRASH, S_TIMEOUT, S_LEAK, S_MEMORY, S_INFO };
int		pt_run_child(pt_test_fn fn, int fail_at, char *msg, size_t msgsz);

/* temp-file capture for output tests */
int					pt_tmpfd(void);
const unsigned char	*pt_slurp(int fd, size_t *n);

#endif
