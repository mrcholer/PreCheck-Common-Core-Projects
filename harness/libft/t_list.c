#include "../core.h"
#include "proto.h"

#define MAXD 4096
static int		g_del_calls;
static int		g_seen[MAXD];
static int		g_f_calls;
static int		g_order[MAXD];

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

static t_list *build(int n)
{
	t_list *head = NULL, **tail = &head;

	for (int i = 0; i < n; i++)
	{
		*tail = malloc(sizeof(t_list));
		(*tail)->content = mk(i);
		(*tail)->next = NULL;
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
	g_del_calls = 0;
	g_f_calls = 0;
}

/* ---------------------------------------------------------------- lstnew */
TEST("ft_lstnew", "list", "content stored by pointer (not copied), next == NULL, distinct nodes", T_SPEC)
{
	int a = 1, b = 2;

	CALLING("ft_lstnew(&a)");
	t_list *x = ft_lstnew(&a), *y = ft_lstnew(&b);
	EXPECT_TRUE(x && y, "two nodes", "NULL", "ft_lstnew(&a), ft_lstnew(&b)");
	EXPECT_PTR(&a, x->content, "ft_lstnew(&a)->content (must be the pointer itself)");
	EXPECT_PTR(NULL, x->next, "ft_lstnew(&a)->next");
	EXPECT_PTR(&b, y->content, "ft_lstnew(&b)->content");
	EXPECT_TRUE(x != y, "distinct nodes", "same node", "ft_lstnew twice");
	free(x);
	free(y);
}

TEST("ft_lstnew", "list", "NULL content is accepted", T_SPEC)
{
	t_list *x = ft_lstnew(NULL);

	EXPECT_TRUE(x != NULL, "a node", "NULL", "ft_lstnew(NULL)");
	EXPECT_PTR(NULL, x->content, "ft_lstnew(NULL)->content");
	EXPECT_PTR(NULL, x->next, "ft_lstnew(NULL)->next");
	free(x);
}

static void scen_lstnew(void)
{
	int a = 5;

	CALLING("ft_lstnew(&a)");
	ARM();
	t_list *x = ft_lstnew(&a);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(x == NULL, "NULL when malloc fails", "non-NULL pointer", "ft_lstnew with malloc failing");
		return ;
	}
	free(x);
}
TEST("ft_lstnew", "list", "malloc failure is reported with NULL [inject]", T_SPEC)
{
	pt_inject_sweep(scen_lstnew);
}

/* ---------------------------------------------------------------- add_front / add_back */
TEST("ft_lstadd_front", "list", "empty list, then several nodes: head updated, order reversed", T_SPEC)
{
	t_list *l = NULL, *nodes[5];

	for (int i = 0; i < 5; i++)
	{
		nodes[i] = malloc(sizeof(t_list));
		nodes[i]->content = mk(i);
		nodes[i]->next = NULL;
		CALLING("ft_lstadd_front(&lst, node%d)", i);
		ft_lstadd_front(&l, nodes[i]);
		EXPECT_PTR(nodes[i], l, "head after ft_lstadd_front(&lst, node%d)", i);
		EXPECT_PTR(i ? nodes[i - 1] : NULL, nodes[i]->next, "node%d->next after add_front", i);
	}
	free_list(l);
}

TEST("ft_lstadd_back", "list", "empty list, then several nodes: appended in order", T_SPEC)
{
	t_list *l = NULL, *nodes[5];

	for (int i = 0; i < 5; i++)
	{
		nodes[i] = malloc(sizeof(t_list));
		nodes[i]->content = mk(i);
		nodes[i]->next = NULL;
		CALLING("ft_lstadd_back(&lst, node%d)", i);
		ft_lstadd_back(&l, nodes[i]);
		EXPECT_PTR(nodes[0], l, "head after ft_lstadd_back(&lst, node%d)", i);
		if (i)
			EXPECT_PTR(nodes[i], nodes[i - 1]->next, "node%d->next after add_back of node%d", i - 1, i);
		EXPECT_PTR(NULL, nodes[i]->next, "node%d->next after add_back (must stay NULL)", i);
	}
	free_list(l);
}

