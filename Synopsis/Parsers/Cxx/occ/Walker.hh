//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Walker_hh
#define _Walker_hh

#include <Synopsis/PTree.hh>

using namespace Synopsis;

class Environment;
class Class;
namespace Synopsis {class Buffer;}

class Walker : public PTree::Visitor
{
public:
  Walker(Buffer *);
  Walker(Buffer *, Environment *);
  Walker(Environment*);
  Walker(Walker*);

  PTree::Node *translate(PTree::Node *);

  virtual bool is_class_walker() { return false;}

  void visit(PTree::Node *node) { my_result = node;}
  void visit(PTree::List *);
  void visit(PTree::Identifier *node) { my_result = node;}
  void visit(PTree::Kwd::This *node) { my_result = node;}
  void visit(PTree::Brace *);
  void visit(PTree::Block *);
  void visit(PTree::ClassBody *);
  void visit(PTree::Typedef *);
  void visit(PTree::TemplateDecl *);
  void visit(PTree::TemplateInstantiation *);
  void visit(PTree::ExternTemplate *node) { my_result = node;}
  void visit(PTree::MetaclassDecl *);
  void visit(PTree::LinkageSpec *);
  void visit(PTree::NamespaceSpec *);
  void visit(PTree::NamespaceAlias *);
  void visit(PTree::UsingDirective *);
  void visit(PTree::Declaration *);
  void visit(PTree::UsingDeclaration *);
  void visit(PTree::Name *node) { my_result = node;}
  void visit(PTree::FstyleCastExpr *);
  void visit(PTree::ClassSpec *);
  void visit(PTree::EnumSpec *);
  void visit(PTree::AccessSpec *node) { my_result = node;}
  void visit(PTree::AccessDecl *node) { my_result = node;}
  void visit(PTree::UserAccessSpec *node) { my_result = node;}
  void visit(PTree::IfStatement *);
  void visit(PTree::SwitchStatement *);
  void visit(PTree::WhileStatement *);
  void visit(PTree::DoStatement *);
  void visit(PTree::ForStatement *);
  void visit(PTree::TryStatement *);
  void visit(PTree::BreakStatement *node) { my_result = node;}
  void visit(PTree::ContinueStatement *node) { my_result = node;}
  void visit(PTree::ReturnStatement *);
  void visit(PTree::GotoStatement *node) { my_result = node;}
  void visit(PTree::CaseStatement *);
  void visit(PTree::DefaultStatement *);
  void visit(PTree::LabelStatement *);
  void visit(PTree::ExprStatement *);
  void visit(PTree::Expression *);
  void visit(PTree::AssignExpr *);
  void visit(PTree::CondExpr *);
  void visit(PTree::InfixExpr *);
  void visit(PTree::PmExpr *);
  void visit(PTree::CastExpr *);
  void visit(PTree::UnaryExpr *);
  void visit(PTree::ThrowExpr *);
  void visit(PTree::SizeofExpr *);
  void visit(PTree::TypeidExpr *);
  void visit(PTree::TypeofExpr *);
  void visit(PTree::NewExpr *);
  void visit(PTree::DeleteExpr *);
  void visit(PTree::ArrayExpr *);
  void visit(PTree::FuncallExpr *); // and fstyle cast
  void visit(PTree::PostfixExpr *);
  void visit(PTree::UserStatementExpr *node) { my_result = node;}
  void visit(PTree::DotMemberExpr *);
  void visit(PTree::ArrowMemberExpr *);
  void visit(PTree::ParenExpr *);
  void visit(PTree::StaticUserStatementExpr *node) { my_result = node;}

  virtual PTree::ClassSpec *translate_class_spec(PTree::ClassSpec *spec,
						 PTree::Node *userkey,
						 PTree::Node *def,
						 Class *metaobject);
  virtual PTree::Node *translate_template_instantiation(PTree::TemplateInstantiation *,
							PTree::Node *,
							PTree::ClassSpec *,
							Class *);
  virtual PTree::TemplateDecl *translate_template_class(PTree::TemplateDecl *,
							PTree::ClassSpec *);
  virtual PTree::TemplateDecl *translate_template_function(PTree::TemplateDecl *,
							   PTree::Node *);
  virtual Class *make_template_instantiation_metaobject(PTree::Node *full_class_spec,
							PTree::Node *userkey,
							PTree::ClassSpec *class_spec);

