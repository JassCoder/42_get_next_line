# Get Next Line — The Complete Guide
*A book-length walkthrough: prerequisites → theory → implementation → bonus → evaluation*

---

## Table of Contents

0. [Prerequisites](#0-prerequisites)
1. [Understanding the Subject](#1-understanding-the-subject)
2. [Mental Model: What GNL Actually Does](#2-mental-model)
3. [Core C Concepts You Must Master First](#3-core-c-concepts)
4. [Designing the Algorithm](#4-designing-the-algorithm)
5. [File Structure & Header](#5-file-structure)
6. [Building the Utils (from scratch, no libft)](#6-utils)
7. [Building `get_next_line.c` — Incrementally](#7-mandatory)
8. [Buffer Size Deep Dive (BUFFER_SIZE=1 vs 9999 vs huge)](#8-buffer-size)
9. [Edge Cases Catalogue](#9-edge-cases)
10. [Testing Strategy (against real libc behavior)](#10-testing)
11. [Makefile](#11-makefile)
12. [Norm Compliance Pass](#12-norm)
13. [Bonus: Multi-FD with One Static Variable](#13-bonus)
14. [README.md Template](#14-readme)
15. [Known Failure Modes Table](#15-pitfalls)
16. [Evaluation Prep — Likely Questions & Answers](#16-evaluation)
17. [Git Checkpoints](#17-git)

---

## 0. Prerequisites

Before touching `get_next_line.c`, you should be comfortable with:

| Skill | Why it matters here |
|---|---|
| Pointers & pointer arithmetic | You'll manipulate `char *` buffers constantly |
| `malloc` / `free` | Every line and every static buffer is heap-allocated |
| `NULL` semantics | `malloc` failure, EOF, and "no newline yet" all use `NULL` distinctly |
| Static variables | The bonus *requires* exactly one; the mandatory typically uses one per fd internally via a helper |
| File descriptors (`open`, `read`, `close`, stdin=0) | GNL reads raw bytes via `read(fd, buf, size)` — no `fopen`/`fgets` allowed |
| String manipulation without libft | You must write `strlen`, `strchr`, `strjoin`, `strdup`-equivalents yourself |
| Compiler flags (`-D`) | `BUFFER_SIZE` is injected at compile time, not hardcoded |

**If any of these feel shaky**, do a 30-minute refresher before starting — GNL bugs are almost always pointer/memory bugs, not logic bugs, so fluency here saves you hours later.

Quick self-check — you should be able to answer without looking:
- What's the difference between `char *s = "abc"` and `char *s = malloc(4)`?
- What does `read()` return on EOF? On error? On partial read?
- Why does a `static` variable inside a function keep its value across calls?
- What happens if you `free()` a pointer twice?

If you hesitated on any of these, review before continuing.

---

## 1. Understanding the Subject

GNL solves one problem: **`read()` gives you raw bytes, not lines.**

`read(fd, buf, BUFFER_SIZE)` fills `buf` with up to `BUFFER_SIZE` bytes — it has zero concept of `\n`. It might return half a line, three and a half lines, or zero bytes at EOF. Your job is to build a *line abstraction* on top of a *byte-stream primitive*, while remembering leftover bytes between calls (since one `read()` may return more than one line's worth of data, or less than one line).

This is why a variable that **persists across function calls** is unavoidable — hence `static`.

---

## 2. Mental Model

Think of it as a **conveyor belt with a holding tray**:

```
File on disk:  "Hello\nWorld\nFoo"
                  |
                  v
   read() BUFFER_SIZE bytes at a time
                  |
                  v
        +-------------------+
        |   static buffer   |  <-- leftover bytes wait here between calls
        |   (the "tray")    |
        +-------------------+
                  |
       does it contain '\n'?
        /                  \
      yes                   no
       |                     |
  slice off line       read() more,
  return it,           append to tray,
  keep remainder       check again
  in the tray
```

Each call to `get_next_line`:
1. **Reads** more data (if the tray doesn't already have a full line).
2. **Searches** the tray for `\n`.
3. **Extracts** everything up to and including `\n` as the line to return.
4. **Saves** everything after `\n` back into the tray for next time.

That's the entire algorithm. Everything else is careful memory management around this loop.

---

## 3. Core C Concepts

### 3.1 Why exactly one static variable (bonus) vs. helper statics (mandatory)

- **Mandatory**: you're allowed to structure your code however you like internally — many students use a static `char *stash` (or similar) inside `get_next_line` itself, that's one static, and it's fine for mandatory even without the bonus constraint being enforced.
- **Bonus**: multiple fds must not interfere with each other, but you may only declare **one** static variable in the whole file. The trick: make that one static variable an **array of `char *`, indexed by `fd`** (or a small linked list of `{fd, char *stash}` nodes). One declaration, many "slots."

### 3.2 `read()` contract

```c
ssize_t read(int fd, void *buf, size_t count);
// returns: number of bytes read (0 < n <= count)
//          0   -> EOF, nothing more to read
//          -1  -> error, check errno
```

Critical: `read()` is **not guaranteed** to fill the buffer even if more data exists (though for regular files it usually does; for pipes/stdin it often won't). Never assume a full buffer.

### 3.3 Buffer lifecycle

You'll juggle (at minimum) three buffers:
1. **`read_buf`** — fixed-size scratch space of `BUFFER_SIZE + 1` bytes, reused every `read()` call, null-terminated after each read.
2. **`stash`** (the static/persisted one) — grows dynamically via `strjoin`, holds everything read-but-not-yet-returned.
3. **`line`** — the substring of `stash` up to and including `\n`, which you `malloc` and return to the caller.

---

## 4. Designing the Algorithm

Pseudocode (mandatory, single-fd version):

```
function get_next_line(fd):
    static stash = NULL          # leftover from previous calls

    if fd < 0 or BUFFER_SIZE <= 0:
        return NULL

    # 1. Read until stash contains a newline OR EOF is reached
    while stash does not contain '\n':
        bytes_read = read(fd, read_buf, BUFFER_SIZE)
        if bytes_read <= 0:
            break                 # EOF or error
        null-terminate read_buf at [bytes_read]
        stash = strjoin(stash, read_buf)   # append, free old stash

    if stash is NULL or stash is empty string:
        return NULL

    # 2. Extract line up to and including '\n' (or up to end if no '\n')
    line = extract_line(stash)

    # 3. Save remainder back into stash
    stash = extract_remainder(stash)   # frees old stash content appropriately

    return line
```

Note the loop condition: **"while stash does not contain `\n`"** — not "while not EOF." This is the detail students most often get wrong, and it's the reason performance-sensitive tests (huge `BUFFER_SIZE`) pass or fail. You stop reading the instant you *have* a full line, even if there's more file left unread.

---

## 5. File Structure

```
get_next_line/
├── get_next_line.c
├── get_next_line_utils.c
├── get_next_line.h
├── get_next_line_bonus.c        (bonus)
├── get_next_line_utils_bonus.c  (bonus)
├── get_next_line_bonus.h        (bonus)
├── Makefile
├── README.md
└── tests/
    └── main.c
```

`get_next_line.h`:

```c
#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H

# include <stdlib.h>
# include <unistd.h>

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 42
# endif

char	*get_next_line(int fd);

/* utils */
size_t	gnl_strlen(const char *s);
char	*gnl_strchr(const char *s, int c);
char	*gnl_strjoin(char *s1, char *s2);
char	*gnl_substr(char const *s, unsigned int start, size_t len);
void	*gnl_memcpy(void *dst, const void *src, size_t n);

#endif
```

Bonus header adds nothing structurally different — same prototypes, just compiled with `-D GNL_BONUS` or kept in separate `_bonus` files per subject convention.

---

## 6. Utils

You cannot use libft. Here are minimal, GNL-scoped reimplementations — deliberately small, no unrelated features.

### 6.1 `gnl_strlen`

```c
size_t	gnl_strlen(const char *s)
{
	size_t	i;

	i = 0;
	if (!s)
		return (0);
	while (s[i])
		i++;
	return (i);
}
```

### 6.2 `gnl_strchr` — must find `\0` too (used to locate end of buffer)

```c
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
```

### 6.3 `gnl_strjoin` — frees `s1`, this is the workhorse of the whole project

```c
char	*gnl_strjoin(char *s1, char *s2)
{
	char	*result;
	size_t	len1;
	size_t	len2;

	len1 = gnl_strlen(s1);
	len2 = gnl_strlen(s2);
	result = malloc(len1 + len2 + 1);
	if (!result)
		return (NULL);
	gnl_memcpy(result, s1, len1);
	gnl_memcpy(result + len1, s2, len2 + 1); /* includes s2's '\0' */
	free(s1);
	return (result);
}
```

**Why free `s1` inside `strjoin`?** Because `stash = gnl_strjoin(stash, read_buf)` is the pattern used every loop iteration. If `strjoin` didn't free the old `stash`, you'd leak it every single read. This one design decision eliminates an entire category of leaks — internalize it.

### 6.4 `gnl_substr` — extracts the line

```c
char	*gnl_substr(char const *s, unsigned int start, size_t len)
{
	char	*sub;
	size_t	i;

	if (!s)
		return (NULL);
	if (start > gnl_strlen(s))
		return (gnl_strjoin(malloc(1) ? (char *)"" : NULL, "")); /* edge-safe empty */
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
```

*(In practice, keep `gnl_substr` simple and don't over-engineer the out-of-range case — the version above is intentionally defensive; a leaner version that trusts internal callers is also acceptable and more Norm-friendly.)*

### 6.5 `gnl_memcpy`

```c
void	*gnl_memcpy(void *dst, const void *src, size_t n)
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
```

---

## 7. Building `get_next_line.c` — Incrementally

Build this in stages and **test after each one**. Don't write the whole thing at once.

### Stage 1 — Read the whole file into stash, ignore lines (sanity check plumbing)

```c
char	*get_next_line(int fd)
{
	static char	*stash;
	char		read_buf[BUFFER_SIZE + 1];
	ssize_t		bytes_read;

	bytes_read = 1;
	while (bytes_read > 0)
	{
		bytes_read = read(fd, read_buf, BUFFER_SIZE);
		if (bytes_read < 0)
			return (NULL);
		read_buf[bytes_read] = '\0';
		stash = gnl_strjoin(stash, read_buf);
	}
	return (stash); /* TEMP: return everything, don't do this in final version */
}
```
Compile, run against a small file, confirm the *entire* file content comes back concatenated. This proves `read`/`strjoin`/null-termination are wired correctly before you add line-splitting complexity.

### Stage 2 — Stop reading once `\n` is found

```c
char	*get_next_line(int fd)
{
	static char	*stash;
	char		read_buf[BUFFER_SIZE + 1];
	ssize_t		bytes_read;

	if (fd < 0 || BUFFER_SIZE <= 0)
		return (NULL);
	bytes_read = 1;
	while (!stash || !gnl_strchr(stash, '\n'))
	{
		bytes_read = read(fd, read_buf, BUFFER_SIZE);
		if (bytes_read < 0)
		{
			free(stash);
			stash = NULL;
			return (NULL);
		}
		if (bytes_read == 0)
			break ;
		read_buf[bytes_read] = '\0';
		stash = gnl_strjoin(stash, read_buf);
		if (!stash)
			return (NULL);
	}
	if (!stash || stash[0] == '\0')
	{
		free(stash);
		stash = NULL;
		return (NULL);
	}
	return (stash); /* STILL TEMP: full stash, not yet split */
}
```
Test: with `BUFFER_SIZE=5` on a file with long lines, confirm the loop performs *multiple* reads per call but stops the moment a `\n` appears (add a `printf` to stderr counting reads — remove before submission).

### Stage 3 — Split off the line, keep the remainder

```c
static char	*extract_line(char *stash)
{
	char	*newline_pos;
	size_t	len;

	newline_pos = gnl_strchr(stash, '\n');
	if (newline_pos)
		len = (size_t)(newline_pos - stash) + 1;
	else
		len = gnl_strlen(stash);
	return (gnl_substr(stash, 0, len));
}

static char	*extract_remainder(char *stash)
{
	char	*newline_pos;
	char	*remainder;

	newline_pos = gnl_strchr(stash, '\n');
	if (!newline_pos)
	{
		free(stash);
		return (NULL);
	}
	remainder = gnl_substr(stash, (newline_pos - stash) + 1,
			gnl_strlen(stash));
	free(stash);
	return (remainder);
}

char	*get_next_line(int fd)
{
	static char	*stash;
	char		read_buf[BUFFER_SIZE + 1];
	ssize_t		bytes_read;
	char		*line;

	if (fd < 0 || BUFFER_SIZE <= 0)
		return (NULL);
	bytes_read = 1;
	while (!stash || !gnl_strchr(stash, '\n'))
	{
		bytes_read = read(fd, read_buf, BUFFER_SIZE);
		if (bytes_read < 0)
		{
			free(stash);
			stash = NULL;
			return (NULL);
		}
		if (bytes_read == 0)
			break ;
		read_buf[bytes_read] = '\0';
		stash = gnl_strjoin(stash, read_buf);
		if (!stash)
			return (NULL);
	}
	if (!stash || stash[0] == '\0')
	{
		free(stash);
		stash = NULL;
		return (NULL);
	}
	line = extract_line(stash);
	stash = extract_remainder(stash);
	return (line);
}
```

This is your **final mandatory version**. Walk through it line by line and make sure you can explain *why* each branch exists before moving on — that's exactly what the evaluator will ask you to do.

---

## 8. Buffer Size Deep Dive

| `BUFFER_SIZE` | Behavior | What it stress-tests |
|---|---|---|
| `1` | One byte per `read()` call — the while loop iterates once per character until a `\n` shows up | Correctness of the "keep reading until newline" loop; off-by-one errors in `read_buf[bytes_read] = '\0'` |
| `42` (default) | Moderate, typical multi-read-per-line behavior on normal text files | General correctness |
| `9999` / large | Often reads an entire small file in one `read()` call | Whether you correctly handle a *single* read containing *multiple* lines (remainder logic) |
| Huge (e.g. `1000000`) on a large file | One `read()` may return several megabytes containing dozens of lines | Performance — you must **not** re-read the file for every line; the remainder must be reused, not re-fetched |

**Common bug**: code that works at `BUFFER_SIZE=42` but segfaults at `BUFFER_SIZE=1` almost always has an off-by-one in the null-termination (`read_buf[bytes_read]` when `bytes_read` could legitimately be 0 or 1) or an unguarded `gnl_strchr` call on a `NULL` stash.

Compile and test against **all** of these values before considering the mandatory part done:
```bash
cc -Wall -Wextra -Werror -D BUFFER_SIZE=1     get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
cc -Wall -Wextra -Werror -D BUFFER_SIZE=42    get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
cc -Wall -Wextra -Werror -D BUFFER_SIZE=9999  get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
cc -Wall -Wextra -Werror -D BUFFER_SIZE=10000000 get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
```

---

## 9. Edge Cases Catalogue

| Case | Expected behavior |
|---|---|
| Empty file | First call returns `NULL` immediately |
| File with no trailing `\n` | Last call returns the final chunk *without* a `\n`, next call returns `NULL` |
| File that's exactly one line ending in `\n` | One call returns the line+`\n`, next call returns `NULL` |
| `fd = -1` or invalid fd | Return `NULL`, no crash |
| `BUFFER_SIZE = 0` or negative (if testable via macro) | Guard against infinite loop / `read()` with size 0 always returning 0 |
| Reading from `stdin` (fd 0) interactively | Should behave identically to file reads — blocks on `read()` until input or EOF (Ctrl-D) |
| Very long single line (larger than several `BUFFER_SIZE`s) | `strjoin` must keep growing the stash correctly across many reads |
| File with only `\n\n\n` | Each call returns `"\n"` |
| Calling `get_next_line` on a **closed** fd | `read()` returns -1, function returns `NULL`, and (for the bonus) that fd's stash entry must not corrupt others |
| Multiple fds interleaved (bonus) | fd 3 and fd 4 read alternately must never mix their leftover bytes |
| `malloc` failure mid-read (rare, hard to test) | Should not crash; returning `NULL` and leaking rather than crashing is the pragmatic minimum, though a fully clean implementation frees what it can |

---

## 10. Testing Strategy (against real libc behavior)

The gold standard: compare your output **byte-for-byte** against `fgets`/`cat` on the same file.

```c
/* tests/main.c */
#include "../get_next_line.h"
#include <fcntl.h>
#include <stdio.h>

int	main(int argc, char **argv)
{
	int		fd;
	char	*line;
	int		count;

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return (1);
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("open");
		return (1);
	}
	count = 0;
	while ((line = get_next_line(fd)) != NULL)
	{
		printf("[%d] %s", ++count, line);
		free(line);
	}
	close(fd);
	printf("--- EOF, total lines: %d ---\n", count);
	return (0);
}
```

Cross-check script:
```bash
#!/bin/bash
# compare_gnl.sh — diff your GNL output against cat
for bs in 1 5 42 1000 100000; do
  cc -Wall -Wextra -Werror -D BUFFER_SIZE=$bs get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
  ./gnl_test testfile.txt | grep -v '^\[' > /tmp/gnl_out.txt   # strip the [n] prefixes for comparison, or write a stripped print mode
  echo "BUFFER_SIZE=$bs: $(diff <(cat testfile.txt) /tmp/gnl_out.txt && echo OK || echo MISMATCH)"
done
```

Also run under **Valgrind** for leaks and invalid access, at minimum with `BUFFER_SIZE=1` and one large `BUFFER_SIZE`:
```bash
valgrind --leak-check=full --show-leak-kinds=all ./gnl_test testfile.txt
```
A clean mandatory implementation should report **0 bytes lost** (the *last* stash is freed either by the final `NULL`-returning call, or must be manually freed if you stop early — see pitfalls table).

---

## 11. Makefile

```makefile
NAME		= libgnl.a
CC			= cc
CFLAGS		= -Wall -Wextra -Werror
BUFFER_SIZE	?= 42

SRCS		= get_next_line.c get_next_line_utils.c
OBJS		= $(SRCS:.c=.o)

BONUS_SRCS	= get_next_line_bonus.c get_next_line_utils_bonus.c
BONUS_OBJS	= $(BONUS_SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	ar rcs $(NAME) $(OBJS)

%.o: %.c get_next_line.h
	$(CC) $(CFLAGS) -D BUFFER_SIZE=$(BUFFER_SIZE) -c $< -o $@

bonus: $(BONUS_OBJS)
	ar rcs $(NAME) $(BONUS_OBJS)

clean:
	rm -f $(OBJS) $(BONUS_OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all bonus clean fclean re
```

Adjust `NAME` if your subject wants a binary instead of a `.a` archive — GNL is frequently linked into a test binary rather than archived, check your campus's exact wording, but the rule set (`$(NAME)`, `all`, `clean`, `fclean`, `re`) must exist regardless.

---

## 12. Norm Compliance

Run `norminette` before every commit. Common GNL-specific violations:

- Functions over 25 lines — split `get_next_line` into `get_next_line` + `extract_line` + `extract_remainder` (as done above) specifically to stay under the limit.
- More than 4 parameters — none of the GNL functions need more than 3, so this is rarely an issue unless you over-engineer.
- More than 5 variables per function — count carefully in your main loop; the Stage-3 version above uses exactly `stash` (static, doesn't count toward the 5), `read_buf`, `bytes_read`, `line` = 3 locals, well under budget.
- Ternaries — forbidden by Norm; the `(newline_pos - stash) + 1` style logic above avoids them, don't be tempted to write `len = newline_pos ? ... : ...`.
- `for` loops — Norm allows only `while`; every loop above uses `while`.

---

## 13. Bonus: Multi-FD with One Static Variable

### 13.1 The constraint

> "The get_next_line() function must be written using a **single static variable** of type char *, declared in the get_next_line() function."

This is stricter than it sounds — not "one static per function," but **one static variable total** in the entire bonus implementation, and it must live inside `get_next_line` itself.

### 13.2 The trick: a static array of stashes, indexed by fd

```c
#define MAX_FD 1024   /* or dynamically sized if your subject forbids fixed limits */

char	*get_next_line(int fd)
{
	static char	
