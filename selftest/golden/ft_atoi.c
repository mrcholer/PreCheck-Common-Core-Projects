#include "libft.h"

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
}
