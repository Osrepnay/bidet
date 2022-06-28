#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "fmt_error.h"

// finds line and column of an offset
// returns false if offset not found, true otherwise
bool offset_line_col (Prog prog, size_t offset, size_t *offset_line, size_t *offset_col) {
    // current line and column
    int line = 1;
    int col = 1;

    for (size_t i = 0; prog.text[i] != '\0'; ++i) {
        // handle newlines
        switch (prog.text[i]) {
            case '\n':
                // reset col and inc line
                ++line;
                col = 0;
                break;
            case '\r':
                ++line;
                col = 0;
                // if crlfs
                // doesn't overreach text because text always has null terminator
                if (prog.text[i + 1] == '\n') {
                    ++i; // skip \r, so that the entire \r\n is skipped
                }
                break;
        }

        if (i == offset) {
            *offset_line = line;
            *offset_col = col;
            return true;
        }
    }
    return false;
}

// format error, e.g. [file at line,col to line_end,col_end] message
// err is alloced in here
// returns false when can't find either offset or offset+length
char *fmt_err (Prog prog, size_t offset, const char *message) {
    // get line and columns for offset and offset + length
    size_t offset_line;
    size_t offset_col;
    // populate line and col
    // assert is fine because invalid offsets shouldn't be given ever
    assert(offset_line_col(prog, offset, &offset_line, &offset_col));

    char *err;
    int err_len = snprintf(NULL, 0, "[%s at %zu,%zu] %s",
            prog.filename,
            offset_line, offset_col,
            message);
    err = malloc(err_len + 1);
    sprintf(err, "[%s at %zu,%zu] %s",
            prog.filename,
            offset_line, offset_col,
            message);
    return err;
}
