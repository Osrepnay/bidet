#include <stddef.h>

typedef struct {
    size_t start;
    size_t length;
    const char *back;
} StringSlice;

char *slice_to_str (StringSlice);
StringSlice str_to_slice (const char *, size_t, size_t);
