// Synopsis C++ Parser: typeinfo.hh header file
// The TypeInfo class determines information about a type

// $Id: typeinfo.hh,v 1.3 2002/11/17 12:11:44 chalky Exp $
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

#ifndef H_SYNOPSIS_CPP_TYPEINFO
#define H_SYNOPSIS_CPP_TYPEINFO

#include "type.hh"
#include "dumper.hh"


// The TypeInfo class determines information about a type
class TypeInfo : public Types::Visitor
{
public:
    Types::Type* type;
    bool is_const;
    bool is_volatile;
    bool is_null;
    size_t deref;

    //. Constructor
    TypeInfo(Types::Type* t)
    {
        type = t;
        is_const = is_volatile = is_null = false;
        deref = 0;
        set(t);
    }
    //. Set to the given type
    void set(Types::Type* t)
    {
        type = t;
        t->accept(this);
    }
    //. Base -- null is flagged since it is special
    void visit_base(Types::Base* base)
    {
        if (base->name().back() == "__null_t")
            is_null = true;
    }
    //. Modifiers -- recurse on the alias type
    void visit_modifier(Types::Modifier* mod)
    {
        Types::Type::Mods::const_iterator iter;
        // Check for const
        for (iter = mod->pre().begin(); iter != mod->pre().end(); iter++)
            if (*iter == "const")
                is_const = true;
            else if (*iter == "volatile")
                is_volatile = true;
        // Check for derefs
        for (iter = mod->post().begin(); iter != mod->post().end(); iter++)
            if (*iter == "*")
                deref++;
            else if (*iter == "[]")
                deref++;

        set(mod->alias());
    }
    //. Declared -- check for typedef
    void visit_declared(Types::Declared* t)
    {
        try
        {
            AST::Typedef* tdef = Types::declared_cast<AST::Typedef>(t);
            // Recurse on typedef alias
            set(tdef->alias());
        }
        catch (const Types::wrong_type_cast&)
        { /* Ignore -- just means not a typedef */
        }
    }
};

//. Output operator for debugging
std::ostream& operator << (std::ostream& o, TypeInfo& i);
#endif
// vim: set ts=8 sts=4 sw=4 et:
