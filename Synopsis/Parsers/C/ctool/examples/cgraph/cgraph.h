
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
    ##     File:         cgraph.h
    ##
    ##     Programmer:   Shawn Flisakowski
    ##     Date:         Nov 27, 1998
    ##
    ###############################################################  */

#ifndef     CGRAPH_H
#define     CGRAPH_H
 
#ifndef     _HPUX_SOURCE
#define     _HPUX_SOURCE
#endif

#include <vector>    // STL
#include <string>

#include <iostream>

/*  ###############################################################  */

#define    ON    (1)
#define    OFF   (0)

class Symbol;
class cgNode;

typedef    std::vector<cgNode*>   cgNodeList;
 
/*  ###############################################################  */

class cgNode
{
  public:
    cgNode();
    cgNode( Symbol *nme = NULL );
    ~cgNode();

    void print(ostream& out) const;
    void prolog_print(ostream& out) const;

    Symbol        *name;        // function name
    std::string    filename;    // defined in this file.

    bool           defined;
    bool           used;
    bool           std;         // Is this defined in the C library?

    long           nUses;       // list of functions used in this one.
    cgNodeList     uses;

    cgNode        *next;        // for storage in cgNodeTbl.
};

/*  ###############################################################  */

const int kMaxNodeTblBckts = 256;

class cgNodeTbl
{
  public:
    cgNodeTbl();
    ~cgNodeTbl();

    cgNode    *insert( Symbol *sym );
    void       setStd();

    cgNode    *tbl[kMaxNodeTblBckts];
};

/*  ###############################################################  */

#endif     /* CGRAPH_H */
