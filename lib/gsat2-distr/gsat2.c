#include <stdlib.h>             /* strtol */
#include <stdio.h>              /* printf */
#include <string.h>             /* strcmp */
#include <math.h>               /* isnan etc. */
#ifdef _MSC_VER
#include "getopt.h"
#include <windows.h>            /* ctrl c handler */
#undef max
#undef min
#else
#include <unistd.h>             /* getopt */
#include <limits.h>
#include <signal.h>
#endif
#include "ctrlc_handler.h"
#include "util_files.h"
#include "sat_inst.h"
#include "sat_sol.h"
#include "rngctrl.h"
#include "num_opts.h"
/*-----------------------------------------------------------------------------*/
char synopsis[] = "gsat <options> [dimacs-file]\n"
"\t Input format control\n"
"\t-w number                        max literals in a clause, default 3\n"
"\t Iteration control\n"
"\t-i number                        max iterations (flips)\n"
"\t-T number                        max tries (restarts)\n"
"\t-p number                        probability of a random step, float, 0..1.0\n"
"\t Output control (iteration count and sat clauses to stdout)\n"
"\t-d <file>                        output iteration log into <file>\n"
"\t-t <file>                        detailed trace into <file>\n"
"\t-D                               debug info to stderr\n"
"\t-e string                        resulting line specifier\n"
;

/*-----------------------------------------------------------------------------*/
typedef int* best_list_t;

/*-----------------------------------------------------------------------------*/
/*      for all clauses in sol, update the number of true literals in cnt      */
/*-----------------------------------------------------------------------------*/
int gw_eval (sol_t sol, inst_t* inst, cnt_t cnt) {
    int sat = 0, i, v;
    literal_t* clause;
    for (i=0, clause=inst->body; i<inst->length; i++, clause+=inst->width) {
        cnt[i] = 0;
        for(v=0; v<inst->width; v++) {
    	    if (clause[v] != 0) cnt[i]+=sol[clause[v]];
        }
        if (cnt[i] > 0) sat++;
    }
    return sat;
}
/*-----------------------------------------------------------------------------*/
/*      build the var_info structure telling where each variable is used       */
/*-----------------------------------------------------------------------------*/
var_info_t gw_varinf_build (inst_t* inst) {
    var_info_t varinf;
    literal_t* clause;
    int i,v,l;
    varinf = calloc (inst->vars_no+1, sizeof(var_info));        /* item 0 is bogus */
    if (!varinf) return NULL;
    for (i=0, clause=inst->body; i<inst->length; i++, clause+=inst->width) {
        for(l=0; l<inst->width; l++) {
    	    if (clause[l] > 0) varinf[clause[l]].pos_occ_no++; else
    	    if (clause[l] < 0) varinf[-clause[l]].neg_occ_no++;
        }
    }
    for (v=1; v<=inst->vars_no; v++) {
        varinf[v].pos_occ = calloc (varinf[v].pos_occ_no, sizeof(clause_ix_t));
        varinf[v].pos_occ_no = 0;
        varinf[v].neg_occ = calloc (varinf[v].neg_occ_no, sizeof(clause_ix_t));
        varinf[v].neg_occ_no = 0;
    }
    for (i=0, clause=inst->body; i<inst->length; i++, clause+=inst->width) {
        for(l=0; l<inst->width; l++) {
    	    if (clause[l] > 0) {
    	        v=clause[l];
    	        varinf[v].pos_occ [varinf[v].pos_occ_no] = i; 
    	        varinf[v].pos_occ_no++; 
    	    } else
    	    if (clause[l] < 0) {
    	        v=-clause[l];
    	        varinf[v].neg_occ [varinf[v].neg_occ_no] = i; 
    	        varinf[v].neg_occ_no++; 
    	    }
        }
    }
    return varinf;
}
/*-----------------------------------------------------------------------------*/
/*      debug dump of the var_info structure to out                            */
/*-----------------------------------------------------------------------------*/
int gw_varinf_dump (var_info_t varinf,inst_t* inst, FILE* out) {
    int i,v;
    for (v=1; v<=inst->vars_no; v++) {
        fprintf(out,"%3d P %3d:", v, varinf[v].pos_occ_no); 
        for (i=0; i<varinf[v].pos_occ_no; i++) fprintf(out," %3d",varinf[v].pos_occ[i]);
        fprintf(out,"\n");
        fprintf(out,"%3d N %3d:", v, varinf[v].neg_occ_no); 
        for (i=0; i<varinf[v].neg_occ_no; i++) fprintf(out," %3d",varinf[v].neg_occ[i]);
        fprintf(out,"\n");
    }
    return 0;
}