  virtual Class *make_template_class_metaobject(PTree::Node *,
						PTree::Node *,
						PTree::Node *);
  virtual Class *make_class_metaobject(PTree::ClassSpec *,
				       PTree::Node *, PTree::Node *);
  // TranslateStorageSpecifiers() also deals with inline, virtual, etc.
  virtual PTree::Node *translate_storage_specifiers(PTree::Node *node) { return node;}
  virtual PTree::Node *translate_declarators(PTree::Node *);
  virtual PTree::Node *translate_declarator(bool, PTree::Declarator*);
  static bool GetArgDeclList(PTree::Declarator*, PTree::Node *&);
  virtual PTree::Node *translate_arg_decl_list(bool, PTree::Node *, PTree::Node *);
  static PTree::Node *translate_arg_decl_list2(bool, Environment*, bool, bool, int,
				      PTree::Node *);
  static PTree::Node *fill_argument_name(PTree::Node *, PTree::Node *, int arg_name);
  virtual PTree::Node *translate_initialize_args(PTree::Declarator*, PTree::Node *);
  virtual PTree::Node *translate_assign_initializer(PTree::Declarator*, PTree::Node *);
  virtual PTree::Node *translate_function_implementation(PTree::Node *);
  virtual PTree::Node *record_args_and_translate_fbody(Class*, PTree::Node *args,
						       PTree::Node *body);
  virtual PTree::Node *translate_function_body(PTree::Node *);
  virtual PTree::ClassBody *translate_class_body(PTree::ClassBody *, PTree::Node *, Class *);
  virtual PTree::Node *translate_type_specifier(PTree::Node *);  
  virtual PTree::Node *translate_new2(PTree::Node *, PTree::Node *, PTree::Node *, PTree::Node *,
				      PTree::Node *, PTree::Node *, PTree::Node *);
  virtual PTree::Node *translate_new3(PTree::Node *type);
public:
  struct NameScope 
  {
    Environment* env;
    Walker* walker;
  };
  
  void new_scope();
  void new_scope(Class*);
  Environment *exit_scope();
  void RecordBaseclassEnv(PTree::Node *);
  NameScope change_scope(Environment*);
  void restore_scope(NameScope&);
  
protected:
  PTree::Node *translate_declarators(PTree::Node *, bool);
  Class* LookupMetaclass(PTree::Node *, PTree::Node *, PTree::Node *, bool);
  
private:
  Class* LookupBaseMetaclass(PTree::Node *, PTree::Node *, bool);
  static void show_message_head(const char *position);
public:
  PTree::Node *translate_new_declarator(PTree::Node *decl);
  PTree::Node *translate_new_declarator2(PTree::Node *decl);
  PTree::Node *translate_arguments(PTree::Node *);
  static PTree::Node *get_class_or_enum_spec(PTree::Node *);
  static PTree::ClassSpec *get_class_template_spec(PTree::Node *);
  static PTree::Node *strip_cv_from_integral_type(PTree::Node *);
  static void SetDeclaratorComments(PTree::Node *, PTree::Node *);
  static PTree::Node *FindLeftLeaf(PTree::Node *node, PTree::Node *& parent);
  static void SetLeafComments(PTree::Node *, PTree::Node *);
  static PTree::Node *NthDeclarator(PTree::Node *, int&);
  
  static void error_message(const char *msg, PTree::Node *name, PTree::Node *where);
  static void warning_message(const char *msg, PTree::Node *name, PTree::Node *where);
  
  static void InaccurateErrorMessage(const char*, PTree::Node *, PTree::Node *);
  static void InaccurateWarningMessage(const char*, PTree::Node *, PTree::Node *);
  
  static void ChangeDefaultMetaclass(const char*);
  
public:
  Buffer *buffer() { return my_buffer;}
  Environment *environment() { return my_environment;}
  
protected:
  Buffer      *my_buffer;
  Environment *my_environment;
  PTree::Node *my_result;
  
public:
  static const char *argument_name;
  
private:
  static Buffer     *default_buffer;
  static const char *default_metaclass;
};

#endif
