//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef H_SYNOPSIS_CPP_SWALKER
#define H_SYNOPSIS_CPP_SWALKER

#include <occ/PTree.hh>
#include <occ/Walker.hh>
// Stupid occ
#undef Scope

#include <vector>
#include <string>

// Forward declarations
class Builder;
class Buffer;
class Decoder;
class TypeFormatter;
class LinkStore;
class Lookup;
class FileFilter;

namespace AST
{
class SourceFile;
class Parameter;
class Inheritance;
class Declaration;
class Function;
class Scope;
}
namespace Types
{
class Type;
}

//. A walker that creates an AST. All Translate* methods have been overridden
//. to remove the translation code.
class SWalker : public Walker
{
public:
  //. Constructor
  SWalker(FileFilter*, Builder*, Buffer*);
  virtual ~SWalker();

  //. Sets store links to true.
  //. This will cause the whole ptree to be traversed, and any linkable
  //. identifiers found will be stored
  void set_store_links(LinkStore*);

  //. Get a name from the ptree
  std::string parse_name(PTree::Node *) const;
  
  //. Get the Buffer object
  Buffer* buffer() { return my_buffer;}
  //. Get the Builder object
  Builder* builder() { return my_builder;}
  //. Get the TypeFormatter object
  TypeFormatter* type_formatter(){ return my_type_formatter;}
#if 0
  //. Returns true if the current filename from the last getLine or
  //. updateLineNumber call is equal to the main source filename
  bool is_main_file() { return (my_filename == my_source);}
#endif

  //. Get the line number of the given Ptree node
  int line_of_ptree(PTree::Node *);
  //. Update the line number
  void update_line_number(PTree::Node *);

  void add_comments(AST::Declaration* decl, PTree::Node *comments);
  void add_comments(AST::Declaration* decl, PTree::CommentedAtom *node);
  void add_comments(AST::Declaration* decl, PTree::Declarator *node);
  void add_comments(AST::Declaration* decl, PTree::Declaration *decl);
  void add_comments(AST::Declaration* decl, PTree::NamespaceSpec *decl);
  //. Traverses left side of tree till it finds a leaf, and if that is a
  //. CommentedAtom then it adds those comments as spans
  void find_comments(PTree::Node * node);

  // Takes the (maybe 0) args list and puts them in my_params
  void translate_function_args(PTree::Node * args);
  void translate_parameters(PTree::Node * p_params, std::vector<AST::Parameter*>& params);
  void translate_function_name(const PTree::Encoding &encname, std::string& realname, Types::Type*& returnType);
  PTree::Node *translate_declarator(PTree::Node *);
  PTree::Node *translate_function_declarator(PTree::Node *, bool is_const);
  PTree::Node *translate_variable_declarator(PTree::Node *, bool is_const);
  void translate_typedef_declarator(PTree::Node *node);
  std::vector<AST::Inheritance*> translate_inheritance_spec(PTree::Node *node);
  //. Returns a formatter string of the parameters. The idea is that this
  //. string will be appended to the function name to form the 'name' of the
  //. function.
  std::string format_parameters(std::vector<AST::Parameter*>& params);

  //. Translates the template parameters and creates the template type.
  void SWalker::translate_template_params(PTree::Node *params);

  //. Overridden to catch exceptions
  void translate(PTree::Node *);

  void visit(PTree::Atom *);
  void visit(PTree::Typedef *);
  void visit(PTree::TemplateDecl *);
  void visit(PTree::TemplateInstantiation *);
  //virtual PTree::Node * TranslateTemplateInstantiation(PTree::Node *, PTree::Node *, PTree::Node *, Class*);
  void visit(PTree::ExternTemplate *);
  void visit(PTree::Identifier *node) { translate_variable(node);}
  void visit(PTree::Brace *);
  void visit(PTree::Block *);
  void visit(PTree::ClassSpec *);
  void visit(PTree::EnumSpec *);
  void visit(PTree::MetaclassDecl *);
  void visit(PTree::LinkageSpec *);
  void visit(PTree::NamespaceSpec *);
  void visit(PTree::Using *);
  void visit(PTree::Declaration *);
  void visit(PTree::Name *node) { translate_variable(node);}
  void visit(PTree::AccessSpec *);
  void visit(PTree::AccessDecl *);
  void visit(PTree::UserAccessSpec *);
  void visit(PTree::IfStatement *);
  void visit(PTree::SwitchStatement *);
  void visit(PTree::WhileStatement *);
  void visit(PTree::DoStatement *);
  void visit(PTree::ForStatement *);
  void visit(PTree::TryStatement *);
  void visit(PTree::BreakStatement *);
  void visit(PTree::ContinueStatement *);
  void visit(PTree::ReturnStatement *);
  void visit(PTree::GotoStatement *);
  void visit(PTree::CaseStatement *);
  void visit(PTree::DefaultStatement *);
  void visit(PTree::LabelStatement *);
  void visit(PTree::ExprStatement *);
  void visit(PTree::CommaExpr *);
  void visit(PTree::AssignExpr *);
  void visit(PTree::CondExpr *);
  void visit(PTree::InfixExpr *);
  void visit(PTree::PmExpr *);
  void visit(PTree::CastExpr *);
  void visit(PTree::UnaryExpr *);
  void visit(PTree::ThrowExpr *);
  void visit(PTree::SizeofExpr *);
  void visit(PTree::NewExpr *);
  void visit(PTree::DeleteExpr *);
  void visit(PTree::This *);
  void visit(PTree::FstyleCastExpr *);
  void visit(PTree::ArrayExpr *);
  void visit(PTree::FuncallExpr *); // and fstyle cast
  void visit(PTree::PostfixExpr *);
  void visit(PTree::UserStatementExpr *);
  void visit(PTree::DotMemberExpr *);
  void visit(PTree::ArrowMemberExpr *);
  void visit(PTree::ParenExpr *);
  void visit(PTree::StaticUserStatementExpr *);
  
