/* Test registry, forked runner, message helpers, main(). */
#include "core.h"
#include "proto.h"
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>

t_pt_shared	*pt_sh;
int			pt_stress;
unsigned	pt_seed = 0x2f9c41aa;
int			pt_timeout = 10;
int			pt_passthru;
int			pt_cur_tier = T_SPEC;
static int	g_resfd = -1;
static int	g_vg;

typedef struct { const char *func, *cat, *name; int tier; pt_test_fn fn; } t_test;
static t_test	*g_tests;
static size_t	g_ntests, g_cap;

void pt_register(const char *func, const char *cat, const char *name, int tier, pt_test_fn fn)
{
	if (g_ntests == g_cap)
	{
		g_cap = g_cap ? g_cap * 2 : 256;
		g_tests = realloc(g_tests, g_cap * sizeof(*g_tests));
	}
	g_tests[g_ntests++] = (t_test){func, cat, name, tier, fn};
}

/* ------------------------------------------------------------ messages */
char *pt_fmt(const char *f, ...)
{
	static char	ring[16][1024];
	static int	idx;
	char		*b;
	va_list		ap;

	b = ring[idx++ & 15];
	va_start(ap, f);
	vsnprintf(b, 1024, f, ap);
	va_end(ap);
	return (b);
}

const char *pt_escn(const void *s, size_t n)
{
	static char			ring[8][400];
	static int			idx;
	char				*b;
	const unsigned char	*p = s;
	size_t				i, o = 0;

	b = ring[idx++ & 7];
	for (i = 0; i < n && o < 380; i++)
	{
		if (i == 60 && n > 70)
		{
			o += snprintf(b + o, 400 - o, "...(%zu more)...", n - 62);
			i = n - 2;
			continue ;
		}
		if (p[i] == '\n') o += snprintf(b + o, 400 - o, "\\n");
		else if (p[i] == '\t') o += snprintf(b + o, 400 - o, "\\t");
		else if (p[i] == '\\') o += snprintf(b + o, 400 - o, "\\\\");
		else if (p[i] == '"') o += snprintf(b + o, 400 - o, "\\\"");
		else if (p[i] < 32 || p[i] >= 127) o += snprintf(b + o, 400 - o, "\\x%02x", p[i]);
		else b[o++] = p[i];
	}
	b[o] = 0;
	return (b);
}

const char *pt_off(const void *base, const void *p)
{
	long	d;

	if (!p)
		return ("NULL");
	d = (const char *)p - (const char *)base;
	if (d < 0 || d > (1L << 26))
		return (pt_fmt("a pointer unrelated to the input (%p)", p));
	return (pt_fmt("s+%ld", d));
}

const char *pt_esc(const char *s)
{
	return (pt_escn(s, strlen(s)));
}

void pt_set_call(const char *f, ...)
{
	va_list	ap;

	va_start(ap, f);
	vsnprintf(pt_sh->call, sizeof(pt_sh->call), f, ap);
	va_end(ap);
}

void pt_set_hint(const char *h)
{
	snprintf(pt_sh->hint, sizeof(pt_sh->hint), "%s", h ? h : "");
}

size_t pt_memdiff(const void *a, const void *b, size_t n)
{
	const unsigned char	*x = a, *y = b;
	size_t				i;

	for (i = 0; i < n; i++)
		if (x[i] != y[i])
			return (i);
	return ((size_t)-1);
}

unsigned pt_rand(void)
{
	static unsigned	st;

	if (!st)
		st = pt_seed ? pt_seed : 1;
	st ^= st << 13;
	st ^= st >> 17;
	st ^= st << 5;
	return (st);
}

void pt_finish(int rc, const char *call, const char *exp, const char *act, const char *hint)
{
	char	buf[3000];
	int		n;

	n = snprintf(buf, sizeof(buf), "%s\x1f%s\x1f%s\x1f%s", call ? call : "", exp ? exp : "", act ? act : "", hint ? hint : "");
	if (g_resfd >= 0)
		(void)!write(g_resfd, buf, n < (int)sizeof(buf) ? (size_t)n : sizeof(buf) - 1);
	_exit(rc);
}

void pt_fail3(const char *call, const char *exp, const char *act)
{
	pt_finish(PT_RC_FAIL, call, exp, act, pt_sh->hint);
}

/* ------------------------------------------------------------ temp files */
int pt_tmpfd(void)
{
	char	path[] = "/tmp/prtest-out-XXXXXX";
	int		fd;

	fd = mkstemp(path);
	if (fd >= 0)
		unlink(path);
	return (fd);
}

