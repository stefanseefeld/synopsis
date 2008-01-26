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
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/Buffer.hh>
#include <stack>

using namespace Synopsis;
namespace bpl = boost::python;

class ASGTranslator : private SymbolTable::Walker
{
public:
  ASGTranslator(SymbolTable::Scope *scope,
		std::string const &filename,
		std::string const &base_path, bool primary_file_only,
		bpl::object ir, bool v, bool d);

  void translate(PTree::Node *, Buffer &);

private:
  typedef std::stack<bpl::object> ScopeStack;

  virtual void visit(PTree::NamespaceSpec *);
  virtual void visit(PTree::Declarator *);
  virtual void visit(PTree::SimpleDeclaration *);
  virtual void visit(PTree::FunctionDefinition *);
  virtual void visit(PTree::ClassSpec *);
  virtual void visit(PTree::EnumSpec *);
  virtual void visit(PTree::ElaboratedTypeSpec *);
  virtual void visit(PTree::TemplateDeclaration *);
  virtual void visit(PTree::TypeParameter *);
  virtual void visit(PTree::ParameterDeclaration *);

  //. The lexer returns comments in units as they are defined by
  //. the language, i.e. blocks marked up by '/*' and '*/' or
  //. blocks marked up by '//' and '\n'.
  //. Here we concatenate subsequent '//' blocks if they are
  //. adjacent.
  //. Further, if they are separated from the following declaration,
  //. they are annotated as 'suspect', and thus can easily be filtered
  //. out in later processing steps.
  template <typename T>
  bpl::list translate_comments(T *);
  //. Update positional information for the given
  //. node. This will reset 'my_lineno' and may change
  //. 'my_file'.
  //. Return whether or not the node should be translated,
  //. according to the current file and the 'main_file_only' flag.
  bool update_position(PTree::Node *);

  //. look up and return the named type. If this is a derived type,
  //. it may create a modifier and return that.
  bpl::object lookup(PTree::Encoding const &name);
  bpl::object lookup_function_types(PTree::Encoding const &name, bpl::list);
  bpl::object declare(bpl::tuple qname,
                      bpl::object declaration);
  bpl::object declare(bpl::tuple qname,
                      bpl::object declaration,
                      bpl::list parameters);
  bpl::object create_dependent(bpl::tuple qname);

  void declare(bpl::object);

  PTree::Encoding::iterator decode_type(PTree::Encoding::iterator, bpl::object);
  PTree::Encoding::iterator decode_qtype(PTree::Encoding::iterator, bpl::object);
  PTree::Encoding::iterator decode_template(PTree::Encoding::iterator, bpl::object);

  PTree::Encoding::iterator decode_func_ptr(PTree::Encoding::iterator,
					    bpl::object type,
					    bpl::list postmod);
  PTree::Encoding::iterator decode_name(PTree::Encoding::iterator,
					std::string &name);


  bpl::object         asg_module_;
  bpl::object         sf_module_;
  bpl::dict           files_;
  bpl::dict           types_;
  bpl::list           declarations_;
  bpl::object         file_;
  std::string         raw_filename_;
  std::string         base_path_;
  bool                primary_file_only_;
  unsigned long       lineno_;
  bpl::list           template_parameters_;
  ScopeStack          scope_;
  bool                in_class_;
  bool                verbose_;
  bool                debug_;
  Buffer             *buffer_;
  PTree::Declaration *declaration_;
  bpl::object         parameter_;
  PTree::Encoding     name_;
};

#endif
