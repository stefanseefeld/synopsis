
/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

    CTool Library
    Copyright (C) 1998-2001	Shaun Flisakowski

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

/*  ###############################################################
    ##
    ##     Call Graph Generator
    ##
    ##     File:         cgraph.cpp
    ##
    ##     Programmer:   Shaun Flisakowski
    ##     Date:         Nov 27, 1998
    ##
    ###############################################################  */

#include    <stdio.h>
#include    <iostream.h>
#include    <string.h>
#include    <stdlib.h>
#include    <assert.h>

#include    "cgraph.h"
#include    "ctool.h"

/*  ###############################################################  */

bool   show_graph = false;
bool   prolog_graph = false;
bool   show_std = false;
bool   report_defd = false;
bool   report_used = false;

TransUnit  *currUnit;
cgNode     *current;
cgNodeTbl  *nodeTbl;

const int MAX_FILES    = 256;

/* To hold the files/directories requested at startup */
char *file_list[MAX_FILES];

/*  ###############################################################  */

char *stdc[] = 
    {
        "setlocale", "localeconv", "mblen", "mbtowc", "wctomb",
        "btowc", "wctob", "mbsinit", "mbrlen", "mbrtowc", "wcrtomb",
        "mbstowcs", "wcstombs", "mbsrtowcs", "wcsrtobms",

        "fopen", "freopen", "fflush", "fclose", "remove", "rename",
        "tmpfile", "tmpnam", "setvbuf", "setbuf", "fwide",

        "fprintf", "printf", "sprintf", "vprintf", "vfprintf", "vsprintf",

        "fscanf", "scanf", "sscanf",

        "fwscanf", "wscanf", "swscanf",
        "fwprintf", "wprintf", "swprintf", "vfwprintf", "vwprintf",
        "vswprintf", 

        "fgetc", "fgets", "fputc", "fputs", "getc", "getchar",
        "gets", "putc", "putchar", "puts", "ungetc",

        "fgetwc", "getwc", "getwchar", "ungetwc", "fgetws",
        "fputwc", "putwc", "putwchar", "fputws",

        "fread", "fwrite",

        "fseek", "ftell", "rewind", "fgetpos", "fsetpos",

        "clearerr", "feof", "ferror", "perror",

        "isalnum", "isalpha", "iscntrl", "isdigit", "isgraph",
        "islower", "isprint", "ispunct", "isspace", "isupper",
        "isxdigit", "tolower", "toupper",

        "iswalnum", "iswalpha", "iswcntrl", "iswdigit", "iswgraph",
        "iswlower", "iswprint", "iswpunct", "iswspace", "iswupper",
        "iswxdigit", "towlower", "towupper", "wctype", "iswctype",
        "wctrans", "towctrans",

        "strcpy", "strncpy", "strcat", "strncat", "strcmp", "strncmp",
        "strchr", "strrchr", "strspn", "strcspn", "strpbrk", "strstr",
        "strlen", "strerror", "strtok", "memcpy", "memmove", "memcmp",
        "memchr", "memset", "strcoll", "strxfrm",

        "wcscat", "wcsncat", "wcscmp", "wcsncmp", "wcscpy", "wcsncpy",
        "wcslen", "wcschr", "wcsrchr", "wcsspn", "wcscspn", "wcspbrk",
        "wcstok", "wcsstr", "wcstod", "wcstol", "wcstoul",  "wmemchr",
        "wmemcmp", "wmemcpy", "wmemmove", "wmemset", "wcscoll", "wcsxfrm", 

        "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "sinh", "cosh", "tanh", "exp", "log", "log10", "pow",
        "sqrt", "ceil", "floor", "fabs", "ldexp", "frexp", "modf", "fmod",

        "atof", "atoi", "atol", "strtod", "strtol", "strtoul", "rand",
        "srand", "calloc", "malloc", "realloc", "free", "abort", "exit",
        "atexit", "system", "getenv", "bsearch", "qsort", "abs", "labs",
        "div", "ldiv",

        "assert",

        "va_start", "va_arg", "va_end",

        "setjmp", "longjmp",

        "signal", "raise",

        "clock", "time", "difftime", "mktime", "asctime", "ctime",
        "gmtime", "localtime", "strftime", "wcsftime",

        NULL
    };

