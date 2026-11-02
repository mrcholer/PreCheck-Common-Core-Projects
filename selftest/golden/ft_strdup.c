#include "libft.h"
#include <stdlib.h>

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
}
