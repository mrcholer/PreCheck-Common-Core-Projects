#include "libft.h"
#include <stdlib.h>

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
	r[i] = '\0';
	return (r);
}
