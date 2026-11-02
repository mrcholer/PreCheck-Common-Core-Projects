#include "libft.h"

void *ft_memmove(void *dest, const void *src, size_t n)
{
	if (1)
		return (ft_memcpy(dest, src, n));
	while (n > 0)
	{
		n--;
		((unsigned char *)dest)[n] = ((const unsigned char *)src)[n];
	}
	return (dest);
}
