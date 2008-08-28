//
// Copyright (C) 2001 Stephen Davies
// Copyright (C) 2001 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Dictionary.hh"
#include "ASG.hh"
#include "Types.hh"
#include <cassert>
#include <iostream>


//. Lookup single
//. TODO: the forward filtering could probably be done much simpler!
//. TODO: The exception throwing is heavy too, besides all the vector copying!
Types::Named*
Dictionary::lookup(const std::string& name)
{
  iterator i = map_.lower_bound(name);
  iterator end = map_.upper_bound(name);

  // Check for not found
  if (i == end)
    throw KeyError(name);

  Types::Named* type = i->second;
  if (++i == end)
    return type;

  // Check for Unknown types
  if (dynamic_cast<Types::Unknown*>(type))
  {
    // Skip further unknown types
    while (i != end && dynamic_cast<Types::Unknown*>(i->second))
      ++i;
    if (i == end)
      // No choice but to return the Unknown
      return type;
    type = (i++)->second;
    // Any other types that aren't unknown cause error
    while (i != end && dynamic_cast<Types::Unknown*>(i->second))
      ++i;
    if (i == end)
      // No more non-Unknown types, so return the one we found
      return type;
  }
  // Create exception object
  MultipleError exc(name);
  exc.types.push_back(type);
  do
    exc.types.push_back(i->second);
  while (++i != end);
  throw exc;
}

//. Lookup multiple
std::vector<Types::Named*>
Dictionary::lookup_multiple(const std::string& name) throw (KeyError)
{
  iterator i = map_.lower_bound(name);
  iterator end = map_.upper_bound(name);

  // Check for not found
  if (i == end)
    throw KeyError(name);

  // Store type pointers in a vector
  std::vector<Types::Named*> types;
  do
    types.push_back(i->second);
  while (++i != end);
  return types;
}

void
Dictionary::insert(Types::Named* type)
{
  std::string key = type->name().back();
  map_.insert(value_type(key, type));
}

void
Dictionary::insert(ASG::Declaration* decl)
{
  Types::Declared* declared = new Types::Declared(decl->name(), decl);
  insert(declared);
  if (ASG::Function* func = dynamic_cast<ASG::Function*>(decl))
    // Also insert real (unmangled) name of functions
    map_.insert(value_type(func->realname(), declared));
}

void
Dictionary::remove(std::string const &name)
{
  map_.erase(name);
}

void
Dictionary::dump()
{
  iterator i = map_.begin(), end = map_.end();
  std::cout << "Dumping dictionary: " << map_.size() << " items.\n";
  while (i != end)
  {
    value_type p = *i++;
    std::cout << "   " << p.first << "\t-> " << p.second->name() << "\n";
  }
  std::cout.flush();
}
