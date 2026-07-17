# Get Next Line — The Crash Course
*Theory → Visual → Practice → Exercise, for every concept, front to back.*

Format used for every topic in this book:
> **Definition** — plain-language explanation
> **Visual** — ASCII diagram of what's happening in memory/the OS
> **In GNL** — why this concept exists in this specific project
> **Practice** — real code
> **Exercise** — a GNL-scoped problem to solve yourself before moving on

---

# PART I — FOUNDATIONS

## Chapter 1: Pointers & Memory Addresses

**Definition**
A pointer is a variable whose value is a memory address rather than data itself. `char *p` means "p holds the address of a `char`." Dereferencing (`*p`) means "go to that address and give me the byte stored there."

**Visual**
```
Variable:     p                 Memory:
            +--------+          Address   Value
            | 0x1004 |   ---->  0x1004    'H'
            +--------+          0x1005    'e'
              (p itself           0x1006    'l'
               lives at             0x1007    'l'
               0x2000)              0x1008    'o'
                                     0x1009    '\0'
```
`p` points at `0x1004`. `*p` is `'H'`. `p + 1` is the address `0x1005` (pointer arithmetic advances by `sizeof(char)` = 1 byte). `*(p+1)` is `'e'`.

**In GNL**
Every string you touch — `read_buf`, `stash`, the returned `line` — is a `char *`. You will constantly walk these with pointer arithmetic (`s + i`, `newline_pos - stash`) to compute lengths and offsets. If your mental model of "a pointer is just a number, an address" isn't solid, every GNL bug will look mysterious.

**Practice**
```c
char buf[6] = "Hello";
char *p = buf;
printf("%c\n", *p);       // H
printf("%c\n", *(p + 1)); // e
printf("%ld\n", (long)(p + 5 - p)); // 5 -> pointer subtraction gives a distance
```

**Exercise**
Given `char *stash = "line1\nline2\n";` and a pointer `char *nl` that you set to point at the first `\n`, write the one-line expression that computes *how many characters come before that newline* (this is exactly the computation `extract_line` needs). 
*(Answer: `nl - stash`)*

---

## Chapter 2: Dynamic Memory — `malloc` and `free`

**Definition**
`malloc(n)` asks the OS/allocator for `n` contiguous bytes on the **heap** and returns a pointer to the first byte, or `NULL` if it fails. Heap memory persists until you explicitly `free()` it — unlike stack variables (like `read_buf` inside a function), which vanish when the function returns.

**Visual**
```
Stack (per function call, auto-destroyed on return):
  read_buf[BUFFER_SIZE+1]   <-- gone the instant get_next_line() returns

Heap (persists until free()'d):
  stash = malloc(50)   ---->   [ 5 0  b y t e s   o f   m e m o r y ]
                                 ^
                                 stash points here, survives across calls
```

**In GNL**
This is *the* reason GNL works at all: the line you return must **outlive** the function call (the caller uses it after `get_next_line` returns), so it must be heap-allocated. Meanwhile `stash` also needs to survive between calls — but for a different reason (see Chapter 3, static storage), even though it's also heap-allocated.

Golden rule: **every `malloc` has exactly one matching `free`.** Two frees on the same pointer = crash (double free). Zero frees = leak.

**Practice**
```c
char *line = malloc(10);
if (!line)
    return (NULL);          // ALWAYS check malloc's return
line[0] = 'h';
line[1] = '\0';
printf("%s\n", line);
free(line);                  // exactly once
line = NULL;                 // good habit: avoid dangling pointer reuse
```

**Exercise**
Spot the bug:
```c
char *a = gnl_strjoin(stash, buf);
stash = gnl_strjoin(a, buf2);
```
*(Answer: `gnl_strjoin` already frees its first argument internally per our design in Chapter 6 — but here `a`'s result is never assigned back to a variable that outlives this snippet before being immediately overwritten as `stash`'s new value's first argument; actually the subtler bug is that `a` itself was never freed if `gnl_strjoin` doesn't free `s1`... Reinforces: know exactly which of your functions own the free responsibility for which argument, and document it in a comment above the function.)*

---

## Chapter 3: Static Variables — Scope vs. Lifetime

**Definition**
A `static` local variable is declared inside a function but **initialized only once**, and it **keeps its value between calls** instead of being destroyed when the function returns. Scope (where you can *see* the name) stays local to the function; lifetime (how long the *memory* exists) becomes the whole program's runtime.

