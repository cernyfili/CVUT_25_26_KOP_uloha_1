#ifndef NUM_OPTS_H
#define NUM_OPTS_H

int par_int (char* prog, char opt, int* err);               /* no limits */
int par_int_min (char* prog, char opt, int* err, int lo);   /* including positive and non-negative */

double par_double (char* prog, char opt, int* err);         /* no limits */
double par_double_rng (char* prog, char opt, int* err, double lo, double hi); /* incl. one-sided limits */

void par_double_pair (char* prog, char opt, int* err, double* p1, double* p2);
void par_double_pair_pos (char* prog, char opt, int* err, double* p1, double* p2);

#endif

