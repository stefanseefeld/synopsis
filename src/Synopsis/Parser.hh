//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_Parser_hh_
#define Synopsis_Parser_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Token.hh>
#include <Synopsis/SymbolFactory.hh>
#include <vector>
#include <stack>

namespace Synopsis
{

//. Recursive-descent C/C++ Parser.
class Parser
{
public:
  //. RuleSet defines non-standard optional rules that can be chosen at runtime.
  enum RuleSet { CXX = 0x01, GCC = 0x02, MSVC = 0x04};

  //. Error is used to cache parse errors encountered during the execution
  //. of the parse method.
  class Error
  {
  public:
    virtual ~Error() {}
    virtual void write(std::ostream &) const = 0;
  };
  typedef std::vector<Error *> ErrorList;

  Parser(Lexer &lexer, SymbolFactory &symbols, int ruleset = CXX|GCC);
  ~Parser();

  ErrorList const &errors() const { return my_errors;}

  //. Return the origin of the given pointer
  //. (filename and line number)
  unsigned long origin(const char *, std::string &) const;

  PTree::Node *parse();

private:
  struct Tentative;
  friend struct Tentative;

  enum DeclaratorKind { ABSTRACT = 0x1, NAMED = 0x2, EITHER = 0x3};

  struct ScopeGuard;
  friend struct ScopeGuard;

  //. Issue an error message, associated with the current position
  //. in the buffer.
  bool error(std::string const & = "");
  //. Require the next token to be token, else emitting an error message.
  bool require(char token);
  bool require(Token::Type token, std::string const &name);
  //. Require node to be non-zero, else emitting an error message.
  bool require(PTree::Node *node, std::string const &name);
  bool require(PTree::Node *node, char token);

  //. Are we in a tentative parse ?
  bool is_tentative() const { return my_is_tentative;}
  //. Commit to the current tentative parse.
  void commit();

  template <typename T>
  bool declare(T *);
  bool lookup_class_name(PTree::Encoding const &);
  bool lookup_template_name(PTree::Encoding const &);
  bool lookup_namespace_name(PTree::Encoding const &);
  bool lookup_type_name(PTree::Encoding const &);

  //. :: [opt]
  PTree::Atom *opt_scope(PTree::Encoding &);

  //. identifier
  PTree::Identifier *identifier(PTree::Encoding &);

  //. namespace-name:
  //.   original-namespace-name
  //.   namespace-alias
  PTree::Identifier *namespace_name(PTree::Encoding &);

  //. class-name:
  //.   identifier
  //.   template-id
  PTree::Node *class_name(PTree::Encoding &,
			  bool is_typename,
			  bool is_template);

  //. class-or-namespace-name:
  //.   class-name
  //.   namespace-name
  PTree::Node *class_or_namespace_name(PTree::Encoding &,
				       bool is_typename,
				       bool is_template);

  //. type-name:
  //.   class-name
  //.   enum-name
  //.   typedef-name
  //.
  //. enum-name:
  //.   identifier
  //.
  //. typedef-name:
  //.   identifier
  PTree::Node *type_name(PTree::Encoding &);

  //. nested-name-specifier:
  //.   class-or-namespace-name :: nested-name-specifier [opt]
  //.   class-or-namespace-name :: template nested-name-specifier
  PTree::List *nested_name_specifier(PTree::Encoding &,
				     bool is_template = false);

  //. simple-type-specifier:
  //.   :: [opt] nested-name-specifier [opt] type-name
  //.   :: [opt] nested-name-specifier template template-id
  //.   char
  //.   wchar_t
  //.   bool
  //.   short
  //.   int
  //.   long
  //.   signed
  //.   unsigned
  //.   float
  //.   double
  //.   void
  //.   typeof-expr
  PTree::Node *simple_type_specifier(PTree::Encoding &, bool allow_user_defined);

  //. enum-specifier:
  //.   enum identifier [opt] { enumerator-list [opt] }
  //.
  //. enumerator-list:
  //.   enumerator-definition
  //.   enumerator-list , enumerator-definition
  //.
  //. enumerator-definition:
  //.   enumerator
  //.   enumerator = constant-expression
  //.
  //. enumerator:
  //.   identifier
  PTree::EnumSpec *enum_specifier(PTree::Encoding &);

