#include <stdlib.h>             /* strtol */
#include <stdio.h>              /* printf */
#include <string.h>             /* strcmp */
#include <math.h>               /* isnan etc. */
#ifdef _MSC_VER
#include "getopt.h"
#undef max
#undef min
#else
#include <unistd.h>             /* getopt */
#include <limits.h>
#endif
#include "rngctrl.h"
/*-----------------------------------------------------------------------------*/
char synopsis[] = "rngstub <options> \n";
/*-----------------------------------------------------------------------------*/
int main (int argc, char** argv) {
    
    int  err=0;      /* err indicator */
    char opt;
    /* ---------------------- options ----------------------- */
    while ((opt = getopt(argc, argv, "r:R:s:S:")) != -1) {
         switch (opt) {
         case 'r':      /* PRNG controls */
         case 'R': 
	     case 's':
         case 'S': if (rng_options (opt, optarg, argv[0]) != 0) err++;
                   break;
         default:  fprintf (stderr, "%s%s", synopsis, rng_synopsis); 
                   return EXIT_FAILURE;  /* unknown parameter, e.g. -h */
         }
    }
    if (err) return EXIT_FAILURE;                                   /* stop here if any error */

    /* ----------------- RNG controls ------------------------ */
    if (!rng_apply_options (argv[0])) return EXIT_FAILURE;          /* errors have been reported already */
    fprintf(stderr,"%f\n",rng_next_double());
    rng_end_options (argv[0]);
    return EXIT_SUCCESS;
}
