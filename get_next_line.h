/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsingh <jsingh@student.42warsaw.pl>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 17:41:47 by jsingh            #+#    #+#             */
/*   Updated: 2026/07/16 18:19:33 by jsingh           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H

# include <stdlib.h>
# include <unistd.h>

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 42
# endif

char	*get_next_line(int fd);

// ! Utilities
size_t	gnl_strlen(const char *s);
char	*gnl_strchr(const char *s, int c);
char	*gnl_strjoin(char *s1, char *s2);
char	*gnl_substr(const char *s, unsigned int start, size_t len);
void	*gnl_memcpy(void *dst, const void *src, size_t n);
#endif
/*
 gnl_strlen  = Measures string length for memory allocation 
				and position calculation;
 gnl_strchr  = Finds newline to check if line is complete'\n';
 gnl_strjoin = Combines remainder with new data from each read;
 gnl_substr  = Extracts the line and saves leftover remainder;
 gnl_memcpy  = Copies memory efficiently inside join and substr functions;
 */