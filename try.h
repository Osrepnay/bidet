#define TRYBOOL(v) \
do { \
    if (!(v)) return 0; \
} while (0);

#define TRYBOOL_R(v, ...) \
do { \
    if (!(v)) { \
        __VA_ARGS__; \
        return 0; \
    } \
} while (0);
