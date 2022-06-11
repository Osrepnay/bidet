#include <stddef.h>

// program info
typedef struct {
    const char *filename;
    const char *text;
} Prog;

char *fmt_err (Prog, size_t, size_t, const char *);
