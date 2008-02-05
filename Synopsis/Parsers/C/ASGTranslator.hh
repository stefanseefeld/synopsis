//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef ASGTranslator_hh_
#define ASGTranslator_hh_

#include <Synopsis/ASG/ASGKit.hh>
#include <Synopsis/ASG/TypeKit.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/Buffer.hh>
#include <stack>

using namespace Synopsis;

class ASGTranslator : private PTree::Visitor
{
public:
  ASGTranslator(std::string const &filename,
		std::string const &base_path, bool primary_file_only,
		IR a, bool v, bool d);

  void translate(PTree::Node *, Buffer &);

private:
  typedef std::stack<ASG::Scope> ScopeStack;

  virtual void visit(PTree::List *);
  virtual void visit(PTree::Declarator *);
  virtual void visit(PTree::Declaration *);
  virtual void visit(PTree::FunctionDefinition *);
  virtual void visit(PTree::ClassSpec *);
  virtual void visit(PTree::EnumSpec *);
  virtual void visit(PTree::Typedef *);

  void translate_parameters(PTree::Node *,
			    ASG::TypeList, ASG::Function::Parameters &);

  void add_comments(ASG::Declaration, PTree::Node *);
  //. Update positional information for the given
  //. node. This will reset 'my_lineno' and may change
  //. 'my_file'.
  //. Return whether or not the node should be translated,
  //. according to the current file and the 'primary_file_only' flag.
  bool update_position(PTree::Node *);

  //. look up and return the named type. If this is a derived type,
  //. it may create a modifier and return that.
  ASG::Type lookup(PTree::Encoding const &name);
  ASG::Type lookup_function_types(PTree::Encoding const &name, ASG::TypeList &);


  void declare(ASG::Declaration);
  ASG::Type declare(ScopedName name, ASG::Declaration declaration);

  PTree::Encoding::iterator decode_type(PTree::Encoding::iterator, ASG::Type &);
  PTree::Encoding::iterator decode_func_ptr(PTree::Encoding::iterator,
					    ASG::Type &type,
					    ASG::Modifiers &postmod);
  PTree::Encoding::iterator decode_name(PTree::Encoding::iterator,
					std::string &name);


  ASG::ASGKit         asg_kit_;
  ASG::TypeKit        type_kit_;
  SourceFileKit       sf_kit_;
  Python::List        declarations_;
  Python::Dict        types_;
  Python::Dict        files_;
  SourceFile          file_;
  std::string         raw_filename_;
  std::string         base_path_;
  bool                primary_file_only_;
  unsigned long       lineno_;
  ScopeStack          scope_;
  bool                verbose_;
  bool                debug_;
  Buffer             *buffer_;
  PTree::Declaration *declaration_;
  PTree::Encoding     name_;
};

#endif

