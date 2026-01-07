#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sat_inst.h"
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif


literal_t* inst_reserve (int clause_no, int clause_w) {
    return calloc (clause_no*clause_w, sizeof(literal_t));
}

void inst_forget (inst_t* inst) {
    inst->vars_no=0;
    inst->length=0;
    inst->width=0;
    free (inst->body);
}

int inst_width (inst_t* inst, FILE* dimacs) {
    char c; int rtn; char problem[8] = "";
    int i,j; literal_t* clause; int lit;
    int width = 0;
    
    c = fgetc(dimacs);
    while (c != EOF) {
        switch (c) {
        case 'c':
        case 'C': while ((c = fgetc(dimacs)) != EOF && c != '\n') {}; break;
        case 'p':
        case 'P': if ((rtn = fscanf(dimacs," %s8", problem)) == EOF) return ERR_FORMAT;
                  if (strcasecmp (problem, "cnf") != 0) return ERR_PROBLEM;
                  if ((rtn = fscanf(dimacs," %d", &(inst->vars_no)))  == EOF || rtn == 0) return ERR_FORMAT;
                  if ((rtn = fscanf(dimacs," %d", &(inst->length)))  == EOF || rtn == 0) return ERR_FORMAT;
                  break;
        case '\n': break;
        default:  ungetc (c, dimacs); c = EOF; break;
        }
        if (c != EOF) c = fgetc(dimacs);
    }			/* header done */

    if (problem[0]==0)  return ERR_FORMAT;	/* problem not given at all */
    for (i=0, clause=inst->body; i<inst->length; i++, clause+=inst->width) {
        j=0;
        rtn = fscanf(dimacs, " %d", &lit);	/* next line */
        if (rtn == EOF)  return ERR_FORMAT;	/* too few clauses */
        while (rtn != EOF && lit != 0) {
            j++;
            rtn = fscanf(dimacs, " %d", &lit);
        }
        if (j > width) width = j;
    }
    return width;
}

int inst_read (inst_t* inst, FILE* dimacs, int clause_w) {
    char c; int rtn; char problem[8] = "";
    int i,j; literal_t* clause; int lit;
    
    inst->width = clause_w;
    c = fgetc(dimacs);
    while (c != EOF) {
        switch (c) {
        case 'c':
        case 'C': while ((c = fgetc(dimacs)) != EOF && c != '\n') {}; break;
        case 'p':
        case 'P': if ((rtn = fscanf(dimacs," %s8", problem)) == EOF) return ERR_FORMAT;
                  if (strcasecmp (problem, "cnf") != 0) return ERR_PROBLEM;
                  if ((rtn = fscanf(dimacs," %d", &(inst->vars_no)))  == EOF || rtn == 0) return ERR_FORMAT;
                  if ((rtn = fscanf(dimacs," %d", &(inst->length)))  == EOF || rtn == 0) return ERR_FORMAT;
                  break;
        case '\n': break;
        default:  ungetc (c, dimacs); c = EOF; break;
        }
        if (c != EOF) c = fgetc(dimacs);
    }			/* header done */
    if (problem[0]==0)  return ERR_FORMAT;	/* problem not given at all */
    inst->width = clause_w;
    inst->body = inst_reserve(inst->length, inst->width);
    if (!inst->body) return ERR_ALLOC;
    
    for (i=0, clause=inst->body; i<inst->length; i++, clause+=inst->width) {
        j=0;
        rtn = fscanf(dimacs, " %d", &lit); 
        if (rtn == EOF)  return ERR_FORMAT;	                /* too few clauses */
        while (rtn != EOF && lit != 0 && j<inst->width) {
            clause[j]=lit;
            j++;
            rtn = fscanf(dimacs, " %d", &lit);
        }
        if (j==inst->width && lit !=0) return ERR_WIDTH;
    }
    return 0;
}

void inst_read_fail (int err, const char* prog) {
    switch (err) {
    case ERR_PROBLEM: fprintf (stderr, "%s: problem type not a CNF\n", prog); break;
    case ERR_FORMAT:  fprintf (stderr, "%s: input not in DIMACS file format\n", prog); break;
    case ERR_WIDTH:   fprintf (stderr, "%s: clause width exceeded\n", prog); break;
    case ERR_ALLOC:   fprintf (stderr, "%s: allocation failure\n", prog); break;
    default:          fprintf (stderr, "%s: data input failure (%d)\n", prog, err);break;
    }
}

int inst_write (inst_t* inst, FILE* dimacs) {
    int i,j; literal_t* clause; 
    fprintf(dimacs,"p cnf %d %d\n", inst->vars_no, inst->length);
    fprintf(dimacs,"c width %d\n", inst->width);
    for (i=0, clause=inst->body; i<inst->length; i++, clause+=inst->width) {
        j=0;    
        while (j<inst->width && clause[j] != 0) {
            fprintf(dimacs, "%d ", clause[j]);
            j++;
        }
        fprintf(dimacs,"0\n");
    }
    return 0;
}