/* ---------------------------------------------------------------- size / last */
TEST("ft_lstsize", "list", "lists of 1, 2, 10, 1000 nodes", T_SPEC)
{
	static const int ns[] = {1, 2, 10, 1000};

	for (int k = 0; k < 4; k++)
	{
		t_list *l = build(ns[k]);
		CALLING("ft_lstsize(<list of %d>)", ns[k]);
		EXPECT_INT(ns[k], ft_lstsize(l), "ft_lstsize(<list of %d nodes>)", ns[k]);
		free_list(l);
	}
}

TEST("ft_lstsize", "list", "NULL list has size 0", T_CONV)
{
	EXPECT_INT(0, ft_lstsize(NULL), "ft_lstsize(NULL)");
}

TEST("ft_lstsize", "list", "200000-node list (no recursion depth problems) [stress]", T_CONV)
{
	t_list *l = build(200000);

	EXPECT_INT(200000, ft_lstsize(l), "ft_lstsize(<list of 200000 nodes>)");
	free_list(l);
}

TEST("ft_lstlast", "list", "returns the real last node for lists of 1, 2, 10, 1000", T_SPEC)
{
	static const int ns[] = {1, 2, 10, 1000};

	for (int k = 0; k < 4; k++)
	{
		t_list *l = build(ns[k]), *last = l;
		while (last->next)
			last = last->next;
		CALLING("ft_lstlast(<list of %d>)", ns[k]);
		EXPECT_PTR(last, ft_lstlast(l), "ft_lstlast(<list of %d nodes>)", ns[k]);
		free_list(l);
	}
}

TEST("ft_lstlast", "list", "NULL list returns NULL", T_CONV)
{
	EXPECT_PTR(NULL, ft_lstlast(NULL), "ft_lstlast(NULL)");
}

/* ---------------------------------------------------------------- delone / clear */
TEST("ft_lstdelone", "list", "del called once with the content, node freed, next node left alone", T_SPEC)
{
	HINT("call del(content) then free(node); never touch node->next");
	t_list *n = malloc(sizeof(t_list)), *other = build(2);

	reset_counters();
	n->content = mk(7);
	n->next = other;
	CALLING("ft_lstdelone(node, del)");
	ft_lstdelone(n, del_free);
	EXPECT_INT(1, g_del_calls, "number of calls to del in ft_lstdelone");
	EXPECT_INT(1, g_seen[7], "del called with the node's content");
	EXPECT_INT(0, *(int *)other->content, "the next node's content after ft_lstdelone (must be untouched)");
	EXPECT_TRUE(other->next != NULL, "next node still linked", "link lost", "ft_lstdelone(node, del): the next node's link");
	free_list(other);
}

TEST("ft_lstclear", "list", "empty list: del never called, pointer stays NULL", T_SPEC)
{
	t_list *l = NULL;

	reset_counters();
	CALLING("ft_lstclear(&lst /* NULL */, del)");
	ft_lstclear(&l, del_free);
	EXPECT_INT(0, g_del_calls, "number of calls to del");
	EXPECT_PTR(NULL, l, "*lst after ft_lstclear");
}

TEST("ft_lstclear", "list", "lists of 1, 2, 10, 1000: del once per node, all nodes freed, *lst == NULL", T_SPEC)
{
	static const int ns[] = {1, 2, 10, 1000};

	HINT("save next before freeing; call del on each content; set *lst = NULL at the end");
	for (int k = 0; k < 4; k++)
	{
		t_list *l = build(ns[k]);
		reset_counters();
		CALLING("ft_lstclear(&lst /* %d nodes */, del)", ns[k]);
		ft_lstclear(&l, del_free);
		EXPECT_INT(ns[k], g_del_calls, "number of calls to del for a %d-node list", ns[k]);
		for (int i = 0; i < ns[k]; i++)
			EXPECT_INT(1, g_seen[i], "number of times del received content #%d", i);
		EXPECT_PTR(NULL, l, "*lst after ft_lstclear");
	}
}

/* ---------------------------------------------------------------- iter / map */
static void f_inc(void *p)
{
	int v = *(int *)p;

	if (g_f_calls < MAXD)
		g_order[g_f_calls] = v;
	g_f_calls++;
	*(int *)p = v + 100;
}