const unsigned char *pt_slurp(int fd, size_t *n)
{
	static unsigned char	buf[8 << 20];
	ssize_t					r;

	lseek(fd, 0, SEEK_SET);
	r = read(fd, buf, sizeof(buf));
	*n = r < 0 ? 0 : (size_t)r;
	return (buf);
}

/* ------------------------------------------------------------ isolation */
static void child_setup(int fail_at)
{
	struct rlimit	rl;
	extern int		pt_fail_at;
	extern int		pt_cnt;
	int				fd;

	rl.rlim_cur = rl.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &rl);
	if (!pt_passthru)
	{
		rl.rlim_cur = rl.rlim_max = 6UL << 30;
		setrlimit(RLIMIT_AS, &rl);
		getrlimit(RLIMIT_STACK, &rl);
		rl.rlim_cur = 1UL << 20;   /* 1 MiB: deep recursion (e.g. recursive lstsize on 100000 nodes) crashes */
		setrlimit(RLIMIT_STACK, &rl);
	}
	pt_fail_at = fail_at;
	pt_cnt = 0;
	pt_mem_reset();
	signal(SIGPIPE, SIG_IGN);
	fd = pt_tmpfd();
	if (fd >= 0)
	{
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
	}
	alarm(pt_timeout);
}

int pt_run_child(pt_test_fn fn, int fail_at, char *msg, size_t msgsz)
{
	int		p[2], st, rc;
	pid_t	pid;
	ssize_t	r;
	char	*sig;

	msg[0] = 0;
	if (pipe(p) < 0)
		return (S_FAIL);
	fflush(NULL);
	pid = fork();
	if (pid == 0)
	{
		close(p[0]);
		g_resfd = p[1];
		child_setup(fail_at);
		fn();
		{
			char	b[1024];
			off_t	stray;

			if (pt_mem_leaks(b, sizeof(b)))
				pt_finish(PT_RC_LEAK, pt_sh->call, "every allocation freed", b, pt_sh->hint);
			stray = lseek(1, 0, SEEK_END);
			if (stray > 0)
				pt_finish(PT_RC_STRAY, "", "no output on stdout/stderr", pt_fmt("%lld byte(s) written (leftover debug print?)", (long long)stray), "");
		}
		_exit(0);
	}
	close(p[1]);
	waitpid(pid, &st, 0);
	r = read(p[0], msg, msgsz - 1);
	msg[r > 0 ? r : 0] = 0;
	close(p[0]);
	if (WIFSIGNALED(st))
	{
		int s = WTERMSIG(st);
		sig = s == SIGSEGV ? "segmentation fault" : s == SIGBUS ? "bus error" : s == SIGABRT ? "abort (double free / assertion?)"
			: s == SIGFPE ? "floating point exception" : s == SIGALRM ? "timeout" : s == SIGKILL ? "killed (out of memory?)" : "fatal signal";
		snprintf(msg, msgsz, "%s\x1f%s\x1f%s (signal %d)\x1f%s", pt_sh->call, "no crash", sig, s,
			s == SIGSEGV ? "likely out-of-bounds read/write, NULL deref or stack overflow (guard pages catch overruns)" : s == SIGALRM ? "infinite loop or very slow implementation" : "");
		return (s == SIGALRM ? S_TIMEOUT : S_CRASH);
	}
	rc = WEXITSTATUS(st);
	if (rc == PT_RC_PASS) return (S_PASS);
	if (rc == PT_RC_FAIL) return (S_FAIL);
	if (rc == PT_RC_LEAK) return (S_LEAK);
	if (rc == PT_RC_MEMORY) return (S_MEMORY);
	if (rc == PT_RC_CRASH) return (S_CRASH);
	if (rc == PT_RC_TIMEOUT) return (S_TIMEOUT);
	if (rc == PT_RC_STRAY) return (S_WARN);
	{
		char	lp[512], det[700] = "";
		const char *pre = getenv("PRTEST_LOG");
		int		lf;

		if (pre)
		{
			snprintf(lp, sizeof(lp), "%s.%d", pre, (int)pid);
			lf = open(lp, O_RDONLY);
			if (lf >= 0)
			{
				ssize_t n = read(lf, det, sizeof(det) - 1);
				det[n > 0 ? n : 0] = 0;
				close(lf);
				for (char *q = det; *q; q++)
					if (*q == '\n' || *q == '\t' || *q == '\x1f')
						*q = ' ';
			}
		}
		snprintf(msg, msgsz, "%s\x1f%s\x1f%s\x1f%s", pt_sh->call, "no sanitizer/valgrind error",
			pt_fmt("exit code %d: %s", rc, det), "");
	}
	(void)g_vg;
	return (S_FAIL);
}