  PTree::TemplateDecl *translate_template_class(PTree::TemplateDecl *,
						PTree::ClassSpec *);
  PTree::TemplateDecl *translate_template_function(PTree::TemplateDecl *,
						   PTree::Node *);
  PTree::Node *translate_storage_specifiers(PTree::Node *);
  PTree::Node *translate_declarators(PTree::Node *);
  PTree::Node *translate_arg_decl_list(bool, PTree::Node *, PTree::Node *);
  PTree::Node *translate_initialize_args(PTree::Declarator *, PTree::Node *);
  PTree::Node *translate_assign_initializer(PTree::Declarator *, PTree::Node *);
  PTree::Node *translate_function_implementation(PTree::Node *);
  PTree::Node *translate_function_body(PTree::Node *);
  //virtual PTree::Node * TranslateClassBody(PTree::Node *, PTree::Node *, Class*);
  
  //virtual Class* MakeClassMetaobject(PTree::Node *, PTree::Node *, PTree::Node *);
  //virtual PTree::Node * TranslateClassSpec(PTree::Node *, PTree::Node *, PTree::Node *, Class*);
  
  virtual PTree::Node *translate_type_specifier(PTree::Node *);
  virtual PTree::Node *translate_typeof(PTree::Node *, PTree::Node * declarations);
  virtual PTree::Node *translate_new3(PTree::Node *type);
  void translate_variable(PTree::Node *);
  
  AST::SourceFile* current_file() const { return my_file;}
  int current_lineno() const { return my_lineno;}
  static SWalker *instance() { return g_swalker;}
private:
  // the 'current' walker is a debugging aid.
  static SWalker* g_swalker;

  Builder* my_builder;
  FileFilter* my_filter;
  Buffer* my_buffer;
  Decoder* my_decoder;
  Lookup* my_lookup;

  //. A pointer to the currect declaration ptree, if any, used to get the
  //. return type and modifiers, etc.
  PTree::Node * my_declaration;
  //. A pointer to the current template parameters, if any, used to get the
  //. template parameters and set in the declaration. Should be 0 if not
  //. in a template.
  std::vector<AST::Parameter*>* my_template;
  //. this reflects the filename containing the currently processed node.
  std::string my_filename;
  int my_lineno;
  //. The current file, set by update_line_number
  AST::SourceFile* my_file;

  //. True if should try and extract tail comments before }'s
  bool my_extract_tails;
  //. Storage for links. This is only set if we should be storing links, so
  //. it must be checked before every use
  LinkStore* my_links;
  //. True if this TranslateDeclarator should try to store the decl type
  bool my_store_decl;
  
  //. A dummy name used for tail comments
  std::vector<std::string> my_dummyname;
  
  //. An instance of TypeFormatter for formatting types
  TypeFormatter* my_type_formatter;
  
  //. The current function, if in a function block
  AST::Function* my_function;
  //. The params found before a function block. These may be different from
  //. the ones that are in the original declaration(s), but it is these names
  //. we need for referencing names inside the block, so a reference is stored
  //. here.
  std::vector<AST::Parameter*> my_param_cache;
  //. The types accumulated for function parameters in function calls
  std::vector<Types::Type*> my_params;
  //. The type returned from the expression-type translators
  Types::Type* my_type;
  //. The Scope to use for name lookups, or 0 to use enclosing default
  //. scope rules. This is for when we are at a Variable, and already know it
  //. must be part of a given class (eg, foo->bar .. bar must be in foo's
  //. class)
  AST::Scope* my_scope;
  
  //. The state of postfix translation. This is needed for constructs like
  //. foo->var versus var or foo->var(). The function call resolution needs
  //. to be done in the final TranslateVariable, since that's where the last
  //. name (which is to be linked) is handled.
  enum Postfix_Flag
  {
    Postfix_Var, //.< Lookup as a variable
    Postfix_Func, //.< Lookup as a function, using my_params for parameters
  } my_postfix_flag;
  
  //. Info about one stored function impl. Function impls must be stored
  //. till the end of a class.
  struct FuncImplCache
  {
    AST::Function* func;
    std::vector<AST::Parameter*> params;
    PTree::Node * body;
  };
  //. A vector of function impls
  typedef std::vector<FuncImplCache> FuncImplVec;
  //. A stack of function impl vectors
  typedef std::vector<FuncImplVec> FuncImplStack;
  //. The stack of function impl vectors
  FuncImplStack my_func_impl_stack;
  void SWalker::translate_func_impl_cache(const FuncImplCache &cache);
  
  //. Finds the column given the start ptr and the current position. The
  //. derived column number is processed with the macro call dictionary
  //. from the current file before returning,
  //. so -1 may be returned to indicate "inside macro".
  int find_col(const char* start, const char* find);
};

#endif // header guard
