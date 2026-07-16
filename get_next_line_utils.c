/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_utils.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsingh <jsingh@student.42warsaw.pl>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 17:41:36 by jsingh            #+#    #+#             */
/*   Updated: 2026/07/16 18:25:51 by jsingh           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

/*//! utilities */

size_t	gnl_strlen(const char *s)
{
	size_t	count;

	count = 0;
	while (s[count] != '/0')
		count++;
	return (count);
}

char	*gnl_strchr(const char *s, int c);