/* ------------------------------------------------------------ main */
static int in_list(const char *csv, const char *s)
{
	size_t	l;
	char	*p;

	if (!csv)
		return (1);
	l = strlen(s);
	for (p = (char *)csv; p && *p; )
	{
		if (!strncmp(p, s, l) && (p[l] == ',' || p[l] == 0))
			return (1);
		p = strchr(p, ',');
		if (p)
			p++;
	}
	return (0);
}

static void sanitize_field(char *s)
{
	for (; *s; s++)
		if (*s == '\t' || *s == '\n' || *s == '\r')
			*s = ' ';
}

static void emit(t_test *t, int s, char *msg, int front)
{
	char		*f[4], *q;
	int			n = 0, i;
	const char	*status;

	if (t->tier == T_CONV && s != S_PASS)
		s = S_WARN;
	if (t->tier == T_UB)
		s = S_INFO;
	status = (const char *[]){"PASS", "FAIL", "WARN", "CRASH", "TIMEOUT", "LEAK", "MEMORY", "INFO"}[s];
	f[0] = msg;
	f[1] = f[2] = f[3] = (char *)"";
	for (q = msg; *q && n < 3; q++)
		if (*q == '\x1f')
		{
			*q = 0;
			f[++n] = q + 1;
		}
	for (i = 0; i < 4; i++)
		sanitize_field(f[i]);
	printf("T\t%s\t%s\t%s%s\t%d\t%s\t%s\t%s\t%s\t%s\t%ld\n", t->func, t->cat, t->name, front ? " [front guard]" : "",
		t->tier, status, f[0], f[1], f[2], f[3], pt_sh->checks);
}

int main(int argc, char **argv)
{
	int		front_on = 1;

	const char	*funcs = NULL, *cats = NULL;
	int			list = 0, probe = 0, i;
	char		msg[4096];
	size_t		k;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--list")) list = 1;
		else if (!strcmp(argv[i], "--probe-ub")) probe = 1;
		else if (!strcmp(argv[i], "--stress")) pt_stress = 1;
		else if (!strcmp(argv[i], "--passthrough")) pt_passthru = 1;
		else if (!strcmp(argv[i], "--no-front")) front_on = 0;
		else if (!strcmp(argv[i], "--poison") && i + 1 < argc) pt_poison = (unsigned char)strtoul(argv[++i], NULL, 0);
		else if (!strcmp(argv[i], "--function") && i + 1 < argc) funcs = argv[++i];
		else if (!strcmp(argv[i], "--category") && i + 1 < argc) cats = argv[++i];
		else if (!strcmp(argv[i], "--seed") && i + 1 < argc) pt_seed = (unsigned)strtoul(argv[++i], NULL, 0);
		else if (!strcmp(argv[i], "--timeout") && i + 1 < argc) pt_timeout = atoi(argv[++i]);
	}
	pt_sh = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	pt_mem_init();
	setvbuf(stdout, NULL, _IOLBF, 0);
	for (k = 0; k < g_ntests; k++)
	{
		t_test *t = &g_tests[k];
		int s;

		if (!in_list(funcs, t->func) || !in_list(cats, t->cat))
			continue ;
		if (t->tier == T_UB && !probe)
			continue ;
		if (strstr(t->name, "[stress]") && !pt_stress)
			continue ;
		if (list)
		{
			printf("L\t%s\t%s\t%s\t%d\n", t->func, t->cat, t->name, t->tier);
			continue ;
		}
		if (!pt_fn_present(t->func))
			continue ;
		pt_cur_tier = t->tier;
		pt_sh->call[0] = 0;
		pt_sh->hint[0] = 0;
		pt_sh->checks = 0;
		pt_sh->used_gb = 0;
		pt_front = 0;
		s = pt_run_child(t->fn, 0, msg, sizeof(msg));
		emit(t, s, msg, 0);
		if (s == S_PASS && front_on && pt_sh->used_gb && t->tier != T_UB)
		{
			pt_sh->call[0] = 0;
			pt_sh->hint[0] = 0;
			pt_sh->checks = 0;
			pt_front = 1;
			s = pt_run_child(t->fn, 0, msg, sizeof(msg));
			pt_front = 0;
			emit(t, s, msg, 1);
		}
	}
	return (0);
}