TEST("ft_lstiter", "list", "f applied once per node, in order, and modifications are kept", T_SPEC)
{
	t_list *l = build(6), *p;

	reset_counters();
	CALLING("ft_lstiter(<list of 6>, f)");
	ft_lstiter(l, f_inc);
	EXPECT_INT(6, g_f_calls, "number of calls to f");
	for (int i = 0; i < 6; i++)
		EXPECT_INT(i, g_order[i], "content seen by f at call #%d (order)", i);
	p = l;
	for (int i = 0; i < 6; i++, p = p->next)
		EXPECT_INT(i + 100, *(int *)p->content, "content of node %d after ft_lstiter", i);
	free_list(l);
}

TEST("ft_lstiter", "list", "NULL list: f never called", T_CONV)
{
	reset_counters();
	ft_lstiter(NULL, f_inc);
	EXPECT_INT(0, g_f_calls, "ft_lstiter(NULL, f) number of calls to f");
}

static void *f_dbl(void *p)
{
	int *r;

	pt_cb_enter();
	r = malloc(sizeof(int));
	pt_cb_leave();
	*r = 2 * *(int *)p;
	if (g_f_calls < MAXD)
		g_order[g_f_calls] = *(int *)p;
	g_f_calls++;
	return (r);
}

TEST("ft_lstmap", "list", "new list of f(content): same length/order, original list untouched, del not called", T_SPEC)
{
	t_list *l = build(7), *r, *p;
	t_list *keep[7];
	int i;

	reset_counters();
	for (i = 0, p = l; p; p = p->next)
		keep[i++] = p;
	CALLING("ft_lstmap(<list of 7>, f, del)");
	r = ft_lstmap(l, f_dbl, del_free);
	EXPECT_TRUE(r != NULL, "a new list", "NULL", "ft_lstmap(<list of 7>, f, del)");
	EXPECT_INT(7, g_f_calls, "number of calls to f");
	EXPECT_INT(0, g_del_calls, "number of calls to del when nothing fails");
	for (i = 0, p = r; i < 7; i++, p = p->next)
	{
		EXPECT_TRUE(p != NULL, "7 nodes", pt_fmt("list ends after %d node(s)", i), "ft_lstmap result length");
		EXPECT_INT(2 * i, *(int *)p->content, "content of result node %d", i);
		EXPECT_TRUE(p != keep[i], "new nodes", "the original node was reused", "ft_lstmap result node %d", i);
	}
	EXPECT_PTR(NULL, p, "node after the 7th in the result (list must end)");
	for (i = 0, p = l; i < 7; i++, p = p->next)
	{
		EXPECT_PTR(keep[i], p, "original node %d (list structure changed)", i);
		EXPECT_INT(i, *(int *)p->content, "original content of node %d (must be unchanged)", i);
	}
	free_list(r);
	free_list(l);
}

TEST("ft_lstmap", "list", "f is applied in list order", T_SPEC)
{
	t_list *l = build(5), *r;

	reset_counters();
	r = ft_lstmap(l, f_dbl, del_free);
	for (int i = 0; i < 5; i++)
		EXPECT_INT(i, g_order[i], "content passed to f at call #%d", i);
	free_list(r);
	free_list(l);
}

TEST("ft_lstmap", "list", "NULL list returns NULL", T_CONV)
{
	reset_counters();
	EXPECT_PTR(NULL, ft_lstmap(NULL, f_dbl, del_free), "ft_lstmap(NULL, f, del)");
}

static void scen_lstmap(void)
{
	t_list *l = build(4);

	reset_counters();
	CALLING("ft_lstmap(<list of 4>, f, del)");
	ARM();
	t_list *r = ft_lstmap(l, f_dbl, del_free);
	DISARM();
	if (pt_fired())
	{
		EXPECT_TRUE(r == NULL, "NULL when a node allocation fails", "a non-NULL list", "ft_lstmap(<list of 4>, f, del) with malloc failing");
		EXPECT_INT(g_f_calls, g_del_calls, "calls to del vs results produced by f (every result must be released with del)");
		free_list(l);
		return ;
	}
	free_list(r);
	free_list(l);
}
TEST("ft_lstmap", "list", "failing at every malloc: NULL, results released via del, partial list freed [inject]", T_SPEC)
{
	HINT("on failure: del(content) for the value f just produced, clear the nodes already built, return NULL");
	pt_inject_sweep(scen_lstmap);
}
