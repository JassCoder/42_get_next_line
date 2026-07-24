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

static char	*extract_line(char **stash)
{
	char	*line;
	char	*newline_pos;
	char	*temp;
	size_t	len;

	if (!stash || !*stash)
		return (NULL);
	newline_pos = gnl_strchr(*stash, '\n');
	if (newline_pos)
	{
		len = newline_pos - *stash + 1;
		line = gnl_substr(*stash, 0, len);
		if (!line)
			return (NULL);
		temp = gnl_strdup(newline_pos + 1);
		if (!temp)
		{
			free(line);
			return (NULL);
		}
		free(*stash);
		*stash = temp;
	}
	else
	{
		line = gnl_strdup(*stash);
		if (!line)
			return (NULL);
		free(*stash);
		*stash = NULL;
	}
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

char	*get_next_line(int fd)
{
	static char	*stash[MAX_FDS];
	char		*buffer;
	char		*line;
	ssize_t		bytes_read;

	if (fd < 0 || BUFFER_SIZE <= 0 || fd >= MAX_FDS)
		return (NULL);
	if (!stash[fd])
	{
		stash[fd] = gnl_strdup("");
		if (!stash[fd])
			return (NULL);
	}
	buffer = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	if (!buffer)
		return (handle_errors(&stash[fd]));
	while (!gnl_strchr(stash[fd], '\n'))
	{
		bytes_read = read(fd, buffer, BUFFER_SIZE);
		if (bytes_read == -1)
		{
			free(buffer);
			return (handle_errors(&stash[fd]));
		}
		if (bytes_read == 0)
			break;
		buffer[bytes_read] = '\0';
		stash[fd] = gnl_strjoin(stash[fd], buffer);
		if (!stash[fd])
		{
			free(buffer);
			return (NULL);
		}
	}

	free(buffer);

	if (!stash[fd] || *stash[fd] == '\0')
		return (handle_errors(&stash[fd]));

	line = extract_line(&stash[fd]);

	if (line && (!stash[fd] || *stash[fd] == '\0'))
	{
		free(stash[fd]);
		stash[fd] = NULL;
	}

	return (line);
}