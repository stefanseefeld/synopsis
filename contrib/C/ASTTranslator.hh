//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef ASTTranslator_hh_
#define ASTTranslator_hh_

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/AST/TypeKit.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/Buffer.hh>
#include "TypeTranslator.hh"

using namespace Synopsis;

class ASTTranslator : private PTree::Visitor
{
public:
  ASTTranslator(AST::AST a, bool v, bool d);

  void translate(PTree::Node *, Buffer &);

private:
  virtual void visit(PTree::List *node);
  virtual void visit(PTree::Declarator *decl);
  virtual void visit(PTree::Typedef *typed);

  void add_comments(AST::Declaration, PTree::Node *);
  //. update positional information for the given
  //. node. This will reset 'my_lineno' and may change
  //. 'my_file'.
  void update_position(PTree::Node *);

  AST::AST        my_ast;
  AST::ASTKit     my_ast_kit;
  AST::SourceFile my_file;
  unsigned long   my_lineno;
  TypeTranslator  my_types;
  bool            my_verbose;
  bool            my_debug;
  Buffer         *my_buffer;
};

#endif

