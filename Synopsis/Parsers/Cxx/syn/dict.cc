// Synopsis C++ Parser: dict.cc source file
// Implementation of the Dictionary class

// $Id: dict.cc,v 1.12 2002/11/17 12:11:43 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2001, 2002 Stephen Davies
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

// $Log: dict.cc,v $
// Revision 1.12  2002/11/17 12:11:43  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
//


#include "dict.hh"
#include "ast.hh"
#include "type.hh"

#include <map>
#include <vector>
#include <list>
#include <string>
#include <iostream>

typedef std::multimap<std::string, Types::Named*> name_map;

//. Define nested class
struct Dictionary::Data
{
    name_map map;
};

//. Constructor
Dictionary::Dictionary()
{
    m = new Data;
}

//. Destructor
Dictionary::~Dictionary()
{
    delete m;
}

//. Quick check if the name is in the dict
bool
Dictionary::has_key(const std::string& name)
{
    return m->map.find(name) != m->map.end();
}

//. Lookup single
//. TODO: the forward filtering could probably be done much simpler!
//. TODO: The exception throwing is heavy too, besides all the vector copying!
Types::Named*
Dictionary::lookup(const std::string& name)
{
    name_map::iterator iter = m->map.lower_bound(name);
    name_map::iterator end = m->map.upper_bound(name);

    // Check for not found
    if (iter == end)
        throw KeyError(name);

    Types::Named* type = iter->second;
    if (++iter == end)
        return type;

    // Check for Unknown types
    if (dynamic_cast<Types::Unknown*>(type))
    {
        // Skip further unknown types
        while (iter != end && dynamic_cast<Types::Unknown*>(iter->second)
              )
            ++iter;
        if (iter == end)
            // No choice but to return the Unknown
            return type;
        type = (iter++)->second;
        // Any other types that aren't unknown cause error
        while (iter != end && dynamic_cast<Types::Unknown*>
                (iter->second))
            ++iter;
        if (iter == end)
            // No more non-Unknown types, so return the one we found
            return type;
    }
    // Create exception object
    MultipleError exc;
    exc.types.push_back(type)
    ;
    do
        exc.types.push_back(iter->second);
    while (++iter != end);
    throw exc;
}

//. Lookup multiple
std::vector<Types::Named*>
Dictionary::lookupMultiple(const std::string& name) throw (KeyError)
{
    name_map::iterator iter = m->map.lower_bound(name);
    name_map::iterator end = m->map.upper_bound(name);

    // Check for not found
    if (iter == end)
        throw KeyError(name);

    // Store type pointers in a vector
    std::vector<Types::Named*> types;
    do
        types.push_back(iter->second);
    while (++iter != end);
    return types;
}

void
Dictionary::insert(Types::Named* type)
{
    std::string key = type->name().back();
    m->map.insert(name_map::value_type(key, type));
}

void
Dictionary::insert(AST::Declaration* decl)
{
    Types::Declared* declared = new Types::Declared(decl->name(), decl);
    insert(declared);
    if (AST::Function* func = dynamic_cast<AST::Function*>(decl))
        // Also insert real (unmangled) name of functions
        m->map.insert(name_map::value_type(func->realname(), declared));
}

void
Dictionary::dump()
{
    name_map::iterator iter = m->map.begin(), end = m->map.end();
    std::cout << "Dumping dictionary: " << m->map.size() << " items.\n";
    while (iter != end)
    {
        name_map::value_type p = *iter++;
        std::cout << "   " << p.first << "\t-> " << p.second->name() << "\n";
    }
    std::cout.flush();
}
// vim: set ts=8 sts=4 sw=4 et:
