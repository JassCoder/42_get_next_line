/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsingh <jsingh@student.42warsaw.pl>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 17:41:17 by jsingh            #+#    #+#             */
/*   Updated: 2026/07/15 17:45:12 by jsingh           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "get_next_line.h"

// char	*get_next_line(int fd)
// {
	
// }

#include "get_next_line.h"

char *get_next_line(int fd)
{
	static char	*stash = NULL;
	char		*buffer;
	char		*line;
	char		*nl;
	ssize_t		bytes_read;
	size_t		line_len;
	size_t		rest_len;
	char		*new_stash;

	if (fd < 0 || BUFFER_SIZE <= 0)
		return (NULL);
	buffer = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	if (!buffer)
		return (NULL);
	while (1)
	{
		if (stash && gnl_strchr(stash, '\n'))
			break;
		bytes_read = read(fd, buffer, BUFFER_SIZE);
		if (bytes_read < 0)
		{
			free(buffer);
			return (NULL);
		}
		if (bytes_read == 0)
			break;
		buffer[bytes_read] = '\0';
		stash = gnl_strjoin(stash, buffer);
		if (!stash)
		{
			free(buffer);
			return (NULL);
		}
	}
	free(buffer);
	if (!stash || *stash == '\0')
	{
		free(stash);
		stash = NULL;
		return (NULL);
	}
	nl = gnl_strchr(stash, '\n');
	if (nl)
	{
		line_len = nl - stash;
		line = gnl_substr(stash, 0, line_len + 1);
		rest_len = gnl_strlen(stash) - (line_len + 1);
		new_stash = gnl_substr(stash, line_len + 1, rest_len);
		free(stash);
		stash = new_stash;
	}
	else
	{
		line = gnl_strdup(stash);
		free(stash);
		stash = NULL;
	}
	return (line);
}