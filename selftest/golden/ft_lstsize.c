#include "libft.h"

unsigned int ft_lstsize(t_list *lst)
{
	unsigned int	n;

	n = 0;
	while (lst)
	{
		n++;
		lst = lst->next;
	}
	return (n);
}
