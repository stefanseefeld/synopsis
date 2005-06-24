//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Kit.hh"
#include <Synopsis/Trace.hh>
#include <sstream>
#include <stdexcept>

using namespace Synopsis;
using namespace TypeAnalysis;

Kit::Kit()
{
}

Type const *Kit::lookup(PTree::Encoding const &name, SymbolTable::Scope *scope)
{
  Trace trace("Kit::lookup", Trace::TYPEANALYSIS, name);
  PTree::Encoding::iterator i = name.begin();
  switch (*i)
  {
    case 'C':
    case 'V':
    {
      CVType::Qualifier qual = CVType::NONE;
      if (*i == 'C')
      {
	qual = CVType::CONST;
	++i;
      }
      if (*i == 'V')
      {
	qual = qual == CVType::CONST ? CVType::CV : CVType::VOLATILE;
	++i;
      }
      return cvtype(lookup(PTree::Encoding(i, name.end()), scope), qual);
    }
    case 'A':
    {
      std::string size;
      while (*++i != '_') size += *i;
      std::istringstream iss(size);
      unsigned long dim;
      iss >> dim;
      return array(lookup(PTree::Encoding(++i, name.end()), scope), dim);
    }
    case 'P': return pointer(lookup(PTree::Encoding(++i, name.end()), scope));
    case 'R': return reference(lookup(PTree::Encoding(++i, name.end()), scope));
    case 'b': return &BOOL;
    case 'c': return &CHAR;
    case 'w': return &WCHAR;
    case 's': return &SHORT;
    case 'i': return &INT;
    case 'l': return &LONG;
    case 'j': return &LONGLONG;
    case 'f': return &FLOAT;
    case 'd': return &DOUBLE;
    default:
      std::cerr << "lookup " << name << std::endl;
      return 0;
  }
}

Type const *Kit::builtin(std::string const &name)
{
  if (name == "bool") return &BOOL;
  else if (name == "char") return &CHAR;
  else if (name == "wchar_t") return &WCHAR;
  else if (name == "short") return &SHORT;
  else if (name == "int") return &INT;
  else if (name == "long") return &LONG;
  else if (name == "long long") return &LONGLONG;
  else if (name == "float") return &FLOAT;
  else if (name == "double") return &DOUBLE;
  else if (name == "unsigned char") return &UCHAR;
  else if (name == "unsigned short") return &USHORT;
  else if (name == "unsigned int") return &UINT;
  else if (name == "unsigned long") return &ULONG;
  else if (name == "signed char") return &SCHAR;
  else if (name == "signed short") return &SSHORT;
  else if (name == "signed int") return &SINT;
  else if (name == "signed long") return &SLONG;
  std::string error = "Unknown builtin type: " + name;
  throw std::runtime_error(error);
}

Type const *Kit::enum_(std::string const &)
{
  return 0;
}

Type const *Kit::class_(std::string const &)
{
  return 0;
}

Type const *Kit::union_(std::string const &)
{
  return 0;
}

Type const *Kit::cvtype(Type const *type, CVType::Qualifier qualifier)
{
  return new CVType(type, qualifier);
}

Type const *Kit::pointer(Type const *type)
{
  return new Pointer(type);
}

Type const *Kit::reference(Type const *type)
{
  return new Reference(type);
}

Type const *Kit::array(Type const *type, unsigned long dim)
{
  return new Array(type, dim);
}

Type const *Kit::pointer_to_member(Type const *, Type const *)
{
  return 0;
}
