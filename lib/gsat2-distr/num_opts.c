/* #include <unistd.h> */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "num_opts.h"

extern char* optarg;

/*-----------------------------------------------------------------------------*/
int par_int (char* prog, char opt, int* err) {     /* optarg is global */   
    int arg; char* epf;
    arg = strtol(optarg, &epf, 0); 
    if (*epf != 0) { fprintf (stderr, "%s: the arg to -%c should be numeric\n", prog, opt); (*err)++; }
    return arg;
}
/*-----------------------------------------------------------------------------*/
int par_int_min (char* prog, char opt, int* err, int lo) {     /* optarg is global */   
    int arg;
    arg = par_int (prog, opt, err);
    if (arg < lo) { 
        fprintf (stderr, "%s: the arg to -%c should be ", prog, opt);
        if (lo == 0) fprintf (stderr,"non-negative\n");
        else if (lo==1) fprintf (stderr,"positive\n");
        else fprintf(stderr,"at least %d\n",  lo); 
        (*err)++; 
    }
    return arg;
}
/*-----------------------------------------------------------------------------*/
double par_double (char* prog, char opt, int* err) {     /* optarg is global */   
    double arg; char* epf;
    arg = strtod(optarg, &epf); 
    if (*epf != 0) { fprintf (stderr, "%s: the arg to -%c should be numeric\n", prog, opt); (*err)++; }
    return arg;
}
/*-----------------------------------------------------------------------------*/
void par_double_pair (char* prog, char opt, int* err, double* p1, double* p2) {     /* optarg is global */   
    char *epf1, *epf2 = "";
    *p1 = strtod(optarg, &epf1); 
    if (*epf1 == ',') *p2 = strtod(epf1+1, &epf2); 
    if (*epf1 != ',' || *epf2 != 0) { 
        fprintf (stderr, "%s: the arg to -%c should be num,num\n", prog, opt); (*err)++; 
    }
}
/*-----------------------------------------------------------------------------*/
double par_double_rng (char* prog, char opt, int* err, double lo, double hi) {     /* optarg is global */   
    double arg;
    arg = par_double (prog, opt, err); 
    if (isnan(hi)) {        /* one-sided limit */
        if (arg < lo) {
            fprintf (stderr, "%s: the arg to -%c should be ", prog, opt);
            if (lo==0.0) fprintf (stderr,"non-negative\n");
            else fprintf(stderr,"at least %g\n", lo); 
            (*err)++; 
        }
    } else if (arg < lo || arg > hi) { 
        fprintf (stderr, "%s: the arg to -%c should be between %g and %g\n", prog, opt, lo, hi);
        (*err)++; 
    }
    return arg;
}
/*-----------------------------------------------------------------------------*/
void par_double_pair_pos (char* prog, char opt, int* err, double* p1, double* p2) {     /* optarg is global */   
    par_double_pair (prog, opt, err, p1, p2);
    if (*p1 <= 0.0 || *p2 <= 0.0) { fprintf (stderr, "%s: the args to -%c should be positive\n", prog, opt); (*err)++; }
}

