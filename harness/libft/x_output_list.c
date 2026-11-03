/* Extended suite: put*_fd and the linked-list functions. */
#include "ref.h"
#include <fcntl.h>

/* ================================================================ output */
static void expect_file(int fd, const void *exp, size_t n, const char *call)
{
	size_t got;
	const unsigned char *d = pt_slurp(fd, &got);

	EXPECT_TRUE(got == n, pt_fmt("%zu byte(s) written", n), pt_fmt("%zu byte(s) written", got), "%s", call);
	EXPECT_MEM(exp, d, n, "%s (bytes written differ)", call);
}

TEST("ft_putchar_fd", "output", "all 256 byte values written in order to one file", T_SPEC)
{
	unsigned char all[256];
	int fd = pt_tmpfd();

	for (int c = 0; c < 256; c++)
	{
		all[c] = (unsigned char)c;
		CALLING("ft_putchar_fd(0x%02x, fd)", c);
		ft_putchar_fd((char)c, fd);
	}
	expect_file(fd, all, 256, "ft_putchar_fd(0..255)");
}
TEST("ft_putchar_fd", "output", "uses the fd it is given (fd 101 via dup2): writes 'Q' then NUL, nothing to stdout", T_SPEC)
{
	int fd = pt_tmpfd();

	dup2(fd, 101);
	ft_putchar_fd('Q', 101);
	ft_putchar_fd('\0', 101);
	expect_file(fd, "Q\0", 2, "ft_putchar_fd('Q'), ft_putchar_fd('\\0') on fd 101");
	close(101);
}

TEST("ft_putstr_fd", "output", "fuzz: 300 random strings (len 0..3000, bytes 1..255) each to a fresh file, byte exact", T_SPEC)
{
	static char s[3100];

	for (int t = 0; t < 300; t++)
	{
		size_t n = pt_rand() % 3001, m = pt_gb_mark();
		int fd = pt_tmpfd();
		rf_rbytes(s, n);
		s[n] = 0;
		char *g = pt_gb_str(s);
		CALLING("ft_putstr_fd(<%zu-byte string>, fd)", n);
		ft_putstr_fd(g, fd);
		expect_file(fd, s, n, pt_fmt("ft_putstr_fd(<%zu-byte random string>, fd)", n));
		close(fd);
		pt_gb_reset(m);
	}
}
TEST("ft_putstr_fd", "output", "4 MiB string in one call, and uses the fd it is given (fd 100)", T_SPEC)
{
	size_t n = 4 << 20;
	char *s = pt_gb_alloc(n + 1);
	int fd = pt_tmpfd();

	for (size_t i = 0; i < n; i++)
		s[i] = (char)(1 + (i * 13) % 250);
	s[n] = 0;
	dup2(fd, 100);
	ft_putstr_fd(s, 100);
	expect_file(fd, s, n, "ft_putstr_fd(<4 MiB string>, 100)");
	close(100);
}
TEST("ft_putstr_fd", "output", "consecutive calls append in order (no extra bytes between them)", T_SPEC)
{
	int fd = pt_tmpfd();

	ft_putstr_fd("ab", fd);
	ft_putstr_fd("", fd);
	ft_putstr_fd("cd", fd);
	expect_file(fd, "abcd", 4, "ft_putstr_fd(\"ab\"), (\"\"), (\"cd\")");
}

TEST("ft_putendl_fd", "output", "fuzz: 300 random strings each to a fresh file: content + exactly one '\\n'", T_SPEC)
{
	static char s[3100], exp[3200];

	for (int t = 0; t < 300; t++)
	{
		size_t n = pt_rand() % 3001, m = pt_gb_mark();
		int fd = pt_tmpfd();
		rf_rbytes(s, n);
		s[n] = 0;
		memcpy(exp, s, n);
		exp[n] = '\n';
		char *g = pt_gb_str(s);
		CALLING("ft_putendl_fd(<%zu-byte string>, fd)", n);
		ft_putendl_fd(g, fd);
		expect_file(fd, exp, n + 1, pt_fmt("ft_putendl_fd(<%zu-byte random string>, fd)", n));
		close(fd);
		pt_gb_reset(m);
	}
}
TEST("ft_putendl_fd", "output", "uses the fd it is given (fd 100) and is one logical write sequence for 2 calls", T_SPEC)
{
	int fd = pt_tmpfd();

	dup2(fd, 100);
	ft_putendl_fd("a", 100);
	ft_putendl_fd("", 100);
	expect_file(fd, "a\n\n", 3, "ft_putendl_fd(\"a\", 100), ft_putendl_fd(\"\", 100)");
	close(100);
}

