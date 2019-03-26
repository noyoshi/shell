/* debug.h: Debugging Macros */
/* Courtesy of Peter Bui */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define HASHTABLE_SIZE 20000
#define DATA_SIZE 2400

#define debug(M, ...) \
    fprintf(stderr, "DEBUG %s:%d:%s: " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define ERROR \
    { \
    fprintf(stderr, "[ERROR] %s\n", strerror(errno)); \
    debug("An error has occured"); \
    exit(EXIT_FAILURE); \
    }

#define streq(s0, s1) (strcmp((s0), (s1)) == 0)

#define check(r) \
    if (r < 0) {\
        ERROR;\
    }\
