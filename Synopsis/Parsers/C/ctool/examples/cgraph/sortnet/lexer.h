/**************************************************************************
/    lexer.h
/ *************************************************************************/

#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#include "netwrk.h"

/******************************************************/

/*    For Flex compatibility  */

typedef union {
    int          num;

    CmpNode     *node;
    CmpTuple    *tuple;
    CmpList     *list;
    CmpProg     *prog;
} netwrk_union;

#undef  YYSTYPE
#define YYSTYPE netwrk_union

/******************************************************/

int  yyerror (char *s);

/******************************************************/

#endif  /* LEXER_H */
