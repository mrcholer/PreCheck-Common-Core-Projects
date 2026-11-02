#include "libft.h"

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
}