  //. elaborated-type-specifier:
  //.   class-key :: [opt] nested-name-specifier [opt] identifier
  //.   class-key :: [opt] nested-name-specifier [opt] template [opt] template-id [dr68]
  //.   enum :: [opt] nested-name-specifier [opt] identifier
  //.   typename :: [opt] nested-name-specifier identifier
  //.   typename :: [opt] nested-name-specifier template [opt] template-id
  PTree::ElaboratedTypeSpec *elaborated_type_specifier(PTree::Encoding &);

  //. function-specifier:
  //.   inline
  //.   virtual
  //.   explicit
  PTree::Keyword *opt_function_specifier();

  //. class-key:
  //.   class
  //.   struct
  //.   union
  PTree::Keyword *class_key();

  //. base-clause:
  //.   : base-specifier-list
  //.
  //. base-specifier-list:
  //.   base-specifier
  //.   base-specifier-list , base-specifier
  //.
  //. base-specifier:
  //.   :: [opt] nested-name-specifier [opt] class-name
  //.   virtual access-specifier [opt] :: [opt] nested-name-specifier [opt] class-name
  //.   access-specifier virtual [opt] :: [opt] nested-name-specifier [opt] class-name
  PTree::List *base_clause();

  //. ctor-initializer:
  //.   : mem-initializer-list
  //.
  //. mem-initializer-list:
  //.   mem-initializer
  //.   mem-initializer , mem-initializer-list
  //.
  //. mem-initializer:
  //.   mem-initializer-id ( expression-list [opt] )
  //.
  //. mem-initializer-id:
  //.   :: [opt] nested-name-specifier [opt] class-name
  //.   identifier
  PTree::List *opt_ctor_initializer();

  //. member-specification:
  //.   member-declaration member-specification [opt]
  //.   access-specifier : member-specification [opt]
  //.
  //. member-declaration:
  //.   decl-specifier-seq [opt] member-declarator-list [opt] ;
  //.   function-definition ; [opt]
  //.   :: [opt] nested-name-specifier template [opt] unqualified-id ;
  //.   using-declaration
  //.   template-declaration
  //.
  //. member-declarator-list:
  //.   member-declarator
  //.   member-declarator-list , member-declarator
  //.
  //. member-declarator:
  //.   declarator pure-specifier [opt]
  //.   declarator constant-initializer [opt]
  //.   identifier [opt] : constant-expression
  //.
  //. constant-initializer:
  //.   = constant-expression
  PTree::Node *opt_member_specification();

  //. class-specifier:
  //.   class-head { member-specification [opt] }
  //.
  //. class-head:
  //.   class-key identifier [opt] base-clause [opt]
  //.   class-key nested-name-specifier identifier base-clause [opt]
  //.   class-key nested-name-specifier [opt] template-id base-clause [opt]
  PTree::ClassSpec *class_specifier(PTree::Encoding &encoding);

  //. cv-qualifier-seq:
  //.   cv-qualifier cv-qualifier-seq [opt]
  //.
  //. cv-qualifier:
  //.   const
  //.   volatile
  PTree::List *opt_cv_qualifier_seq(PTree::Encoding &);

  //. storage-class-specifier:
  //.   auto
  //.   register
  //.   static
  //.   extern
  //.   mutable
  //.   thread [GCC]
  PTree::Keyword *opt_storage_class_specifier(PTree::DeclSpec::StorageClass &);

  //. type-specifier:
  //.   simple-type-specifier
  //.   class-specifier
  //.   enum-specifier
  //.   elaborated-type-specifier
  //.   cv-qualifier
  //.   __complex__ [GCC]
  PTree::Node *type_specifier(PTree::Encoding &, bool allow_user_defined);

  //. type-specifier-seq:
  //.   type-specifier type-specifier-seq [opt]
  PTree::List *type_specifier_seq(PTree::Encoding &);

  //. linkage-specification:
  //.   extern string-literal { declaration-seq [opt] }
  //.   extern string-literal declaration
  PTree::LinkageSpec *linkage_specification();

  //. Return true if next comes a constructor-declarator.
  bool is_constructor_declarator();

