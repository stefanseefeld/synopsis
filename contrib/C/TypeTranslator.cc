//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "TypeTranslator.hh"
#include <Synopsis/Trace.hh>

using namespace Synopsis;

namespace
{
  AST::ScopedName qname(std::string const &name) { return AST::ScopedName(name);}
}

TypeTranslator::TypeTranslator(Python::Object types, bool v, bool d)
  : my_types(types), my_verbose(v), my_debug(d)
{
  Trace trace("TypeTranslator::TypeTranslator");
  // define all the builtin types
  Python::Object define = my_types.attr("__setitem__");
  define(Python::Tuple(qname("char"), my_type_kit.create_base("C", qname("char"))));
  define(Python::Tuple(qname("short"), my_type_kit.create_base("C", qname("short"))));
  define(Python::Tuple(qname("int"), my_type_kit.create_base("C", qname("int"))));
  define(Python::Tuple(qname("long"), my_type_kit.create_base("C", qname("long"))));
  define(Python::Tuple(qname("unsigned"), my_type_kit.create_base("C", qname("unsigned"))));
  define(Python::Tuple(qname("unsigned long"), my_type_kit.create_base("C", qname("unsigned long"))));
  define(Python::Tuple(qname("float"), my_type_kit.create_base("C", qname("float"))));
  define(Python::Tuple(qname("double"), my_type_kit.create_base("C", qname("double"))));
  define(Python::Tuple(qname("void"), my_type_kit.create_base("C", qname("void"))));
  define(Python::Tuple(qname("..."), my_type_kit.create_base("C", qname("..."))));
  define(Python::Tuple(qname("long long"), my_type_kit.create_base("C", qname("long long"))));
  define(Python::Tuple(qname("long double"), my_type_kit.create_base("C", qname("long double"))));

  // some GCC extensions...
  define(Python::Tuple(qname("__builtin_va_list"), my_type_kit.create_base("C", qname("__builtin_va_list"))));
}

AST::Type TypeTranslator::lookup(PTree::Encoding const &name)
{
  Trace trace("TypeTranslator::lookup");
  trace << name;
  my_name = name;
  AST::Type type;
  decode_type(name.begin(), type);
  return type;
}

AST::Type TypeTranslator::lookup_function_types(PTree::Encoding const &name,
						AST::TypeList &parameters)
{
  Trace trace("TypeTranslator::lookup_function_types");
  trace << name;
  my_name = name;

  PTree::Encoding::iterator i = name.begin();
  assert(*i == 'F');
  ++i;
  while (true)
  {
    AST::Type parameter;
    i = decode_type(i, parameter);
    if (parameter)
      parameters.append(parameter);
    else
      break;
  }
  ++i; // skip over '_'
  AST::Type return_type;
  i = decode_type(i, return_type);
  return return_type;
}

AST::Type TypeTranslator::declare(AST::ScopedName name,
				  AST::Declaration declaration)
{
  Trace trace("TypeTranslator::declare");
  trace << name;
  AST::Type type = my_type_kit.create_declared("C", name, declaration);
  my_types.attr("__setitem__")(Python::Tuple(name, type));
  return type;
}

// This is almost a verbatim copy of the Decoder::decode
// methods from Synopsis/Parsers/Cxx/syn/decoder.cc
// with some minor modifications to disable the C++ specific things.
// FIXME: this ought to be part of SymbolLookup::Type.
PTree::Encoding::iterator TypeTranslator::decode_name(PTree::Encoding::iterator i,
						      std::string &name)
{
  Trace trace("TypeTranslator::decode_name");
  size_t length = *i++ - 0x80;
  name = std::string(length, '\0');
  std::copy(i, i + length, name.begin());
  i += length;
  return i;
}

PTree::Encoding::iterator TypeTranslator::decode_type(PTree::Encoding::iterator i,
						      AST::Type &type)
{
  Trace trace("TypeTranslator::decode_type");
  AST::Modifiers premod, postmod;
  std::string name;
  AST::Type base;

  // Loop forever until broken
  while (i != my_name.end() && !name.length() && !base)
  {
    int c = *i++;
    switch (c)
    {
      case 'P':
	postmod.insert(0, "*");
	break;
//       case 'R':
// 	postmod.insert(0, "&");
// 	break;
      case 'S':
	premod.append("signed");
	break;
      case 'U':
	premod.append("unsigned");
	break;
      case 'C':
	premod.append("const");
	break;
      case 'V':
	premod.append("volatile");
	break;
      case 'A':
      {
	std::string array("[");
	while (*i != '_') array.push_back(*i++);
	array.push_back(']');
	++i;
	premod.append(array);
	  break;
      }
//       case '*':
//       {
// 	ScopedName n;
// 	n.push_back("*");
// 	base = new Types::Dependent(n);
// 	break;
//       }
      case 'i':
	name = "int";
	break;
      case 'v':
	name = "void";
	break;
//       case 'b':
// 	name = "bool";
// 	break;
      case 's':
	name = "short";
	break;
      case 'c':
	name = "char";
	break;
//       case 'w':
// 	name = "wchar_t";
// 	break;
      case 'l':
	name = "long";
	break;
      case 'j':
	name = "long long";
	break;
      case 'f':
	name = "float";
	break;
      case 'd':
	name = "double";
	break;
      case 'r':
	name = "long double";
	break;
      case 'e':
	name = "...";
	break;
      case '?':
	name = "int"; // in C, no return type spec defaults to int
	break;
//       case 'Q':
// 	base = decodeQualType();
// 	break;
      case '_':
	--i;
	type = AST::Type();
	return i; // end of func params
      case 'F':
	i = decode_func_ptr(i, base, postmod);
	break;
//       case 'T':
// 	base = decodeTemplate();
// 	break;
//       case 'M':
// 	// Pointer to member. Format is same as for named types
// 	name = decodeName() + "::*";
// 	break;
      default:
	if (c > 0x80)
	{
	  --i;
	  i = decode_name(i, name);
	  break;
	}
	// FIXME
	// 	    std::cerr << "\nUnknown char decoding '"<<m_string<<"': "
	// 		 << char(c) << " " << c << " at "
	// 		 << (m_iter - m_string.begin()) << std::endl;
    } // switch
  } // while
  if (!base && !name.length())
  {
    // FIXME
    // 	std::cerr << "no type or name found decoding " << m_string << std::endl;
    type = AST::Type();
    return i;
  }
  if (!base)
    base = my_types.attr("__getitem__")(Python::Tuple(AST::ScopedName(name)));
  if (premod.empty() && postmod.empty())
    type = base;
  else
    type = my_type_kit.create_modifier("C", base, premod, postmod);

  return i;
}

PTree::Encoding::iterator TypeTranslator::decode_func_ptr(PTree::Encoding::iterator i,
							  AST::Type &type,
							  AST::Modifiers &postmod)
{
  Trace trace("TypeTranslator::decode_func_ptr");
  // Function ptr. Encoded same as function
  AST::Modifiers premod;
  // Move * from postmod to funcptr's premod. This makes the output be
  // "void (*convert)()" instead of "void (convert)()*"
  if (postmod.size() > 0 && postmod.get(0) == "*")
  {
    premod.append(postmod.get(0));
    postmod.erase(postmod.begin());
  }
  AST::TypeList parameters;
  while (true)
  {
    AST::Type parameter;
    i = decode_type(i, parameter);
    if (parameter)
      parameters.append(parameter);
    else
      break;
  }
  ++i; // skip over '_'
  i = decode_type(i, type);
  type = my_type_kit.create_function_ptr("C", type, premod, parameters);
  return i;
}
