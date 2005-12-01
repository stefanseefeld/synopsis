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
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/Buffer.hh>
#include "TypeTranslator.hh"
#include <stack>

using namespace Synopsis;

class ASTTranslator : private SymbolTable::Walker
{
public:
  ASTTranslator(SymbolTable::Scope *scope,
		std::string const &filename,
		std::string const &base_path, bool main_file_only,
		AST::AST a, bool v, bool d);

  void translate(PTree::Node *, Buffer &);

private:
  class TemplateParameterTranslator;
  friend class TemplateParameterTranslator;
  typedef std::stack<AST::Scope> ScopeStack;

  virtual void visit(PTree::NamespaceSpec *spec);
  virtual void visit(PTree::Declarator *decl);
  virtual void visit(PTree::Declaration *decl);
  virtual void visit(PTree::FunctionDefinition *fdef);
  virtual void visit(PTree::ClassSpec *class_spec);
  virtual void visit(PTree::EnumSpec *enum_spec);
  virtual void visit(PTree::TemplateDecl *templ);

  void translate_parameters(PTree::List *,
			    AST::TypeList, AST::Function::Parameters &);

  void add_comments(AST::Declaration, PTree::Node *);
  //. Update positional information for the given
  //. node. This will reset 'my_lineno' and may change
  //. 'my_file'.
  //. Return whether or not the node should be translated,
  //. according to the current file and the 'main_file_only' flag.
  bool update_position(PTree::Node *);

  void declare(AST::Declaration);

  AST::AST            my_ast;
  AST::ASTKit         my_ast_kit;
  AST::SourceFile     my_file;
  std::string         my_raw_filename;
  std::string         my_base_path;
  bool                my_main_file_only;
  unsigned long       my_lineno;
  TypeTranslator      my_types;
  Python::List        my_template_parameters;
  ScopeStack          my_scope;
  bool                my_in_class;
  bool                my_verbose;
  bool                my_debug;
  Buffer             *my_buffer;
  PTree::Declaration *my_declaration;
};

//. Translates template parameters for a given TemplateDecl.
class ASTTranslator::TemplateParameterTranslator 
  : private Synopsis::PTree::Visitor
{
public:
  TemplateParameterTranslator(ASTTranslator &);
  Python::List translate(Synopsis::PTree::TemplateDecl *);

private:
  virtual void visit(Synopsis::PTree::TypeParameter *);
  virtual void visit(Synopsis::PTree::ParameterDeclaration *);

  ASTTranslator &                     my_parent;
  Synopsis::AST::Template::Parameters my_parameters;
};

#endif

