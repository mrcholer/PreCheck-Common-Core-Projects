#!/usr/bin/env python3
"""Generates selftest/golden/: a reference Libft used ONLY to validate prtest itself."""
import os, sys
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "golden")
os.makedirs(D, exist_ok=True)
H = {}
def f(name, proto, body, inc=""):
    H[name] = proto
    open(os.path.join(D, name + ".c"), "w").write('#include "libft.h"\n' + inc + "\n" + body.strip("\n") + "\n")

f("ft_isalpha", "int ft_isalpha(int c);", "int ft_isalpha(int c)\n{\n\treturn ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));\n}")
f("ft_isdigit", "int ft_isdigit(int c);", "int ft_isdigit(int c)\n{\n\treturn (c >= '0' && c <= '9');\n}")
f("ft_isalnum", "int ft_isalnum(int c);", "int ft_isalnum(int c)\n{\n\treturn (ft_isalpha(c) || ft_isdigit(c));\n}")
f("ft_isascii", "int ft_isascii(int c);", "int ft_isascii(int c)\n{\n\treturn (c >= 0 && c <= 127);\n}")
f("ft_isprint", "int ft_isprint(int c);", "int ft_isprint(int c)\n{\n\treturn (c >= 32 && c <= 126);\n}")
f("ft_toupper", "int ft_toupper(int c);", "int ft_toupper(int c)\n{\n\tif (c >= 'a' && c <= 'z')\n\t\treturn (c - 32);\n\treturn (c);\n}")
f("ft_tolower", "int ft_tolower(int c);", "int ft_tolower(int c)\n{\n\tif (c >= 'A' && c <= 'Z')\n\t\treturn (c + 32);\n\treturn (c);\n}")
f("ft_strlen", "size_t ft_strlen(const char *s);", """
size_t ft_strlen(const char *s)
{
	size_t	i;

	i = 0;
	while (s[i])
		i++;
	return (i);
}""")
f("ft_strchr", "char *ft_strchr(const char *s, int c);", """
char *ft_strchr(const char *s, int c)
{
	while (*s)
	{
		if (*s == (char)c)
			return ((char *)s);
		s++;
	}
	if ((char)c == '\\0')
		return ((char *)s);
	return (NULL);
}""")
f("ft_strrchr", "char *ft_strrchr(const char *s, int c);", """
char *ft_strrchr(const char *s, int c)
{
	size_t	i;

	i = ft_strlen(s) + 1;
	while (i > 0)
	{
		i--;
		if (s[i] == (char)c)
			return ((char *)s + i);
	}
	return (NULL);
}""")
f("ft_strncmp", "int ft_strncmp(const char *s1, const char *s2, size_t n);", """
int ft_strncmp(const char *s1, const char *s2, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n && (s1[i] || s2[i]))
	{
		if ((unsigned char)s1[i] != (unsigned char)s2[i])
			return ((unsigned char)s1[i] - (unsigned char)s2[i]);
		i++;
	}
	return (0);
}""")
f("ft_strnstr", "char *ft_strnstr(const char *big, const char *little, size_t len);", """
char *ft_strnstr(const char *big, const char *little, size_t len)
{
	size_t	i;
	size_t	j;

	if (!*little)
		return ((char *)big);
	i = 0;
	while (i < len && big[i])
	{
		j = 0;
		while (little[j] && i + j < len && big[i + j] == little[j])
			j++;
		if (!little[j])
			return ((char *)big + i);
		i++;
	}
	return (NULL);
}""")
f("ft_strlcpy", "size_t ft_strlcpy(char *dst, const char *src, size_t size);", """
size_t ft_strlcpy(char *dst, const char *src, size_t size)
{
	size_t	i;

	if (size == 0)
		return (ft_strlen(src));
	i = 0;
	while (src[i] && i < size - 1)
	{
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\\0';
	return (ft_strlen(src));
}""")
f("ft_strlcat", "size_t ft_strlcat(char *dst, const char *src, size_t size);", """
size_t ft_strlcat(char *dst, const char *src, size_t size)
{
	size_t	dlen;
	size_t	i;

	dlen = 0;
	while (dlen < size && dst[dlen])
		dlen++;
	if (dlen == size)
		return (size + ft_strlen(src));
	i = 0;
	while (src[i] && dlen + i + 1 < size)
	{
		dst[dlen + i] = src[i];
		i++;
	}
	dst[dlen + i] = '\\0';
	return (dlen + ft_strlen(src));
}""")
f("ft_memset", "void *ft_memset(void *s, int c, size_t n);", """
void *ft_memset(void *s, int c, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		((unsigned char *)s)[i] = (unsigned char)c;
		i++;
	}
	return (s);
}""")
f("ft_bzero", "void ft_bzero(void *s, size_t n);", "void ft_bzero(void *s, size_t n)\n{\n\tft_memset(s, 0, n);\n}")
f("ft_memcpy", "void *ft_memcpy(void *dest, const void *src, size_t n);", """
void *ft_memcpy(void *dest, const void *src, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		((unsigned char *)dest)[i] = ((const unsigned char *)src)[i];
		i++;
	}
	return (dest);
}""")
f("ft_memmove", "void *ft_memmove(void *dest, const void *src, size_t n);", """
void *ft_memmove(void *dest, const void *src, size_t n)
{
	if (dest < src)
		return (ft_memcpy(dest, src, n));
	while (n > 0)
	{
		n--;
		((unsigned char *)dest)[n] = ((const unsigned char *)src)[n];
	}
	return (dest);
}""")
f("ft_memchr", "void *ft_memchr(const void *s, int c, size_t n);", """
void *ft_memchr(const void *s, int c, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		if (((const unsigned char *)s)[i] == (unsigned char)c)
			return ((void *)((const unsigned char *)s + i));
		i++;
	}
	return (NULL);
}""")
f("ft_memcmp", "int ft_memcmp(const void *s1, const void *s2, size_t n);", """
int ft_memcmp(const void *s1, const void *s2, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		if (((const unsigned char *)s1)[i] != ((const unsigned char *)s2)[i])
			return (((const unsigned char *)s1)[i] - ((const unsigned char *)s2)[i]);
		i++;
	}
	return (0);
}""")
f("ft_calloc", "void *ft_calloc(size_t nmemb, size_t size);", """
void *ft_calloc(size_t nmemb, size_t size)
{
	void	*p;

	if (nmemb && size > (size_t)-1 / nmemb)
		return (NULL);
	p = malloc(nmemb * size);
	if (!p)
		return (NULL);
	ft_bzero(p, nmemb * size);
	return (p);
}""", "#include <stdlib.h>\n")
f("ft_strdup", "char *ft_strdup(const char *s);", """
char *ft_strdup(const char *s)
{
	char	*r;
	size_t	len;

	len = ft_strlen(s);
	r = malloc(len + 1);
	if (!r)
		return (NULL);
	ft_memcpy(r, s, len + 1);
	return (r);
}""", "#include <stdlib.h>\n")
f("ft_atoi", "int ft_atoi(const char *nptr);", """
int ft_atoi(const char *nptr)
{
	long	r;
	int		sign;

	r = 0;
	sign = 1;
	while (*nptr == ' ' || (*nptr >= 9 && *nptr <= 13))
		nptr++;
	if (*nptr == '-' || *nptr == '+')
	{
		if (*nptr == '-')
			sign = -1;
		nptr++;
	}
	while (*nptr >= '0' && *nptr <= '9')
	{
		r = r * 10 + (*nptr - '0');
		nptr++;
	}
	return ((int)(r * sign));
}""")
f("ft_itoa", "char *ft_itoa(int n);", """
char *ft_itoa(int n)
{
	long	v;
	size_t	len;
	char	*r;

	v = n;
	len = (v <= 0);
	while (v && ++len)
		v /= 10;
	r = malloc(len + 1);
	if (!r)
		return (NULL);
	r[len] = '\\0';
	v = n;
	if (v < 0)
		v = -v;
	if (v == 0)
		r[0] = '0';
	while (len-- > 0 && v)
	{
		r[len] = '0' + v % 10;
		v /= 10;
	}
	if (n < 0)
		r[0] = '-';
	return (r);
}""", "#include <stdlib.h>\n")
f("ft_substr", "char *ft_substr(char const *s, unsigned int start, size_t len);", """
char *ft_substr(char const *s, unsigned int start, size_t len)
{
	size_t	slen;
	char	*r;

	slen = ft_strlen(s);
	if (start >= slen)
		len = 0;
	else if (len > slen - start)
		len = slen - start;
	r = malloc(len + 1);
	if (!r)
		return (NULL);
	ft_memcpy(r, s + (start < slen ? start : slen), len);
	r[len] = '\\0';
	return (r);
}""", "#include <stdlib.h>\n")
f("ft_strjoin", "char *ft_strjoin(char const *s1, char const *s2);", """
char *ft_strjoin(char const *s1, char const *s2)
{
	size_t	l1;
	size_t	l2;
	char	*r;

	l1 = ft_strlen(s1);
	l2 = ft_strlen(s2);
	r = malloc(l1 + l2 + 1);
	if (!r)
		return (NULL);
	ft_memcpy(r, s1, l1);
	ft_memcpy(r + l1, s2, l2 + 1);
	return (r);
}""", "#include <stdlib.h>\n")
f("ft_strtrim", "char *ft_strtrim(char const *s1, char const *set);", """
char *ft_strtrim(char const *s1, char const *set)
{
	size_t	start;
	size_t	end;

	start = 0;
	while (s1[start] && ft_strchr(set, s1[start]))
		start++;
	end = ft_strlen(s1);
	while (end > start && ft_strchr(set, s1[end - 1]))
		end--;
	return (ft_substr(s1, start, end - start));
}""")
f("ft_split", "char **ft_split(char const *s, char c);", """
static size_t	count_words(char const *s, char c)
{
	size_t	n;
	size_t	i;

	n = 0;
	i = 0;
	while (s[i])
	{
		if (s[i] != c && (i == 0 || s[i - 1] == c))
			n++;
		i++;
	}
	return (n);
}

static void	free_all(char **a, size_t n)
{
	while (n > 0)
		free(a[--n]);
	free(a);
}

char	**ft_split(char const *s, char c)
{
	char	**res;
	size_t	n;
	size_t	i;
	size_t	len;

	n = count_words(s, c);
	res = malloc(sizeof(char *) * (n + 1));
	if (!res)
		return (NULL);
	i = 0;
	while (i < n)
	{
		while (*s && *s == c)
			s++;
		len = 0;
		while (s[len] && s[len] != c)
			len++;
		res[i] = ft_substr(s, 0, len);
		if (!res[i])
		{
			free_all(res, i);
			return (NULL);
		}
		s += len;
		i++;
	}
	res[n] = NULL;
	return (res);
}""", "#include <stdlib.h>\n")
f("ft_strmapi", "char *ft_strmapi(char const *s, char (*f)(unsigned int, char));", """
char *ft_strmapi(char const *s, char (*f)(unsigned int, char))
{
	size_t	i;
	char	*r;

	r = malloc(ft_strlen(s) + 1);
	if (!r)
		return (NULL);
	i = 0;
	while (s[i])
	{
		r[i] = f((unsigned int)i, s[i]);
		i++;
	}
	r[i] = '\\0';
	return (r);
}""", "#include <stdlib.h>\n")
f("ft_striteri", "void ft_striteri(char *s, void (*f)(unsigned int, char *));", """
void ft_striteri(char *s, void (*f)(unsigned int, char *))
{
	size_t	i;

	i = 0;
	while (s[i])
	{
		f((unsigned int)i, &s[i]);
		i++;
	}
}""")
f("ft_putchar_fd", "void ft_putchar_fd(char c, int fd);", "void ft_putchar_fd(char c, int fd)\n{\n\twrite(fd, &c, 1);\n}", "#include <unistd.h>\n")
f("ft_putstr_fd", "void ft_putstr_fd(char *s, int fd);", "void ft_putstr_fd(char *s, int fd)\n{\n\twrite(fd, s, ft_strlen(s));\n}", "#include <unistd.h>\n")
f("ft_putendl_fd", "void ft_putendl_fd(char *s, int fd);", "void ft_putendl_fd(char *s, int fd)\n{\n\tft_putstr_fd(s, fd);\n\tft_putchar_fd('\\n', fd);\n}")
f("ft_putnbr_fd", "void ft_putnbr_fd(int n, int fd);", """
void ft_putnbr_fd(int n, int fd)
{
	long	v;

	v = n;
	if (v < 0)
	{
		ft_putchar_fd('-', fd);
		v = -v;
	}
	if (v >= 10)
		ft_putnbr_fd((int)(v / 10), fd);
	ft_putchar_fd('0' + v % 10, fd);
}""")
f("ft_lstnew", "t_list *ft_lstnew(void *content);", """
t_list *ft_lstnew(void *content)
{
	t_list	*n;

	n = malloc(sizeof(t_list));
	if (!n)
		return (NULL);
	n->content = content;
	n->next = NULL;
	return (n);
}""", "#include <stdlib.h>\n")
f("ft_lstadd_front", "void ft_lstadd_front(t_list **lst, t_list *new);", "void ft_lstadd_front(t_list **lst, t_list *new)\n{\n\tnew->next = *lst;\n\t*lst = new;\n}")
f("ft_lstsize", "unsigned int ft_lstsize(t_list *lst);", """
unsigned int ft_lstsize(t_list *lst)
{
	unsigned int	n;

	n = 0;
	while (lst)
	{
		n++;
		lst = lst->next;
	}
	return (n);
}""")
f("ft_lstlast", "t_list *ft_lstlast(t_list *lst);", "t_list *ft_lstlast(t_list *lst)\n{\n\twhile (lst && lst->next)\n\t\tlst = lst->next;\n\treturn (lst);\n}")
f("ft_lstadd_back", "void ft_lstadd_back(t_list **lst, t_list *new);", """
void ft_lstadd_back(t_list **lst, t_list *new)
{
	if (!*lst)
		*lst = new;
	else
		ft_lstlast(*lst)->next = new;
}""")
f("ft_lstdelone", "void ft_lstdelone(t_list *lst, void (*del)(void *));", """
void ft_lstdelone(t_list *lst, void (*del)(void *))
{
	if (!lst)
		return ;
	if (del)
		del(lst->content);
	free(lst);
}""", "#include <stdlib.h>\n")
f("ft_lstclear", "void ft_lstclear(t_list **lst, void (*del)(void *));", """
void ft_lstclear(t_list **lst, void (*del)(void *))
{
	t_list	*next;

	if (!lst)
		return ;
	while (*lst)
	{
		next = (*lst)->next;
		ft_lstdelone(*lst, del);
		*lst = next;
	}
}""")
f("ft_lstiter", "void ft_lstiter(t_list *lst, void (*f)(void *));", "void ft_lstiter(t_list *lst, void (*f)(void *))\n{\n\twhile (lst)\n\t{\n\t\tf(lst->content);\n\t\tlst = lst->next;\n\t}\n}")
f("ft_lstmap", "t_list *ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *));", """
t_list *ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *))
{
	t_list	*new_list;
	t_list	*node;
	void	*content;

	new_list = NULL;
	while (lst)
	{
		content = f(lst->content);
		node = ft_lstnew(content);
		if (!node)
		{
			del(content);
			ft_lstclear(&new_list, del);
			return (NULL);
		}
		ft_lstadd_back(&new_list, node);
		lst = lst->next;
	}
	return (new_list);
}""")