/*  ###############################################################  */
cgNode::cgNode( Symbol *nme /* = NULL */ )
{
    name = nme;
    filename = NULL;

    defined = false;
    used    = false;
    std     = false;

    nUses = 0;
    next  = NULL;
}

cgNode::~cgNode()
{
}

/*  ###############################################################  */
void
cgNode::print(ostream& out) const
{
    out << (*name) << ": ";

    for (int j=0; j < nUses; j++)
        if (show_std || !uses[j]->std)
            out << *(uses[j]->name) << " ";
    out << endl;
}

/*  ###############################################################  */
void
cgNode::prolog_print(ostream& out) const
{
    out << "defined(" << (*name) << ").\n";

    for (int j=0; j < nUses; j++)
    {
        if (show_std || !uses[j]->std)
        {
            out << "uses(" << (*name) << ","
                << *(uses[j]->name) << ").\n";
        }
    }
}

/*  ###############################################################  */
cgNodeTbl::cgNodeTbl()
{
    for (int j=0; j < kMaxNodeTblBckts; j++)
        tbl[j] = NULL;
}

cgNodeTbl::~cgNodeTbl()
{
    cgNode *item, *prev=NULL;

    for (int j=0; j < kMaxNodeTblBckts; j++)
    {
        prev = NULL;

        for (item=tbl[j]; item; item=item->next)
        {
            delete prev;
            prev = item;
        }

        delete prev;
    }
}

/*  ###############################################################  */
cgNode*
cgNodeTbl::insert( Symbol *sym )
{
    uint hsh;
    int bckt;
    cgNode  *nodePtr;

    hsh  = sym->hash();
    bckt = hsh % kMaxNodeTblBckts;

    for (nodePtr=tbl[bckt]; nodePtr; nodePtr=nodePtr->next)
    {
      if (nodePtr->name->name == sym->name)
          return nodePtr;
    }

    nodePtr = new cgNode(sym);
    nodePtr->next = tbl[bckt];
    tbl[bckt] = nodePtr;

    return nodePtr;
}

/*  ###############################################################  */
void
cgNodeTbl::setStd()
{
    int     bckt;
    cgNode  *nodePtr;

    int k = 0;

    while (stdc[k] != NULL)
    {
        for (bckt=0; bckt < kMaxNodeTblBckts; bckt++)
        {
            for (nodePtr=tbl[bckt]; nodePtr; nodePtr=nodePtr->next)
            {
              if (nodePtr->name->name == stdc[k])
              {
                  nodePtr->std = true;
              }
            }
        }

        k++;
    }
}

/*  ###############################################################  */
void
Usage(char *prog)
{
    cerr << "Usage: " << prog << " [options] [filename(s)]\n\n";

    cerr << "This program parses C code, and shows its call-graph.\n\n";

    cerr << "Options:\n";
    cerr << "\t-g:\t\t Show call graph.\n";
    cerr << "\t-p:\t\t Show call graph as prolog facts.\n";
    cerr << "\t-s:\t\t Show standard functions (printf,strcpy,etc.).\n";
    cerr << "\t-d:\t\t Report defined but unused functions.\n";
    cerr << "\t-u:\t\t Report used but not defined functions.\n";
    cerr << "\t-help:\t\t Show usage (-h).\n";
    
    exit(0);
}

