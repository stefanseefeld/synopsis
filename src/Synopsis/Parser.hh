//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_Parser_hh_
#define Synopsis_Parser_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolLookup.hh>
#include <vector>

namespace Synopsis
{

class Lexer;
class Token;

//. C++ Parser
//.
//. This parser is a LL(k) parser with ad hoc rules such as
//. backtracking.
//.
//. <name>() is the grammer rule for a non-terminal <name>.
//. opt_<name>() is the grammer fule for an optional non-terminal <name>.
//. is_<name>() looks ahead and returns true if the next symbol is <name>.
class Parser
{
public:
  //. RuleSet defines non-standard optional rules that can be chosen at runtime.
  enum RuleSet { CXX = 0x01, MSVC = 0x02};

  //. Error is used to cache parse errors encountered during the execution
  //. of the parse method.
  class Error
  {
  public:
    virtual ~Error() {}
    virtual void write(std::ostream &) const = 0;
  };
  typedef std::vector<Error *> ErrorList;

  Parser(Lexer &lexer, SymbolLookup::Table &table, int ruleset = CXX);
  ~Parser();

  const ErrorList &errors() const { return my_errors;}

  //. Return the origin of the given pointer
  //. (filename and line number)
  unsigned long origin(const char *, std::string &) const;

  PTree::Node *parse();

private:
  enum DeclKind { kDeclarator, kArgDeclarator, kCastDeclarator };
  enum TemplateDeclKind { tdk_unknown, tdk_decl, tdk_instantiation, 
			  tdk_specialization, num_tdks };

  bool mark_error();
  template <typename T>
  bool declare(T *);
  void show_message_head(const char*);

  bool definition(PTree::Node *&);
  bool null_declaration(PTree::Node *&);
  bool typedef_(PTree::Typedef *&);
  bool type_specifier(PTree::Node *&, bool, PTree::Encoding&);
  bool is_type_specifier();
  bool metaclass_decl(PTree::Node *&);
  bool meta_arguments(PTree::Node *&);
  bool linkage_spec(PTree::Node *&);
  bool namespace_spec(PTree::NamespaceSpec *&);
  bool namespace_alias(PTree::Node *&);
  bool using_(PTree::Using *&);
  bool linkage_body(PTree::Node *&);
  bool template_decl(PTree::Node *&);
  bool template_decl2(PTree::TemplateDecl *&, TemplateDeclKind &kind);
  bool template_arg_list(PTree::Node *&);
  bool template_arg_declaration(PTree::Node *&);
  bool extern_template_decl(PTree::Node *&);

  bool declaration(PTree::Declaration *&);
  bool integral_declaration(PTree::Declaration *&, PTree::Encoding&, PTree::Node *, PTree::Node *, PTree::Node *);
  bool const_declaration(PTree::Declaration *&, PTree::Encoding&, PTree::Node *, PTree::Node *);
  bool other_declaration(PTree::Declaration *&, PTree::Encoding&, PTree::Node *, PTree::Node *, PTree::Node *);
  bool condition(PTree::Node *&);
  bool simple_declaration(PTree::Declaration *&);

  bool is_constructor_decl();
  bool is_ptr_to_member(int);
  bool opt_member_spec(PTree::Node *&);
  bool opt_storage_spec(PTree::Node *&);
  bool opt_cv_qualify(PTree::Node *&);
  bool opt_integral_type_or_class_spec(PTree::Node *&, PTree::Encoding&);
  bool constructor_decl(PTree::Node *&, PTree::Encoding&);
  bool opt_throw_decl(PTree::Node *&);
  
  bool declarators(PTree::Node *&, PTree::Encoding&, bool, bool = false);
  bool declarator_with_init(PTree::Node *&, PTree::Encoding&, bool, bool);
  bool declarator(PTree::Node *&, DeclKind, bool, PTree::Encoding&, PTree::Encoding&, bool,
		    bool = false);
  bool declarator2(PTree::Node *&, DeclKind, bool, PTree::Encoding&, PTree::Encoding&, bool,
		     bool, PTree::Node **);
  bool opt_ptr_operator(PTree::Node *&, PTree::Encoding&);
  bool member_initializers(PTree::Node *&);
  bool member_init(PTree::Node *&);
  
