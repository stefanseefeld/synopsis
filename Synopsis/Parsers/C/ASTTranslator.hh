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
#include <stack>

using namespace Synopsis;

class ASTTranslator : private PTree::Visitor
{
public:
  ASTTranslator(std::string const &filename,
		std::string const &base_path, bool main_file_only,
		AST::AST a, bool v, bool d);

  void translate(PTree::Node *, Buffer &);

private:
  typedef std::stack<AST::Scope> ScopeStack;

  virtual void visit(PTree::List *);
  virtual void visit(PTree::Declarator *);
  virtual void visit(PTree::SimpleDeclaration *);
  virtual void visit(PTree::ClassSpec *);
  virtual void visit(PTree::EnumSpec *);
  virtual void visit(PTree::ParameterDeclaration *);

  void add_comments(AST::Declaration, PTree::Node *);
  //. Update positional information for the given
  //. node. This will reset 'lineno_' and may change
  //. 'file_'.
  //. Return whether or not the node should be translated,
  //. according to the current file and the 'primary_file_only' flag.
  bool update_position(PTree::Node *);

  void declare(AST::Declaration);

  AST::AST            ast_;
  AST::ASTKit         ast_kit_;
  AST::SourceFileKit  sf_kit_;
  AST::SourceFile     file_;
  std::string         raw_filename_;
  std::string         base_path_;
  bool                primary_file_only_;
  unsigned long       lineno_;
  TypeTranslator      types_;
  ScopeStack          scope_;
  bool                verbose_;
  bool                debug_;
  Buffer             *buffer_;
  PTree::Declaration *declaration_;
  AST::Parameter      parameter_;
};

#endif

