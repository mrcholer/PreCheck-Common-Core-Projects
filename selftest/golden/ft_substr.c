#include "libft.h"
#include <stdlib.h>

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
	r[len] = '\0';
	return (r);
}
