#define TRYBOOL(v) \
do { \
    if (!(v)) return 0; \
} while (0);

#define TRYBOOL_R(v, rescue) \
do { \
    if (!(v)) { \
        rescue; \
        return 0; \
    } \
} while (0);
