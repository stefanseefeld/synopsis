//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef TypeTranslator_hh_
#define TypeTranslator_hh_

#include <Synopsis/ASG/TypeKit.hh>
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
  ASG::Type lookup(PTree::Encoding const &name);
  ASG::Type lookup_function_types(PTree::Encoding const &name, ASG::TypeList &);
  ASG::Type declare(ASG::ScopedName name,
		    ASG::Declaration declaration);

private:

  PTree::Encoding::iterator decode_type(PTree::Encoding::iterator, ASG::Type &);
  PTree::Encoding::iterator decode_func_ptr(PTree::Encoding::iterator,
					    ASG::Type &type,
					    ASG::Modifiers &postmod);
  PTree::Encoding::iterator decode_name(PTree::Encoding::iterator,
					std::string &name);

  Python::Object  my_types;
  ASG::TypeKit    my_type_kit;
  SourceFile      my_file;
  PTree::Encoding my_name;
  bool            my_verbose;
  bool            my_debug;  
};

#endif