/*  ###############################################################  */
int
processArgs( int argc, char **argv )
{
    int num_files = 0;
    char *arg;
    char *prog = argv[0];

    argc--; 
    argv++;

    while(argc-- > 0)
    {
        arg = *argv++;

        if ( strcmp(arg,"-g") == 0 )
          show_graph = true;

        if ( strcmp(arg,"-p") == 0 )
          prolog_graph = true;

        else if ( strcmp(arg,"-s") == 0 )
          show_std = true;

        else if ( strcmp(arg,"-d") == 0 )
          report_defd = true;

        else if ( strcmp(arg,"-u") == 0 )
          report_used = true;

        else if ((strcmp(arg,"-help") == 0)
            || (strcmp(arg,"-h") == 0))
          Usage(prog);

        else
          file_list[num_files++] = arg;    
    }

    file_list[num_files] = NULL;
    return num_files;
}

/*  ###############################################################  */
void
atFuncCall( Expression *call )
{
    if (call->etype == ET_FunctionCall)
    {
        FunctionCall *fcall = (FunctionCall*) call;
        
        Expression *expr = fcall->function;

        if (expr->etype == ET_Variable)
        {
            Variable *var = (Variable*) expr;

            cgNode *me = nodeTbl->insert( var->name );
            me->used = true;

            int j;
            for (j=0; j < current->nUses; j++)
                if (current->uses[j] == me)
                    break;

            if (j == current->nUses)
            {
                current->uses.push_back(me);
                current->nUses++;
            }
        }
        else
        {
            cout << "Non-constant function call:    ";
            expr->print(cout);
            cout << endl;
        }
    }
}

/*  ###############################################################  */
void
atFuncDef( FunctionDef *function )
{
    current = nodeTbl->insert( function->FunctionName() );
    current->filename = currUnit->filename;

    if (current->defined)
        cerr << "Error: " << *(current->name) << " is multiply defined.\n";
    else
        current->defined = true;

    function->findExpr(atFuncCall);
}

/*  ###############################################################  */

int
main( int argc, char **argv )
{
    int   i, nf;

    InitStdPath();

    nodeTbl = new cgNodeTbl;

    if ((nf = processArgs(argc,argv)) < 0)
        return -1;

    if (nf == 0)
        Usage(argv[0]);

    Project  *prj = new Project();

    for (i=0; i < nf; i++)
    {
        cout << "Parsing file: " << file_list[i] << endl;

        currUnit = prj->parse(file_list[i]);
        if (currUnit != NULL)
            currUnit->findFunctionDef(atFuncDef);
    }

    nodeTbl->setStd();

    // Show the call-graph
    if (show_graph || show_std)
    {
        for (int j=0; j < kMaxNodeTblBckts; j++)
        {
            for (cgNode *node=nodeTbl->tbl[j]; node; node=node->next)
                if (node->defined
                   && ((show_std && node->std) || (show_graph && !node->std)))
                {
                    node->print(cout);
                    cout << endl;
                }
        }
    }

    // Show graph as prolog facts
    if (prolog_graph)
    {
        for (int j=0; j < kMaxNodeTblBckts; j++)
        {
            for (cgNode *node=nodeTbl->tbl[j]; node; node=node->next)
                if (node->defined && (show_std || !node->std))
                {
                    node->prolog_print(cout);
                    cout << endl;
                }
        }
    }

    if (report_defd)
    {
        cout << "Defined but not used:\n";

        for (int j=0; j < kMaxNodeTblBckts; j++)
        {
            for (cgNode *node=nodeTbl->tbl[j]; node; node=node->next)
                if (node->defined && !node->used)
                {
                    cout << "    " << *(node->name) << endl;
                }
        }
    }

    if (report_used)
    {
        cout << "Used but not defined:\n";

        for (int j=0; j < kMaxNodeTblBckts; j++)
        {
            for (cgNode *node=nodeTbl->tbl[j]; node; node=node->next)
                if (!node->defined && node->used
                    && (!node->std || show_std) )
                {
                    cout << "    " << *(node->name) << endl;
                }
        }
    }


    delete nodeTbl;
    delete prj;

    return 0;
}

/*  ###############################################################  */