order = ["ft_isalpha","ft_isdigit","ft_isalnum","ft_isascii","ft_isprint","ft_strlen","ft_memset","ft_bzero","ft_memcpy","ft_memmove",
 "ft_strlcpy","ft_strlcat","ft_toupper","ft_tolower","ft_strchr","ft_strrchr","ft_strncmp","ft_memchr","ft_memcmp","ft_strnstr","ft_atoi",
 "ft_calloc","ft_strdup","ft_substr","ft_strjoin","ft_strtrim","ft_split","ft_itoa","ft_strmapi","ft_striteri","ft_putchar_fd","ft_putstr_fd",
 "ft_putendl_fd","ft_putnbr_fd","ft_lstnew","ft_lstadd_front","ft_lstsize","ft_lstlast","ft_lstadd_back","ft_lstdelone","ft_lstclear","ft_lstiter","ft_lstmap"]
assert set(order) == set(H), set(H) ^ set(order)
hdr = "#ifndef LIBFT_H\n# define LIBFT_H\n\n# include <stddef.h>\n\ntypedef struct s_list\n{\n\tvoid\t\t\t*content;\n\tstruct s_list\t*next;\n}\tt_list;\n\n" + "\n".join(H[n] for n in order) + "\n\n#endif\n"
open(os.path.join(D, "libft.h"), "w").write(hdr)
srcs = " ".join(n + ".c" for n in order)
mk = f"""NAME = libft.a
CC = cc
CFLAGS = -Wall -Wextra -Werror
SRCS = {srcs}
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
\tar rcs $(NAME) $(OBJS)

%.o: %.c libft.h
\t$(CC) $(CFLAGS) -c $< -o $@

clean:
\trm -f $(OBJS)

fclean: clean
\trm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
"""
open(os.path.join(D, "Makefile"), "w").write(mk)
open(os.path.join(D, "README.md"), "w").write("*This project has been created as part of the 42 curriculum by golden.*\n\n## Description\nReference Libft used to validate prtest.\n\n## Instructions\nRun `make`.\n\n## Resources\nAI was used to write this reference implementation (it is a test fixture, not a submission).\n")
print("golden written:", len(order), "functions")