  //. decl-specifier-seq:
  //.   decl-specifier-seq [opt] decl-specifier
  //.
  //. decl-specifier:
  //.   storage-class-specifier
  //.   type-specifier
  //.   function-specifier
  //.   friend
  //.   typedef
  PTree::DeclSpec *decl_specifier_seq(PTree::Encoding &);

  //. declarator-id:
  //.   id-expression
  //.   :: [opt] nested-name-specifier [opt] type-name
  PTree::Node *declarator_id(PTree::Encoding &);

  //. ptr-operator:
  //.   * cv-qualifier-seq [opt]
  //.   &
  //.   :: [opt] nested-name-specifier * cv-qualifier-seq [opt]
  PTree::List *ptr_operator(PTree::Encoding &);

  //. direct-declarator:
  //.   declarator-id
  //.   direct-declarator ( parameter-declaration-clause )
  //.     cv-qualifier-seq [opt] exception-specification [opt]
  //.   direct-declarator [ constant-expression [opt] ]
  //.   ( declarator )
  //.
  //. direct-abstract-declarator:
  //.   direct-abstract-declarator [opt] ( parameter-declaration-clause )
  //.     cv-qualifier-seq [opt] exception-specification [opt]
  //.   direct-abstract-declarator [opt] [ constant-expression [opt] ]
  //.   ( abstract-declarator )
  PTree::List *direct_declarator(PTree::Encoding &, PTree::Encoding &,
				 DeclaratorKind);

  //. declarator:
  //.   direct-declarator
  //.   ptr-operator declarator
  //.
  //. abstract-declarator:
  //.   ptr-operator abstract-declarator [opt]
  //.   direct-abstract-declarator
  PTree::Declarator *declarator(PTree::Encoding &, PTree::Encoding &,
				DeclaratorKind);

  //. initializer-clause:
  //.   assignment-expression
  //.   { initializer-list , [opt] }
  //.   { }
  PTree::Node *initializer_clause(bool constant);

  //. initializer-list:
  //.   initializer-clause
  //.   initializer-list , initializer-clause
  PTree::List *initializer_list(bool constant);

  //. init-declarator:
  //.   declarator initializer [opt]
  PTree::Declarator *init_declarator(PTree::Encoding &type);

  //. parameter-declaration-clause:
  //.   parameter-declaration-list [opt] ... [opt]
  //.   parameter-declaration-list , ...
  PTree::List *parameter_declaration_clause(PTree::Encoding &);

  //. parameter-declaration-list:
  //.   parameter-declaration
  //.   parameter-declaration-list , parameter-declaration
  PTree::List *parameter_declaration_list(PTree::Encoding &);

  //. parameter-declaration:
  //.   decl-specifier-seq declarator
  //.   decl-specifier-seq declarator = assignment-expression
  //.   decl-specifier-seq abstract-declarator [opt]
  //.   decl-specifier-seq abstract-declarator [opt] = assignment-expression
  PTree::ParameterDeclaration *parameter_declaration(PTree::Encoding &);

  //. simple-declaration:
  //.   decl-specifier-seq [opt] init-declarator-list [opt] ;
  //.
  //. init-declarator-list:
  //.   init-declarator
  //.   init-declarator-list , init-declarator
  PTree::Declaration *simple_declaration(bool function_definition_allowed);

  //. block-declaration:
  //.   simple-declaration
  //.   asm-definition
  //.   namespace-alias-definition
  //.   using-declaration
  //.   using-directive
  PTree::Declaration *block_declaration();

  //. declaration:
  //.   block-declaration
  //.   function-definition
  //.   template-declaration
  //.   explicit-instantiation
  //.   explicit-specialization
  //.   linkage-specification
  //.   namespace-definition
  PTree::Declaration *declaration();

  //. declaration-seq:
  //.   declaration
  //.   declaration-seq declaration
  PTree::List *opt_declaration_seq();

  //. namespace-definition:
  //.   named-namespace-definition
  //.   unnamed-namespace-definition
  //.
  //. named-namespace-definition:
  //.   original-namespace-definition
  //.   extension-namespace-definition
  //.
  //. original-namespace-definition:
  //.   namespace identifier { namespace-body }
  //.
  //. extension-namespace-definition:
  //.   namespace original-namespace-name { namespace-body }
  //.
  //. unnamed-namespace-definition:
  //.   namespace { namespace-body }
  //.
  //. namespace-body:
  //.   declaration-seq [opt]
  PTree::NamespaceSpec *namespace_definition();

