//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef H_SYNOPSIS_CPP_SWALKER
#define H_SYNOPSIS_CPP_SWALKER

#include <occ/AST.hh>
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
  std::string parse_name(Ptree*) const;
  
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
  int line_of_ptree(Ptree*);
  //. Update the line number
  void update_line_number(Ptree*);

  void add_comments(AST::Declaration* decl, Ptree* comments);
  void add_comments(AST::Declaration* decl, CommentedLeaf* node);
  void add_comments(AST::Declaration* decl, PtreeDeclarator* node);
  void add_comments(AST::Declaration* decl, PtreeDeclaration* decl);
  void add_comments(AST::Declaration* decl, PtreeNamespaceSpec* decl);
  //. Traverses left side of tree till it finds a leaf, and if that is a
  //. CommentedLeaf then it adds those comments as spans
  void find_comments(Ptree* node);

  // Takes the (maybe 0) args list and puts them in my_params
  void TranslateFunctionArgs(Ptree* args);
  void TranslateParameters(Ptree* p_params, std::vector<AST::Parameter*>& params);
  void TranslateFunctionName(char* encname, std::string& realname, Types::Type*& returnType);
  Ptree* TranslateDeclarator(Ptree*);
  Ptree* TranslateFunctionDeclarator(Ptree*, bool is_const);
  Ptree* TranslateVariableDeclarator(Ptree*, bool is_const);
  void TranslateTypedefDeclarator(Ptree* node);
  std::vector<AST::Inheritance*> TranslateInheritanceSpec(Ptree *node);
  //. Returns a formatter string of the parameters. The idea is that this
  //. string will be appended to the function name to form the 'name' of the
  //. function.
  std::string format_parameters(std::vector<AST::Parameter*>& params);

  //. Translates the template parameters and creates the template type.
  void SWalker::TranslateTemplateParams(Ptree* params);

  // default translation
  virtual Ptree* TranslatePtree(Ptree*);

  //. Overridden to catch exceptions
  void Translate(Ptree*);

  virtual Ptree* TranslateTypedef(Ptree*);
  virtual Ptree* TranslateTemplateDecl(Ptree*);
  virtual Ptree* TranslateTemplateInstantiation(Ptree*);
  //virtual Ptree* TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*);
  virtual Ptree* TranslateExternTemplate(Ptree*);
  virtual Ptree* TranslateTemplateClass(Ptree*, Ptree*);
  virtual Ptree* TranslateTemplateFunction(Ptree*, Ptree*);
  virtual Ptree* TranslateMetaclassDecl(Ptree*);
  virtual Ptree* TranslateLinkageSpec(Ptree*);
  virtual Ptree* TranslateNamespaceSpec(Ptree*);
  virtual Ptree* TranslateUsing(Ptree*);
  virtual Ptree* TranslateDeclaration(Ptree*);
  virtual Ptree* TranslateStorageSpecifiers(Ptree*);
  virtual Ptree* TranslateDeclarators(Ptree*);
  virtual Ptree* TranslateArgDeclList(bool, Ptree*, Ptree*);
  virtual Ptree* TranslateInitializeArgs(PtreeDeclarator*, Ptree*);
  virtual Ptree* TranslateAssignInitializer(PtreeDeclarator*, Ptree*);
  
  virtual Ptree* TranslateFunctionImplementation(Ptree*);

  virtual Ptree* TranslateFunctionBody(Ptree*);
  virtual Ptree* TranslateBrace(Ptree*);
  virtual Ptree* TranslateBlock(Ptree*);
  //virtual Ptree* TranslateClassBody(Ptree*, Ptree*, Class*);
  
  virtual Ptree* TranslateClassSpec(Ptree*);
  //virtual Class* MakeClassMetaobject(Ptree*, Ptree*, Ptree*);
  //virtual Ptree* TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*);
  virtual Ptree* TranslateEnumSpec(Ptree*);
  
  virtual Ptree* TranslateAccessSpec(Ptree*);
  virtual Ptree* TranslateAccessDecl(Ptree*);
  virtual Ptree* TranslateUserAccessSpec(Ptree*);
  
  virtual Ptree* TranslateIf(Ptree*);
  virtual Ptree* TranslateSwitch(Ptree*);
  virtual Ptree* TranslateWhile(Ptree*);
  virtual Ptree* TranslateDo(Ptree*);
  virtual Ptree* TranslateFor(Ptree*);
  virtual Ptree* TranslateTry(Ptree*);
  virtual Ptree* TranslateBreak(Ptree*);
  virtual Ptree* TranslateContinue(Ptree*);
  virtual Ptree* TranslateReturn(Ptree*);
  virtual Ptree* TranslateGoto(Ptree*);
  virtual Ptree* TranslateCase(Ptree*);
  virtual Ptree* TranslateDefault(Ptree*);
  virtual Ptree* TranslateLabel(Ptree*);
  virtual Ptree* TranslateExprStatement(Ptree*);
  
  virtual Ptree* TranslateTypespecifier(Ptree*);
  virtual Ptree* TranslateTypeof(Ptree*, Ptree* declarations);
  
  virtual Ptree* TranslateComma(Ptree*);
  virtual Ptree* TranslateAssign(Ptree*);
  virtual Ptree* TranslateCond(Ptree*);
  virtual Ptree* TranslateInfix(Ptree*);
  virtual Ptree* TranslatePm(Ptree*);
  virtual Ptree* TranslateCast(Ptree*);
  virtual Ptree* TranslateUnary(Ptree*);
  virtual Ptree* TranslateThrow(Ptree*);
  virtual Ptree* TranslateSizeof(Ptree*);
  virtual Ptree* TranslateNew(Ptree*);
  virtual Ptree* TranslateNew3(Ptree* type);
  virtual Ptree* TranslateDelete(Ptree*);
  virtual Ptree* TranslateThis(Ptree*);
  virtual Ptree* TranslateVariable(Ptree*);
  virtual Ptree* TranslateFstyleCast(Ptree*);
  virtual Ptree* TranslateArray(Ptree*);
  virtual Ptree* TranslateFuncall(Ptree*);	// and fstyle cast
  virtual Ptree* TranslatePostfix(Ptree*);
  virtual Ptree* TranslateUserStatement(Ptree*);
  virtual Ptree* TranslateDotMember(Ptree*);
  virtual Ptree* TranslateArrowMember(Ptree*);
  virtual Ptree* TranslateParen(Ptree*);
  virtual Ptree* TranslateStaticUserStatement(Ptree*);
  
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
  Ptree* my_declaration;
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
    Ptree* body;
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
