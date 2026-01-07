#include <string.h>             /* strcmp */
#include "util_files.h"

int util_file_in (file_t* f) {
    if (f->name) {
        if (strcmp (f->name, "-") == 0) {        
            f->file = stdin;
        } else {    
            f->file = fopen (f->name, "r");
            if (!f->file) { perror(f->name); return 0; }
        }
    }
    return 1;
}
int util_file_out (file_t* f) {
    if (f->name) {
        if (strcmp (f->name, "-") == 0) {        
            f->file = stdout;
        } else {    
            f->file = fopen (f->name, "w");
            if (!f->file) { perror(f->name); return 0; }
        }
    }
    return 1;
}
int util_file_log (file_t* f) {
    if (f->name) {
        if (strcmp (f->name, "-") == 0) {        
            f->file = stderr;
        } else {    
            f->file = fopen (f->name, "w");
            if (!f->file) { perror(f->name); return 0; }
        }
    }
    return 1;
}

int util_file_close (file_t* f) {
    if (f->name && strcmp (f->name, "-") != 0) fclose (f->file);
    return 1;
}

