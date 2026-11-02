#include "libft.h"
#include <stdlib.h>

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
	r[len] = '\0';
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
}