  //. namespace-alias-definition:
  //.   namespace identifier = qualified-namespace-specifier ;
  PTree::NamespaceAlias *namespace_alias_definition();

  //. using-directive:
  //.   using namespace :: [opt] nested-name-specifier [opt] namespace-name ;
  PTree::UsingDirective *using_directive();

  //. using-declaration:
  //.   using typename [opt] :: [opt] nested-name-specifier unqualified-id ;
  //.   using :: unqualified-id ;
  PTree::UsingDeclaration *using_declaration();

  //. type-parameter:
  //.   class identifier [opt]
  //.   class identifier [opt] = type-id
  //.   typename identifier [opt]
  //.   typename identifier [opt] = type-id
  //.   template < template-parameter-list > class identifier [opt]
  //.   template < template-parameter-list > class identifier [opt] = id-expression
  PTree::Node *type_parameter(PTree::Encoding &);

  //. template-parameter:
  //.   type-parameter
  //.   parameter-declaration
  PTree::Node *template_parameter(PTree::Encoding &);

  //. template-parameter-list:
  //.   template-parameter
  //.   template-parameter-list , template-parameter
  PTree::Node *template_parameter_list(PTree::Encoding &);

  //. template-declaration:
  //.   export [opt] template < template-parameter-list > declaration
  PTree::TemplateDeclaration *template_declaration();

  //. explicit-specialization:
  //.   template < > declaration
  PTree::TemplateDeclaration *explicit_specialization();

  //. explicit-instantiation:
  //.   template declaration
  PTree::TemplateInstantiation *explicit_instantiation();

  //. operator-name:
  //.   new delete new[] delete[] + - * / % ^ & | ~ ! = < >
  //.   += -= *= /= %= ^= &= |= << >> >>= <<= == != <= >= &&
  //.   || ++ -- , ->* -> () []
  PTree::Node *operator_name(PTree::Encoding &);

  //. operator-function-id:
  //.   operator operator-name
  PTree::List *operator_function_id(PTree::Encoding &);

  //. conversion-function-id:
  //.   operator conversion-type-id
  //.
  //. conversion-type-id:
  //.   type-specifier-seq conversion-declarator [opt]
  //.
  //. conversion-declarator:
  //.   ptr-operator conversion-declarator [opt]
  PTree::List *conversion_function_id(PTree::Encoding &);

  //. template-name:
  //.   identifier
  //.   operator-function-id [GCC]
  PTree::Node *template_name(PTree::Encoding &, bool lookup = true);

  //. template-argument:
  //.   assignment-expression
  //.   type-id
  //.   id-expression
  PTree::Node *template_argument(PTree::Encoding &);

  //. template-id:
  //.   template-name < template-argument-list [opt] >
  //.
  //. template-argument-list:
  //.   template-argument
  //.   template-argument-list , template-argument
  PTree::List *template_id(PTree::Encoding &, bool is_template);

  //. condition:
  //.   expression
  //.   type-specifier-seq declarator = assignment-expression
  PTree::Node *condition();

  //. expression:
  //.   assignment-expression
  //.   expression , assignment-expression
  PTree::Node *expression();

  //. assignment-expression:
  //.   conditional-expression
  //.   logical-or-expression assignment-operator assignment-expression
  //.   throw-expression
  PTree::Node *assignment_expression();

  //. conditional-expression:
  //.   logical-or-expression
  //.   logical-or-expression ? expression : assignment-expression
  PTree::Node *conditional_expression();

  //. constant-expression:
  //.   conditional-expression
  PTree::Node *constant_expression();

  //. logical-or-expression:
  //.   logical-and-expression
  //.   logical-or-expression || logical-and-expression
  PTree::Node *logical_or_expression();

  //. logical-and-expression:
  //.   inclusive-or-expression
  //.   logical-and-expr && inclusive-or-expression
  PTree::Node *logical_and_expression();

  //. inclusive-or-expression:
  //.   exclusive-or-expression
  //.   inclusive-or-expression | exclusive-or-expression
  PTree::Node *inclusive_or_expression();