TEST("ft_putnbr_fd", "output", "exhaustive: every integer -20000..20000 appended to one file vs snprintf", T_SPEC)
{
	static char exp[400000];
	int fd = pt_tmpfd();
	size_t o = 0;

	for (int v = -20000; v <= 20000; v++)
	{
		o += snprintf(exp + o, 16, "%d", v);
		CALLING("ft_putnbr_fd(%d, fd)", v);
		ft_putnbr_fd(v, fd);
	}
	expect_file(fd, exp, o, "ft_putnbr_fd(-20000 .. 20000)");
}
TEST("ft_putnbr_fd", "output", "INT_MIN..INT_MIN+500, INT_MAX-500..INT_MAX, powers of ten +-1, and 20000 random values", T_SPEC)
{
	static char exp[400000];
	int fd = pt_tmpfd();
	size_t o = 0;
	long p = 1;

#define PN(v) do { int pt_v = (v); o += snprintf(exp + o, 16, "%d", pt_v); CALLING("ft_putnbr_fd(%d, fd)", pt_v); ft_putnbr_fd(pt_v, fd); } while (0)
	for (int d = 0; d <= 500; d++)
	{
		PN(INT_MIN + d);
		PN(INT_MAX - d);
	}
	for (int k = 0; k <= 9; k++, p *= 10)
		for (long d = -1; d <= 1; d++)
		{
			if (p + d <= INT_MAX) PN((int)(p + d));
			if (-p + d >= INT_MIN) PN((int)(-p + d));
		}
	for (int t = 0; t < 20000; t++)
		PN((int)pt_rand() >> (pt_rand() % 32));
	expect_file(fd, exp, o, "ft_putnbr_fd(extremes + random)");
}
TEST("ft_putnbr_fd", "output", "uses the fd it is given (fd 100)", T_SPEC)
{
	int fd = pt_tmpfd();

	dup2(fd, 100);
	ft_putnbr_fd(-42, 100);
	ft_putnbr_fd(0, 100);
	expect_file(fd, "-420", 4, "ft_putnbr_fd(-42, 100), ft_putnbr_fd(0, 100)");
	close(100);
}

/* ================================================================ lists */
#define MAXD 8192
static int g_del_calls, g_seen[MAXD], g_f_calls, g_order[MAXD];

