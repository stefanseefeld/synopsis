//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef ASGTranslator_hh_
#define ASGTranslator_hh_

#include <boost/python.hpp>
#include <Synopsis/PTree.hh>
#include <Synopsis/Buffer.hh>
#include <stack>

using namespace Synopsis;
namespace bpl = boost::python;

class ASGTranslator : private PTree::Visitor
{
public:
  ASGTranslator(std::string const &filename,
		std::string const &base_path, bool main_file_only,
		bpl::object ir, bool v, bool d);

  void translate(PTree::Node *, Buffer &);

private:
  typedef std::stack<bpl::object> ScopeStack;

  virtual void visit(PTree::List *);
  virtual void visit(PTree::Declarator *);
  virtual void visit(PTree::SimpleDeclaration *);
  virtual void visit(PTree::ClassSpec *);
  virtual void visit(PTree::EnumSpec *);
  virtual void visit(PTree::ParameterDeclaration *);

  void add_comments(bpl::object declaration, PTree::Node *);
  //. Update positional information for the given
  //. node. This will reset 'lineno_' and may change
  //. 'file_'.
  //. Return whether or not the node should be translated,
  //. according to the current file and the 'primary_file_only' flag.
  bool update_position(PTree::Node *);

  //. look up and return the named type. If this is a derived type,
  //. it may create a modifier and return that.
  bpl::object lookup(PTree::Encoding const &name);
  bpl::object lookup_function_types(PTree::Encoding const &name, bpl::list types);
  bpl::object declare(bpl::object qname, bpl::object declaration);

  void declare(bpl::object declaration);

  PTree::Encoding::iterator decode_type(PTree::Encoding::iterator, bpl::object &);
  PTree::Encoding::iterator decode_func_ptr(PTree::Encoding::iterator,
					    bpl::object &type,
					    bpl::list postmod);
  PTree::Encoding::iterator decode_name(PTree::Encoding::iterator,
					std::string &name);

  bpl::object         ir_;
  bpl::object         asg_module_;
  bpl::object         sf_module_;
  bpl::object         file_;
  std::string         raw_filename_;
  std::string         base_path_;
  bool                primary_file_only_;
  unsigned long       lineno_;
  bpl::dict           types_;
  ScopeStack          scope_;
  bool                verbose_;
  bool                debug_;
  Buffer             *buffer_;
  PTree::Declaration *declaration_;
  bpl::object         parameter_;
  PTree::Encoding     name_;
};

#endif

