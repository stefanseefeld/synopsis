//
// Copyright (C) 2001 Stephen Davies
// Copyright (C) 2001 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef Dictionary_hh_
#define Dictionary_hh_

#include <vector>
#include <string>
#include <map>
#include <list>

#include "QName.hh"
#include "FakeGC.hh"

// Forward declaration of Type::Named
namespace Types
{
class Named;
}

// Forward declaration of ASG::Declaration
namespace ASG
{
class Declaration;
}

//. Dictionary of named declarations with lookup.
//. This class maintains a dictionary of names, which index types,
//. supposedly declared in the scope that has this dictionary. There may be
//. only one declaration per name, except in the case of function names.
class Dictionary : public FakeGC::LightObject
{
public:
    //. The type of multiple entries. We don't want to include type.hh just for
    //. this, so this is a workaround
    typedef std::vector<Types::Named*> Type_vector;

    //. Exception thrown when multiple declarations are found when one is
    //. expected. The list of declarations is stored in the exception.
    struct MultipleError
    {
        MultipleError(const std::string& n) : name(n) {}
        // The vector of types that *were* found. This is returned so that whoever
        // catches the error can proceed straight away
        std::string name;
        Type_vector types;
    };

    //. Exception thrown when a name is not found in lookup*()
    struct KeyError
    {
        //. Constructor
        KeyError(const std::string& n)
                : name(n)
        { }
        //. The name which was not found
        std::string name;
    };

    //. Returns true if name is in dict
    bool has_key(const std::string& name) const { return map_.find(name) != map_.end();}

    //. Lookup a name in the dictionary. If more than one declaration has this
    //. name then an exception is thrown.
    Types::Named* lookup(const std::string& name);// throw (MultipleError, KeyError);

    //. Lookup a name in the dictionary expecting multiple decls. Use this
    //. method if you expect to find more than one declaration, eg importing
    //. names via a using statement.
    Type_vector lookup_multiple(const std::string& name) throw (KeyError);

    //. Add a declaration to the dictionary. The name() is extracted from the
    //. declaration and its last string used as the key. The declaration is
    //. stored as a Type::Declared which is created inside this method.
    void insert(ASG::Declaration* decl);

    void remove(std::string const &name);

    //. Add a named type to the dictionary. The name() is extracted from the
    //. type and its last string used as they key.
    void insert(Types::Named* named);

    //. Dump the contents for debugging
    void dump();


private:
  typedef std::multimap<std::string, Types::Named*> NameMap;
  typedef NameMap::iterator iterator;
  typedef NameMap::value_type value_type;

  NameMap map_;
};

#endif // header guard
