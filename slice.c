#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "slice.h"

char *slice_to_str (StringSlice slice) {
    char *str = malloc(slice.length + 1);
    slice.back += slice.start;
    char *c = str;
    for (; slice.length >= 0; --slice.length, ++slice.back, ++c) {
        *c = *slice.back;
    }
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
