
#include <stdio.h>

typedef    const short int  *nub;

typedef struct fob fob;

struct fob {
    int hi;
};

/* A little harder now */
typedef int (*PFI)(int val);

typedef int (*PFD)(double);

short int main(nub argc)

    int argc;
{
        /* Both are valid references */
    struct fob* fob1;
    fob *fob2;
    FILE *fp;

    /* How about these */
    PFI ff;

        fclose(fp);
        return(0);
}

/*  ###############################################################  */
