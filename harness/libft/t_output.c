#include "../core.h"
#include "proto.h"
#include <fcntl.h>

static void expect_file(int fd, const void *exp, size_t n, const char *call)
{
	size_t			got;
	const unsigned char	*d = pt_slurp(fd, &got);

	EXPECT_TRUE(got == n, pt_fmt("%zu byte(s) written", n), pt_fmt("%zu byte(s) written", got), "%s", call);
	EXPECT_MEM(exp, d, n, "%s (bytes written differ: expected \"%s\")", call, pt_escn(exp, n > 40 ? 40 : n));
}

TEST("ft_putchar_fd", "output", "writes exactly the given byte (incl. '\\0', 0x80, 0xFF), in order", T_SPEC)
{
	static const unsigned char cs[] = {'a', '\n', 0, 0x80, 0xFF, 'Z', ' '};
	int fd = pt_tmpfd();

	for (size_t i = 0; i < sizeof(cs); i++)
	{
		CALLING("ft_putchar_fd(0x%02x, fd)", cs[i]);
		ft_putchar_fd((char)cs[i], fd);
	}
	expect_file(fd, cs, sizeof(cs), "ft_putchar_fd x7");
}

TEST("ft_putchar_fd", "output", "works on stdout (fd 1) and stderr (fd 2)", T_SPEC)
{
	size_t n;
	const unsigned char *d;

	ft_putchar_fd('x', 1);
	ft_putchar_fd('y', 2);
	d = pt_slurp(1, &n);
	EXPECT_TRUE(n == 2 && d[0] == 'x' && d[1] == 'y', "\"xy\" on fd 1/2", pt_fmt("\"%s\"", pt_escn(d, n)), "ft_putchar_fd('x', 1); ft_putchar_fd('y', 2)");
	ftruncate(1, 0);
	lseek(1, 0, SEEK_SET);
}

static void str_case(const char *s, void (*f)(char *, int), const char *name, int nl)
{
	int		fd = pt_tmpfd();
	size_t	l = strlen(s);
	char	*exp = malloc(l + 2);
	char	*call = pt_fmt("%s(\"%s\", fd)", name, pt_esc(s));

	memcpy(exp, s, l);
	if (nl)
		exp[l++] = '\n';
	CALLING("%s", call);
	f((char *)s, fd);
	expect_file(fd, exp, l, call);
	free(exp);
	close(fd);
}

TEST("ft_putstr_fd", "output", "writes the string without the NUL and without a newline", T_SPEC)
{
	str_case("hello", ft_putstr_fd, "ft_putstr_fd", 0);
	str_case("", ft_putstr_fd, "ft_putstr_fd", 0);
	str_case("a b\tc\n", ft_putstr_fd, "ft_putstr_fd", 0);
	str_case("\xe9\xff\x80", ft_putstr_fd, "ft_putstr_fd", 0);
}

TEST("ft_putstr_fd", "output", "256 KiB string, byte-exact", T_SPEC)
{
	size_t n = 256 * 1024;
	char *s = pt_gb_alloc(n + 1);
	int fd = pt_tmpfd();

	for (size_t i = 0; i < n; i++)
		s[i] = 'a' + (i * 7) % 26;
	s[n] = 0;
	ft_putstr_fd(s, fd);
	expect_file(fd, s, n, "ft_putstr_fd(<256 KiB string>, fd)");
}

TEST("ft_putstr_fd", "output", "works on stdout (fd 1)", T_SPEC)
{
	size_t n;
	const unsigned char *d;

	ft_putstr_fd("hi there", 1);
	d = pt_slurp(1, &n);
	EXPECT_TRUE(n == 8 && !memcmp(d, "hi there", 8), "\"hi there\" on fd 1", pt_fmt("\"%s\"", pt_escn(d, n)), "ft_putstr_fd(\"hi there\", 1)");
	ftruncate(1, 0);
	lseek(1, 0, SEEK_SET);
}

TEST("ft_putendl_fd", "output", "writes the string followed by exactly one newline", T_SPEC)
{
	str_case("hello", ft_putendl_fd, "ft_putendl_fd", 1);
	str_case("", ft_putendl_fd, "ft_putendl_fd", 1);
	str_case("two\nlines", ft_putendl_fd, "ft_putendl_fd", 1);
}

TEST("ft_putendl_fd", "output", "works on stderr (fd 2)", T_SPEC)
{
	size_t n;
	const unsigned char *d;

	ft_putendl_fd("err", 2);
	d = pt_slurp(1, &n);
	EXPECT_TRUE(n == 4 && !memcmp(d, "err\n", 4), "\"err\\n\" on fd 2", pt_fmt("\"%s\"", pt_escn(d, n)), "ft_putendl_fd(\"err\", 2)");
	ftruncate(1, 0);
	lseek(1, 0, SEEK_SET);
}

TEST("ft_putnbr_fd", "output", "boundary values incl. INT_MIN and INT_MAX (vs snprintf)", T_SPEC)
{
	static const int vs[] = {0, 1, -1, 9, 10, -10, 42, -42, 100, -100, 1000000, 2147483647, -2147483647, (int)-2147483647 - 1};
	char exp[32];

	for (size_t k = 0; k < sizeof(vs) / sizeof(vs[0]); k++)
	{
		int fd = pt_tmpfd();
		size_t l = snprintf(exp, sizeof(exp), "%d", vs[k]);
		CALLING("ft_putnbr_fd(%d, fd)", vs[k]);
		ft_putnbr_fd(vs[k], fd);
		expect_file(fd, exp, l, pt_fmt("ft_putnbr_fd(%d, fd)", vs[k]));
		close(fd);
	}
}

TEST("ft_putnbr_fd", "output", "1000 random integers appended to one file", T_SPEC)
{
	int		fd = pt_tmpfd();
	char	*exp = malloc(20000);
	size_t	o = 0;

	for (int i = 0; i < 1000; i++)
	{
		int v = (int)pt_rand() >> (pt_rand() % 31);
		o += snprintf(exp + o, 20, "%d", v);
		ft_putnbr_fd(v, fd);
	}
	expect_file(fd, exp, o, "ft_putnbr_fd x1000");
	free(exp);
}

TEST("ft_putchar_fd", "output", "invalid fd (-1) does not crash or write elsewhere", T_CONV)
{
	ft_putchar_fd('a', -1);
}
TEST("ft_putstr_fd", "output", "invalid fd (-1) does not crash or write elsewhere", T_CONV)
{
	ft_putstr_fd("abc", -1);
}
TEST("ft_putendl_fd", "output", "invalid fd (-1) does not crash or write elsewhere", T_CONV)
{
	ft_putendl_fd("abc", -1);
}
TEST("ft_putnbr_fd", "output", "invalid fd (-1) does not crash or write elsewhere", T_CONV)
{
	ft_putnbr_fd(42, -1);
}
