// Synopsis C++ Parser: scopeinfo.hh header file
// Definition of the ScopeInfo class which stores info about each scope for
// Builder and Lookup to use

// $Id: scopeinfo.hh,v 1.4 2002/11/17 12:11:44 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef H_SYNOPSIS_CPP_SCOPEINFO
#define H_SYNOPSIS_CPP_SCOPEINFO

#include <string>
#include <map>
#include <vector>
#include "common.hh"

class Dictionary;

//. Forward declare ScopeInfo, used later in this file
class ScopeInfo;

//. Typedef for a Scope Search
typedef std::vector<ScopeInfo*> ScopeSearch;

//. This class encapsulates information about a Scope and its dictionary of names.
//. Since the AST::Scope is meant purely for documentation purposes, this
//. class provides the extra information needed for operations such as name
//. lookup (a dictionary of types, and links to using scopes).
//.
//. The using directives work as follows: Using directives effectively add all
//. contained declarations to the namespace containing the directive, but they
//. still need to be processed separately. To handle this each ScopeInfo
//. remembers which other namespaces it is using {@see using_scopes}.
//.
//. One quirk is that all used namespaces are considered as a whole for
//. overload resolution, *not* one at a time. To facilitate this 'dummy'
//. scopeinfos are inserted into the ScopeSearch of a containing namespace,
//. which the algorithms recognize. (TODO: consider a wrapper(?) a.la
//. AST::Inheritance)
struct ScopeInfo : public cleanup
{
    //. Constructor
    ScopeInfo(AST::Scope* s);

    //. Constructor that creates a Dummy 'using' scope referencing 's'
    ScopeInfo(ScopeInfo* s);

    //. Destructor
    ~ScopeInfo();

    //. Dictionary for this scope
    Dictionary* dict;
    //class Dictionary* dict; <-- used to be this, but it confuses parser
    //--->MAKE INTO TESTCASE!!!

    //. The declaration for this scope
    AST::Scope* scope_decl;

    //. The list of scopes to search for this scope, including this
    ScopeSearch search;

    //. The list of scopes in the search because they are 'using'd
    ScopeSearch using_scopes;

    //. The list of scopes 'using' this one
    ScopeSearch used_by;

    //. True only if this is a dummy Scope representing the use of
    //. another. If this is the case, then only 'dict' is set
    bool is_using;

    //. Current accessability
    AST::Access access;

    //. Counts of named sub-namespaces. The names are things like "if", "while"
    //. etc. This is for the purely aesthetic purpose of being able to name
    //. anonymous namespaces like "`if `while `if2 `do" instead of "`if1 `while2
    //. `if3 `do4" or even "`0001 `0002 `0003 `0004" as OpenC++ does.
    std::map<std::string, int> nscounts;
    int getCount(const std::string& name);
};

#endif
// vim: set ts=8 sts=4 sw=4 et:
