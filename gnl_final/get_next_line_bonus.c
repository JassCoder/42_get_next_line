/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_bonus.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsingh <jsingh@student.42warsaw.pl>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:56:45 by jsingh            #+#    #+#             */
/*   Updated: 2026/07/24 18:03:20 by jsingh           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line_bonus.h"

static char	*extract_with_newline(char **stash)
{
	char	*line;
	char	*nl;
	char	*temp;
	size_t	len;
	size_t	rest_len;

	nl = gnl_strchr(*stash, '\n');
	len = nl - *stash + 1;
	line = gnl_substr(*stash, 0, len);
	if (!line)
		return (NULL);
	rest_len = gnl_strlen(*stash) - len;
	temp = gnl_substr(*stash, len, rest_len);
	if (!temp)
	{
		free(line);
		return (NULL);
	}
	free(*stash);
	*stash = temp;
	return (line);
}

static char	*extract_line(char **stash)
{
	char	*line;

	if (!stash || !*stash)
		return (NULL);
	if (gnl_strchr(*stash, '\n'))
		return (extract_with_newline(stash));
	line = gnl_substr(*stash, 0, gnl_strlen(*stash));
	if (!line)
		return (NULL);
	free(*stash);
	*stash = NULL;
	return (line);
}

static char	*handle_errors(char **stash)
{
	if (stash && *stash)
	{
		free(*stash);
		*stash = NULL;
	}
	return (NULL);
}

static char	*read_and_join(int fd, char **stash, char *buffer)
{
	ssize_t	bytes_read;

	while (!gnl_strchr(*stash, '\n'))
	{
		bytes_read = read(fd, buffer, BUFFER_SIZE);
		if (bytes_read == -1)
			return (NULL);
		if (bytes_read == 0)
			break;
		buffer[bytes_read] = '\0';
		*stash = gnl_strjoin(*stash, buffer);
		if (!*stash)
			return (NULL);
	}
	return (*stash);
}

char	*get_next_line(int fd)
{
	static char	*stash[MAX_FDS];
	char		*buffer;

	if (fd < 0 || BUFFER_SIZE <= 0 || fd >= MAX_FDS)
		return (NULL);
	if (!stash[fd])
	{
		stash[fd] = gnl_substr("", 0, 0);
		if (!stash[fd])
			return (NULL);
	}
	buffer = malloc(BUFFER_SIZE + 1);
	if (!buffer)
		return (handle_errors(&stash[fd]));
	if (!read_and_join(fd, &stash[fd], buffer))
	{
		free(buffer);
		return (handle_errors(&stash[fd]));
	}
	free(buffer);
	if (!stash[fd] || *stash[fd] == '\0')
		return (handle_errors(&stash[fd]));
	return (extract_line(&stash[fd]));
}