static void del_free(void *p)
{
	pt_cb_enter();
	if (p && *(int *)p >= 0 && *(int *)p < MAXD)
		g_seen[*(int *)p]++;
	g_del_calls++;
	free(p);
	pt_cb_leave();
}
static int *mk(int v) { int *p = malloc(sizeof(int)); *p = v; return (p); }
static t_list *mknode(int v)
{
	t_list *n = malloc(sizeof(t_list));

	n->content = mk(v);
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
static void reset_counters(void)
{
	memset(g_seen, 0, sizeof(g_seen));
	memset(g_order, 0, sizeof(g_order));
	g_del_calls = g_f_calls = 0;
}

TEST("ft_lstnew", "list", "5000 nodes with odd content pointers ((void*)1, stack, heap, NULL): content kept as is, next NULL", T_SPEC)
{
	static t_list *ns[5000];
	int local = 3;

	for (int i = 0; i < 5000; i++)
	{
		void *c = (i % 4 == 0) ? NULL : (i % 4 == 1) ? (void *)(uintptr_t)(i * 2 + 1) : (i % 4 == 2) ? (void *)&local : (void *)ns;
		CALLING("ft_lstnew(%p)", c);
		ns[i] = ft_lstnew(c);
		EXPECT_TRUE(ns[i] != NULL, "a node", "NULL", "ft_lstnew(%p)", c);
		EXPECT_PTR(c, ns[i]->content, "ft_lstnew(%p)->content", c);
		EXPECT_PTR(NULL, ns[i]->next, "ft_lstnew(%p)->next", c);
		if (i)
			EXPECT_TRUE(ns[i] != ns[i - 1], "distinct nodes", "the same node twice", "ft_lstnew called twice");
	}
	for (int i = 0; i < 5000; i++)
		free(ns[i]);
}
static int g_lnew_n;
static void scen_lstnew_x(void)
{
	int a = 5;

	CALLING("ft_lstnew(&a) [call %d]", g_lnew_n);
	ARM();
	t_list *x = ft_lstnew(&a);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(x == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_lstnew with malloc failing");
		return ;
	}
	EXPECT_PTR(&a, x->content, "ft_lstnew(&a)->content");
	free(x);
}
TEST("ft_lstnew", "list", "malloc failure sweep [inject]", T_SPEC)
{
	pt_inject_sweep(scen_lstnew_x);
}

static int check_model(t_list *head, t_list **model, size_t n, const char *what)
{
	t_list *p = head;

	for (size_t i = 0; i < n; i++, p = p->next)
	{
		EXPECT_TRUE(p == model[i], pt_fmt("node #%zu = %p", i, (void *)model[i]), pt_fmt("node #%zu = %p", i, (void *)p), "%s: list structure", what);
	}
	EXPECT_TRUE(p == NULL, "list ends after the last node", "extra node / missing NULL terminator", "%s: list structure", what);
	return (1);
}

TEST("ft_lstadd_front", "list", "model test: 4000 random add_front/add_back operations checked against an array model", T_SPEC)
{
	static t_list *model[4100];
	t_list *head = NULL;
	size_t n = 0;

	for (int t = 0; t < 4000; t++)
	{
		t_list *nd = mknode(t);
		if (pt_rand() % 2)
		{
			CALLING("ft_lstadd_front(&lst, node) [op %d, size %zu]", t, n);
			ft_lstadd_front(&head, nd);
			memmove(model + 1, model, n * sizeof(*model));
			model[0] = nd;
		}
		else
		{
			CALLING("ft_lstadd_back(&lst, node) [op %d, size %zu]", t, n);
			ft_lstadd_back(&head, nd);
			model[n] = nd;
		}
		n++;
		if (t < 150 || t % 53 == 0 || t == 3999)
			check_model(head, model, n, "add_front/add_back sequence");
	}
	free_list(head);
}

TEST("ft_lstadd_back", "list", "appending 8000 nodes one by one: head stays, order kept, new->next NULL", T_SPEC)
{
	t_list *head = NULL, *first = NULL, *prev = NULL;

	for (int i = 0; i < 8000; i++)
	{
		t_list *nd = mknode(i);
		CALLING("ft_lstadd_back(&lst, node%d)", i);
		ft_lstadd_back(&head, nd);
		if (!i)
			first = nd;
		EXPECT_PTR(first, head, "head after ft_lstadd_back of node%d", i);
		if (prev)
			EXPECT_PTR(nd, prev->next, "node%d->next after add_back of node%d", i - 1, i);
		EXPECT_PTR(NULL, nd->next, "node%d->next after add_back", i);
		prev = nd;
	}
	free_list(head);
}

TEST("ft_lstsize", "list", "grows one node at a time up to 1500; size and last checked at every step", T_SPEC)
{
	t_list *head = NULL, *tail = NULL;

	for (int i = 1; i <= 1500; i++)
	{
		t_list *nd = mknode(i);
		if (tail)
			tail->next = nd;
		else
			head = nd;
		tail = nd;
		CALLING("ft_lstsize(<list of %d>)", i);
		EXPECT_INT(i, ft_lstsize(head), "ft_lstsize(<list of %d nodes>)", i);
	}
	free_list(head);
}
TEST("ft_lstlast", "list", "grows one node at a time up to 1500; last node checked at every step", T_SPEC)
{
	t_list *head = NULL, *tail = NULL;

	for (int i = 1; i <= 1500; i++)
	{
		t_list *nd = mknode(i);
		if (tail)
			tail->next = nd;
		else
			head = nd;
		tail = nd;
		CALLING("ft_lstlast(<list of %d>)", i);
		EXPECT_PTR(tail, ft_lstlast(head), "ft_lstlast(<list of %d nodes>)", i);
	}
	free_list(head);
}

TEST("ft_lstdelone", "list", "random-position deletions: del once with the right content, node freed, a dangling ->next target stays untouched", T_SPEC)
{
	enum { N = 40 };
	t_list *nodes[N], *head;
	int alive[N], live = N;

	HINT("call del(content) then free(node); never touch or free node->next");
	reset_counters();
	head = build(N);
	{
		t_list *p = head;
		for (int i = 0; i < N; i++, p = p->next)
		{
			nodes[i] = p;
			alive[i] = 1;
		}
	}
	while (live > 0)
	{
		int k;
		do k = (int)(pt_rand() % N); while (!alive[k]);
		t_list *n = nodes[k], **pp = &head;
		while (*pp != n)
			pp = &(*pp)->next;
		*pp = n->next;
		int other = -1;
		for (int i = 0; i < N; i++)
			if (alive[i] && i != k && (other < 0 || pt_rand() % 3 == 0))
				other = i;
		n->next = other >= 0 ? nodes[other] : NULL;
		alive[k] = 0;
		live--;
		g_del_calls = 0;
		CALLING("ft_lstdelone(node %d, del) [%d left]", k, live);
		ft_lstdelone(n, del_free);
		EXPECT_INT(1, g_del_calls, "ft_lstdelone(node %d): number of calls to del", k);
		EXPECT_INT(1, g_seen[k], "ft_lstdelone(node %d): del must receive that node's content", k);
		t_list *p = head;
		for (int i = 0; i < N; i++)
			if (alive[i])
			{
				EXPECT_TRUE(p == nodes[i], "remaining nodes untouched", "a remaining node was freed/modified", "after ft_lstdelone(node %d)", k);
				EXPECT_INT(i, *(int *)p->content, "content of node %d after ft_lstdelone(node %d)", i, k);
				p = p->next;
			}
	}
	free_list(head);
}

TEST("ft_lstclear", "list", "fuzz: 300 lists of random size 0..300: del once per node, nothing leaks, *lst == NULL", T_SPEC)
{
	for (int t = 0; t < 300; t++)
	{
		int n = (int)(pt_rand() % 301);
		t_list *l = build(n);
		reset_counters();
		CALLING("ft_lstclear(&lst /* %d nodes */, del)", n);
		ft_lstclear(&l, del_free);
		EXPECT_INT(n, g_del_calls, "ft_lstclear(<%d nodes>): number of calls to del", n);
		for (int i = 0; i < n; i++)
			EXPECT_INT(1, g_seen[i], "ft_lstclear(<%d nodes>): times del received content #%d", n, i);
		EXPECT_PTR(NULL, l, "*lst after ft_lstclear(<%d nodes>)", n);
	}
}
TEST("ft_lstclear", "list", "5000-node list; nodes whose content is NULL are cleared too", T_SPEC)
{
	t_list *l = build(5000), *p;

	reset_counters();
	for (p = l; p; p = p->next)
		if (*(int *)p->content % 2)
		{
			free(p->content);
			p->content = NULL;
		}
	g_del_calls = 0;
	CALLING("ft_lstclear(&lst /* 5000 nodes, half with NULL content */, del)");
	ft_lstclear(&l, del_free);
	EXPECT_INT(5000, g_del_calls, "ft_lstclear: number of calls to del (also called for NULL contents)");
	EXPECT_PTR(NULL, l, "*lst after ft_lstclear");
}

static void f_inc(void *p)
{
	if (g_f_calls < MAXD)
		g_order[g_f_calls] = *(int *)p;
	g_f_calls++;
	*(int *)p += 1000;
}
TEST("ft_lstiter", "list", "fuzz: 300 lists of size 0..300: f once per node in order, edits kept, structure unchanged", T_SPEC)
{
	for (int t = 0; t < 300; t++)
	{
		int n = (int)(pt_rand() % 301);
		t_list *l = build(n), *p, *keep[301];
		int i = 0;
		reset_counters();
		for (p = l; p; p = p->next)
			keep[i++] = p;
		CALLING("ft_lstiter(<list of %d>, f)", n);
		ft_lstiter(l, f_inc);
		EXPECT_INT(n, g_f_calls, "ft_lstiter(<%d nodes>): number of calls to f", n);
		for (i = 0, p = l; i < n; i++, p = p->next)
		{
			EXPECT_INT(i, g_order[i], "ft_lstiter(<%d nodes>): content seen at call #%d (order)", n, i);
			EXPECT_PTR(keep[i], p, "ft_lstiter(<%d nodes>): node %d (structure changed)", n, i);
			EXPECT_INT(i + 1000, *(int *)p->content, "ft_lstiter(<%d nodes>): content of node %d after the call", n, i);
		}
		free_list(l);
	}
}

static int g_mapmode, g_fcnt;
static void *f_map(void *p)
{
	int *r = NULL;

	if (g_f_calls < MAXD)
		g_order[g_f_calls] = *(int *)p;
	g_f_calls++;
	if (g_mapmode == 1)
		return (NULL);
	pt_cb_enter();
	r = malloc(sizeof(int));
	pt_cb_leave();
	*r = 2 * *(int *)p + 1;
	return (r);
}

TEST("ft_lstmap", "list", "fuzz: 300 lists of size 0..200: values, order, fresh nodes, result stays valid after the original is freed", T_SPEC)
{
	for (int t = 0; t < 300; t++)
	{
		int n = 1 + (int)(pt_rand() % 200);
		t_list *l = build(n), *r, *p, *keep[201];
		int i = 0;
		g_mapmode = 0;
		reset_counters();
		for (p = l; p; p = p->next)
			keep[i++] = p;
		CALLING("ft_lstmap(<list of %d>, f, del)", n);
		r = ft_lstmap(l, f_map, del_free);
		EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap(<list of %d>, f, del)", n);
		EXPECT_INT(n, g_f_calls, "ft_lstmap(<%d nodes>): number of calls to f", n);
		EXPECT_INT(0, g_del_calls, "ft_lstmap(<%d nodes>): number of calls to del when nothing fails", n);
		for (i = 0, p = l; i < n; i++, p = p->next)
		{
			EXPECT_PTR(keep[i], p, "original node %d (structure changed)", i);
			EXPECT_INT(g_order[i], *(int *)p->content, "f must be applied in order: call #%d", i);
		}
		free_list(l);
		for (i = 0, p = r; i < n; i++, p = p->next)
		{
			EXPECT_TRUE(p != NULL, "n nodes", pt_fmt("list ends after %d node(s)", i), "ft_lstmap(<%d nodes>) result length", n);
			EXPECT_INT(2 * i + 1, *(int *)p->content, "ft_lstmap(<%d nodes>) result content #%d", n, i);
		}
		EXPECT_PTR(NULL, p, "ft_lstmap(<%d nodes>): node after the last one", n);
		free_list(r);
	}
}
TEST("ft_lstmap", "list", "f returning NULL contents is fine: n nodes with NULL content, del never called", T_SPEC)
{
	t_list *l = build(25), *r, *p;
	int i;

	reset_counters();
	g_mapmode = 1;
	r = ft_lstmap(l, f_map, del_free);
	EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap(<list of 25>, f returning NULL, del)");
	EXPECT_INT(0, g_del_calls, "number of calls to del");
	for (i = 0, p = r; i < 25; i++, p = p->next)
	{
		EXPECT_TRUE(p != NULL, "25 nodes", pt_fmt("list ends after %d", i), "ft_lstmap result length");
		EXPECT_PTR(NULL, p->content, "result node %d content", i);
	}
	EXPECT_PTR(NULL, p, "node after the 25th");
	for (p = r; p; )
	{
		t_list *nx = p->next;
		free(p);
		p = nx;
	}
	free_list(l);
}

static int g_ln;
static void scen_lstmap_x(void)
{
	t_list *l = build(g_ln);

	g_mapmode = 0;
	reset_counters();
	CALLING("ft_lstmap(<list of %d>, f, del)", g_ln);
	ARM();
	t_list *r = ft_lstmap(l, f_map, del_free);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when a node allocation fails", "a non-NULL list", "ft_lstmap(<list of %d>, f, del) with malloc failing", g_ln);
		EXPECT_INT(g_f_calls, g_del_calls, "ft_lstmap(<list of %d>): del calls vs results produced by f", g_ln);
		free_list(l);
		return ;
	}
	free_list(r);
	free_list(l);
}
TEST("ft_lstmap", "list", "malloc failure sweep over list sizes 1..7: NULL, every f result released through del, no leaks [inject]", T_SPEC)
{
	HINT("on failure: del(content) for the value f just produced, free the nodes already built, return NULL");
	for (g_ln = 1; g_ln <= 7; g_ln++)
		pt_inject_sweep(scen_lstmap_x);
}

