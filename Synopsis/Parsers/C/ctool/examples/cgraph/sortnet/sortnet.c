/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         sortnet.c
    o+
    o+     Programmer:   Shaun Flisakowski
    o+     Date:         Aug 19, 1998
    o+
    o+     Main file for sorting network generator.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#include <stdio.h>
#include <string.h>

#include "netwrk.h"
#include "codegen.h"

#define VERSION "SortNet v1.0"

int  sort_increasing = 1;
int  print_description = 0;
int  generate_test_code = 0;
char outfile[256];
int  debug_on = 0;

CmpProg *network = NULL;

#define MAX_FILES 32
char *files[MAX_FILES];

extern FILE *yyin;
extern int yyparse(void);

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void Usage(char *prog)
{
    fprintf(stderr,"Usage: %s [options] [filename(s)]\n\n",prog);

    fprintf(stderr,"This program reads a description of a sorting network,\n");
    fprintf(stderr,"and writes an implementation of it in C.\n\n");

    fprintf(stderr,"Options:\n");
    fprintf(stderr,"\t-incr:\t\t Sort in increasing order (-I, default)\n");
    fprintf(stderr,"\t-decr:\t\t Sort in decreasing order (-D).\n");
    fprintf(stderr,"\t-o<file>:\t Output to this filename "
                                                "(default stdout).\n");
    fprintf(stderr,"\t-print:\t\t Print description of input (-p).\n");
    fprintf(stderr,"\t-debug:\t\t Turn debugging printing on (-d).\n");
    fprintf(stderr,"\t-test:\t\t Generate code to test the sorting"
                                                   " network (-t).\n");
    fprintf(stderr,"\t-version:\t Show sortnet version (-V).\n");
    fprintf(stderr,"\t-help:\t\t Show usage (-h).\n");

    exit(0);
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
int getArgs( int argc, char **argv )
{
    int num_files = 0;
    char *arg;
    char *prog = argv[0];

    argc--;
    argv++;

   *outfile = '\0';

    while(argc-- > 0)
    {
        arg = *argv++;

        if ((strcmp(arg,"-incr") == 0)
              || (strcmp(arg,"-I") == 0))
          sort_increasing = 1;

        else if ((strcmp(arg,"-decr") == 0)
              || (strcmp(arg,"-D") == 0))
          sort_increasing = 0;

        else if ((strcmp(arg,"-test") == 0)
              || (strcmp(arg,"-t") == 0))
          generate_test_code = 1;

        else if ((strcmp(arg,"-print") == 0)
              || (strcmp(arg,"-p") == 0))
          print_description = 1;

        else if ((strncmp(arg,"-o",2) == 0))
          strcpy(outfile,&arg[2]);

        else if ((strcmp(arg,"-debug") == 0)
              || (strcmp(arg,"-d") == 0))
          debug_on = 1;

        else if ((strcmp(arg,"-version") == 0)
              || (strcmp(arg,"-V") == 0))
        {
          fprintf(stdout, VERSION "\n");
          exit(0);
        }

        else if ((strcmp(arg,"-help") == 0)
            || (strcmp(arg,"-h") == 0))
          Usage(prog);

        else
          files[num_files++] = arg;
    }

    files[num_files] = NULL;
    return num_files;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
CmpProg*
parseNetwork()
{
    while(yyparse())
        ;

    return linkNetwork(network);
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
int
main(int ac, char **av)
{
    int nF;
    CmpProg *netwrk;
    FILE *fp, *outfp = stdout;

    if ((nF = getArgs(ac,av)) <= 0)
        return 0;

    if ((fp = fopen(files[0],"r")) != NULL)
    {
        yyin = fp;
        netwrk = parseNetwork();
    
        if (netwrk)
        {
            if (*outfile)
            {
                outfp = fopen(outfile, "w");

                if (outfp == NULL)
                {
                    fprintf(stderr,"Error: Could not open file '%s'"
                                   " for writing.\n", outfile);
                }
            }

            if (outfp)
            {
                if (print_description)
                    prntDescription(outfp,netwrk);

                prntHeader(outfp,netwrk->nIn,generate_test_code);
                prntCmp(outfp,sort_increasing);
                prntNetwork(outfp,netwrk);

                if (generate_test_code)
                    prntTest(outfp,netwrk->nIn,
                             sort_increasing, debug_on);
            }

            if (*outfile)
                fclose(outfp);
        }

        fclose(fp);
    }

    return 0;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

