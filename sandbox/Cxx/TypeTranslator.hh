//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef TypeTranslator_hh_
#define TypeTranslator_hh_

#include <Synopsis/AST/TypeKit.hh>
#include <Synopsis/PTree.hh>
#include <string>
#include <vector>

using namespace Synopsis;

class TypeTranslator
{
public:
  TypeTranslator(Python::Object types, bool v, bool d);

  //. look up and return the named type. If this is a derived type,
  //. it may create a modifier and return that.
  AST::Type lookup(PTree::Encoding const &name);
  AST::Type lookup_function_types(PTree::Encoding const &name, AST::TypeList &);
  AST::Type declare(AST::ScopedName name,
		    AST::Declaration declaration);
  AST::Type declare(AST::ScopedName name,
		    AST::Declaration declaration,
		    AST::Template::Parameters parameters);
  AST::Type create_dependent(AST::ScopedName name);

private:

  PTree::Encoding::iterator decode_type(PTree::Encoding::iterator, AST::Type &);
  PTree::Encoding::iterator decode_func_ptr(PTree::Encoding::iterator,
					    AST::Type &type,
					    AST::Modifiers &postmod);
  PTree::Encoding::iterator decode_name(PTree::Encoding::iterator,
					std::string &name);

  Python::Object  my_types;
  AST::TypeKit    my_type_kit;
  AST::SourceFile my_file;
  PTree::Encoding my_name;
  bool            my_verbose;
  bool            my_debug;  
};

#endif
