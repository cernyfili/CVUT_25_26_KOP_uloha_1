#ifndef SAT_INST_H
#define SAT_INST_H

#include <stdio.h>

#define ERR_PROBLEM -1
#define ERR_FORMAT  -2
#define ERR_WIDTH   -3
#define ERR_ALLOC -4

typedef int literal_t;

typedef struct  {
    int vars_no;
    int length;
    int width;
    literal_t* body;
} inst_t;

literal_t* inst_reserve (int clause_no, int clause_w);
void inst_forget (inst_t* inst);
int inst_width (inst_t* inst, FILE* dimacs);
int inst_read (inst_t* inst, FILE* dimacs, int clause_w);
void inst_read_fail (int err, const char* prog);
int inst_write (inst_t* inst, FILE* dimacs);


#endif

