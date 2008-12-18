//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef TypeInfo_hh_
#define TypeInfo_hh_

#include "Types.hh"


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
            ASG::Typedef* tdef = Types::declared_cast<ASG::Typedef>(t);
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