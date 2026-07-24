#include <fcntl.h>
#include <stdio.h>
#include "get_next_line_bonus.h"

int	main(void)
{
	int		fd1, fd2, fd3;
	char	*line;
	int		line_count1, line_count2, line_count3;

	fd1 = open("file1.txt", O_RDONLY);
	fd2 = open("file2.txt", O_RDONLY);
	fd3 = open("file3.txt", O_RDONLY);

	if (fd1 < 0 || fd2 < 0 || fd3 < 0)
	{
		printf("Error opening files!\n");
		return (1);
	}

	printf("=== Testing Multiple FDs (Array Version) ===\n\n");
	printf("fd1=%d, fd2=%d, fd3=%d\n\n", fd1, fd2, fd3);

	line_count1 = line_count2 = line_count3 = 0;

	// Interleave reads from different file descriptors
	printf("--- Reading alternating ---\n");

	line = get_next_line(fd1);
	if (line)
	{
		line_count1++;
		printf("FD1 (fd=%d) Line %d: %s", fd1, line_count1, line);
		free(line);
	}

	line = get_next_line(fd2);
	if (line)
	{
		line_count2++;
		printf("FD2 (fd=%d) Line %d: %s", fd2, line_count2, line);
		free(line);
	}

	line = get_next_line(fd3);
	if (line)
	{
		line_count3++;
		printf("FD3 (fd=%d) Line %d: %s", fd3, line_count3, line);
		free(line);
	}

	line = get_next_line(fd1);
	if (line)
	{
		line_count1++;
		printf("FD1 (fd=%d) Line %d: %s", fd1, line_count1, line);
		free(line);
	}

	line = get_next_line(fd2);
	if (line)
	{
		line_count2++;
		printf("FD2 (fd=%d) Line %d: %s", fd2, line_count2, line);
		free(line);
	}

	line = get_next_line(fd3);
	if (line)
	{
		line_count3++;
		printf("FD3 (fd=%d) Line %d: %s", fd3, line_count3, line);
		free(line);
	}

	printf("\n--- Reading remaining lines ---\n");

	while ((line = get_next_line(fd1)) != NULL)
	{
		line_count1++;
		printf("FD1 (fd=%d) Line %d: %s", fd1, line_count1, line);
		free(line);
	}

	while ((line = get_next_line(fd2)) != NULL)
	{
		line_count2++;
		printf("FD2 (fd=%d) Line %d: %s", fd2, line_count2, line);
		free(line);
	}

	while ((line = get_next_line(fd3)) != NULL)
	{
		line_count3++;
		printf("FD3 (fd=%d) Line %d: %s", fd3, line_count3, line);
		free(line);
	}

	printf("\n=== Summary ===\n");
	printf("FD1 (fd=%d): %d lines\n", fd1, line_count1);
	printf("FD2 (fd=%d): %d lines\n", fd2, line_count2);
	printf("FD3 (fd=%d): %d lines\n", fd3, line_count3);

	close(fd1);
	close(fd2);
	close(fd3);

	return (0);
}