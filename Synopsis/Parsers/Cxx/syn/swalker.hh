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
  SWalker(FileFilter*, Parser*, Builder*, Buffer*);
  virtual ~SWalker();

  //. Sets store links to true.
  //. This will cause the whole ptree to be traversed, and any linkable
  //. identifiers found will be stored
  void set_store_links(LinkStore*);

  //. Get a name from the ptree
  std::string parse_name(PTree::Node *) const;
  
  //. Get the Parser object
  Parser* parser() { return my_parser;}
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
  void TranslateFunctionArgs(PTree::Node * args);
  void TranslateParameters(PTree::Node * p_params, std::vector<AST::Parameter*>& params);
  void TranslateFunctionName(char* encname, std::string& realname, Types::Type*& returnType);
  PTree::Node * TranslateDeclarator(PTree::Node *);
  PTree::Node * TranslateFunctionDeclarator(PTree::Node *, bool is_const);
  PTree::Node * TranslateVariableDeclarator(PTree::Node *, bool is_const);
  void TranslateTypedefDeclarator(PTree::Node * node);
  std::vector<AST::Inheritance*> TranslateInheritanceSpec(PTree::Node *node);
  //. Returns a formatter string of the parameters. The idea is that this
  //. string will be appended to the function name to form the 'name' of the
  //. function.
  std::string format_parameters(std::vector<AST::Parameter*>& params);

  //. Translates the template parameters and creates the template type.
  void SWalker::TranslateTemplateParams(PTree::Node * params);

  // default translation
  virtual PTree::Node * TranslatePtree(PTree::Node *);

  //. Overridden to catch exceptions
  void Translate(PTree::Node *);

  virtual PTree::Node * TranslateTypedef(PTree::Node *);
  virtual PTree::Node * TranslateTemplateDecl(PTree::Node *);
  virtual PTree::Node * TranslateTemplateInstantiation(PTree::Node *);
  //virtual PTree::Node * TranslateTemplateInstantiation(PTree::Node *, PTree::Node *, PTree::Node *, Class*);
  virtual PTree::Node * TranslateExternTemplate(PTree::Node *);
  virtual PTree::Node * TranslateTemplateClass(PTree::Node *, PTree::Node *);
  virtual PTree::Node * TranslateTemplateFunction(PTree::Node *, PTree::Node *);
  virtual PTree::Node * TranslateMetaclassDecl(PTree::Node *);
  virtual PTree::Node * TranslateLinkageSpec(PTree::Node *);
  virtual PTree::Node * TranslateNamespaceSpec(PTree::Node *);
  virtual PTree::Node * TranslateUsing(PTree::Node *);
  virtual PTree::Node * TranslateDeclaration(PTree::Node *);
  virtual PTree::Node * TranslateStorageSpecifiers(PTree::Node *);
  virtual PTree::Node * TranslateDeclarators(PTree::Node *);
  virtual PTree::Node * TranslateArgDeclList(bool, PTree::Node *, PTree::Node *);
  virtual PTree::Node * TranslateInitializeArgs(PTree::Declarator*, PTree::Node *);
  virtual PTree::Node * TranslateAssignInitializer(PTree::Declarator*, PTree::Node *);
  
  virtual PTree::Node * TranslateFunctionImplementation(PTree::Node *);

  virtual PTree::Node * TranslateFunctionBody(PTree::Node *);
  virtual PTree::Node * TranslateBrace(PTree::Node *);
  virtual PTree::Node * TranslateBlock(PTree::Node *);
  //virtual PTree::Node * TranslateClassBody(PTree::Node *, PTree::Node *, Class*);
  
  virtual PTree::Node * TranslateClassSpec(PTree::Node *);
  //virtual Class* MakeClassMetaobject(PTree::Node *, PTree::Node *, PTree::Node *);
  //virtual PTree::Node * TranslateClassSpec(PTree::Node *, PTree::Node *, PTree::Node *, Class*);
  virtual PTree::Node * TranslateEnumSpec(PTree::Node *);
  
  virtual PTree::Node * TranslateAccessSpec(PTree::Node *);
  virtual PTree::Node * TranslateAccessDecl(PTree::Node *);
  virtual PTree::Node * TranslateUserAccessSpec(PTree::Node *);
  
  virtual PTree::Node * TranslateIf(PTree::Node *);
  virtual PTree::Node * TranslateSwitch(PTree::Node *);
  virtual PTree::Node * TranslateWhile(PTree::Node *);
  virtual PTree::Node * TranslateDo(PTree::Node *);
  virtual PTree::Node * TranslateFor(PTree::Node *);
  virtual PTree::Node * TranslateTry(PTree::Node *);
  virtual PTree::Node * TranslateBreak(PTree::Node *);
  virtual PTree::Node * TranslateContinue(PTree::Node *);
  virtual PTree::Node * TranslateReturn(PTree::Node *);
  virtual PTree::Node * TranslateGoto(PTree::Node *);
  virtual PTree::Node * TranslateCase(PTree::Node *);
  virtual PTree::Node * TranslateDefault(PTree::Node *);
  virtual PTree::Node * TranslateLabel(PTree::Node *);
  virtual PTree::Node * TranslateExprStatement(PTree::Node *);
  
  virtual PTree::Node * TranslateTypespecifier(PTree::Node *);
  virtual PTree::Node * TranslateTypeof(PTree::Node *, PTree::Node * declarations);
  
  virtual PTree::Node * TranslateComma(PTree::Node *);
  virtual PTree::Node * TranslateAssign(PTree::Node *);
  virtual PTree::Node * TranslateCond(PTree::Node *);
  virtual PTree::Node * TranslateInfix(PTree::Node *);
  virtual PTree::Node * TranslatePm(PTree::Node *);
  virtual PTree::Node * TranslateCast(PTree::Node *);
  virtual PTree::Node * TranslateUnary(PTree::Node *);
  virtual PTree::Node * TranslateThrow(PTree::Node *);
  virtual PTree::Node * TranslateSizeof(PTree::Node *);
  virtual PTree::Node * TranslateNew(PTree::Node *);
  virtual PTree::Node * TranslateNew3(PTree::Node * type);
  virtual PTree::Node * TranslateDelete(PTree::Node *);
  virtual PTree::Node * TranslateThis(PTree::Node *);
  virtual PTree::Node * TranslateVariable(PTree::Node *);
  virtual PTree::Node * TranslateFstyleCast(PTree::Node *);
  virtual PTree::Node * TranslateArray(PTree::Node *);
  virtual PTree::Node * TranslateFuncall(PTree::Node *);	// and fstyle cast
  virtual PTree::Node * TranslatePostfix(PTree::Node *);
  virtual PTree::Node * TranslateUserStatement(PTree::Node *);
  virtual PTree::Node * TranslateDotMember(PTree::Node *);
  virtual PTree::Node * TranslateArrowMember(PTree::Node *);
  virtual PTree::Node * TranslateParen(PTree::Node *);
  virtual PTree::Node * TranslateStaticUserStatement(PTree::Node *);
  
  AST::SourceFile* current_file() const { return my_file;}
  int current_lineno() const { return my_lineno;}
  static SWalker *instance() { return g_swalker;}
private:
  // the 'current' walker is a debugging aid.
  static SWalker* g_swalker;

  Parser* my_parser;
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
  void SWalker::TranslateFuncImplCache(const FuncImplCache& cache);
  
  //. Finds the column given the start ptr and the current position. The
  //. derived column number is processed with the macro call dictionary
  //. from the current file before returning,
  //. so -1 may be returned to indicate "inside macro".
  int find_col(const char* start, const char* find);
};

#endif // header guard
