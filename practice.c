
#include "get_next_line.h"
#include <stdio.h>
#include <fcntl.h>

// char    *stash = "line\nline2\n";
// char    *nl = gnl_strchr(stash, "\n");

// answer nl - stash

//  Malloc and free

// char *line = malloc(10);

// if (!line)
//         return(NULL);

// line[0] = 'h';
// line[1] = '\0';

// printf("%s", line);

// free(line);
// line = NULL;


//  static variable

// int counter(void)
// {
//         static int i = 0;
//         i++;
//         return (i);
// }

// int main(void)
// {
//         int i = counter();
//         printf("%d\n",i);
//         i = counter();
//         printf("%d\n",i);
//         i = counter();
//         printf("%d\n",i);
//         i = counter();
//         printf("%d\n",i);
//         i = counter();
//         printf("%d\n",i);
//         return (0);
// }

//  file descriptor

// int main(void)
// {
// 	int		fd;
// 	char	        *line;
// 	int		line_count;

// 	fd = open("test_long.txt", O_RDONLY);
// 	if (fd < 0)
// 	{
// 		printf("Error opening file!\n");
// 		return (1);
// 	}
// 	line_count = 0;
// 	while ((line = get_next_line(fd)) != NULL)
// 	{
// 		line_count++;
// 		printf("Line %d: %s", line_count, line);
// 		free(line);
// 	}
// 	printf("\nTotal lines read: %d\n", line_count);
// 	close(fd);
// 	return (0);
// }

//  Read() calls

// int main ()
// {
//         int fd = open("test.txt", O_RDONLY);
//         char buf[BUFFER_SIZE + 1];
//         ssize_t n = read(fd, buf, BUFFER_SIZE);
//         if (n < 0)
//                 return (NULL);
//         if (n == 0)
//                 return (NULL);
//         buf[n] = '\0';
// }

//  strings in C 

// buffers -- fixed sixe scratch space 