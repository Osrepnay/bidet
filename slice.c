#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "slice.h"

char *slice_to_str (StringSlice slice) {
    char *str = malloc(slice.length + 1);
    strncpy(str, slice.back + slice.start, slice.length);
    return str;
}

StringSlice str_to_slice (const char *str, size_t start, size_t length) {
    assert(strlen(str) - start >= length);
    return (StringSlice) {
        .start = start,
        .length = length,
        .back = str
    };
}

StringSlice str_to_slice_raw (const char *str) {
    return (StringSlice) {
        .start = 0,
        .length = strlen(str),
        .back = str
    };
}
