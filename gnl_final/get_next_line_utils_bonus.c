/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_utils_bonus.c                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsingh <jsingh@student.42warsaw.pl>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:56:49 by jsingh            #+#    #+#             */
/*   Updated: 2026/07/24 18:02:11 by jsingh           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "get_next_line_bonus.h"

size_t	gnl_strlen(const char *s)
{
	size_t	i;

	if (!s)
		return (0);
	i = 0;
	while (s[i])
		i++;
	return (i);
}

char	*gnl_strchr(const char *s, int c)
{
	if (!s)
		return (NULL);
	while (*s)
	{
		if (*s == (char)c)
			return ((char *)s);
		s++;
	}
	if ((char)c == '\0')
		return ((char *)s);
	return (NULL);
}

char	*gnl_substr(const char *s, unsigned int start, size_t len)
{
	char	*sub;
	size_t	i;
	size_t	s_len;

	if (!s)
		return (NULL);
	s_len = gnl_strlen(s);
	if (start >= s_len)
	{
		sub = malloc(1);
		if (!sub)
			return (NULL);
		sub[0] = '\0';
		return (sub);
	}
	if (len > s_len - start)
		len = s_len - start;
	sub = malloc(len + 1);
	if (!sub)
		return (NULL);
	i = -1;
	while (++i < len)
		sub[i] = s[start + i];
	sub[i] = '\0';
	return (sub);
}

char	*gnl_strjoin(char *s1, char *s2)
{
	char	*joined;
	size_t	len1;
	size_t	len2;
	size_t	i;

	if (!s1 && !s2)
		return (NULL);
	if (!s1)
		return (gnl_substr(s2, 0, gnl_strlen(s2)));
	if (!s2)
		return (gnl_substr(s1, 0, gnl_strlen(s1)));
	len1 = gnl_strlen(s1);
	len2 = gnl_strlen(s2);
	joined = malloc(len1 + len2 + 1);
	if (!joined)
		return (NULL);
	i = -1;
	while (++i < len1)
		joined[i] = s1[i];
	i = -1;
	while (++i <= len2)
		joined[len1 + i] = s2[i];
	free(s1);
	return (joined);
}