  bool name(PTree::Node *&, PTree::Encoding&);
  bool operator_name(PTree::Node *&, PTree::Encoding&);
  bool cast_operator_name(PTree::Node *&, PTree::Encoding&);
  bool ptr_to_member(PTree::Node *&, PTree::Encoding&);
  bool template_args(PTree::Node *&, PTree::Encoding&);
  
  bool arg_decl_list_or_init(PTree::Node *&, bool&, PTree::Encoding&, bool);
  bool arg_decl_list(PTree::Node *&, PTree::Encoding&);
  bool arg_declaration(PTree::Node *&, PTree::Encoding&);
  
  bool function_arguments(PTree::Node *&);
  bool initialize_expr(PTree::Node *&);
  
  bool enum_spec(PTree::EnumSpec *&, PTree::Encoding&);
  bool enum_body(PTree::Node *&);
  bool class_spec(PTree::ClassSpec *&, PTree::Encoding&);
  bool base_specifiers(PTree::Node *&);
  bool class_body(PTree::Node *&);
  bool class_member(PTree::Node *&);
  bool access_decl(PTree::Node *&);
  bool user_access_spec(PTree::Node *&);
  
  bool comma_expression(PTree::Node *&);
  
  bool expression(PTree::Node *&);
  bool conditional_expr(PTree::Node *&, bool);
  bool logical_or_expr(PTree::Node *&, bool);
  bool logical_and_expr(PTree::Node *&, bool);
  bool inclusive_or_expr(PTree::Node *&, bool);
  bool exclusive_or_expr(PTree::Node *&, bool);
  bool and_expr(PTree::Node *&, bool);
  bool equality_expr(PTree::Node *&, bool);
  bool relational_expr(PTree::Node *&, bool);
  bool shift_expr(PTree::Node *&);
  bool additive_expr(PTree::Node *&);
  bool multiply_expr(PTree::Node *&);
  bool pm_expr(PTree::Node *&);
  bool cast_expr(PTree::Node *&);
  bool typename_(PTree::Node *&);
  bool typename_(PTree::Node *&, PTree::Encoding&);
  bool unary_expr(PTree::Node *&);
  bool throw_expr(PTree::Node *&);
  bool sizeof_expr(PTree::Node *&);
  bool typeid_expr(PTree::Node *&);
  bool is_allocate_expr(int);
  bool allocate_expr(PTree::Node *&);
  bool userdef_keyword(PTree::Node *&);
  bool allocate_type(PTree::Node *&);
  bool new_declarator(PTree::Node *&, PTree::Encoding&);
  bool allocate_initializer(PTree::Node *&);
  bool postfix_expr(PTree::Node *&);
  bool primary_expr(PTree::Node *&);
  bool typeof_expr(PTree::Node *&);
  bool userdef_statement(PTree::Node *&);
  bool var_name(PTree::Node *&);
  bool var_name_core(PTree::Node *&, PTree::Encoding&);
  bool is_template_args();
  
  bool function_body(PTree::Block *&);
  bool compound_statement(PTree::Block *&, bool create_scope = false);
  bool statement(PTree::Node *&);
  bool if_statement(PTree::Node *&);
  bool switch_statement(PTree::Node *&);
  bool while_statement(PTree::Node *&);
  bool do_statement(PTree::Node *&);
  bool for_statement(PTree::Node *&);
  bool try_statement(PTree::Node *&);
  
  bool expr_statement(PTree::Node *&);
  bool declaration_statement(PTree::Declaration *&);
  bool integral_decl_statement(PTree::Declaration *&, PTree::Encoding&, PTree::Node *, PTree::Node *, PTree::Node *);
  bool other_decl_statement(PTree::Declaration *&, PTree::Encoding&, PTree::Node *, PTree::Node *);

  bool maybe_typename_or_class_template(Token&);
  void skip_to(Token::Type token);
  
private:
//   typedef std::stack<SymbolTable::Scope *> Scopes;
//   struct                                   ScopeGuard;

  bool more_var_name();

  Lexer               &my_lexer;
  int                  my_ruleset;
  SymbolLookup::Table &my_symbols;
  ErrorList            my_errors;
  PTree::Node         *my_comments;
  bool                 my_in_template_decl;
};

}

#endif