/*-----------------------------------------------------------------------------*/
var_info_t gw_varinf_forget (var_info_t varinf, inst_t* inst) {
    int v;
    if (varinf) {
        for (v=1; v<=inst->vars_no; v++) {
            free (varinf[v].pos_occ);
            free (varinf[v].neg_occ); 
        }  
        free (varinf);
    }
    return NULL;
}
/*-----------------------------------------------------------------------------*/
/*      determine the change in satisfied clause number when variable v 1->0   */
/*-----------------------------------------------------------------------------*/
int gw_neg_flip_gain (var_info_t varinf, cnt_t cnt, int v) {
    int i, gain=0;
    for (i=0; i<varinf[v].pos_occ_no; i++) {    /* for all clauses where the variable occurs in a positive literal */
        if (cnt[varinf[v].pos_occ[i]] == 1) gain--;
    }
    for (i=0; i<varinf[v].neg_occ_no; i++) {    /* for all clauses where the variable occurs in a negative literal */
        if (cnt[varinf[v].neg_occ[i]] == 0) gain++;
    }
    return gain;
}
/*-----------------------------------------------------------------------------*/
/*      determine the change in satisfied clause number when variable v 0->1   */
/*-----------------------------------------------------------------------------*/
int gw_pos_flip_gain (var_info_t varinf, cnt_t cnt, int v) {
    int i, gain=0;
    for (i=0; i<varinf[v].pos_occ_no; i++) {    /* for all clauses where the variable occurs in a positive literal */
        if (cnt[varinf[v].pos_occ[i]] == 0) gain++;
    }
    for (i=0; i<varinf[v].neg_occ_no; i++) {    /* for all clauses where the variable occurs in a negative literal */
        if (cnt[varinf[v].neg_occ[i]] == 1) gain--;
    }
    return gain;
}
/*-----------------------------------------------------------------------------*/
/*   determine which variable flip gives the max gain                          */
/*-----------------------------------------------------------------------------*/
best_list_t best_reserve (int n) {
    return calloc (n, sizeof(int));
}
void best_forget (best_list_t* list) {
    free (*list);
    list=NULL;
}
void best_new_max (best_list_t list, unsigned* occ, int v) {
    *occ = 1;
    list[0] = v;
}
void best_new (best_list_t list, unsigned* occ, int v) {
    list[*occ] = v;
    *occ = *occ + 1;
}
/*-----------------------------------------------------------------------------*/
int gw_max_flip_var (var_info_t varinf, inst_t* inst, cnt_t cnt, sol_t sol, best_list_t list) {
    unsigned chosen, listocc;
    int maxgain, v, gain=0;
    maxgain = INT_MIN;
    listocc = 0;
    for (v=1; v<=inst->vars_no; v++) {
        gain = sol[v] ? gw_neg_flip_gain (varinf, cnt, v) : gw_pos_flip_gain (varinf, cnt, v);
        if (gain > maxgain) { 
            maxgain = gain;
            best_new_max (list, &listocc, v);
        } else if (gain == maxgain) {
            best_new (list, &listocc, v);
        }
        /* fprintf(stderr,"%d %s flip, gain: %d\n", v, (sol[v] ? "neg" : "pos"), gain); */
    }
    if (listocc == 1) return list[0];
    /* fprintf(stderr, "%u flips, gain: %d\n", listocc, gain); */
    chosen = rng_next_range (0, listocc-1);
    return list[chosen];
}
/*-----------------------------------------------------------------------------*/
/*      realize flip 1->0 of variable v, update cnt                            */
/*-----------------------------------------------------------------------------*/
int gw_make_neg_flip (var_info_t varinf, cnt_t cnt, int v) {
    int i, gain=0;
    for (i=0; i<varinf[v].pos_occ_no; i++) {    /* for all clauses where the variable occurs in a positive literal */
        if (cnt[varinf[v].pos_occ[i]] == 1) gain--;
        cnt[varinf[v].pos_occ[i]]--;
    }
    for (i=0; i<varinf[v].neg_occ_no; i++) {    /* for all clauses where the variable occurs in a negative literal */
        if (cnt[varinf[v].neg_occ[i]] == 0) gain++;
        cnt[varinf[v].neg_occ[i]]++;
    }
    return gain;
}
/*-----------------------------------------------------------------------------*/
/*      realize flip 0->1 of variable v, update cnt                            */
/*-----------------------------------------------------------------------------*/
int gw_make_pos_flip (var_info_t varinf, cnt_t cnt, int v) {
    int i, gain=0;
    for (i=0; i<varinf[v].pos_occ_no; i++) {    /* for all clauses where the variable occurs in a positive literal */
        if (cnt[varinf[v].pos_occ[i]] == 0) gain++;     /* it is a waste to compute gain again */
        cnt[varinf[v].pos_occ[i]]++;
    }
    for (i=0; i<varinf[v].neg_occ_no; i++) {    /* for all clauses where the variable occurs in a negative literal */
        if (cnt[varinf[v].neg_occ[i]] == 1) gain--;
        cnt[varinf[v].neg_occ[i]]--;
    }
    return gain;
}
/*-----------------------------------------------------------------------------*/
/*      realize flip 1->0 of variable v, update cnt                            */
/*-----------------------------------------------------------------------------*/
int gw_make_flip (var_info_t varinf, inst_t* inst, cnt_t cnt, sol_t sol, int v) {
    int gain=0;
    if (sol[v]) {
        gain += gw_make_neg_flip (varinf, cnt, v);    /* update true literal counters */
    } else {
        gain += gw_make_pos_flip (varinf, cnt, v);
    }
    sol_flip (sol, v);
    return gain;
}
/*-----------------------------------------------------------------------------*/
/*      randomly choose an unsatisfied clause                                  */
/*-----------------------------------------------------------------------------*/
int gw_pick_unsat (inst_t* inst, cnt_t cnt, int satisfied) {
    unsigned c; int i,pick;
    c = rng_next_range(1, inst->length-satisfied);
    pick=0;
    for (i=0; i<inst->length; i++) {
        if (cnt[i] == 0) {
            pick++;
            if (pick == c) return i;
        }
    }
    return 0;
}
/*-----------------------------------------------------------------------------*/
/*      randomly choose a variable in a clause                                 */
/*-----------------------------------------------------------------------------*/
int gw_pick_var (inst_t* inst, cnt_t cnt, int cli) {
    literal_t* clause;
    int pick,i;

    clause = inst->body+cli*inst->width;
    for (i=0; i<inst->width; i++) if (clause[i] == 0) break;
    pick = rng_next_range(0, i-1);
    if (clause[pick] < 0) return -clause[pick];
    return clause[pick];
}
/*-----------------------------------------------------------------------------*/
int main (int argc, char** argv) {
    /* parameters and default values*/
    int         width=3;
    int         itrmax=300; /* max iterations */
    int         triesmax=1; /* max tries */
    double      p=0.4;      /* gredy / random probability */
    int         debug=0;    /* debug info to stderr */
    file_t      in =    {NULL, stdin};  /* instance input */
    file_t      data =  {NULL, NULL};   /* evolution records, outsep applies */
    file_t      trace = {NULL, NULL};   /* detailed trace */
    
    const char* outsep=" ";                        /* output separator */   

    int         err=0;      /* err indicator */
    char        opt;        /* options scanning */
    
    inst_t      inst;       /* instance */
    sol_t       sol;        /* solution */
    cnt_t       cnt=NULL;   /* true literals counters, per clause */
    int         satisfied;  /* current no. of sat clauses */ 
    
    int         flipvar;    /* max gain or picked flipping variable */
    int         ucli;       /* picked unsat clause */
    int         gain;       /* flip gain */
    best_list_t best_list;  /* list of vars giving max gain */
    
    var_info_t  varinf;     /* inverted instance */
    int         itrno;      /* iteration number within a try */
    int         tryno;	    /* number of restarts */
    double      dec;        /* greedy / random decision */
    char*       itype;      /* greedy or random */

    /* --------------------- CTRL-C handling ---------------- */    
    int*        pcont = establish_handler(argv[0]);
    if (!pcont) return EXIT_FAILURE;   

    /* ---------------------- options ----------------------- */
    while ((opt = getopt(argc, argv, "T:t:d:Di:p:w:r:R:s:S:e:")) != -1) {
         switch (opt) {
         case 'd': data.name = optarg; break;    /* datafile required */
         case 't': trace.name = optarg; break;   /* trace required */
         case 'e': outsep = optarg; break;      /* separator */
         case 'D': debug=1; break;              /* debugging required */
         case 'p': p = par_double_rng (argv[0], opt, &err, 0.0, 1.0); /* probability of random steps in an iteration */
                   break;
         case 'w': width = par_int_min (argv[0], opt, &err, 1);     /* max clause length - needed when input from stdin */
                   break;
         case 'i': itrmax = par_int_min (argv[0], opt, &err, 0);    /* max no. of iteration - 0 means no limit */
                   break;
         case 'T': triesmax = par_int_min (argv[0], opt, &err, 0);  /* max no. of tries - 0 means no limit */
                   break;
         case 'r':      /* PRNG controls */
         case 'R': 
         case 's':
         case 'S': if (rng_options (opt, optarg, argv[0]) != 0) err++;
                   break;
         default:  fprintf (stderr, "%s%s", synopsis, rng_synopsis); 
                   return EXIT_FAILURE;  /* unknown parameter, e.g. -h */
         }
    }
    if (optind < argc) in.name = argv[optind];                       /* input file on the command line */
    /* fprintf (stderr,"input err: %d\n", err); */
    if (err) return EXIT_FAILURE;                                   /* stop here if any error */
    /* fprintf(stderr,"options OK\n"); */
    /* ----------------- RNG controls ------------------------ */
    if (!rng_apply_options (argv[0])) return EXIT_FAILURE;          /* errors are reported already */
    
    /* ----------------------- instance input ---------------- */
    if (! util_file_in (&in)) return EXIT_FAILURE;
    if (in.name) {
        if ((width = inst_width(&inst, in.file)) < 0) {
            inst_read_fail (width, argv[0]);
            return EXIT_FAILURE;
        }
        rewind(in.file);    
    }
    err = inst_read(&inst, in.file, width);                        /* if any error so far, report and exit */
    if (err) {
        inst_read_fail (err, argv[0]);
        return EXIT_FAILURE;
    }
     
    /* ------------------------ datafile output -------------- */   
    if (! util_file_log (&data)) return EXIT_FAILURE;    

    /* ------------------------ tracefile output ------------- */   
    if (! util_file_log (&trace)) return EXIT_FAILURE;    
    
    /* ----------------------- instance inversion ------------- */
    if (!(varinf = gw_varinf_build (&inst))) {                      /* build the 'where used' structure */
        fprintf (stderr, "%s: allocation failure\n", argv[0]); return EXIT_FAILURE;
    }	
    if (debug) gw_varinf_dump (varinf, &inst, stderr);

    /* ----------------------- solution  space  --------------- */
    if (!(sol = sol_reserve(inst.vars_no))) {                       /* build the solution arrray  */
        fprintf (stderr, "%s: allocation failure\n", argv[0]); return EXIT_FAILURE;
    }	
    if (!(best_list = best_reserve(inst.vars_no))) {                       /* build the solution arrray  */
        fprintf (stderr, "%s: allocation failure\n", argv[0]); return EXIT_FAILURE;
    }	

    tryno = 1;
    itrno = 0; 
    satisfied = 0;
    while (satisfied < inst.length && *pcont && ((!triesmax) || tryno <= triesmax)) {

        sol_rand (sol, inst.vars_no);                                   /* random 0/1 assignment */
        /* ----------------------- evaluation --------------------- */
        if (!(cnt = cnt_reserve(inst.length))) {                        /* build the array of true literal counts */
            fprintf (stderr, "%s: allocation failure\n", argv[0]); return EXIT_FAILURE;
        }	
        satisfied = gw_eval (sol, &inst, cnt);                          /* evaluate true literals and count sat clauses */
    
        /* ----------------------- debug and trace ---------------- */
        if (data.file) fprintf (data.file, "%d %d\n", 0, satisfied);
        if (debug) {
            sol_write(sol, stderr, inst.vars_no); 
            fprintf(stderr,"satisfied: %d\n",satisfied);
            for (int i=0; i<inst.length; i++) fprintf(stderr, " %d", cnt[i]); 
            fprintf(stderr, "\n");
        }
        if (trace.file) { 
            fprintf (trace.file, "initial: satisfied %d, solution: ", satisfied);
            sol_write(sol, trace.file,  inst.vars_no); 
            fprintf (trace.file, "true literals: ");
            for (int i=0; i<inst.length; i++) fprintf(trace.file, " %d", cnt[i]); 
            fprintf(trace.file, "\n");
        }
        /* ----------------------- gsat inner iteration ----------- */
        itrno = 1; gain=1;                                              /* stop when formula satisfied, CTRL-C occurs */
                                                                    /* and then either iterations unlimited or still below limit */
        while (satisfied < inst.length && *pcont && ((!itrmax) || itrno <= itrmax)) {
            dec = rng_next_double();                                    /* choose a greedy or random step */
            if (dec > p) {                                              /* greedy */
                flipvar = gw_max_flip_var (varinf, &inst, cnt, sol, best_list);    /* select the var with max gain to flip */
                gain = gw_make_flip (varinf, &inst, cnt, sol, flipvar); /* update the true literals counters, determine gain */
                itype = "greedy";
            } else {
                ucli = gw_pick_unsat (&inst, cnt, satisfied);           /* pick some unsat clause at random */
                flipvar = gw_pick_var (&inst, cnt, ucli);               /* pick a variable in that clause */
                gain = gw_make_flip (varinf, &inst, cnt, sol, flipvar); /* update the true literals counters, determine gain */
                itype = "random";
            }       
            satisfied += gain;                                          /* update sat clauses no. */
            if (data.file) fprintf (data.file, "%d %d\n", itrno, satisfied);      /* datafile line */
            if (debug) {                                                /* debug info */
                fprintf(stderr,"%s flipvar %d, satisfied: %d\n",itype, flipvar, satisfied);
            }
            if (trace.file) {                                                /* readable trace info */
                fprintf (trace.file, "itr %d, %s, flipvar %d, satisfied %d, solution: ", itrno, itype, flipvar, satisfied);
                sol_write(sol, trace.file,  inst.vars_no); 
                fprintf (trace.file, "true literals: ");
                for (int i=0; i<inst.length; i++) fprintf(trace.file, " %d", cnt[i]); 
                fprintf(trace.file, "\n");
            }
            itrno++;
        }
        tryno++;
    }
    fprintf (stderr, "%d%s%d%s%d%s%d\n", (tryno-2)*itrmax+itrno-1, outsep, triesmax*itrmax, outsep, satisfied, outsep, inst.length);    /* final information */
    sol_write (sol, stdout, inst.vars_no);
    rng_end_options (argv[0]);
    
    varinf = gw_varinf_forget(varinf, &inst);
    cnt = cnt_forget(cnt);
    sol = sol_forget(sol, inst.vars_no);
    inst_forget(&inst);
    best_forget(&best_list);
    
    util_file_close (&data);
    util_file_close (&trace);
    util_file_close (&in);

    return EXIT_SUCCESS;
}
