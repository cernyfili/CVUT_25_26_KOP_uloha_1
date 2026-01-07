#ifndef SAT_SOL_H
#define SAT_SOL_H
#include <stdio.h>
#include "rngctrl.h"

typedef unsigned char bool_val;
typedef bool_val* sol_t;

sol_t sol_reserve (int vars);
sol_t sol_forget (sol_t sol, int vars);

/* --- variable indices from 1 ---------- */
int sol_set (sol_t sol, int ix, bool_val val);
int sol_flip (sol_t sol, int ix);
int sol_rand (sol_t sol, int vars);
void sol_copy (sol_t from, sol_t to, int vars);

int sol_write (sol_t sol, FILE* out, int vars);
int sol_read (sol_t sol, FILE* in, int vars);
int sol_read_str (sol_t sol, const char* in, int vars);

/* --- aux true literals count --------- */
typedef unsigned int cnt_val;
typedef cnt_val* cnt_t;
cnt_t cnt_reserve (int length);
cnt_t cnt_forget (cnt_t cnt);

/* --- aux inverted instance ----------- */
typedef int clause_ix_t;
typedef struct {
    int pos_occ_no;
    int neg_occ_no;
    clause_ix_t* pos_occ;
    clause_ix_t* neg_occ;
} var_info;
typedef var_info* var_info_t;

#endif

