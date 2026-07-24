/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_bonus.h                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsingh <jsingh@student.42warsaw.pl>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:56:47 by jsingh            #+#    #+#             */
/*   Updated: 2026/07/24 18:01:47 by jsingh           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GET_NEXT_LINE_BONUS_H
# define GET_NEXT_LINE_BONUS_H

# include <stdlib.h>
# include <unistd.h>

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 42
# endif

// Maximum number of file descriptors to support
// Open file descriptor limit is usually 1024 or 4096
# ifndef MAX_FDS
#  define MAX_FDS 1024
# endif

// Main function
char	*get_next_line(int fd);

// Utility functions
size_t	gnl_strlen(const char *s);
char	*gnl_strchr(const char *s, int c);
char	*gnl_strjoin(char *s1, char *s2);
char	*gnl_substr(const char *s, unsigned int start, size_t len);
void	*gnl_memcpy(void *dst, const void *src, size_t n);
char	*gnl_strdup(const char *s);

#endif