  //. exclusive-or-expression:
  //.   and-expression
  //.   exclusive-or-expression ^ and-expression
  PTree::Node *exclusive_or_expression();

  //. and-expression:
  //.   equality-expression
  //.   and-expression & equality-expression
  PTree::Node *and_expression();

  //. equality-expression:
  //.   relational-expression
  //.   equality-expression == relational-expression
  //.   equality-expression != relational-expression
  PTree::Node *equality_expression();

  //. relational-expression:
  //.   shift-expression
  //.   relational-expression < shift-expression
  //.   relational-expression > shift-expression
  //.   relational-expression <= shift-expression
  //.   relational-expression >= shift-expression
  PTree::Node *relational_expression();

  //. shift-expression:
  //.   additive-expression
  //.   shift-expression << additive-expression
  //.   shift-expression >> additive-expression
  PTree::Node *shift_expression();

  //. additive-expression:
  //.   multiplicative-expression
  //.   additive-expression + multiplicative-expression
  //.   additive-expression - multiplicative-expression
  PTree::Node *additive_expression();

  //. multiplicative-expression:
  //.   pm-expression
  //.   multiplicative-expression * pm-expression
  //.   multiplicative-expression / pm-expression
  //.   multiplicative-expression % pm-expression
  PTree::Node *multiplicative_expression();

  //. pm-expression:
  //.   cast-expression
  //.   pm-expression .* cast-expression
  //.   pm-expression ->* cast-expression
  PTree::Node *pm_expression();

  //. cast-expression:
  //.   unary-expression
  //.   ( type-id ) cast-expression
  PTree::Node *cast_expression();

  //. type-id:
  //.   type-specifier-seq abstract-declarator [opt]
  PTree::Node *type_id(PTree::Encoding&);

  //. type-id-list:
  //.   type-id
  //.   type-id-list , type-id
  PTree::List *type_id_list();

  //. unary-expression:
  //.   postfix-expression
  //.   ++ cast-expression
  //.   -- cast-expression
  //.   unary-operator cast-expression
  //.   sizeof unary-expression
  //.   sizeof ( unary-expression )
  //.   new-expression
  //.   delete-expression
  //.
  //. unary-operator:
  //.   *
  //.   &
  //.   +
  //.   -
  //.   !
  //.   ~
  PTree::Node *unary_expression();

  //. throw-expression:
  //.   throw assignment-expression [opt]
  PTree::Node *throw_expression();

  //. sizeof-expression:
  //.   sizeof unary-expression
  //.   sizeof ( type-id )
  PTree::Node *sizeof_expression();

  //. new-expression:
  //.  :: [opt] new new-placement [opt] new-type-id new-initializer [opt]
  //.  :: [opt] new new-placement [opt] ( type-id ) new-initializer [opt]
  PTree::Node *new_expression();

  //. delete-expression:
  //.   :: [opt] delete cast-expression
  //.   :: [opt] delete [ ] cast-expression
  PTree::Node *delete_expression();

  //. expression-list:
  //.   assignment-expression
  //.   expression-list, assignment-expression
  PTree::List *expression_list(bool is_const);

  PTree::List *functional_cast();

  //. pseudo-destructor-name:
  //.   :: [opt] nested-name-specifier [opt] type-name :: ~ type-name
  //.   :: [opt] nested-name-specifier template template-id :: ~ type-name
  //.   :: [opt] nested-name-specifier [opt] ~ type-name
  PTree::List *pseudo_destructor_name();

  //. postfix-expression:
  //.   primary-expression
  //.   postfix-expression [ expression ]
  //.   postfix-expression ( expression-list [opt] )
  //.   simple-type-specifier ( expression-list [opt] )
  //.   typename :: [opt] nested-name-specifier identifier
  //.     ( expression-list [opt] )
  //.   typename :: [opt] nested-name-specifier template [opt] template-id
  //.     ( expression-list [opt] )
  //.   postfix-expression . template [opt] id-expression
  //.   postfix-expression -> template [opt] id-expression
  //.   postfix-expression . pseudo-destructor-name
  //.   postfix-expression -> pseudo-destructor-name
  //.   postfix-expression ++
  //.   postfix-expression --
  //.   dynamic_cast < type-id > ( expression )
  //.   static_cast < type-id > ( expression )
  //.   reinterpret_cast < type-id > ( expression )
  //.   const_cast < type-id > ( expression )
  //.   typeid ( expression )
  //.   typeid ( type-id )
  PTree::Node *postfix_expression();

