
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
    ##     cTool Demo
    ##
    ##     File:         ctdemo.cpp
    ##
    ##     Programmer:   Shaun Flisakowski
    ##     Date:         Dec 16, 1994
    ##
    ##
    ###############################################################  */

#include    <cstdio>
#include    <iostream>
#include    <cstring>
#include    <cstdlib>
#include    <cassert>

#include    "ctdemo.h"
#include    "ctool.h"

#ifdef	WINDOWS
#include "ctool/StdAfx.h"
#endif

/*  ###############################################################  */

char  *cur_file;
bool   debug = false;
bool   show_syms = false;
bool   use_cpp = true;
bool   keep_cpp_file = false;
char   gcc_for_cpp_cmmd[] ="gcc -E -ansi -pedantic %s > %s" ;
char   cc_for_cpp_cmmd[] ="cc -E -Xc -C %s > %s" ;
char  *cpp_cmmd =NULL;
char  *cd_cmmd =NULL;
char  *cpp_dir =NULL;
char  *cpp_file =NULL;

const int MAX_FILES    = 256;

/* To hold the files/directories requested at startup */
char *file_list[MAX_FILES];

/*  ###############################################################  */

void Usage(char *prog)
{
    std::cerr << "Usage: " << prog << " [options] [filename(s)]\n\n";

    std::cerr << "This program parses C code.\n\n";

    std::cerr << "Options:\n";
    std::cerr << "\t-nocpp:\t\t Don't use preprocessor.\n";
    std::cerr <<
      "\t-cppcmmd <Preprocessor command>:\t\t Use this Preprocessor command.\n";
    std::cerr <<
      "\t-cppdir <Preprocessing directory>:\t\t Change to this Preprocessing directory.\n";
    std::cerr <<
      "\t-cdcmmd <Change directory command>:\t\t Use this Change directory command.\n";
    std::cerr <<
      "\t-gcc:\t\t Use \"gcc -E -ansi -pedantic %s > %s\" as preprocessor command.\n";

    std::cerr << "\t-cc:\t\t Use \"cc -E -Xc -C %s > %s\" as preprocessor command.\n";
    std::cerr << "\t-keepcppfile:\t\t Don't remove preprocessed output file.\n";
    std::cerr <<
      "\t-cppfile <Preprocessing output file>:\t\t Use this name as Output preprocessed file.\n";

    std::cerr << "\t-syms:\t\t Show symbol table. (-s).\n";
    std::cerr << "\t-version:\t Show ctool version (-V).\n";
    std::cerr << "\t-debug:\t\t Printing for regression test. (-d).\n";
    std::cerr << "\t-help:\t\t Show usage (-h).\n";
    
    exit(0);
}

/*  ###############################################################  */

int processArgs( int argc, char **argv )
{
    int num_files = 0;
    char *arg;
    char *prog = argv[0];
    int next_arg_needed = 0;

    argc--; 
    argv++;

    while(argc-- > 0)
    {
        arg = *argv++;

        if (next_arg_needed)
        {
          if (next_arg_needed == 1)
            cpp_cmmd = arg;    // "-cppcmmd" <arg>
          else if (next_arg_needed == 2)
            cd_cmmd = arg;     // "-cdcmmd" <arg>
          else if (next_arg_needed == 3)
            cpp_dir = arg;      // "-cppdir" <arg>
          else if (next_arg_needed == 4)
            cpp_file = arg;     // "-cppfile" <arg>
          
          next_arg_needed = 0;
        }
          
        else if ((strcmp(arg,"-cppcmmd") == 0))
          next_arg_needed = 1;

        else if ((strcmp(arg,"-cdcmmd") == 0))
          next_arg_needed = 2;

        else if ((strcmp(arg,"-cppdir") == 0))
          next_arg_needed = 3;

        else if ((strcmp(arg,"-cppfile") == 0))
          next_arg_needed = 4;

        else if ((strcmp(arg,"-nocpp") == 0))
          use_cpp = false;

        else if ((strcmp(arg,"-keepcppfile") == 0))
          keep_cpp_file = true;
          
        else if ((strcmp(arg,"-gcc") == 0))
          cpp_cmmd = gcc_for_cpp_cmmd;
          
        else if ((strcmp(arg,"-cc") == 0))
          cpp_cmmd = cc_for_cpp_cmmd;
          
        else if ((strcmp(arg,"-syms") == 0)
              || (strcmp(arg,"-s") == 0))
          show_syms = true;

        else if ((strcmp(arg,"-version") == 0)
              || (strcmp(arg,"-V") == 0))
          std::cout << VERSION << std::endl;

        else if ((strcmp(arg,"-debug") == 0)
              || (strcmp(arg,"-d") == 0))
          debug = true;

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

int main( int argc, char **argv )
{
    int   i, nf;

    if ((nf = processArgs(argc,argv)) < 0)
        return -1;

    if (nf == 0)
        Usage(argv[0]);

    Project  *prj = new Project();
    prj->SetPrintOption(debug);
    
    for (i=0; i < nf; i++)
    {
        TransUnit *unit = prj->parse(file_list[i], use_cpp, cpp_dir,
                                     keep_cpp_file, cpp_file, cpp_cmmd, cd_cmmd);

        if (unit) 
        {
            /* Show the parse tree. */
            std::cout << *unit << std::endl;
        } else {
          std::cout << " translation unit for " << file_list[i] << " is NULL." << std::endl;
        }

        if ((show_syms || debug) && unit)
        {
          std::cout << "Symbols:";
          unit->contxt.syms->Show(std::cout);
          std::cout << "Tags:";
          unit->contxt.tags->Show(std::cout);
          std::cout << "Labels:";
          unit->contxt.labels->Show(std::cout);
        }
    }

    delete prj;

    return 0;
}

/*  ###############################################################  */
