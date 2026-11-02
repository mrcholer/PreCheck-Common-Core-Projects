#include "libft.h"
#include <stdlib.h>

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
}