**Visual**
```
Call 1 of get_next_line(fd):
  static char *stash = NULL;   <-- created once, ever
  stash = "leftover after call 1"

Call 2 of get_next_line(fd):
  static char *stash;    <-- NOT reset to NULL! still holds "leftover after call 1"
```
Compare to a normal local variable, which would reset to garbage/uninitialized every call — static is the *only* built-in way in C to remember state across calls without global variables (which GNL's subject forbids) or without the caller passing state back in explicitly (which the mandatory prototype `get_next_line(int fd)` doesn't allow, since there's no extra parameter for it).

**In GNL**
This is the mechanism, full stop. There is no other legal way to remember "bytes I already read but haven't returned yet" between one call and the next.

**Practice**
```c
int counter(void)
{
    static int n = 0;
    n++;
    return (n);
}
// counter() -> 1
// counter() -> 2
// counter() -> 3
```

**Exercise**
Why can't you just declare `char *stash = NULL;` (without `static`) at the top of `get_next_line` and expect it to remember data between calls? Trace through what happens on the second call.
*(Answer: without `static`, `stash` is a fresh stack variable every call, re-initialized to `NULL` each time — any data from the previous call is lost, and worse, the heap memory it pointed to is now leaked since nothing points to it anymore.)*

---

## Chapter 4: File Descriptors & the OS I/O Model

**Definition**
A file descriptor (fd) is a small non-negative integer the OS hands you when you `open()` a file — it's an index into the process's private table of "currently open things." `0` = stdin, `1` = stdout, `2` = stderr are pre-opened for every process. Regular files, pipes, and sockets are all just fds to your code — `read()` treats them uniformly.

**Visual**
```
Process's fd table:
  fd 0  -> stdin
  fd 1  -> stdout
  fd 2  -> stderr
  fd 3  -> "myfile.txt" (opened by your open() call)
  fd 4  -> another opened file

get_next_line(3)  reads from myfile.txt
get_next_line(4)  reads from the other file
                  (these must NOT share state — see Chapter 13, bonus)
```

**In GNL**
The function signature `get_next_line(int fd)` is deliberately fd-agnostic — it doesn't know or care if it's reading a real file, `stdin`, or a pipe. This is also *why* the bonus part (multiple fds) is meaningfully harder: your single static variable must be able to distinguish "leftover bytes for fd 3" from "leftover bytes for fd 4."

**Practice**
```c
int fd = open("data.txt", O_RDONLY);
if (fd < 0) { perror("open"); return (1); }
// fd is now e.g. 3
char *line = get_next_line(fd);
close(fd);
```

**Exercise**
If you `open()` the same file twice, getting `fd = 3` and `fd = 4`, and call `get_next_line(3)` then `get_next_line(4)`, should the second call return the *same* first line as the first call, or continue from where fd 3 left off? Why?
*(Answer: the same first line — each fd has its own independent read position maintained by the kernel; they're two separate "cursors" into the file, even though it's the same file on disk.)*

---

## Chapter 5: The `read()` Syscall Contract

**Definition**
```c
ssize_t read(int fd, void *buf, size_t count);
```
Asks the kernel to copy up to `count` bytes from the fd's current position into `buf`, advancing that position by however many bytes it actually copied. Returns the number of bytes actually read (which can be *less* than `count`), `0` at end-of-file, or `-1` on error.

**Visual**
```
File on disk: "Hello, World!\n"    (14 bytes)
Call: read(fd, buf, 5)

Before:  cursor at byte 0
After:   buf = "Hello"   (5 bytes copied)
         cursor now at byte 5
         return value = 5

Next call: read(fd, buf, 5)
After:   buf = ", Wor"
         cursor now at byte 10
         return value = 5

Next call: read(fd, buf, 5)
After:   buf = "ld!\n"   <- only 4 bytes exist, so only 4 are copied
         return value = 4

Next call: read(fd, buf, 5)
After:   nothing to read
         return value = 0   (EOF)
```

**In GNL**
This is the *only* primitive you're given for getting bytes off disk (no `fgets`, no `lseek` to peek ahead). Every design decision in GNL flows from the fact that `read()` has zero concept of lines — it just moves bytes in `count`-sized (or smaller) chunks.

**Practice**
```c
char buf[BUFFER_SIZE + 1];
ssize_t n = read(fd, buf, BUFFER_SIZE);
if (n < 0)
    return (NULL);      // error
if (n == 0)
    { /* EOF, nothing more to read */ }
buf[n] = '\0';           // buf is NOT null-terminated by read()! you must do it
```

**Exercise**
Why is `buf[n] = '\0'` necessary, and why is it a bug to instead always write `buf[BUFFER_SIZE] = '\0'`?
*(Answer: `read()` only guarantees the first `n` bytes are valid data — bytes from index `n` onward in `buf` are leftover garbage from the previous call or uninitialized stack memory. Null-terminating at `[BUFFER_SIZE]` instead of `[n]` would treat garbage bytes as part of your string whenever `n < BUFFER_SIZE`, which happens on every partial read, e.g. anything at EOF or reading from a pipe/terminal.)*

---

## Chapter 6: Strings in C — No Built-in String Type

**Definition**
C has no native "string" object — a string is a *convention*: a contiguous run of `char`s terminated by a `'\0'` (NUL) byte. Every string function (`strlen`, `strcpy`, etc.) works by scanning forward until it hits that `'\0'`. There is no length stored anywhere else — if you lose the `'\0'` or miscompute it, every string function reads into whatever memory follows, which is undefined behavior.

**Visual**
```
char *s = "Hi!";

Memory:  [ H ][ i ][ ! ][\0]
index:     0    1    2    3

gnl_strlen(s) walks: index 0 ('H', not \0, keep going)
                     index 1 ('i', not \0, keep going)
                     index 2 ('!', not \0, keep going)
                     index 3 ('\0', STOP) -> return 3
```

**In GNL**
Every utils function you write (Chapter 7) exists purely to manipulate this convention correctly — concatenating strings (`strjoin`), searching them (`strchr`), slicing them (`substr`). Get the terminator wrong anywhere and you get either truncated output or a crash reading past allocated memory.

**Practice**
```c
size_t gnl_strlen(const char *s)
{
    size_t i = 0;
    while (s[i])       // s[i] != '\0' is implied by "truthiness" of a char
        i++;
    return (i);
}
```

**Exercise**
What does `gnl_strlen(NULL)` do if you don't guard against it, and why is this exact call guaranteed to happen somewhere in GNL?
*(Answer: dereferencing `NULL` crashes with a segfault. It's guaranteed to happen because on the very first call to `get_next_line`, `stash` is `static char *stash;` which defaults to `NULL` — any string function you call on `stash` before it's been assigned must handle `NULL` gracefully, which is why `gnl_strlen`/`gnl_strjoin` above explicitly check `if (!s) return (0);` first.)*

---

## Chapter 7: Buffers — Fixed-Size Scratch Space

**Definition**
A buffer here specifically means a fixed-size chunk of memory (`char read_buf[BUFFER_SIZE + 1]`) used as temporary scratch space for one `read()` call, then discarded/reused on the next. Distinct from the *stash*, which grows dynamically and persists.

**Visual**
```
read_buf (fixed size, stack-allocated, reused every call):
+---+---+---+---+---+---+
| H | e | l | l | o |\0 |   <- BUFFER_SIZE=5, +1 for the '\0' you add
+---+---+---+---+---+---+
 0   1   2   3   4   5

stash (dynamic, heap-allocated, GROWS across calls via strjoin):
call 1: ""
call 2: "Hello"
call 3: "Hello, Wor"
call 4: "Hello, World!\n"   <- now contains a full line, ready to extract
```

**In GNL**
The `+1` on `BUFFER_SIZE + 1` exists solely to leave room for the `'\0'` you manually append after every `read()` — without it, writing `read_buf[BUFFER_SIZE] = '\0'` when `n == BUFFER_SIZE` (a full read) would write one byte past the array, corrupting the stack (undefined behavior, often invisible until it isn't).

**Practice**
```c
char read_buf[BUFFER_SIZE + 1];   // note the +1
ssize_t n = read(fd, read_buf, BUFFER_SIZE);
read_buf[n] = '\0';                // safe: n is at most BUFFER_SIZE
```

**Exercise**
If `BUFFER_SIZE` is `1`, what is the maximum value `n` can be in a single `read()` call, and does `read_buf[n] = '\0'` still stay in-bounds for `char read_buf[BUFFER_SIZE + 1]`?
*(Answer: max `n` is 1 (can't read more than you asked for); `read_buf` has 2 bytes (`BUFFER_SIZE+1` = 2), so `read_buf[1] = '\0'` is index 1 of a 2-byte array — perfectly in bounds.)*

---

# PART II — THE PROBLEM DOMAIN

## Chapter 8: Byte Streams vs. Lines — The Abstraction Gap

**Definition**
A *byte stream* is what `read()` gives you: an undifferentiated sequence of bytes with no structure. A *line* is a higher-level concept — "everything up to and including the next `\n`" — that the OS/filesystem has no knowledge of. GNL's entire job is building the second abstraction on top of the first.

**Visual**
```
Byte stream reality (what read() sees, one read at a time):
[H][e][l][l][o][,][ ][W][o][r][l][d][!][\n][F][o][o]

Line abstraction (what get_next_line must produce, one call at a time):
Call 1 -> "Hello, World!\n"
Call 2 -> "Foo"              (no trailing \n — EOF hit mid-line)
Call 3 -> NULL
```

**In GNL**
Naming this gap explicitly is useful because it explains *why* the project isn't "just call read() in a loop" — you must buffer, search, and slice, none of which `read()` does for you.

**Exercise**
If a single `read()` call (with a large `BUFFER_SIZE`) returns bytes containing **three** complete lines plus a partial fourth, how many calls to `get_next_line` does it take to consume all of that data, and what has to happen to the "extra" two-and-a-bit lines that weren't returned by the first call?
*(Answer: 4 calls total (3 full lines + the partial 4th, assuming a later read completes it or EOF is hit) — the extra lines beyond the first must sit in `stash`, untouched, waiting for the next call, which is why the remainder-saving step is not optional.)*

---

## Chapter 9: The Stash Pattern

**Definition**
"Stash" (a name of convenience, not a required identifier) refers to the static buffer that accumulates raw bytes across `read()` calls and holds the *unconsumed remainder* after each line is sliced off. It's the single piece of shared state connecting one call to the next.

**Visual**
```
                 ┌─────────────────────────────┐
   read() -----> │      stash (grows)          │
                 └─────────────────────────────┘
                          |
                 does stash contain '\n'?
                    /            \
                  no              yes
                   |                |
             read() more      slice off up to
             append to        and including '\n'
             stash, repeat     -> this is "line"
                                     |
                            remainder (after \n)
                            becomes the NEW stash
                            for next call
```

**In GNL**
Every mandatory-part bug ultimately traces back to mismanaging this one variable: growing it wrong (leak), slicing it wrong (off-by-one), or not resetting it correctly at EOF (crash or infinite loop).

**Exercise**
After the *last* line of a file (no trailing `\n`) is returned, what should `stash` be set to, and why does this matter for the *next* call correctly returning `NULL`?
*(Answer: `stash` must be set to `NULL` (after freeing whatever it pointed to) — the next call's very first check, `if (!stash || stash[0] == '\0') return NULL;`, depends on `stash` being `NULL`, not a dangling or empty-but-non-NULL pointer.)*

---

## Chapter 10: Designing the Algorithm

**Definition**
Algorithm design here means translating the stash pattern into an explicit, ordered set of steps — the loop condition, the exit conditions, and the two extraction operations — before writing any C.

**Visual — full state machine**
```
        ┌───────────────┐
        │  fd invalid?  │──yes──> return NULL
        └───────┬───────┘
                │no
                v
        ┌─────────────────────┐
        │ stash has '\n' ?    │<─────────────┐
        └───────┬─────────────┘              │
             no │        yes                 │
                v          │                  │
        ┌───────────────┐  │                  │
        │  read() more  │  │                  │
        └───────┬───────┘  │                  │
                v          │                  │
        ┌───────────────┐  │                  │
        │ n <= 0 ?      │  │                  │
        └───┬───────┬───┘  │                  │
          yes│     no│      │                  │
             v       └──────┼──> append to stash
       (break loop)         │    go back to check ┘
                v            v
        ┌─────────────────────────┐
        │ stash empty/NULL?      │──yes──> return NULL
        └───────────┬─────────────┘
                   no
                    v
        ┌─────────────────────────┐
        │ extract_line(stash)     │──> line to return
        └───────────┬─────────────┘
                    v
        ┌─────────────────────────┐
        │ extract_remainder(stash)│──> new stash value
        └───────────┬─────────────┘
                    v
              return line
```

**Exercise**
Trace this state machine by hand for the input `"ab"` (2 bytes, no newline, then EOF) with `BUFFER_SIZE = 10`. What gets returned on call 1 and call 2?
*(Answer: Call 1 — loop: stash has no `\n` yet (it's `NULL`), so read() runs once, gets `n=2` (`"ab"`), appends to stash -> `"ab"`, loop checks again: still no `\n`, read() again -> `n=0` (EOF) -> break. stash is `"ab"`, non-empty, so extract_line returns `"ab"` (no newline found, so `len = strlen(stash)`), extract_remainder sees no `\n` and returns `NULL`. Call 2 — stash is `NULL`, loop's `read()` runs once more against an already-exhausted fd, gets `n=0` immediately, breaks; stash still `NULL` -> returns `NULL`.)*

---

# PART III — BUILDING GNL, PRACTICALLY

## Chapter 11: The Utils Library

For each function: **definition, visual, code, exercise.**

### 11.1 `gnl_strlen`
**Definition**: counts characters before the terminating `'\0'`.
**Visual**: see Chapter 6.
**Code**:
```c
size_t gnl_strlen(const char *s)
{
    size_t i = 0;
    if (!s)
        return (0);
    while (s[i])
        i++;
    return (i);
}
```
**Exercise**: Why must this function accept `NULL` without crashing, specifically in the context of GNL's very first call? *(Same answer as Ch.6 — `stash` starts `NULL`.)*

### 11.2 `gnl_strchr`
**Definition**: scans a string for the first occurrence of character `c`, returning a pointer to it (or `NULL` if absent). Special case: if `c` is `'\0'`, returns a pointer to the string's terminator itself (matches real libc behavior).
**Visual**:
```
s = "foo\nbar"
gnl_strchr(s, '\n')
        f  o  o \n b  a  r \0
idx:    0  1  2  3  4  5  6  7
                  ^
             returns s+3
```
**Code**: (see previous guide, §6.2)
**Exercise**: Why does GNL need `strchr` to search for `'\n'` specifically, rather than, say, counting characters? *(Answer: you don't know in advance where the newline is or if there even is one — `strchr` is the search primitive that answers "is there a full line ready yet?" every iteration of the main loop.)*

### 11.3 `gnl_strjoin`
**Definition**: allocates a new string containing `s1` followed by `s2`, and **frees `s1`** as part of its contract (a deliberate, non-standard design choice for this project — real libc's `strjoin`-equivalents don't free their inputs, but GNL's internal helper does, by convention, to prevent leaking the old stash on every append).
**Visual**:
```
s1 = "Hello, "      s2 = "World!"
strjoin(s1, s2):
  1. malloc(len1+len2+1)
  2. copy s1 in, copy s2 in (including s2's \0)
  3. free(s1)        <-- old stash reclaimed
  4. return new pointer
result = "Hello, World!"
```
**Code**: (see previous guide §6.3)
**Exercise**: If you called `gnl_strjoin(stash, read_buf)` but *forgot* to reassign the result back to `stash` (i.e., wrote just `gnl_strjoin(stash, read_buf);` as a bare statement), what breaks?
*(Answer: two things — you leak the newly allocated joined string (nothing points to it), AND `stash` still points to memory that was just `free()`'d inside `strjoin`, making `stash` a dangling pointer — using it afterward is undefined behavior, likely a crash or corrupted data.)*

### 11.4 `gnl_substr`
**Definition**: allocates and returns a new string containing `len` characters from `s` starting at index `start`.
**Visual**:
```
s = "Hello, World!\nFoo"
substr(s, 0, 14)  ->  "Hello, World!\n"   (the line)
substr(s, 14, 3)  ->  "Foo"                (the remainder, starting right after \n)
```
**Code**: (see previous guide §6.4)
**Exercise**: In `extract_line`, why is `len` computed as `(newline_pos - stash) + 1` rather than just `newline_pos - stash`?
*(Answer: `newline_pos - stash` gives the *index* of the `\n` itself (0-based), so the *count* of characters from index 0 up to **and including** the `\n` is one more than that index — hence `+1`. Forgetting the `+1` truncates every returned line, dropping its trailing newline.)*

### 11.5 `gnl_memcpy`
**Definition**: copies `n` raw bytes from `src` to `dst`, byte by byte, with no interpretation of content (unlike string functions, doesn't stop at `'\0'`).
**Visual**:
```
src: [H][e][l][l][o]        n=5
dst: [ ][ ][ ][ ][ ][ ]  -> [H][e][l][l][o][ ]
```
**Code**: (see previous guide §6.5)
**Exercise**: Why does `gnl_strjoin` need `memcpy` (which copies exactly `n` bytes) instead of just looping character by character until `'\0'`, when copying `s2` into the result?
*(Answer: it needs to copy `len2 + 1` bytes specifically so that `s2`'s terminating `'\0'` gets copied too — a naive "copy until you hit `\0`" loop would work identically here, but using `memcpy` with an explicit count makes the "we're copying the terminator too" intent explicit and less error-prone to get the count wrong on.)*

---

## Chapter 12: Building `get_next_line.c` — Staged, With Full Explanation

*(This chapter mirrors the incremental build from the previous guide — Stage 1 → 2 → 3 — but here every stage gets its own theory recap and exercise.)*

### Stage 1 Theory: Prove the plumbing works
Before worrying about lines at all, prove that `read()` + `strjoin` + null-termination correctly reconstruct a whole file when concatenated blindly. This isolates "do I understand read()/malloc" bugs from "do I understand line-splitting" bugs.

**Code**: (see previous guide, Stage 1)

**Exercise**: Run Stage 1 against a 100-byte file with `BUFFER_SIZE=10`. How many `read()` calls should occur, and how would you instrument the code (temporarily) to count them?
*(Answer: 10 full reads of 10 bytes each, then an 11th call returning 0 (EOF) — add a `static int reads = 0;` incremented each time `read()` is called, printed to `stderr` (never `stdout`, to avoid polluting the line output) right before the function returns.)*

### Stage 2 Theory: Stop at the newline, not at EOF
The loop condition changes from "keep reading until EOF" to "keep reading until stash **contains** a newline OR EOF." This is the single most important design decision for performance with large `BUFFER_SIZE`.

**Code**: (see previous guide, Stage 2)

**Exercise**: With `BUFFER_SIZE = 1000000` on a file where line 1 is 20 bytes long and the file is 5MB total, how many `read()` calls does *Stage 1's* design perform to get the first line, versus *Stage 2's* design? Why does this matter for the evaluator's performance tests?
*(Answer: Stage 1 (reads until EOF regardless) still performs the same number of physical `read()` calls either way in this single-fd case since both loop until an exit condition is met per call — the real distinction Stage 2 introduces is *not re-reading the whole file for every line thereafter*, since Stage 1 as shown doesn't even return individual lines yet. The performance-critical property Stage 2 introduces is: once `\n` is found, STOP calling read() for *this* line, leaving unread file content on disk for the *next* call to fetch — Stage 1 by contrast would have already pulled the whole file into `stash` in one go and — if it did also split lines — would still be fine performance-wise for a single fd, but the real risk this stage guards against is a design where each call to `get_next_line` mistakenly re-opens/re-reads from the start, which naive first attempts sometimes do.)*

### Stage 3 Theory: Extraction and the remainder handoff
Splitting `stash` into `line` (returned) and new-`stash` (kept) is the only place a line boundary is actually decided and enforced.

**Code**: (see previous guide, Stage 3 — full mandatory version)

**Exercise**: Modify Stage 3 mentally (don't need to write it) to add a `feature`: return lines **without** their trailing `\n`. Which single function would you change, and what's the one-line diff?
*(Answer: change `extract_line`'s length calculation from `len = (newline_pos - stash) + 1` to `len = newline_pos - stash` when a newline is found — dropping the `+1` excludes the `\n` itself from the returned substring. This is exactly the kind of live modification an evaluator may ask for.)*

---

## Chapter 13: Buffer Size — Theory Recap + Exercises

**Definition recap**: `BUFFER_SIZE` is a compile-time constant controlling how many bytes each individual `read()` call requests. It does **not** limit the length of a line GNL can return — arbitrarily long lines are handled by the growing `stash`, regardless of how small `BUFFER_SIZE` is.

**Visual**
```
BUFFER_SIZE=1: many tiny reads per line       [H][e][l][l][o][\n]  -> 6 read() calls
BUFFER_SIZE=6: one read covers the whole line [Hello\n]            -> 1 read() call
BUFFER_SIZE=100 on a 6-byte line: one read overshoots into next line's data,
                                   which must be correctly held in stash as remainder
```

**Exercise Set**
1. Explain why `BUFFER_SIZE` affects *how many times* `read()` is called but never affects *correctness* of the final returned lines (assuming no bugs).
2. What's the maximum number of `read()` calls needed to fetch a single `N`-byte line, in terms of `N` and `BUFFER_SIZE`?
*(Answers: 1) each read just adds more raw bytes into the same growing stash; the line-splitting logic operates on `stash`'s content, not on `read_buf` directly, so it's indifferent to how the bytes arrived. 2) `ceil(N / BUFFER_SIZE)`, plus possibly one more call to detect EOF/confirm no more data if the line is the last in the file.)*

---

## Chapter 14: Edge Cases — Theory + Exercises

Revisit each edge case as a mini-exercise: **predict the output, then verify by running it.**

| # | Scenario | Predict before checking |
|---|---|---|
| 1 | Empty file | What does call 1 return? |
| 2 | File = `"\n"` only | What does call 1 return? Call 2? |
| 3 | File = `"abc"` (no `\n`, then EOF) | Call 1? Call 2? |
| 4 | `fd = -1` | Return value? Any crash risk? |
| 5 | `get_next_line` called on an fd that was `close()`'d | Return value? |
| 6 | Two lines returned by a single giant `read()` (huge `BUFFER_SIZE`) | Does call 2 trigger a *new* `read()`, or does it serve straight from `stash`? |

*(Answers: 1) NULL immediately, no read even needed to notice stash is NULL/empty — though your loop will still call read() once and get 0 back, which is correct and cheap. 2) Call 1 returns `"\n"`; call 2 returns `NULL`. 3) Call 1 returns `"abc"` with no trailing `\n`; call 2 returns `NULL`. 4) NULL; the `fd < 0` guard prevents ever calling `read()` with a bad fd, avoiding relying on `read()`'s own error return. 5) `read()` on a closed fd returns `-1` (EBADF) — your code should catch `bytes_read < 0` and return `NULL`, freeing that fd's stash slot. 6) Straight from stash — no new read() needed, which is the entire point of Chapter 13's performance discussion.)*

---

## Chapter 15: Testing Theory — Why Diff Against `cat`/`fgets`

**Definition**: Differential testing means comparing your function's output against a trusted reference implementation on identical input, rather than eyeballing correctness.

**Visual**
```
testfile.txt --> [ cat testfile.txt ]      --> reference_output.txt
             \-> [ your gnl_test binary ]  --> your_output.txt

diff reference_output.txt your_output.txt
  (empty output) == PASS
  (any lines)    == investigate mismatch
```

**Exercise**: Design a one-file test harness that runs your GNL binary against the *same* file at `BUFFER_SIZE` values 1, 5, 42, 9999, and reports PASS/FAIL for each, without manually re-running commands. *(See the `compare_gnl.sh` script in the previous guide — write your own variant that also captures Valgrind's leak summary per run.)*

---

## Chapter 16: Makefile Theory

**Definition**: `make` is a dependency-graph build tool — it only recompiles files whose dependencies (source or header) have changed since the last build, based on file modification timestamps.

**Visual**
```
get_next_line.o depends on: get_next_line.c, get_next_line.h
                     |
        if get_next_line.h changes (e.g. you add a prototype),
        make recompiles get_next_line.o even though .c itself is unedited
```

**Exercise**: Why does the Makefile pattern rule include `get_next_line.h` as a dependency of every `.o` file (`%.o: %.c get_next_line.h`), rather than just `%.c`?
*(Answer: if you change a macro like `BUFFER_SIZE`'s default or add a new prototype in the header, object files compiled against the *old* header would be stale/inconsistent — listing the header as a dependency forces a rebuild of everything whenever it changes.)*

---

## Chapter 17: Norm Theory

**Definition**: 42's Norm is a style/complexity ruleset (function length, variable count, forbidden constructs) enforced by the `norminette` tool — separate from correctness, but still graded.

**Exercise**: Count the local variables in the Stage-3 `get_next_line` function from Chapter 12 (excluding the static one, which doesn't count against the limit) and confirm it's under Norm's per-function variable cap.
*(Answer: `read_buf`, `bytes_read`, `line` = 3 locals; well under the typical 5-variable cap.)*

---

## Chapter 18: Bonus Theory — One Static, Many File Descriptors

**Definition recap**: the bonus constraint requires exactly one `static` declaration in the whole file, yet multiple fds must maintain independent state.

**Visual**
```
static char *stash[1024];   <-- ONE declaration, but 1024 "slots"

stash[3] -> "leftover for fd 3"
stash[4] -> "leftover for fd 4"
stash[7] -> NULL (fd 7 never used yet)
```

**Exercise**: Explain, in your own words good enough to say out loud to an evaluator, why `static char *stash[1024]` satisfies "one static variable" even though it holds 1024 independent strings.
*(Answer: In C, an array is a single object/variable of a single (array) type — the *declaration* is one line, one identifier, one static storage allocation, regardless of how many elements it holds; "one static variable" refers to the declaration, not the amount of data it can address.)*

---

## Chapter 19: README Theory

**Definition**: the README's job at 42 is to prove you understand and can articulate the algorithm — not just that it compiles.

**Exercise**: Without looking back at the template, write (from memory) a two-sentence explanation of the stash algorithm suitable for the README's "Algorithm" section. Then compare it to Chapter 14 of the previous guide's template and check you covered: (a) why a static/persistent buffer is needed, (b) how a line is extracted, (c) how the remainder is preserved.

---

## Chapter 20: Pitfalls — Theory of *Why* Each One Happens

Revisit the pitfalls table from the previous guide, but for each row, articulate the **underlying conceptual misunderstanding** (not just the code fix):

| Pitfall | Underlying misconception |
|---|---|
| Segfault at `BUFFER_SIZE=1` | Treating `BUFFER_SIZE` as if it only affects "how many reads," forgetting it also constrains the *size of the array you declared* |
| Infinite loop on empty file | Confusing "loop until newline found" with "loop until true forever," forgetting `read()`'s `0` return means "no more data, ever" |
| Lines mixed between fds | Not internalizing that a single `static char *` cannot represent two independent conversations at once |
| Off-by-one dropping last char of line | Confusing an *index* (`newline_pos - stash`) with a *count* (`+1` needed) |
| Double free | Not tracking *which function owns the free* for a given pointer — solved by the `strjoin`-frees-`s1` convention being applied consistently everywhere |

---

## Chapter 21: Evaluation Prep Theory

**Definition**: 42 evaluations are oral — you must explain, not just present, working code. Evaluators intentionally probe the *theory* behind the code (why static, why this loop condition) as much as the code itself.

**Final integrative exercise**: Without any notes, explain out loud (or write from scratch) the entire algorithm end-to-end in under 90 seconds — this is effectively the oral defense you'll give. If you can't do this fluently, that's the concept to revisit, not the code.

---

# PART IV — The Step-by-Step Roadmap to Completion

This is the literal, in-order checklist to go from an empty folder to a submitted, evaluation-ready GNL. Every step has a concrete action, an expected result, and a "don't move on until" gate. Do not skip a gate — GNL punishes shortcuts more than almost any other 42 project because bugs compound silently (a Stage-2 mistake looks fine until Stage 3's edge cases expose it).

### Step 0 — Environment setup
1. Create the folder structure:
   ```bash
   mkdir get_next_line && cd get_next_line
   mkdir tests
   touch get_next_line.c get_next_line_utils.c get_next_line.h Makefile README.md
   touch tests/main.c
   git init
   ```
2. Create a `.gitignore`:
   ```
   *.o
   gnl_test
   libgnl.a
   ```
3. **Gate**: `git status` shows all the new files as untracked. Commit this skeleton:
   ```bash
   git add -A && git commit -m "init: repo skeleton"
   ```

### Step 1 — Write the header first, even though it's "just declarations"
1. Fill in `get_next_line.h` exactly as in Chapter 5 of the previous guide (include guards, `BUFFER_SIZE` default, all prototypes — even for functions you haven't written yet).
2. **Gate**: the header's include guard and braces are syntactically sound — the real test comes once `.c` files include it and compile clean in Step 2.
3. Commit: `git commit -am "header: prototypes and BUFFER_SIZE guard"`

### Step 2 — Build and test each utils function in total isolation
Do **not** write all five utils functions and then test them together — that hides which one is broken. One at a time:

1. Write `gnl_strlen` only.
2. Write a throwaway `tests/utils_test.c`:
   ```c
   #include <stdio.h>
   #include "../get_next_line.h"
   int main(void)
   {
       printf("%zu (expect 5)\n", gnl_strlen("Hello"));
       printf("%zu (expect 0)\n", gnl_strlen(""));
       printf("%zu (expect 0)\n", gnl_strlen(NULL));
       return (0);
   }
   ```
3. Compile and run:
   ```bash
   cc -Wall -Wextra -Werror get_next_line_utils.c tests/utils_test.c -o utils_test && ./utils_test
   ```
4. **Gate**: all three printed numbers match the "(expect N)" comment. If `NULL` crashes, fix the guard before continuing.
5. Repeat this exact cycle for `gnl_strchr`, then `gnl_memcpy`, then `gnl_strjoin`, then `gnl_substr` — one function, one test addition, one compile-run-verify, before starting the next.
6. Commit after each function passes: `git commit -am "utils: gnl_strlen tested"`, `git commit -am "utils: gnl_strchr tested"`, etc. (five small commits, not one big one).

### Step 3 — Stage 1 of `get_next_line.c`: prove `read()` + `strjoin` plumbing
1. Write the Stage 1 version (Chapter 7/12, "read entire file, ignore lines").
2. Create a small test file:
   ```bash
   printf "line one\nline two\nline three" > tests/basic.txt
   wc -c tests/basic.txt   # note the byte count, you'll want it later
   ```
3. Write `tests/main.c` to open `tests/basic.txt`, call `get_next_line` **once**, print the result, `free()` it.
4. Compile and run at the default `BUFFER_SIZE`:
   ```bash
   cc -Wall -Wextra -Werror -D BUFFER_SIZE=42 get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
   ./gnl_test
   ```
5. **Gate**: the full file content prints back, concatenated correctly, no crash, no garbage characters at the end.
6. Run under Valgrind now, even at this unfinished stage, to catch allocation bugs early:
   ```bash
   valgrind --leak-check=full ./gnl_test
   ```
   Expect a leak report here (Stage 1 doesn't free stash on the final return) — that's fine, just confirm there's no "invalid read/write" error, only "still reachable"/"definitely lost" from the known, temporary design.
7. Commit: `git commit -am "gnl: Stage 1 - read/strjoin plumbing verified"`

### Step 4 — Stage 2: stop reading once a newline is found
1. Modify `get_next_line` to the Stage 2 version (loop condition becomes newline-aware).
2. Add an instrumentation line (temporary, remove before final commit):
   ```c
   static int debug_reads = 0;
   /* inside the loop, right after read() */
   debug_reads++;
   fprintf(stderr, "[debug] read() call #%d\n", debug_reads);
   ```
3. Recompile and run against `tests/basic.txt` with `BUFFER_SIZE=5`:
   ```bash
   cc -Wall -Wextra -Werror -D BUFFER_SIZE=5 get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
   ./gnl_test
   ```
4. **Gate**: `stderr` shows the loop performing exactly `ceil(9/5) = 2` reads to get `"line one\n"` (9 bytes including the `\n`) before stopping — if it keeps reading past the point where `\n` appears in stash, your loop condition is checked in the wrong place (likely: checking for `\n` *before* the append instead of *after*).
5. Remove the debug lines. Recompile clean. Confirm it still builds with `-Werror` (stray unused variables from debug code are a common `-Werror` trip-up).
6. Commit: `git commit -am "gnl: Stage 2 - loop stops at newline, verified via debug counter"`

### Step 5 — Stage 3: extraction and remainder, the real mandatory version
1. Write `extract_line` and `extract_remainder` as static helper functions (Chapter 7/12, Stage 3).
2. Wire them into `get_next_line`.
3. Update `tests/main.c` to loop **until `NULL`**, printing every line with a counter (see Chapter 10's testing template).
4. Compile and run against `tests/basic.txt`:
   ```bash
   cc -Wall -Wextra -Werror -D BUFFER_SIZE=42 get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
   ./gnl_test
   ```
5. **Gate**: output is exactly:
   ```
   [1] line one
   [2] line two
   [3] line three
   --- EOF, total lines: 3 ---
   ```
   Note line 3 has no trailing `\n` in the printed output (because the source file doesn't end with one) — verify with `./gnl_test | cat -A` to see `$` markers for actual newlines if unsure.
6. Commit: `git commit -am "gnl: Stage 3 - mandatory complete, basic test passes"`

### Step 6 — Systematic BUFFER_SIZE sweep
1. Write the sweep loop (adapt from Chapter 15):
   ```bash
   for bs in 1 2 5 9999 100000; do
     cc -Wall -Wextra -Werror -D BUFFER_SIZE=$bs get_next_line.c get_next_line_utils.c tests/main.c -o gnl_test
     echo "=== BUFFER_SIZE=$bs ==="
     ./gnl_test
   done
   ```
2. **Gate**: identical `[1]/[2]/[3]` output at every single `BUFFER_SIZE` value, including `1`. Any crash or garbled output at `BUFFER_SIZE=1` specifically points to the `read_buf[BUFFER_SIZE+1]` sizing (Chapter 13).
3. Commit: `git commit -am "test: BUFFER_SIZE sweep passes at 1,2,5,9999,100000"`

### Step 7 — Edge case pass, one at a time, each its own test file
1. Empty file:
   ```bash
   : > tests/empty.txt
   ```
   Parameterize `tests/main.c` to take a filename argument (do this refactor once, now):
   ```c
   int main(int argc, char **argv)
   {
       int fd = open(argc > 1 ? argv[1] : "tests/basic.txt", O_RDONLY);
       /* ...rest unchanged... */
   }
   ```
   Run: `./gnl_test tests/empty.txt` → **Gate**: prints only `--- EOF, total lines: 0 ---`, no crash.
2. No trailing newline:
   ```bash
   printf "no newline at end" > tests/no_trailing_nl.txt
   ```
   Run and confirm the last line prints without a `\n` and the call after it returns `NULL` cleanly.
3. Only newlines:
   ```bash
   printf "\n\n\n" > tests/only_nl.txt
   ```
   **Gate**: three lines returned, each exactly `"\n"`.
4. Single huge line (bigger than several `BUFFER_SIZE`s):
   ```bash
   python3 -c "print('x' * 500000)" > tests/huge_line.txt
   ```
   Run at `BUFFER_SIZE=42`. **Gate**: no crash, correct length returned (check with `wc -c` on the file vs. the length of what you got back, accounting for the added `\n`).
5. Invalid fd:
   ```c
   printf("%p\n", (void *)get_next_line(-1));   // expect (nil)
   ```
6. Closed fd:
   ```c
   int fd = open("tests/basic.txt", O_RDONLY);
   close(fd);
   printf("%p\n", (void *)get_next_line(fd));   // expect (nil), no crash
   ```
7. Commit after each sub-step passes: `git commit -am "test: empty file edge case passes"`, and so on for each of the six.

### Step 8 — Full Valgrind pass on the finished mandatory part
1. Run against every test file, including the huge one:
   ```bash
   for f in tests/*.txt; do
     valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./gnl_test "$f" \
       && echo "$f: VALGRIND OK" || echo "$f: VALGRIND FAIL"
   done
   ```
2. **Gate**: every file reports `0 errors from 0 contexts` and `0 bytes in 0 blocks` for "definitely lost"/"indirectly lost." A few "still reachable" bytes can sometimes appear from libc internals unrelated to your code — if unsure, compare against a Valgrind run on a trivial `printf("hi")` program to see the baseline noise for your system.
3. Commit: `git commit -am "test: valgrind clean across all edge-case files"`

### Step 9 — Norm pass
1. Run `norminette` on both `.c` files and the header:
   ```bash
   norminette get_next_line.c get_next_line_utils.c get_next_line.h
   ```
2. **Gate**: `OK!` on all three files. Fix any reported violations (usually function length or variable count — see Chapter 17).
3. Commit: `git commit -am "norm: mandatory passes norminette"`
4. Tag this milestone: `git tag v1-mandatory`

### Step 10 — Makefile, for real this time
1. Write the full Makefile (Chapter 16 / previous guide's Makefile section).
2. **Gate checklist** — run each and confirm behavior:
   ```bash
   make        # builds libgnl.a from scratch
   make        # re-run: should say "nothing to be done" / do no recompilation
   touch get_next_line.h
   make        # should recompile everything, because header changed
   make clean  # removes .o files, keeps libgnl.a
   make fclean # removes libgnl.a too
   make re     # full clean rebuild
   ```
3. Commit: `git commit -am "build: Makefile with all/clean/fclean/re verified"`

### Step 11 — Bonus: refactor to the static array pattern
1. Copy the mandatory files to `_bonus` versions:
   ```bash
   cp get_next_line.c get_next_line_bonus.c
   cp get_next_line_utils.c get_next_line_utils_bonus.c
   cp get_next_line.h get_next_line_bonus.h
   ```
2. In `get_next_line_bonus.c`, change `static char *stash;` to `static char *stash[1024];` and every bare `stash` reference to `stash[fd]` (Chapter 18).
3. Add the `fd >= 1024` guard to the invalid-fd check.
4. **Gate**: re-run Step 6's `BUFFER_SIZE` sweep and Step 7's edge cases against the bonus files — all must still pass identically (single-fd behavior must be unchanged by the refactor).
5. Commit: `git commit -am "bonus: refactor to static array-of-stashes"`

### Step 12 — Bonus-specific multi-fd test
1. Write `tests/main_bonus.c`:
   ```c
   int main(void)
   {
       int fd1 = open("tests/basic.txt", O_RDONLY);
       int fd2 = open("tests/only_nl.txt", O_RDONLY);
       char *a = get_next_line(fd1);
       char *b = get_next_line(fd2);
       char *a2 = get_next_line(fd1);
       printf("fd1 line1: %s", a);
       printf("fd2 line1: %s", b);
       printf("fd1 line2: %s", a2);
       free(a); free(b); free(a2);
       close(fd1); close(fd2);
       return (0);
   }
   ```
2. Compile and run:
   ```bash
   cc -Wall -Wextra -Werror -D BUFFER_SIZE=5 get_next_line_bonus.c get_next_line_utils_bonus.c tests/main_bonus.c -o gnl_bonus_test
   ./gnl_bonus_test
   ```
3. **Gate**: `fd1 line1` and `fd1 line2` are the two correct, un-mixed lines from `basic.txt`; `fd2 line1` is `"\n"` from the other file — none of the three lines should show cross-contamination between files.
4. Run Valgrind and norminette on the bonus files too (repeat Steps 8 and 9 for `_bonus` files).
5. Commit: `git commit -am "bonus: multi-fd interleaved test passes, valgrind+norm clean"`
6. Tag: `git tag v1-bonus`

### Step 13 — README
1. Fill in the template from Chapter 19 / previous guide's README template, writing the Algorithm section from memory first (per Chapter 21's integrative exercise), then refining it.
2. **Gate**: read it out loud once, pretending you're explaining it to someone who has never seen the project — if any sentence makes you pause to figure out what you meant, rewrite it.
3. Commit: `git commit -am "docs: README complete"`

### Step 14 — Final full regression pass before submission
1. From a completely clean clone (this catches "works on my machine because of a leftover file" bugs):
   ```bash
   cd /tmp && rm -rf gnl_check && git clone /path/to/get_next_line gnl_check && cd gnl_check
   make && make bonus
   ```
2. Re-run the entire `BUFFER_SIZE` sweep, all edge-case files, Valgrind, and norminette one final time against this fresh clone.
3. **Gate**: everything passes exactly as it did in your working copy — if something fails only in the fresh clone, you likely forgot to `git add` a file.
4. Push and you're done: `git push`.

### One-page checklist summary
- [ ] Header written and guarded
- [ ] Every utils function tested in isolation
- [ ] Stage 1 (plumbing) verified
- [ ] Stage 2 (stop-at-newline) verified via read-count instrumentation
- [ ] Stage 3 (extraction) passes basic 3-line test
- [ ] BUFFER_SIZE sweep (1, 2, 5, 9999, 100000+) passes
- [ ] Empty file / no-trailing-newline / only-newlines / invalid fd / closed fd all pass
- [ ] Valgrind clean on mandatory
- [ ] Norminette clean on mandatory
- [ ] Makefile: all/clean/fclean/re all verified by hand
- [ ] Bonus: static-array refactor, single-fd behavior unchanged
- [ ] Bonus: multi-fd interleaved test passes
- [ ] Valgrind + norminette clean on bonus
- [ ] README complete and read aloud once
- [ ] Fresh-clone regression pass done
- [ ] Pushed

---

*End of crash course. Suggested order: read Part I fully once, even the parts that feel "obvious" — GNL bugs are almost always a Part I concept applied incorrectly, not a Part III logic error. Then work Part III staged builds hands-on, solving every exercise before reading its answer. Then follow Part IV top to bottom as your literal execution checklist — don't skip a gate.*