TEST("ft_lstadd_front", "list", "200000 add_front calls, then size/last: no recursion or quadratic behaviour [stress]", T_CONV)
{
	t_list *head = NULL, *first = NULL;

	for (int i = 0; i < 200000; i++)
	{
		t_list *nd = mknode(i);
		if (!i)
			first = nd;
		ft_lstadd_front(&head, nd);
	}
	EXPECT_INT(200000, ft_lstsize(head), "ft_lstsize after 200000 ft_lstadd_front");
	EXPECT_PTR(first, ft_lstlast(head), "ft_lstlast after 200000 ft_lstadd_front");
	free_list(head);
}
TEST("ft_lstclear", "list", "200000-node list: no stack overflow (recursion) [stress]", T_CONV)
{
	t_list *l = build(200000);

	reset_counters();
	ft_lstclear(&l, del_free);
	EXPECT_PTR(NULL, l, "*lst after ft_lstclear(<200000 nodes>)");
}
TEST("ft_lstmap", "list", "30000-node list (large input; quadratic add_back-based maps are fine) [stress]", T_CONV)
{
	t_list *l = build(30000), *r;

	g_mapmode = 0;
	reset_counters();
	r = ft_lstmap(l, f_map, del_free);
	EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap(<30000 nodes>)");
	free_list(l);
	free_list(r);
}
