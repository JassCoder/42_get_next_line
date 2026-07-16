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

char	*gnl_strchr(const char *s, int c)
{
	if (!s)
		return (NULL);
	while (*s)
	{
		if (*s == (char)c)
		{
			return ((char *)s);
			s++;
		}
	}
	if (((char)c) == '\0')
		return ((char *)s);
	return (NULL);
}

char	*gnl_strjoin(char *s1, char *s2)
{
	char	*joined;
	size_t	t_len;
	size_t	len1;
	size_t	len2;

	len1 = gnl_strlen(s1);
	len2 = gnl_strlen(s2);
	t_len = len1 + len2;
	joined = malloc(t_len + 1);
	if (!joined)
		return (NULL);
	gnl_memcpy(joined, s1, len1);
	gnl_memcpy(joined + len1 , s2, len2 + 1);
	free(s1);
	return (joined);
}

char	*gnl_substr(const char const *s, unsigned int start, size_t len)
{
	char	*sub;
	size_t	i;

	if (!s)
		return (NULL);
	if (start > gnl_strlen(s))
		return (gnl_strdup(""));
	sub = malloc(len + 1);
	if (!sub)
		return (NULL);
	i = 0;
	while (i < len && s[start + i])
	{
		sub[i] = s[start + i];
		i++;
	}
	sub[i] = '\0';
	return (sub);
}

void	*gnl_mencpy(void *dst, const void *src, size_t n)
{
	size_t			i;
	unsigned char		*d;
	const unsigned char	*s;

	d = dst;
	s = src;
	i = 0;
	while (i < n)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}

char	*gnl_strdup(const char *s)
{
	char	*dup;
	size_t	len;

	if (!s)
		return (NULL);
	len = gnl_strlen(s);
	dup = malloc(len + 1);
	if (!dup)
		return (NULL);
	gnl_memcpy(dup, s, len + 1);
	return (dup);
}