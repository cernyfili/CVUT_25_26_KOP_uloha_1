#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sat_sol.h"
#include "xoshiro256plus.h"

/* --- structure: -xn | ... | -x1 | 0 | x1 | ... | xn | ------- */
sol_t sol_reserve (int vars) {
    int v;
    sol_t sol;
    sol=calloc (2*vars+1, sizeof(bool_val));
    sol += vars;  /* so that sol[0] is the (nonexistent) x0, sol[i] is xi, sol[-i] is not xi */
    for(v=1; v <= vars; v++) sol_set(sol, v, 0); 
    return sol;		
}

void sol_copy (sol_t from, sol_t to, int vars) {
    memcpy (to-vars, from-vars, (2*vars+1) * sizeof(bool_val));
}

sol_t sol_forget (sol_t sol, int vars) {
    if (sol) {
        free(sol-vars);
    }
    return NULL;
}

int sol_set (sol_t sol, int ix, bool_val val) {
    sol[ix]=val;
    sol[-ix]=val? 0 : 1; 
    return 0;
}
int sol_flip (sol_t sol, int ix) {
    sol[ix]=sol[-ix];
    sol[-ix]=sol[ix]? 0 : 1;
    return 0;
}
int sol_rand (sol_t sol, int vars) {
    int j;
    for (j=1; j<=vars; j++) sol_set (sol, j, rng_next_range (0,1));
    return 0;
}

int sol_write (sol_t sol, FILE* out, int vars) {
    int j;
    for (j=1; j<=vars; j++) fprintf(out, "%d ", sol[j]? j : -j);
    fprintf(out,"0\n");
    return 0;
}

int sol_read (sol_t sol, FILE* in, int vars) {
    int v,r,n;
    for (v=1; v<=vars; v++) {
        r = fscanf (in, "%d", &n);
        if (r == 0 || r == EOF) return 0;
        sol[n] = 1; sol[-n] = 0;
    }
    r = fscanf (in, "%d", &n);
    if (!r || n != 0) return 0;
    return 1;
}

int sol_read_str (sol_t sol, const char* in, int vars) {
    int v,r,n,offset;
    for (v=1; v<=vars; v++) {
        r = sscanf (in, "%d%n", &n,&offset); 
        if (r == 0 || r == EOF) return 0;
        sol[n] = 1; sol[-n] = 0; in += offset;
    }
    r = sscanf (in, "%d", &n);
    if (!r || n != 0) return 0;
    return 1;
}


/* --------------------------------------------------------------- */
cnt_t cnt_reserve (int length) {
    return calloc(length, sizeof(cnt_val));
}
cnt_t cnt_forget (cnt_t cnt) {
    if (cnt) free (cnt);
    return NULL;
}

