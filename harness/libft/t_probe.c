#include "../core.h"
#include "proto.h"

/* Tier C: undefined behaviour. Only run with --probe-ub; the outcome is informational. */
TEST("ft_strlen", "string", "ft_strlen(NULL)", T_UB)
{
	size_t r = ft_strlen(NULL);

	pt_finish(PT_RC_FAIL, "ft_strlen(NULL)", "(undefined in libc too)", pt_fmt("returned %zu", r), "");
}
TEST("ft_atoi", "convert", "ft_atoi(\"99999999999\") overflow", T_UB)
{
	int r = ft_atoi("99999999999");

	pt_finish(PT_RC_FAIL, "ft_atoi(\"99999999999\")", "(undefined in libc too)", pt_fmt("returned %d", r), "");
}
TEST("ft_memcpy", "memory", "ft_memcpy with overlapping buffers", T_UB)
{
	char b[16] = "abcdefgh";
	ft_memcpy(b + 2, b, 6);
	pt_finish(PT_RC_FAIL, "ft_memcpy(b+2, b, 6) on \"abcdefgh\"", "(undefined in libc too)", pt_fmt("result \"%s\"", pt_esc(b)), "");
}
TEST("ft_putstr_fd", "output", "ft_putstr_fd(NULL, 1)", T_UB)
{
	ft_putstr_fd(NULL, 1);
	pt_finish(PT_RC_FAIL, "ft_putstr_fd(NULL, 1)", "(undefined)", "returned normally", "");
}
