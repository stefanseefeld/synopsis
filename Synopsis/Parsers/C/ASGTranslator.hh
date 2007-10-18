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
#include "TypeTranslator.hh"
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

  void declare(ASG::Declaration);

  IR                  my_ir;
  ASG::ASGKit         my_ast_kit;
  SourceFileKit       my_sf_kit;
  SourceFile          my_file;
  std::string         my_raw_filename;
  std::string         my_base_path;
  bool                my_primary_file_only;
  unsigned long       my_lineno;
  TypeTranslator      my_types;
  ScopeStack          my_scope;
  bool                my_verbose;
  bool                my_debug;
  Buffer             *my_buffer;
  PTree::Declaration *my_declaration;
};

#endif

