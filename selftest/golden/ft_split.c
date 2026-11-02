#include "libft.h"
#include <stdlib.h>

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
}
