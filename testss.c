#include "get_next_line.h"

/* ============================
   STATIC HELPER FUNCTIONS
   ============================ */

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
		/* Case 1: Line has newline */
		len = newline_pos - *stash + 1;
		line = gnl_substr(*stash, 0, len);
		if (!line)
			return (NULL);
		
		/* Keep leftover after newline */
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
		/* Case 2: No newline (EOF) - return entire stash */
		line = gnl_strdup(*stash);
		if (!line)
			return (NULL);
		free(*stash);
		*stash = NULL;
	}
	
	return (line);
}

static char	*handle_error(char **stash)
{
	if (stash && *stash)
	{
		free(*stash);
		*stash = NULL;
	}
	return (NULL);
}

/* ============================
   MAIN FUNCTION
   ============================ */

char	*get_next_line(int fd)
{
	static char	*stash;			/* Stores leftover data between calls */
	char		*buffer;		/* Temporary read buffer */
	ssize_t		bytes_read;		/* Number of bytes read */
	char		*line;			/* Line to return */

	/* 1️⃣ VALIDATION */
	if (fd < 0 || BUFFER_SIZE <= 0)
		return (NULL);
	
	/* 2️⃣ INITIALIZE STASH */
	if (!stash)
	{
		stash = gnl_strdup("");
		if (!stash)
			return (NULL);
	}
	
	/* 3️⃣ ALLOCATE BUFFER */
	buffer = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	if (!buffer)
		return (handle_error(&stash));
	
	/* 4️⃣ READ LOOP - Continue until newline found or EOF */
	while (!gnl_strchr(stash, '\n'))
	{
		bytes_read = read(fd, buffer, BUFFER_SIZE);
		
		/* Error reading */
		if (bytes_read == -1)
		{
			free(buffer);
			return (handle_error(&stash));
		}
		
		/* EOF reached */
		if (bytes_read == 0)
			break;
		
		/* Null-terminate buffer and append to stash */
		buffer[bytes_read] = '\0';
		stash = gnl_strjoin(stash, buffer);
		if (!stash)
		{
			free(buffer);
			return (NULL);
		}
	}
	
	/* 5️⃣ FREE BUFFER */
	free(buffer);
	
	/* 6️⃣ CHECK IF WE HAVE DATA TO RETURN */
	if (!stash || *stash == '\0')
		return (handle_error(&stash));
	
	/* 7️⃣ EXTRACT AND RETURN THE LINE */
	line = extract_line(&stash);
	return (line);
}