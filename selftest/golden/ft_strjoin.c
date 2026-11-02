#include "libft.h"
#include <stdlib.h>

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
}
