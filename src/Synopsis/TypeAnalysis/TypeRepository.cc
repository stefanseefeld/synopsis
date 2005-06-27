//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "TypeRepository.hh"
#include <Synopsis/Trace.hh>
#include <sstream>
#include <stdexcept>

using Synopsis::Trace;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

TA::TypeRepository *TA::TypeRepository::instance_ = 0;


TA::TypeRepository::TypeRepository()
{
}

TA::Type const *TA::TypeRepository::lookup(PT::Encoding const &name,
					   ST::Scope *scope)
{
  Trace trace("TypeRepository::lookup", Trace::TYPEANALYSIS, name);
  PT::Encoding::iterator i = name.begin();
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
      return cvtype(lookup(PT::Encoding(i, name.end()), scope), qual);
    }
    case 'A':
    {
      std::string size;
      while (*++i != '_') size += *i;
      std::istringstream iss(size);
      unsigned long dim;
      iss >> dim;
      return array(lookup(PT::Encoding(++i, name.end()), scope), dim);
    }
    case 'F':
    {
      ++i; // skip 'F'
      PT::Encoding::iterator e = i;
      while (*e != '_') ++e;
      PT::Encoding params(i, e);
      Function::ParameterList parameters;
      for (PT::Encoding::name_iterator i = params.begin_name();
	   i != params.end_name();
	   ++i)
	if (*i->begin() != 'v') parameters.push_back(lookup(*i, scope));
      PT::Encoding retn = name.function_return_type();
      Type const *return_type = lookup(retn, scope);
      return function(parameters, return_type);
    }
    case 'P': return pointer(lookup(PT::Encoding(++i, name.end()), scope));
    case 'R': return reference(lookup(PT::Encoding(++i, name.end()), scope));
    case 'b': return &BOOL;
    case 'c': return &CHAR;
    case 'w': return &WCHAR;
    case 's': return &SHORT;
    case 'i': return &INT;
    case 'l': return &LONG;
    case 'j': return &LONGLONG;
    case 'f': return &FLOAT;
    case 'd': return &DOUBLE;
    case 'v': return &VOID;
    default:
      std::cerr << "lookup " << name << std::endl;
      return 0;
  }
}

TA::BuiltinType const *TA::TypeRepository::builtin(std::string const &name)
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

TA::Enum const *TA::TypeRepository::enum_(std::string const &)
{
  return 0;
}

TA::Class const *TA::TypeRepository::class_(std::string const &)
{
  return 0;
}

TA::Union const *TA::TypeRepository::union_(std::string const &)
{
  return 0;
}

TA::CVType const *TA::TypeRepository::cvtype(Type const *type,
					     CVType::Qualifier qualifier)
{
  return new CVType(type, qualifier);
}

TA::Pointer const *TA::TypeRepository::pointer(Type const *type)
{
  return new Pointer(type);
}

TA::Reference const *TA::TypeRepository::reference(Type const *type)
{
  return new Reference(type);
}

TA::Array const *TA::TypeRepository::array(Type const *type, unsigned long dim)
{
  return new Array(type, dim);
}

TA::Function const *TA::TypeRepository::function(Function::ParameterList const &p,
						 Type const *r)
{
  return new Function(p, r);
}

TA::PointerToMember const *TA::TypeRepository::pointer_to_member(Type const *,
								 Type const *)
{
  return 0;
}
