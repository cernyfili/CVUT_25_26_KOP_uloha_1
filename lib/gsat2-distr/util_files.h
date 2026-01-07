#ifndef UTIL_FILES_H
#define UTIL_FILES_H
#include <stdio.h>

typedef struct {
    char* name;
    FILE* file;
} file_t;

int util_file_in (file_t*);
int util_file_out (file_t*);
int util_file_log (file_t*);
int util_file_close (file_t*);
#endif