  //. primary-expression:
  //.   literal
  //.   this
  //.   ( expression )
  //.   id-expression
  PTree::Node *primary_expression();

  //. unqualified-id:
  //.   identifier
  //.   operator-function-id
  //.   conversion-function-id
  //.   ~ class-name
  //.   template-id
  PTree::Node *unqualified_id(PTree::Encoding &, bool is_template);

  //. id-expression:
  //.   unqualified-id
  //.   qualified-id
  //.
  //. qualified-id:
  //.   :: [opt] nested-name-specifier template [opt] unqualified-id
  //.   :: identifier
  //.   :: operator-function-id
  //.   :: template-id
  PTree::Node *id_expression(PTree::Encoding &);
  bool typeof_expression(PTree::Node *&);
  
  //. labeled-statement:
  //.   identifier : statement
  //.   case constant-expression : statement
  //.   default : statement
  PTree::List *labeled_statement();

  //. expression-statement:
  //.   expression [opt] ;
  PTree::ExprStatement *expression_statement();

  //. compound-statement:
  //.   { statement [opt] }
  PTree::Block *compound_statement(bool create_scope = false);

  //. selection-statement:
  //.   if ( condition ) statement
  //.   if ( condition ) statement else statement
  //.   switch ( condition ) statement
  PTree::List *selection_statement();

  //. iteration-statement:
  //.   while ( condition ) statement
  //.   do statement while ( expression ) ;
  //.   for ( for-init-statement condition [opt] ; expression [opt] )
  //.     statement
  //.
  //. for-init-statement:
  //.   expression-statement
  //.   simple-declaration
  PTree::List *iteration_statement();

  //. declaration-statement:
  //.   block-declaration
  PTree::List *declaration_statement();

  //. statement:
  //.   labeled-statement
  //.   expression-statement
  //.   compound-statement
  //.   selection-statement
  //.   iteration-statement
  //.   jump-statement
  //.   declaration-statement
  //.   try-block
  PTree::List *statement();

  //. try-block:
  //.   try compound-statement handler-seq
  //.
  //. handler-seq:
  //.   handler handler-seq [opt]
  //.
  //. handler:
  //.   catch ( exception-declaration ) compound-statement
  //.
  //. exception-declaration:
  //.   type-specifier-seq declarator
  //.   type-specifier-seq abstract-declarator
  //.   type-specifier-seq
  //.   ...
  PTree::List *try_block();
  
  //. exception-specification:
  //.   throw ( type-id-list [opt] )
  PTree::Node *opt_exception_specification();
  
  Lexer &         my_lexer;
  int             my_ruleset;
  SymbolFactory & my_symbols;
  //. used for name lookup in nested-name-specifiers
  SymbolTable::Scope *my_qualifying_scope;

  //. Only record errors if we are not parsing
  //. tentatively.
  bool            my_is_tentative;

  //. Record whether the current scope is valid.
  //. This allows the parser to continue parsing even after
  //. it was unable to enter a scope (such as in a function definition
  //. with a qualified name that wasn't declared before).
  bool            my_scope_is_valid;
  ErrorList       my_errors;
  PTree::Node *   my_comments;
  //. If true, '>' is interpreted as ther greater-than operator.
  //. If false, it marks the end of a template-id or template-parameter-list.
  bool            my_gt_is_operator;
  bool            my_in_template_decl;
  bool            my_in_functional_cast;
  bool            my_accept_default_arg;
  bool            my_in_declarator;
  //. True if we are inside a statement. In that case the number of things that
  //. can be declared is restricted.
  bool            my_in_declaration_statement;
  //. True if we are inside a constant-expression, i.e. should evaluate
  //. the associated value.
  bool            my_in_constant_expression;
  //. True if we have just seen an elaborated-type-specifier 
  //. inside a decl-specifier-seq.
  bool            my_declares_class_or_enum;
  //. True if we have just seen a class-specifier or enum-specifier
  //. inside a decl-specifier-seq.
  bool            my_defines_class_or_enum;
};

}

#endif
