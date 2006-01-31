//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/TypeAnalysis.hh>
#include "Synopsis/Parser.hh"
#include "Synopsis/Lexer.hh"
#include <Synopsis/Trace.hh>
#include <ostream>

namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

namespace Synopsis
{

std::ostream &operator<< (std::ostream &os, Token const &token)
{
  os << token.type << "('" << std::string(token.ptr, token.length) << "')";
  return os;
}

class SyntaxError : public Parser::Error
{
public:
  SyntaxError(std::string const &e, std::string const &f, unsigned long l)
    : my_error(e), my_filename(f), my_line(l) {}
  virtual void write(std::ostream &) const
  {
    std::cerr << "Syntax Error : " << my_filename << ':' << my_line 
	      << ": " << my_error << std::endl;
  }
private:
  std::string   my_error;
  std::string   my_filename;
  unsigned long my_line;
};

class UndefinedSymbol : public Parser::Error
{
public:
  UndefinedSymbol(PT::Encoding const &name,
		  std::string const &f, unsigned long l)
    : my_name(name), my_filename(f), my_line(l) {}
  virtual void write(std::ostream &os) const
  {
    os << "Undefined symbol : " << my_name.unmangled();
    if (!my_filename.empty()) os << " at " << my_filename << ':' << my_line;
    os << std::endl;
  }
private:
  PT::Encoding  my_name;
  std::string   my_filename;
  unsigned long my_line;
};

class SymbolAlreadyDefined : public Parser::Error
{
public:
  SymbolAlreadyDefined(PT::Encoding const &name, 
		       std::string const & f1, unsigned long l1,
		       std::string const & f2, unsigned long l2)
    : my_name(name), my_file1(f1), my_line1(l1), my_file2(f2), my_line2(l2) {}
  virtual void write(std::ostream &os) const
  {
    os << "Symbol already defined : definition of " << my_name.unmangled()
       << " at " << my_file1 << ':' << my_line1 << '\n'
       << "previously defined at " << my_file2 << ':' << my_line2 << std::endl;
  }
private:
  PT::Encoding  my_name;
  std::string   my_file1;
  unsigned long my_line1;
  std::string   my_file2;
  unsigned long my_line2;
};

class SymbolTypeMismatch : public Parser::Error
{
public:
  SymbolTypeMismatch(PT::Encoding const &name, PT::Encoding const &type)
    : my_name(name), my_type(type) {}
  virtual void write(std::ostream &os) const
  {
    os << "Symbol type mismatch : " << my_name.unmangled()
       << " has unexpected type " << my_type.unmangled() << std::endl;
  }
private:
  PT::Encoding my_name;
  PT::Encoding my_type;
};

namespace
{

inline bool starts_function_definition(Token::Type type)
{
  return (type == '{' ||       // function-body
	  type == ':' ||       // ctor-initializer
	  type == Token::TRY); // function-try-block
}

class TypeNameChecker : private ST::SymbolVisitor
{
public:
  TypeNameChecker() : my_result(false) {}
  bool is_type_name(ST::Symbol const *s) 
  {
    s->accept(this);
    return my_result;
  }

private:
  virtual void visit(ST::TypeName const *) { my_result = true;}
  virtual void visit(ST::TypedefName const *) { my_result = true;}
  virtual void visit(ST::ClassName const *) { my_result = true;}
  virtual void visit(ST::EnumName const *) { my_result = true;}

  bool my_result;
};

class TemplateNameChecker : private ST::SymbolVisitor
{
public:
  TemplateNameChecker() : my_symbol(0), my_scope(0) {}
  bool is_template_name(ST::Symbol const *s) 
  {
    Trace trace("TemplateNameChecker::is_template_name", Trace::PARSING);
    s->accept(this);
    return my_symbol;
  }
  ST::Symbol const *symbol() const { return my_symbol;}
  ST::Scope *scope() const { return my_scope;}

private:
  virtual void visit(ST::ClassTemplateName const *symbol)
  {
    my_symbol = symbol;
    my_scope = symbol->scope()->find_scope(symbol->ptree());
  }
  virtual void visit(ST::FunctionTemplateName const *symbol) 
  {
    my_symbol = symbol;
  }

  ST::Symbol const *my_symbol;
  ST::Scope        *my_scope;
};

template <typename T>
struct PGuard
{
  //. Remember the current state of the given member variable.
  PGuard(Parser &p, T Parser::*m) : parser(p), member(m), saved(p.*m) {}
  //. Remember the current state of the given member variable, 
  //. and set it to a new value.
  PGuard(Parser &p, T Parser::*m, T value)
    : parser(p), member(m), saved(p.*m) 
  { parser.*member = value;}
  ~PGuard() { parser.*member = saved;}
  Parser &    parser;
  T Parser::* member;
  T           saved;
};

const unsigned int max_errors = 10;

PT::List *wrap_comments(const Lexer::Comments &c)
{
  PT::List *head = 0;
  for (Lexer::Comments::const_iterator i = c.begin(); i != c.end(); ++i)
    head = PT::snoc(head, new PT::Atom(*i));
  return head;
}

}

struct Parser::Tentative
{
  Tentative(Parser &p, bool rollback = false)
    : my_parser(p), my_lexer(p.my_lexer),
      my_was_tentative(p.my_is_tentative),
      my_qualifying_scope(p.my_qualifying_scope),
      my_symbol(p.my_symbol),
      my_token_mark(my_lexer.save()),
      my_encoding(my_saved_encoding),
      my_restore_encoding(false),
      my_rollback(rollback)
  {
    Trace trace("Tentative::Tentative", Trace::PARSING);
    p.my_is_tentative = true;
  } 
  Tentative(Parser &p, PT::Encoding &e)
    : my_parser(p), my_lexer(p.my_lexer),
      my_was_tentative(p.my_is_tentative),
      my_qualifying_scope(p.my_qualifying_scope),
      my_symbol(p.my_symbol),
      my_token_mark(my_lexer.save()),
      my_saved_encoding(e),
      my_encoding(e),
      my_restore_encoding(true),
      my_rollback(false)
  {
    Trace trace("Tentative::Tentative", Trace::PARSING);
    p.my_is_tentative = true;
  } 
  ~Tentative()
  {
    Trace trace("Tentative::~Tentative", Trace::PARSING);
    if (my_rollback) rollback();
    else if (my_parser.my_is_tentative)
      my_parser.my_is_tentative = my_was_tentative;
  }

  void rollback(bool keep = false)
  {
    Trace trace("Tentative::rollback", Trace::PARSING);
    if (my_parser.my_is_tentative)
    {
      //. Only reset the is_tentative flag it we didn't commit meanwhile.
      //. (That could have happened from within another Tentative on the stack.)
      if (!keep) my_parser.my_is_tentative = my_was_tentative;
      my_parser.my_symbol = my_symbol,
      my_parser.my_qualifying_scope = my_qualifying_scope;
      my_lexer.restore(my_token_mark);
      if (my_restore_encoding)
	my_encoding = my_saved_encoding;
    }
  }
  Parser       & my_parser;
  Lexer        & my_lexer;
  bool           my_was_tentative;
  ST::Scope    * my_qualifying_scope;
  ST::Symbol const * my_symbol;
  char const   * my_token_mark;
  PT::Encoding   my_saved_encoding;
  PT::Encoding & my_encoding;
  bool           my_restore_encoding;
  bool           my_rollback;
};

struct Parser::ScopeGuard
{
  template <typename T>
  ScopeGuard(Parser &p, T const *t)
    : parser(p), noop(t == 0)
  {
    if (!noop) parser.my_symbols.enter_scope(parser.my_qualifying_scope, t);
  }
  ScopeGuard(Parser &p, ST::Scope *scope) : parser(p), noop(scope == 0)
  {
    if (!noop) parser.my_symbols.enter_scope(scope);
  }
  ScopeGuard(Parser &p,
	     PT::ClassSpec const *spec,
	     std::vector<ST::Symbol const *> const &bases)
    : parser(p), noop(false)
  {
    parser.my_symbols.enter_class(parser.my_qualifying_scope, spec, bases);
  }
  ~ScopeGuard() 
  {
    if (!noop) parser.my_symbols.leave_scope();
  }
  Parser & parser;
  bool     noop;
};

Parser::Parser(Lexer &lexer, SymbolFactory &symbols, int ruleset)
  : my_lexer(lexer),
    my_ruleset(ruleset),
    my_symbols(symbols),
    my_in_nested_name_specifier(false),
    my_qualifying_scope(0),
    my_symbol(0),
    my_is_tentative(false),
    my_comments(0),
    my_gt_is_operator(true),
    my_template_parameter_list_cursor(0),
    my_in_class(false),
    my_in_functional_cast(false),
    my_accept_default_arg(true),
    my_in_declarator(false),
    my_in_declaration_statement(false),
    my_in_constant_expression(false),
    my_declares_class_or_enum(false),
    my_defines_class_or_enum(false)
{
}

Parser::~Parser()
{
}

bool Parser::error(std::string const &error)
{
  Trace trace("Parser::error", Trace::PARSING);
  trace << error;
  if (my_is_tentative) return true;

  Token t1, t2;
  my_lexer.look_ahead(0, t1);
  my_lexer.look_ahead(1, t2);

  std::string filename;
  unsigned long line = my_lexer.origin(t1.ptr, filename);

  char const *end = t1.ptr;
  if(t2.type != '\0') end = t2.ptr + t2.length;
  else if(t1.type != '\0') end = t1.ptr + t1.length;
  std::string msg;
  if (error.empty())
    msg = "before " + std::string(t1.ptr, end - t1.ptr);
  else
    msg = error + " before " + std::string(t1.ptr, end - t1.ptr);
  my_errors.push_back(new SyntaxError(msg, filename, line));
  return my_errors.size() < max_errors;
}

inline bool Parser::require(char token)
{
  if (my_lexer.look_ahead() != token)
  {
    std::string msg = "expected ' '";
    msg[10] = token;
    error(msg);
    return false;
  }
  else
  {
    return true;
  }
}

inline bool Parser::require(Token::Type token, std::string const &name)
{
  if (my_lexer.look_ahead() != token)
  {
    error("expected '" + name + "'");
    return false;
  }
  else
  {
    return true;
  }
}

inline bool Parser::require(PTree::Node *node, std::string const &name)
{
  if (!node)
  {
    error("expected " + name);
    return false;
  }
  else
  {
    return true;
  }
}

inline bool Parser::require(PTree::Node *node, char token)
{
  if (!node)
  {
    std::string msg = "expected ' '";
    msg[10] = token;
    error(msg);
    return false;
  }
  else
  {
    return true;
  }
}

inline void Parser::commit()
{
  Trace trace("Parser::commit", Trace::PARSING);
  my_is_tentative = false;
}

template <typename T>
inline bool Parser::declare(T *t)
{
  // If the scope isn't valid, do nothing.
  try
  {
    my_symbols.declare(my_qualifying_scope, t);
  }
  catch (ST::Undefined const &e)
  {
    std::string filename;
    unsigned long line = 0;
    if (e.ptree)
      line = my_lexer.origin(e.ptree->begin(), filename);

    my_errors.push_back(new UndefinedSymbol(e.name, filename, line));
  }
  catch (ST::MultiplyDefined const &e)
  {
    std::string file1;
    unsigned long line1 = my_lexer.origin(e.declaration->begin(), file1);
    std::string file2;
    unsigned long line2 = my_lexer.origin(e.original->begin(), file2);
    my_errors.push_back(new SymbolAlreadyDefined(e.name,
						 file1, line1,
						 file2, line2));
  }
  catch (ST::TypeError const &e)
  {
    my_errors.push_back(new SymbolTypeMismatch(e.name, e.type));
  }
  return my_errors.size() < max_errors;
}

bool Parser::lookup_class_name(PT::Encoding const &name)
{
  Trace trace("Parser::lookup_class_name", Trace::PARSING);
  trace << name;
  // If the next token is '::' and we are in a dependent scope
  // accept the identifier without lookup.
  if (my_in_nested_name_specifier &&
      my_lexer.look_ahead() == Token::Scope &&
      my_qualifying_scope == ST::DEPENDENT_SCOPE) return true;

  ST::SymbolSet symbols;
  // If we are looking at a partial template specialization we have to
  // explicitely look up types in the template parameter scope, too.
  if (my_template_parameter_list_seq.size())
    symbols = my_symbols.lookup_template_parameter(name);
  if (symbols.empty())
  {
    if (my_qualifying_scope)
      symbols = my_qualifying_scope->qualified_lookup(name, ST::Scope::TYPE);
    else
      symbols = my_symbols.current_scope()->unqualified_lookup(name, ST::Scope::TYPE);
    if (symbols.empty()) return false; // no such class-name
  }

  ST::ClassName const *class_name =
    dynamic_cast<ST::ClassName const *>(*symbols.begin());
  // If we don't have a class-name yet, try a typedef
  if (!class_name)
  {
    ST::TypedefName const *type = 
      dynamic_cast<ST::TypedefName const *>(*symbols.begin());
    if (type)
    {
      ST::Symbol const *symbol = resolve_type(type);
      if (symbol) class_name = dynamic_cast<ST::ClassName const *>(symbol);
    }
  }
  // If we still don't have a class-name yet, try a (dependent) typename.
  if (!class_name)
  {
    ST::TypeName const *type = dynamic_cast<ST::TypeName const *>(*symbols.begin());
    // If that worked, mark the qualifying scope as dependent.
    if (type)
    {
      my_symbol = type;
      if (my_in_nested_name_specifier)
	my_qualifying_scope = ST::DEPENDENT_SCOPE;
      return true;
    }
  }
  // If none of this worked, we give up.
  if (!class_name) return false;     // name does not refer to a class-name

  my_symbol = class_name;
  if (my_in_nested_name_specifier)
    my_qualifying_scope = class_name->scope()->find_scope(class_name->ptree());
  return true;
}

bool Parser::lookup_template_name(PT::Encoding const &name)
{
  Trace trace("Parser::lookup_template_name", Trace::PARSING);
  ST::SymbolSet symbols;
  if (my_qualifying_scope)
    symbols = my_qualifying_scope->qualified_lookup(name, ST::Scope::DEFAULT);
  else
    symbols = my_symbols.current_scope()->unqualified_lookup(name, ST::Scope::DEFAULT);
  if (symbols.empty()) return false; // no such name
  TemplateNameChecker checker;
  for (ST::SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
    if (checker.is_template_name(*i))
    {
      my_symbol = checker.symbol();
      if (my_in_nested_name_specifier) my_qualifying_scope = checker.scope();
      return true;
    }
  error(name.unmangled() + " is not a known template.");
  return false;
}

bool Parser::lookup_namespace_name(PT::Encoding const &name)
{
  Trace trace("Parser::lookup_namespace_name", Trace::PARSING);
  ST::SymbolSet symbols;
  if (my_qualifying_scope)
    symbols = my_qualifying_scope->qualified_lookup(name, ST::Scope::DEFAULT);
  else
    symbols = my_symbols.current_scope()->unqualified_lookup(name, ST::Scope::DEFAULT);
  if (symbols.size() != 1) return false; // no such namespace-name

  ST::NamespaceName const *namespace_name =
    dynamic_cast<ST::NamespaceName const *>(*symbols.begin());
  if (!namespace_name) return false;     // name does not refer to a namespace-name

  my_symbol = namespace_name;
  if (my_in_nested_name_specifier)
    my_qualifying_scope = 
      namespace_name->scope()->find_scope(namespace_name->ptree());

  return true;
}

bool Parser::lookup_type_name(PT::Encoding const &name)
{
  Trace trace("Parser::lookup_type_name", Trace::PARSING);
  ST::SymbolSet symbols;
  if (my_qualifying_scope)
    symbols = my_qualifying_scope->qualified_lookup(name, ST::Scope::TYPE);
  else
  {
    if (my_template_parameter_list_seq.size())
      symbols = my_symbols.lookup_template_parameter(name);
    ST::SymbolSet more =
      my_symbols.current_scope()->unqualified_lookup(name, ST::Scope::TYPE);
    symbols.insert(more.begin(), more.end());
  }
  if (symbols.size() != 1) return false;

  TypeNameChecker checker;
  if (symbols.size() != 1 || !checker.is_type_name(*symbols.begin())) return false;
  my_symbol = *symbols.begin();
  return true;
}

unsigned long Parser::origin(const char *ptr,
			     std::string &filename) const
{
  return my_lexer.origin(ptr, filename);
}

PT::Node *Parser::parse()
{
  Trace trace("Parser::parse", Trace::PARSING);
  PT::List *declarations = opt_declaration_seq();
  // TODO: Retrieve trailing comments
  //       We first need to figure out all the right places to
  //       retrieve comments at (declarators, ...), or else
  //       they will just accumulate.
  return declarations;

  PT::List *comments = wrap_comments(my_lexer.get_comments());
  if (comments)
  {
    // Use zero-length CommentedAtom as special marker.
    // Should we define a 'PT::Comment' atom for those comments that
    // don't clash with the grammar ? At least that seems less hackish than this:
    PT::Node *c = new PT::CommentedAtom(comments->begin(), 0, comments);
    declarations = PT::snoc(declarations, c);
  }
  return declarations;
}

// :: [opt]
PT::Atom *Parser::opt_scope(PT::Encoding &encoding)
{
  if (my_lexer.look_ahead() == Token::Scope)
  {
    my_qualifying_scope = my_symbols.current_scope()->global_scope();
    encoding.global_scope();
    return new PT::Atom(my_lexer.get_token());
  }
  else
  {
    my_qualifying_scope = 0;
    return 0;
  }
}

// identifier
PT::Identifier *Parser::identifier(PT::Encoding &encoding)
{
  Token token = my_lexer.get_token();
  if (token.type != Token::Identifier)
  {
    error ("expected identifier");
    return 0;
  }
  PT::Identifier *identifier = new PT::Identifier(token);
  encoding.simple_name(identifier);
  return identifier;
}

// namespace-name:
//   original-namespace-name
//   namespace-alias
PT::Identifier *Parser::namespace_name(PT::Encoding &encoding)
{
  Trace trace("Parser::namespace_name", Trace::PARSING);
  PT::Encoding name;
  PT::Identifier *id = identifier(name);
  if (!id || !lookup_namespace_name(name)) return 0;
  encoding.simple_name(id);
  return id;
}

// class-name:
//   identifier
//   template-id
PT::Node *Parser::class_name(PT::Encoding &encoding,
			     bool is_typename, bool is_template,
			     bool in_base_clause)
{
  Trace trace("Parser::class_name", Trace::PARSING);
  if (!require(Token::Identifier, "identifier")) return 0;
  else if (my_lexer.look_ahead(1) == '<')
    return template_id(encoding, is_template);
  else
  {
    PT::Encoding name;
    PT::Identifier *id = identifier(name);
    // If the 'typename' keyword was seen before, don't look up
    // the class name.
    // If we are in a base-clause, we know this is a class-name,
    // even if we can't look it up (e.g. because it is a dependent
    // name. However, since we do want the symbol associated with
    // the base class, we attempt a lookup anyways.
    if (is_typename || lookup_class_name(name) || in_base_clause)
    {
      encoding.simple_name(id);
      return id;
    }
    else return 0;
  }
}

// class-or-namespace-name:
//   class-name
//   namespace-name
PT::Node *Parser::class_or_namespace_name(PT::Encoding &encoding,
					  bool is_typename,
					  bool is_template)
{
  Trace trace("Parser::class_or_namespace_name", Trace::PARSING);
  // If we have seen the 'template' keyword, or if we are currently in a class scope,
  // we know this has to be a class name.
  if (is_template || 
      (my_qualifying_scope && dynamic_cast<ST::Class *>(my_qualifying_scope)))
    // If we have seen the 'template' keyword, we are (probably) in a dependent
    // name, and thus don't look up the name to make sure it refers to a class.
    // As this is only called during the evaluation of a nested-name-specifier,
    // remember the scope for subsequent lookups.
    return class_name(encoding, is_typename, is_template);

  Tentative tentative(*this);
  PT::Node *name = class_name(encoding, is_typename, is_template);
  if (name) return name;
  tentative.rollback();
  return namespace_name(encoding);
}

// nested-name-specifier:
//   class-or-namespace-name :: nested-name-specifier [opt]
//   class-or-namespace-name :: template nested-name-specifier
PT::List *Parser::nested_name_specifier(PT::Encoding &encoding,
					bool is_template)
{
  Trace trace("Parser::nested_name_specifier", Trace::PARSING);

  // If the next token is not an identifier or the following
  // neither '::' nor '<' this is not a nested-name-specifier.
  if (my_lexer.look_ahead() != Token::Identifier) return 0;

  // If the second-next token is a scope, this definitely is a 
  // nested-name-specifier.
  if (my_lexer.look_ahead(1) == Token::Scope);// commit();

  // On the other hand, if it is neither a scope, nor the beginning of a 
  // template-id, this is definitely not a nested-name-specifier.
  else if (my_lexer.look_ahead(1) != '<') return 0;

  PGuard<bool> guard(*this, &Parser::my_in_nested_name_specifier, true);

  PT::Node *name = class_or_namespace_name(encoding, false, is_template);

  PT::List *qname = 0;
  // if we found a class-name, but the class was forward-declared only,
  // my_qualifying_scope will be reset to 0.
  if (!name) return 0;
//   if (!name || !my_qualifying_scope) return 0;
  else qname = PT::cons(name);
  if (!require(Token::Scope, "::")) return 0;

  if (!encoding.is_qualified()) encoding.qualified(1);

  qname = PT::snoc(qname, new PT::Atom(my_lexer.get_token()));
  Tentative tentative(*this, encoding);
  PT::Keyword *template_ = 0;
  if (my_lexer.look_ahead() == Token::TEMPLATE)
    template_ = new PT::Kwd::Template(my_lexer.get_token());

  PT::List *rest = nested_name_specifier(encoding, template_);
  if (rest)
  {
    if (template_) qname = PT::snoc(qname, template_);
    return PT::snoc(qname, rest);
  }
  else
  {
    tentative.rollback();
    return qname;
  }
}

// unqualified-id:
//   identifier
//   operator-function-id
//   conversion-function-id
//   ~ class-name
//   template-id
PT::Node *Parser::unqualified_id(PT::Encoding &encoding,
				 bool is_template)
{
  Trace trace("Parser::unqualified_id", Trace::PARSING);
  Token::Type type = my_lexer.look_ahead();
  switch (type)
  {
    case Token::Identifier:
      if (my_lexer.look_ahead(1) == '<')
      {
	Tentative tentative(*this, encoding);
	PT::List *id = template_id(encoding, is_template);
	if (id) return id;
	else tentative.rollback();
      }
      return identifier(encoding);
    case '~':
    {
      if(my_lexer.look_ahead(1) != Token::Identifier) return 0;
      PT::Node *comp = new PT::Atom(my_lexer.get_token());
      PT::Identifier *name = new PT::Identifier(my_lexer.get_token());
      encoding.destructor(name);
      return PT::list(comp, name);
    }
    case Token::OPERATOR:
    {
      Tentative tentative(*this, encoding);
      PT::Node *operator_ = new PT::Kwd::Operator(my_lexer.get_token());
      PT::Node *name = template_id(encoding, is_template);
      if (name) return PT::list(operator_, name);
      tentative.rollback();
      name = operator_function_id(encoding);
      if (name) return name;
      tentative.rollback();
      return conversion_function_id(encoding);
    }
    default: return 0;
  }
}

// type-name:
//   class-name
//   enum-name
//   typedef-name
//
// enum-name:
//   identifier
//
// typedef-name:
//   identifier
PT::Node *Parser::type_name(PT::Encoding &encoding)
{
  Trace trace("Parser::type_name", Trace::PARSING);
  {
    Tentative tentative(*this, encoding);
    PT::Node *name = class_name(encoding, false, false);
    if (name) return name;
    tentative.rollback();
  }
  PT::Encoding name;
  PT::Identifier *id = identifier(name);
  if (!id) return 0;

  if (lookup_type_name(name))
  {
    encoding.simple_name(id);
    return id;
  }
  else
  {
    error(std::string(id->position(), id->length()) + " is not a type");
    return 0;
  }
}

// simple-type-specifier:
//   :: [opt] nested-name-specifier [opt] type-name
//   :: [opt] nested-name-specifier template template-id
//   char
//   wchar_t
//   bool
//   short
//   int
//   long
//   signed
//   unsigned
//   float
//   double
//   void
//   typeof-expr
PT::Node *Parser::simple_type_specifier(PT::Encoding &encoding,
					bool allow_user_defined)
{
  Trace trace("Parser::simple_type_specifier", Trace::PARSING);

  PT::Node *scope = 0;
  switch (my_lexer.look_ahead())
  {
    case Token::CHAR:
      encoding.append('c');
      return new PT::Kwd::Char(my_lexer.get_token());
    case Token::WCHAR:
      encoding.append('w');
      return new PT::Kwd::WChar(my_lexer.get_token());
    case Token::BOOLEAN:
      encoding.append('b');
      return new PT::Kwd::Bool(my_lexer.get_token());
    case Token::SHORT:
      encoding.append('s');
      return new PT::Kwd::Short(my_lexer.get_token());
    case Token::INT:
      encoding.append('i');
      return new PT::Kwd::Int(my_lexer.get_token());
    case Token::LONG:
      encoding.append('l');
      return new PT::Kwd::Long(my_lexer.get_token());
    case Token::SIGNED:
      encoding.append('S');
      return new PT::Kwd::Signed(my_lexer.get_token());
    case Token::UNSIGNED:
      encoding.append('U');
      return new PT::Kwd::Unsigned(my_lexer.get_token());
    case Token::FLOAT:
      encoding.append('f');
      return new PT::Kwd::Float(my_lexer.get_token());
    case Token::DOUBLE:
      encoding.append('d');
      return new PT::Kwd::Double(my_lexer.get_token());
    case Token::VOID:
      encoding.append('v');
      return new PT::Kwd::Void(my_lexer.get_token());
    case Token::TYPEOF:
      return typeof_expression(encoding);
    case Token::Scope:
      encoding.global_scope();
      scope = new PT::Atom(my_lexer.get_token());
      my_qualifying_scope = my_symbols.current_scope()->global_scope();
      break;
    default:
      my_qualifying_scope = 0;
      break;
  }

  // It must be a user-defined type.
  if (!allow_user_defined) return 0;
  Tentative tentative(*this, encoding);
  PT::List *qname = nested_name_specifier(encoding);
  if (qname && my_lexer.look_ahead() == Token::TEMPLATE)
  {
    if (scope) qname = PT::cons(scope, qname);
    qname = PT::snoc(qname, new PT::Kwd::Template(my_lexer.get_token()));
    PT::Node *id = template_id(encoding, true);
    if (!require(id, "template-id")) return 0;
    else return new PT::Name(PT::snoc(qname, id), encoding);
  }
  else if (!qname) tentative.rollback();
  
  PT::Node *type_name_ = type_name(encoding);
  if (!require(type_name_, "type-name")) return 0;
  qname = PT::snoc(qname, type_name_);
  if (scope) qname = PT::cons(scope, qname);
  return new PT::Name(qname, encoding);
}

// enum-specifier:
//   enum identifier [opt] { enumerator-list [opt] }
//
// enumerator-list:
//   enumerator-definition
//   enumerator-list , enumerator-definition
//
// enumerator-definition:
//   enumerator
//   enumerator = constant-expression
//
// enumerator:
//   identifier
PT::EnumSpec *Parser::enum_specifier(PT::Encoding &encoding)
{
  Trace trace("Parser::enum_specifier", Trace::PARSING);

  if(my_lexer.look_ahead() != Token::ENUM) return 0;
  Token enum_ = my_lexer.get_token();
  PT::EnumSpec *spec = 0;
  Token ob = my_lexer.get_token();
  if(ob.type == Token::Identifier)
  {
    PT::Identifier *name = new PT::Identifier(ob);
    encoding.simple_name(name);
    spec = new PT::EnumSpec(encoding, new PT::Kwd::Enum(enum_), name);
    if (!require('{')) return 0;
    ob = my_lexer.get_token();
  }
  else // anonymous enum
  {
    encoding.anonymous();
    spec = new PT::EnumSpec(encoding, new PT::Kwd::Enum(enum_), 0);
  }
  if(ob.type != '{') return 0;
  PT::List *enumerators = 0;
  while (my_lexer.look_ahead() != '}')
  {
    if(!require(Token::Identifier, "identifier")) return 0;
    Token token = my_lexer.get_token();
    PT::Node *name;
    PT::List *comments = wrap_comments(my_lexer.get_comments());
    if(my_lexer.look_ahead() != '=')
    {
      name = new PT::Identifier(token, comments);
    }
    else
    {
      Token token2 = my_lexer.get_token();
      PT::Node *expression = constant_expression();
      if (!require(expression, "constant-expression")) return 0;
      name = PT::list(new PT::Identifier(token, comments),
		      new PT::Atom(token2),
		      expression);
    }

    if(my_lexer.look_ahead() != ',')
    {
      enumerators = PT::snoc(enumerators, name);
      break;
    }
    else
    {
      enumerators = PT::conc(enumerators,
			     PT::list(name, new PT::Atom(my_lexer.get_token())));
    }
  }
  if(!require('}')) return 0;
  Token cb = my_lexer.get_token();
  PT::List *comments = wrap_comments(my_lexer.get_comments());
  spec = PT::snoc(spec, new PT::Atom(ob));
  spec = PT::snoc(spec, enumerators);
  spec = PT::snoc(spec, new PT::CommentedAtom(cb, comments));
  declare(spec);
  return spec;
}

// elaborated-type-specifier:
//   class-key :: [opt] nested-name-specifier [opt] identifier
//   class-key :: [opt] nested-name-specifier [opt] template [opt] template-id [dr68]
//   enum :: [opt] nested-name-specifier [opt] identifier
//   typename :: [opt] nested-name-specifier identifier
//   typename :: [opt] nested-name-specifier template [opt] template-id
PT::ElaboratedTypeSpec *Parser::elaborated_type_specifier(PT::Encoding &encoding)
{
  Trace trace("Parser::elaborated_type_specifier", Trace::PARSING);
  PT::Keyword *key;
  Token::Type type = my_lexer.look_ahead();
  switch (type)
  {
    case Token::ENUM:
      key = new PT::Kwd::Enum(my_lexer.get_token());
      break;
    case Token::TYPENAME:
      key = new PT::Kwd::Typename(my_lexer.get_token());
      break;
    case Token::CLASS:
    case Token::STRUCT:
    case Token::UNION:
      key = class_key();
      break;
    default:
      return 0;
  }

  PT::Encoding nested_encoding;
  PT::Atom *scope = opt_scope(nested_encoding);
  Tentative tentative(*this, nested_encoding);
  PT::List *name = nested_name_specifier(nested_encoding);
  if (type == Token::TYPENAME && !require(name, "nested-name-specifier"))
    return 0;
  else if (!name) tentative.rollback();
  if (scope)
  {
    encoding.global_scope();
    if (name) name = PT::cons(scope, PT::cons(name));
    else name = PT::cons(scope);
  }
  if (name) encoding.append(nested_encoding);
  if (type != Token::ENUM)
  {
    // If the next token is 'template', a template-id must follow.
    if (my_lexer.look_ahead() == Token::TEMPLATE)
    {
      name = PT::snoc(name, new PT::Kwd::Template(my_lexer.get_token()));
      PT::Node *template_id_ = template_id(encoding, true);
      if (!require(template_id_, "template-id")) return 0;
      name = PT::snoc(name, template_id_);
      return new PT::ElaboratedTypeSpec(key, new PT::Name(name, encoding));
    }
    // Try a template-id and roll back if it fails.
    else
    {
      Tentative tentative(*this, encoding);
      PT::Node *template_id_ = template_id(encoding, false);
      if (template_id_)
      {
	name = PT::snoc(name, template_id_);
	return new PT::ElaboratedTypeSpec(key, new PT::Name(name, encoding));
      }
      else tentative.rollback();
    }
  }
  PT::Identifier *id = identifier(encoding);
  if (!require(id, "identifier")) return 0;
  // TODO: look up id in the proper scope and make sure it has the appropriate
  //       type (class, enum, or class template), depending on the current context.
  name = PT::snoc(name, id);
  return new PT::ElaboratedTypeSpec(key, id);
}

// function-specifier:
//   inline
//   virtual
//   explicit
PT::Keyword *Parser::opt_function_specifier()
{
  Trace trace("Parser::opt_function_specifier", Trace::PARSING);
  switch (my_lexer.look_ahead())
  {
    case Token::INLINE: return new PT::Kwd::Inline(my_lexer.get_token());
    case Token::VIRTUAL: return new PT::Kwd::Virtual(my_lexer.get_token());
    case Token::EXPLICIT: return new PT::Kwd::Explicit(my_lexer.get_token());
    default: return 0;
  }
}

// class-key:
//   class
//   struct
//   union
PT::Keyword *Parser::class_key()
{
  Trace trace("Parser::class_key", Trace::PARSING);
  switch (my_lexer.look_ahead())
  {
    case Token::CLASS: return new PT::Kwd::Class(my_lexer.get_token());
    case Token::STRUCT: return new PT::Kwd::Struct(my_lexer.get_token());
    case Token::UNION: return new PT::Kwd::Union(my_lexer.get_token());
    default: return 0;
  }
}

// base-clause:
//   : base-specifier-list
//
// base-specifier-list:
//   base-specifier
//   base-specifier-list , base-specifier
//
// base-specifier:
//   :: [opt] nested-name-specifier [opt] class-name
//   virtual access-specifier [opt] :: [opt] nested-name-specifier [opt] class-name
//   access-specifier virtual [opt] :: [opt] nested-name-specifier [opt] class-name
//
// The grammar above is wrong, as it misses the optional 'template' keyword before
// 'class-name'.
PTree::List *Parser::base_clause(std::vector<SymbolTable::Symbol const *> &symbols)
{
  Trace trace("Parser::base_clause", Trace::PARSING);
  assert(my_lexer.look_ahead() == ':');
  PT::List *bases = PT::cons(new PT::Atom(my_lexer.get_token()));
  while(true) // for all base-specifiers
  {
    PT::List *super = 0;
    bool virtual_ = false;
    bool access_specifier = false;
    bool done = false;
    while (!done) // for all 'virtual' and access-specifiers
    {
      switch (my_lexer.look_ahead())
      {
        case Token::VIRTUAL:
	  if (virtual_)
	  {
	    error("'virtual' specified more than once in base-specifier");
	    return 0;
	  }
	  super = PT::snoc(super, new PT::Kwd::Virtual(my_lexer.get_token()));
	  virtual_ = true;
	  break;
        case Token::PUBLIC:
	  if (access_specifier)
	  {
	    error("more than one access specifier in base-specifier");
	    return 0;
	  }
	  super = PT::snoc(super, new PT::Kwd::Public(my_lexer.get_token()));
	  access_specifier = true;
	  break;
        case Token::PROTECTED:
	  if (access_specifier)
	  {
	    error("more than one access specifier in base-specifier");
	    return 0;
	  }
	  super = PT::snoc(super, new PT::Kwd::Protected(my_lexer.get_token()));
	  access_specifier = true;
	  break;
        case Token::PRIVATE:
	  if (access_specifier)
	  {
	    error("more than one access specifier in base-specifier");
	    return 0;
	  }
	  super = PT::snoc(super, new PT::Kwd::Private(my_lexer.get_token()));
	  access_specifier = true;
	  break;
        default:
	  done = true;
	  break;
      }
    }
    PT::Encoding encoding;
    PT::Atom *scope = opt_scope(encoding);
    Tentative tentative(*this, encoding);
    PT::List *nested_name = nested_name_specifier(encoding);
    if (!nested_name) tentative.rollback();
    PT::Keyword *template_ = 0;
    if (my_lexer.look_ahead() == Token::TEMPLATE)
      template_ = new PT::Kwd::Template(my_lexer.get_token());
    // [temp.res]
    //
    // The keyword `typename' is not permitted in a base-specifier or
    // mem-initializer; in these contexts a qualified name that
    // depends on a template-parameter is implicitly assumed to be a
    // type name.
    PT::Node *class_ = class_name(encoding, false, template_, true);
    if (!require(class_, "class-name")) return 0;
    PT::Node *base = class_;
    symbols.push_back(my_symbol);
    if (scope || nested_name || !class_->is_atom())
    {
      PT::List *tmp = PT::cons(class_);
      if (template_) tmp = PT::cons(template_, tmp);
      if (nested_name) tmp = PT::conc(nested_name, tmp);
      if (scope) tmp = PT::cons(scope, tmp);
      base = new PT::Name(tmp, encoding);
    }
    bases = PT::snoc(bases, PT::snoc(super, base));
    if(my_lexer.look_ahead() != ',') break;
    else bases = PT::snoc(bases, new PT::Atom(my_lexer.get_token()));
  }
  return bases;
}

// ctor-initializer:
//   : mem-initializer-list
//
// mem-initializer-list:
//   mem-initializer
//   mem-initializer , mem-initializer-list
//
// mem-initializer:
//   mem-initializer-id ( expression-list [opt] )
//
// mem-initializer-id:
//   :: [opt] nested-name-specifier [opt] class-name
//   identifier
PTree::List *Parser::opt_ctor_initializer()
{
  Trace trace("Parser::opt_ctor_initializer", Trace::PARSING);
  if (my_lexer.look_ahead() != ':') return 0;

  PT::Atom *colon = new PT::Atom(my_lexer.get_token());
  PT::List *members = 0;
  while (true)
  {
    PT::List *member = 0;
    if (my_lexer.look_ahead() == Token::Identifier &&
	my_lexer.look_ahead(1) == '(')
    {
      PT::Identifier *id = new PT::Identifier(my_lexer.get_token());
      // TODO: validate
      member = PT::cons(id);
    }
    else
    {
      PT::Encoding encoding;
      PT::Atom *scope = opt_scope(encoding);
      Tentative tentative(*this, encoding);
      PT::List *nested_name = nested_name_specifier(encoding, false);
      if (!nested_name) tentative.rollback();
      PT::Node *class_ = class_name(encoding, true, false);
      if (!require(class_, "class-name")) return 0;
      PT::List *name = PT::cons(class_);
      if (nested_name) name = PT::conc(nested_name, name);
      if (scope) name = PT::cons(scope, name);
      member = PT::cons(new PT::Name(name, encoding));
    }
    if (!require('(')) return 0;
    member = PT::snoc(member, new PT::Atom(my_lexer.get_token()));
    if (my_lexer.look_ahead() != ')')
    {
      PT::List *expr = expression_list(false);
      if (!require(expr, "expression-list")) return 0;
      member = PT::snoc(member, expr);
    }
    if (!require(')')) return 0;
    member = PT::snoc(member, new PT::Atom(my_lexer.get_token()));
    members = PT::snoc(members, member);
    if (my_lexer.look_ahead() != ',') break;
    members = PT::snoc(members, new PT::Atom(my_lexer.get_token()));
  }
  return PT::cons(colon, members);
}

// member-specification:
//   member-declaration member-specification [opt]
//   access-specifier : member-specification [opt]
//
// member-declaration:
//   decl-specifier-seq [opt] member-declarator-list [opt] ;
//   function-definition ; [opt]
//   :: [opt] nested-name-specifier template [opt] unqualified-id ;
//   using-declaration
//   template-declaration
//
// member-declarator-list:
//   member-declarator
//   member-declarator-list , member-declarator
//
// member-declarator:
//   declarator pure-specifier [opt]
//   declarator constant-initializer [opt]
//   identifier [opt] : constant-expression
//
// constant-initializer:
//   = constant-expression
PT::Node *Parser::opt_member_specification()
{
  Trace trace("Parser::opt_member_specification", Trace::PARSING);

  PT::List *members = 0;
  while (true)
  {
    PGuard<ST::Scope *> pguard2(*this, &Parser::my_qualifying_scope, 0);
    switch (my_lexer.look_ahead())
    {
      case '}':
        return members;
      case Token::PUBLIC:
      {
	PT::Node *access = new PT::Kwd::Public(my_lexer.get_token());
	if (!require(':')) return 0;
	access = PT::snoc(PT::cons(access), new PT::Atom(my_lexer.get_token()));
	members = PT::snoc(members, access);
	break;
      }
      case Token::PROTECTED:
      {
	PT::Node *access = new PT::Kwd::Protected(my_lexer.get_token());
	if (!require(':')) return 0;
	access = PT::snoc(PT::cons(access), new PT::Atom(my_lexer.get_token()));
	members = PT::snoc(members, access);
	break;
      }
      case Token::PRIVATE:
      {
	PT::Node *access = new PT::Kwd::Private(my_lexer.get_token());
	if (!require(':')) return 0;
	access = PT::snoc(PT::cons(access), new PT::Atom(my_lexer.get_token()));
	members = PT::snoc(members, access);
	break;
      }
      case Token::USING:
      {
	PT::UsingDeclaration *decl = using_declaration();
	if (!require(decl, "using-declaration")) return 0;
	else members = PT::snoc(members, decl);
	break;
      }
      case Token::TEMPLATE:
      {
	PT::List *decl = template_declaration();
	if (!require(decl, "template-declaration")) return 0;
	else members = PT::snoc(members, decl);
	break;
      }
      default:
      {
	PT::Encoding encoding;
	PT::DeclSpec *spec = decl_specifier_seq(encoding);
	if (my_lexer.look_ahead() == ';')
	{
	  if (!spec || spec->type().empty())
	  {
	    error("expected type");
	    return 0;
	  }
	  // nested class-specifier ?
	  Token semic = my_lexer.get_token();
	  PT::SimpleDeclaration *decl =
	    new PT::SimpleDeclaration(spec, PT::list(0, new PT::Atom(semic)));
	  if (spec->is_typedef())
	    my_symbols.declare_typedef(decl, static_cast<ST::TypeName const *>(my_symbol));
	  else declare(decl);
	  members = PT::snoc(members, decl);
	}
	else
	{
	  PT::List *comments = wrap_comments(my_lexer.get_comments());

	  PT::List *declarators = 0;
	  while (my_lexer.look_ahead() != ';')
	  {
	    Token::Type token = my_lexer.look_ahead();
	    if (token == ':' || 
		(token == Token::Identifier && my_lexer.look_ahead(1) == ':'))
	    {
	      // the decl-specifier-seq may be ommitted only for functions (C) or
	      // constructors, destructors, and conversion operators (C++).
	      if (!spec || spec->type().empty())
	      {
		error("expected type");
		return 0;
	      }
	      // bitfield declaration
	      PT::Encoding encoding;
	      PT::Identifier *id = token != ':' ? identifier(encoding) : 0;
	      PT::Atom *colon = new PT::Atom(my_lexer.get_token());
	      PT::Node *width = constant_expression();
	      declarators = PT::snoc(declarators, PT::list(id, colon, width));
	    }
	    else
	    {
	      PT::Encoding type = encoding;
	      PT::Encoding dummy_name;
	      PT::Declarator *decl = declarator(type, dummy_name, NAMED);
	      if (!require(decl, "declarator")) return 0;
	      // the decl-specifier-seq may be ommitted only for functions (C) or
	      // constructors, destructors, and conversion operators (C++).
	      else if (!type.is_function() && (!spec || spec->type().empty()))
	      {
		error("expected type");
		return 0;
	      }
	      PT::Node *initializer = 0;
	      if (my_lexer.look_ahead() == '=')
	      {
		Token eq = my_lexer.get_token();
		if (type.is_function())
		{
		  // pure-specifier
		  Token zero = my_lexer.get_token();
		  if (zero.type != Token::Constant || 
		      zero.length != 1 || 
		      *zero.ptr != '0')
		  {
		    error("invalid pure specifier (only '= 0' is allowed)");
		    return 0;
		  }
		  initializer = PT::cons(new PT::Atom(eq),
					 PT::cons(new PT::Atom(zero)));
		}
		else
		{
		  // constant-initializer
		  if (my_lexer.look_ahead() == '{')
		  {
		    error("a brace-enclosed initializer is not allowed here");
		    return 0;
		  }
		  initializer = PT::cons(new PT::Atom(eq),
					 PT::cons(constant_expression()));
		}
	      }
	      if (initializer) decl = PT::snoc(decl, initializer);

	      if (type.is_function() &&
		  !declarators &&
		  starts_function_definition(my_lexer.look_ahead()))
	      {
		// function-definition
		PT::FunctionDefinition *func = 
		  new PT::FunctionDefinition(spec, PT::cons(decl));
		{
		  // FIXME: For now skip the whole function body (incl. ctor-initializer).
		  //        Enhance the Lexer so it can be nested, e.g. to
		  //        scan chunks of the buffer such as member function bodies.
		  Token op = my_lexer.get_token();
		  size_t nesting = op.type == '{' ? 1 : 0;
		  Token cp = skip_statement(nesting);
		  if (cp.type == Token::BadToken)
		  {
		    error("encountering EOF while parsing function body");
		    return 0;
		  }
		  // Need to remember the chunk [op, cp] so we can parse it once
		  // the class body is completed.
// 		  ScopeGuard scope_guard(*this, func);
// 		  PT::Block *body = compound_statement();
// 		  if (!require(body, "compound-statement")) return 0;
		  PT::Block *body = new PT::Block(new PT::CommentedAtom(op, comments),
						  PT::cons(new PT::Atom(cp)));
		  func = PT::snoc(func, body);
		  if (my_lexer.look_ahead() == ';')
		    func = PT::snoc(func, new PT::Atom(my_lexer.get_token()));
		}
		declare(func);
		members = PT::snoc(members, func);
		break;
	      }
	      else
	      {
		declarators = PT::snoc(declarators, decl);
	      }
	    }
	    if (my_lexer.look_ahead() == ',')
	      declarators = PT::snoc(declarators, new PT::Atom(my_lexer.get_token()));
	  }
	  // If declarators is still zero we have processed a function definition.
	  
	  if (declarators)
	  { 
	    if (!require(';')) return 0;

	    PT::Atom *semic = new PT::Atom(my_lexer.get_token());
	    PT::SimpleDeclaration *decl =
	      new PT::SimpleDeclaration(spec, PT::list(declarators, semic));
	    decl->set_comments(comments);
	    if (spec && spec->is_typedef())
	      my_symbols.declare_typedef(decl, static_cast<ST::TypeName const *>(my_symbol));
	    else declare(decl);
	    members = PT::snoc(members, decl);
	  }
	}
	break;
      }
    }
  }
  return members;
}

// class-specifier:
//   class-head { member-specification [opt] }
//
// class-head:
//   class-key identifier [opt] base-clause [opt]
//   class-key nested-name-specifier identifier base-clause [opt]
//   class-key nested-name-specifier [opt] template-id base-clause [opt]
PT::ClassSpec *Parser::class_specifier(PT::Encoding &encoding)
{
  Trace trace("Parser::class_specifier", Trace::PARSING);

  PT::Keyword *key = class_key();
  if (!require(key, "class-key")) return 0;
  Tentative tentative(*this, encoding);
  PT::Node *name = 0;
  PT::List *qname = nested_name_specifier(encoding);
  if (qname)
  {
    Tentative tentative(*this, encoding);
    PT::Node *class_name_ = class_name(encoding, false, false);
    if (!class_name_)
    {
      tentative.rollback();
      class_name_ = identifier(encoding);
      if (!require(class_name_, "identifier")) return 0;
    }
    name = PT::snoc(qname, class_name_);
  }
  else // not qualified
  {
    tentative.rollback(true);
    name = template_id(encoding, false);
    if (!name)
    {
      tentative.rollback();
      if (my_lexer.look_ahead() == Token::Identifier) name = identifier(encoding);
      else encoding.anonymous();
    }
  }
  if (name && !name->is_atom()) name = new PT::Name(name, encoding);
  Token::Type next = my_lexer.look_ahead();
  if (next != ':' && next != '{')
  {
    error("expected '{' or ':'");
    return 0;
  }
  else commit();
  PT::List *comments = wrap_comments(my_lexer.get_comments());
  PT::ClassSpec *spec = new PT::ClassSpec(encoding, key, PT::cons(name), comments);
  declare(spec);
  PT::List *base_clause_ = 0;
  std::vector<SymbolTable::Symbol const *> bases;
  if (next == ':')
  {
    PGuard<ST::Scope *> guard(*this, &Parser::my_qualifying_scope, 0);
    base_clause_ = base_clause(bases);
    if (!require(base_clause_, "base-clause")) return 0;
  }
  spec = PT::snoc(spec, base_clause_);
  if (!require('{')) return 0;
  PT::Atom *ob = new PT::Atom(my_lexer.get_token());
  ScopeGuard scope(*this, spec, bases);
  PGuard<bool> guard(*this, &Parser::my_in_class, true);
  PT::Node *member_specification_ = opt_member_specification();
  if (!require('}')) return 0;
  PT::Atom *cb = new PT::Atom(my_lexer.get_token());
  PT::ClassBody *body = new PT::ClassBody(ob, member_specification_, cb);
  return PT::snoc(spec, body);
}

// storage-class-specifier:
//   auto
//   register
//   static
//   extern
//   mutable
//   thread [GCC]
PT::Keyword *Parser::opt_storage_class_specifier(PT::DeclSpec::StorageClass &storage)
{
  Trace trace("Parser::opt_storage_class_specifier", Trace::PARSING);
  switch (my_lexer.look_ahead())
  {
    case Token::AUTO:
      storage = PT::DeclSpec::AUTO;
      return new PT::Kwd::Auto(my_lexer.get_token());
    case Token::REGISTER:
      storage = PT::DeclSpec::REGISTER;
      return new PT::Kwd::Register(my_lexer.get_token());
    case Token::STATIC:
      storage = PT::DeclSpec::STATIC;
      return new PT::Kwd::Static(my_lexer.get_token());
    case Token::EXTERN:
      storage = PT::DeclSpec::EXTERN;
      return new PT::Kwd::Extern(my_lexer.get_token());
    case Token::MUTABLE:
      storage = PT::DeclSpec::MUTABLE;
      return new PT::Kwd::Mutable(my_lexer.get_token());
    default:
      storage = PT::DeclSpec::UNDEF;
      return 0;
  }
}

// cv-qualifier-seq:
//   cv-qualifier cv-qualifier-seq [opt]
//
// cv-qualifier:
//   const
//   volatile
PT::List *Parser::opt_cv_qualifier_seq(PT::Encoding &encoding)
{
  Trace trace("Parser::opt_cv_qualifier_seq", Trace::PARSING);
  bool is_const = false;
  bool is_volatile = false;
  PT::List *list = 0;
  while (true)
  {
    switch (my_lexer.look_ahead())
    {
      case Token::CONST:
	if (is_const)
	{
	  error("duplicate cv-qualifier");
	  return 0;
	}
	list = PT::snoc(list, new PT::Kwd::Const(my_lexer.get_token()));
	is_const = true;
	break;
      case Token::VOLATILE:
	if (is_volatile)
	{
	  error("duplicate cv-qualifier");
	  return 0;
	}
	list = PT::snoc(list, new PT::Kwd::Volatile(my_lexer.get_token()));
	is_volatile = true;
	break;
      default:
	encoding.cv_qualify(is_const, is_volatile);
	return list;
    }
  }
  return 0;
}

// type-specifier:
//   simple-type-specifier
//   class-specifier
//   enum-specifier
//   elaborated-type-specifier
//   cv-qualifier
//   __complex__ [GCC]
PT::Node *Parser::type_specifier(PT::Encoding &encoding, bool allow_user_defined)
{
  Trace trace("Parser::type_specifier", Trace::PARSING);
  switch (my_lexer.look_ahead())
  {
    case Token::ENUM:
      // Decide whether this is an enum-specifier or an elaborated-type-specifier.
      if (my_lexer.look_ahead(1) == '{' ||
	  (my_lexer.look_ahead(1) == Token::Identifier && 
	   my_lexer.look_ahead(2) == '{'))
      {
	PT::Node *spec = enum_specifier(encoding);
	if (spec) my_defines_class_or_enum = true;
	return spec;
      }
      else
      {
	PT::ElaboratedTypeSpec *spec = elaborated_type_specifier(encoding);
	if (spec) my_declares_class_or_enum = true;
	return spec;
      }
    case Token::CLASS:
    case Token::STRUCT:
    case Token::UNION:
    {
      Tentative tentative(*this, encoding);
      PT::Node *spec = class_specifier(encoding);
      if (spec)
      {
	my_defines_class_or_enum = true;
	return spec;
      }
      tentative.rollback();
      spec = elaborated_type_specifier(encoding);
      if (spec)
      {
	my_declares_class_or_enum = true;
	// If the next token is a ';' we deal with the elaborated-type-specifier
	// as part of the declaration.
	if (my_lexer.look_ahead() != ';')
	  declare(static_cast<PT::ElaboratedTypeSpec *>(spec));
      }
      return spec;
    }
    case Token::TYPENAME:
      return elaborated_type_specifier(encoding);
    case Token::CONST:
      encoding.cv_qualify(true);
      return new PT::Kwd::Const(my_lexer.get_token());
    case Token::VOLATILE:
      encoding.cv_qualify(false, true);
      return new PT::Kwd::Volatile(my_lexer.get_token());
    // This is presently masked by the lexer
//     case Token::RESTRICT:
//     {
//       Token token;
//       my_lexer.get_token(token);
//       return new PT::Kwd::Restrict(token);
//     }
    default:
      break;
  }
  PT::Node *type = simple_type_specifier(encoding, allow_user_defined);
  if (!require(type, "type-specifier")) return 0;
  return type;
}

// type-specifier-seq:
//   type-specifier type-specifier-seq [opt]
PT::List *Parser::type_specifier_seq(PT::Encoding &encoding)
{
  Trace trace("Parser::type_specifier_seq", Trace::PARSING);
  PT::List *spec = 0;
  while (true)
  {
    Tentative tentative(*this);
    PT::Node *type_spec = type_specifier(encoding, true);
    if (!type_spec)
    {
      tentative.rollback();
      if (!spec) error("expected type-specifier");
      return spec;
    }
    spec = PT::snoc(spec, type_spec);
  }
}

// constructor-declarator:
//   class-name ( parameter-declaration-clause [opt] )
//   class-name :: class-name ( parameter-declaration-clause [opt] )
bool Parser::is_constructor_declarator()
{
  Trace trace("Parser::is_constructor_declarator", Trace::PARSING);
  if (dynamic_cast<ST::FunctionScope *>(my_symbols.current_scope())) return false;
  Tentative tentative(*this, true); // make sure we leave this method in this state
  Token token;
  my_lexer.look_ahead(0, token);
  trace << token;
  if (token.type != Token::Identifier && token.type != Token::Scope) return false;
  PT::Encoding name;
  {
    Tentative tentative(*this);
    trace << my_qualifying_scope << ' ' << my_in_nested_name_specifier;
    PT::Node *nested = nested_name_specifier(name);
    if (!nested && !dynamic_cast<ST::Class *>(my_symbols.current_scope()))
      return false;
    else if (!nested) tentative.rollback();
  }
  trace << my_qualifying_scope;
  PT::Node *class_name_ = class_name(name, false, false);
  if (!class_name_ || my_lexer.look_ahead() != '(') return false;
  if (ST::Class *class_ =
      dynamic_cast<ST::Class *>(my_symbols.current_scope()))
  {
    trace << "compare names " << name << ' ' << class_->name();
    if (name.unmangled() != class_->name()) return false;
  }
  // TODO: This needs quite a bit more work !
  return true;
}

// decl-specifier-seq:
//   decl-specifier-seq [opt] decl-specifier
//
// decl-specifier:
//   storage-class-specifier
//   type-specifier
//   function-specifier
//   friend
//   typedef
PT::DeclSpec *Parser::decl_specifier_seq(PT::Encoding &type)
{
  Trace trace("Parser::decl_specifier_seq", Trace::PARSING);
  PGuard<ST::Scope *> guard(*this, &Parser::my_qualifying_scope);

  PT::DeclSpec::StorageClass storage = PT::DeclSpec::UNDEF;
  unsigned int flags = PT::DeclSpec::NONE;
  bool declares_class_or_enum = false;
  bool defines_class_or_enum = false;
  bool allow_user_defined = true;
  bool constructor_possible = true;
  bool is_const = false;
  bool is_volatile = false;
  PT::List *seq = 0;
  while (true)
  {
    // Start by looking for discriminating keywords.
    switch (my_lexer.look_ahead())
    {
      case Token::FRIEND:
	if (flags & PT::DeclSpec::FRIEND)
	{
	  error("duplicate friend");
	  return 0;
	}
	seq = PT::snoc(seq, new PT::Kwd::Friend(my_lexer.get_token()));
	flags |= PT::DeclSpec::FRIEND;
	break;
      case Token::INLINE:
      case Token::VIRTUAL:
      case Token::EXPLICIT:
	seq = PT::snoc(seq, opt_function_specifier());
	break;
      case Token::TYPEDEF:
	if (storage != PT::DeclSpec::UNDEF)
	{
	  // [dcl.stc] (7.1.1/1)
	  // If a storage-class-specifier appears in a decl-specifier-seq,
	  // there can be no typedef specifier in the same decl-specifier-seq...
	  error("'typedef' and storage-class-specifiers not allowed in the same decl-specifier-seq.");
	  return 0;
	}
	flags |= PT::DeclSpec::TYPEDEF;
	constructor_possible = false;
	seq = PT::snoc(seq, new PT::Kwd::Typedef(my_lexer.get_token()));
	break;
      case Token::AUTO:
      case Token::REGISTER:
      case Token::STATIC:
      case Token::EXTERN:
      case Token::MUTABLE:
	// [dcl.stc]
	// At most one storage-class-specifier shall appear 
	// in a given decl-specifier-seq.
	if (storage != PT::DeclSpec::UNDEF)
	{
	  error("multiple storage classes in declaration");
	  return 0;
	}
	else if (flags & PT::DeclSpec::TYPEDEF)
	{
	  error("'typedef' and storage-class-specifiers not allowed in the same decl-specifier-seq.");
	  return 0;
	}
	seq = PT::snoc(seq, opt_storage_class_specifier(storage));
	break;
      default:
      {
	// There is an ambiguity between the start of a constructor-declarator
	// and a type-specifier. If a constructor-declarator would be valid,
	// test whether the following actually is one, and if so, return the
	// decl-specifier-seq parsed so far, letting the declarator production
	// parse the constructor-declarator.
	if (type.empty() && constructor_possible && is_constructor_declarator())
	{
	  if (seq) return new PT::DeclSpec(seq, type, storage, flags, false, false);
	  else return 0;
	}

	PGuard<bool> guard1(*this, &Parser::my_declares_class_or_enum, false);
	PGuard<bool> guard2(*this, &Parser::my_defines_class_or_enum, false);
	Tentative tentative(*this);
	PT::Encoding type_component;
	PT::Node *spec = type_specifier(type_component, allow_user_defined);
	if (spec && !type_component.empty())
	{
	  // Hack:
	  // If the last type-specifier was a cv-qualifier, we
	  // remember it and prepend it to the type encoding at the very end,
	  // so meanwhile the qualified-name marker and counter can be accessed
	  // as the encoding's first two characters.
	  if (type_component.front() == 'C' || type_component.front() == 'V')
	  {
	    if (type_component.size() == 1)
	      if (type_component.front() == 'C') is_const = true;
	      else is_volatile = true;
	    else is_const = is_volatile = true;
	  }
	  else
	  {
	    type.append(type_component);
	    // If the last type-specifier was not a cv-qualifier,
	    // no (more) user-defined types are allowed.
	    allow_user_defined = false;
	  }
	}
	declares_class_or_enum |= my_declares_class_or_enum;
	defines_class_or_enum |= my_defines_class_or_enum;
	if (!spec)
	{
	  tentative.rollback();
	  type.cv_qualify(is_const, is_volatile);
	  if (seq)
	    return new PT::DeclSpec(seq, type, storage, flags,
				    declares_class_or_enum,
				    defines_class_or_enum);
	  else
	    return 0;
	}
	seq = PT::snoc(seq, spec);
	if (!spec->is_atom() ||
	    !(dynamic_cast<PT::Kwd::Const *>(spec) ||
	      dynamic_cast<PT::Kwd::Volatile *>(spec)))
 	  allow_user_defined = false;
      }
    }
  }
}

// declarator-id:
//   id-expression
//   :: [opt] nested-name-specifier [opt] type-name
PT::Node *Parser::declarator_id(PT::Encoding &encoding)
{
  Trace trace("Parser::declarator_id", Trace::PARSING);
  return id_expression(encoding, false);
}

// ptr-operator:
//   * cv-qualifier-seq [opt]
//   &
//   :: [opt] nested-name-specifier * cv-qualifier-seq [opt]
PT::List *Parser::ptr_operator(PT::Encoding &type)
{
  Trace trace("Parser::ptr_operator", Trace::PARSING);
  PT::List *scope = 0;
  size_t length = 0; // FIXME: this isn't really needed
  switch (my_lexer.look_ahead())
  {
    case '&':
      type.ptr_operator('&');
      return PT::cons(new PT::Atom(my_lexer.get_token()));
    case '*':
    {
      type.ptr_operator('*');
      PT::List *node = PT::cons(new PT::Atom(my_lexer.get_token()));
      PT::List *cv_qualifier_seq = opt_cv_qualifier_seq(type);
      if (cv_qualifier_seq) node = PT::snoc(node, cv_qualifier_seq);
      return node;
    }
    case Token::Scope:
      my_qualifying_scope = my_symbols.current_scope()->global_scope();
      length = 1;
      type.global_scope();
      scope = PT::cons(new PT::Atom(my_lexer.get_token()));
      break;
    default:
      my_qualifying_scope = 0;
      break;
  }
  PT::Encoding encoding;
  PT::List *name = nested_name_specifier(encoding, false);
  if (!require(name, "nested-name-specifier") || !require('*')) return 0;
  type.ptr_to_member(encoding, length);
  PT::List *node = PT::snoc(scope, name);
  node = PT::snoc(node, new PT::Atom(my_lexer.get_token()));
  PT::List *cv_qualifiers = opt_cv_qualifier_seq(type);
  if (cv_qualifiers) node = PT::conc(node, cv_qualifiers);
  return node;
}

// direct-declarator:
//   declarator-id
//   direct-declarator ( parameter-declaration-clause )
//     cv-qualifier-seq [opt] exception-specification [opt]
//   direct-declarator [ constant-expression [opt] ]
//   ( declarator )
//
// direct-abstract-declarator:
//   direct-abstract-declarator [opt] ( parameter-declaration-clause )
//     cv-qualifier-seq [opt] exception-specification [opt]
//   direct-abstract-declarator [opt] [ constant-expression [opt] ]
//   ( abstract-declarator )
PT::List *Parser::direct_declarator(PT::Encoding &type,
				    PT::Encoding &name,
				    DeclaratorKind kind)
{
  Trace trace("Parser::direct_declarator", Trace::PARSING);
  PT::Encoding inner;
  PT::List *decl = 0;
  if (my_lexer.look_ahead() == '(')
  {
    Token op = my_lexer.get_token();
    // If we are looking for a direct-declarator, this must be a
    // parenthesized declarator. If it may be an abstract-direct-declarator,
    // we have to try a parameter-declaration-clause first.
    if (kind != NAMED)
    {
      Tentative tentative(*this, type);
      PT::Node *parameters = parameter_declaration_clause(type);
      if (parameters && my_lexer.look_ahead() == ')')
      {
	decl = PT::list(new PT::Atom(op),
			parameters,
			new PT::Atom(my_lexer.get_token()));
      }
      else tentative.rollback();
    }
    if (!decl)
    {
      decl = declarator(inner, name, kind);
      if (!require(decl, "declarator") || !require(')')) return 0;
      decl = PT::list(new PT::Atom(op),
		      decl,
		      new PT::Atom(my_lexer.get_token()));
    }
  }
  else if (my_lexer.look_ahead() == '[' && kind != NAMED)
  {
    Token os = my_lexer.get_token();
    PT::Node *expr = 0;
    if (my_lexer.look_ahead() != ']')
    {
      expr = constant_expression();
      if (!require(expr, "constant-expression") || !require(']')) return 0;
    }
    decl = PT::list(new PT::Atom(os),
		    expr, 
		    new PT::Atom(my_lexer.get_token()));
  }
  else if (kind != ABSTRACT)
  {
    Tentative tentative(*this);
    PT::Node *id = declarator_id(name);
    if (id) decl = PT::cons(id);
    else if (kind == NAMED)
    {
      error("expected declarator-id");
      return 0;
    }
  }
  
  while (true)
  {
    Token::Type token = my_lexer.look_ahead();
    if (token == '(')
    {
      Tentative tentative(*this);
      ScopeGuard scope_guard(*this, decl);
      Token op = my_lexer.get_token();
      PT::Encoding p_type;
      PT::Node *parameters = parameter_declaration_clause(p_type);
      if (!parameters || my_lexer.look_ahead() != ')')
      {
	tentative.rollback();
	break;
      }
      Token cp = my_lexer.get_token();
      // At this point we know we are looking at a function declarator,
      // and an empty type means a function without return type specifier.
      if (type.empty()) type.no_return_type();
      type.function(p_type);
      decl = PT::conc(decl, PT::list(new PT::Atom(op),
				     parameters,
				     new PT::Atom(cp)));
      PT::Node *cv_qualifiers = opt_cv_qualifier_seq(type);
      if (cv_qualifiers) decl = PT::snoc(decl, cv_qualifiers);
      PT::Node *exc_spec = opt_exception_specification();
      if (exc_spec) decl = PT::snoc(decl, exc_spec);
    }
    else if (token == '[')
    {
      Token os = my_lexer.get_token();
      PT::Node *expr = 0;
      if (my_lexer.look_ahead() != ']')
      {
	expr = constant_expression();
	if (!require(expr, "constant-expression") || !require(']')) return 0;
      }
      Token cs = my_lexer.get_token();
      decl = PT::conc(decl, PT::list(new PT::Atom(os), expr, new PT::Atom(cs)));
    }
    else break;
  }
  if (!inner.empty()) type.recursion(inner);
  return decl;
}

// declarator:
//   direct-declarator
//   ptr-operator declarator
//
// abstract-declarator:
//   ptr-operator abstract-declarator [opt]
//   direct-abstract-declarator
PT::Declarator *Parser::declarator(PT::Encoding &type, PT::Encoding &name,
				   DeclaratorKind kind)
{
  Trace trace("Parser::declarator", Trace::PARSING);

  PT::List *ptrs = 0;
  while (true)
  {
    Tentative tentative(*this);
    PT::List *ptr = ptr_operator(type);
    if (ptr)
    {
      ptrs = PT::conc(ptrs, ptr);
    }
    else
    {
      tentative.rollback();
      break;
    }
  }
  PT::List *decl = direct_declarator(type, name, kind);
  if (decl)
  {
    return new PT::Declarator(PT::conc(ptrs, decl), type, name, 0);
  }
  else
  {
    if (!ptrs || kind == NAMED) return 0;
    else
    {
      return new PT::Declarator(ptrs, type, name, 0);
    }
  }
}

// initializer-clause:
//   assignment-expression
//   { initializer-list , [opt] }
//   { }
PT::Node *Parser::initializer_clause(bool constant)
{
  Trace trace("Parser::initializer_clause", Trace::PARSING);
  if (my_lexer.look_ahead() != '{')
  {
    PGuard<bool> guard(*this, &Parser::my_in_constant_expression, constant);
    return assignment_expression();
  }
  Token ob = my_lexer.get_token();
  PT::List *clause = PT::cons(new PT::Atom(ob));
  if (my_lexer.look_ahead() != '}')
  {
    PT::List *initializers = initializer_list(constant);
    if (!require(initializers, "initializer-list")) return 0;
    clause = PT::snoc(clause, initializers);
    if (my_lexer.look_ahead() == ',')
      clause = PT::snoc(clause, new PT::Atom(my_lexer.get_token()));
  }
  if (!require('}')) return 0;
  return PT::snoc(clause, new PT::Atom(my_lexer.get_token()));
}

// initializer-list:
//   initializer-clause
//   initializer-list , initializer-clause
PT::List *Parser::initializer_list(bool constant)
{
  Trace trace("Parser::initializer_list", Trace::PARSING);

  PT::List *initializers = 0;
  while (true)
  {
    PT::Node *initializer = initializer_clause(constant);
    if (!require(initializer, "initializer-clause")) return 0;
    initializers = PT::snoc(initializers, initializer);
    if (my_lexer.look_ahead() != ',') break;
    initializers = PT::snoc(initializers, new PT::Atom(my_lexer.get_token()));
  }
  return initializers;
}

// init-declarator:
//   declarator initializer [opt]
//
// initializer:
//   = initializer-clause
//   ( expression-list )
PT::Declarator *Parser::init_declarator(PT::Encoding &type)
{
  Trace trace("Parser::init_declarator", Trace::PARSING);
  PT::Encoding name;
  PT::Declarator *decl = declarator(type, name, NAMED);
  if (!decl) return 0;

  Token::Type token = my_lexer.look_ahead();
  if (type.is_function() && starts_function_definition(token))
    return decl;

  bool initialized = token == '=' || token == '(';
  if (!initialized && token != ',' && token != ';')
  {
    error("expected initializer");
    return 0;
  }
  if (initialized)
  {
    if (token == '=')
    {
      Token eq = my_lexer.get_token();
      PT::Node *initializer = initializer_clause(/*constant=*/false);
      if (!require(initializer, "initializer-clause")) return 0;
      decl = PT::conc(decl, PT::cons(new PT::Atom(eq), PT::cons(initializer)));
    }
    else
    {
      Token op = my_lexer.get_token();
      PT::Node *expr = expression();
      if (!require(expr, "expression") || !require(')')) return 0;
      Token cp = my_lexer.get_token();
      decl = PT::conc(decl, PT::list(new PT::Atom(op), expr, new PT::Atom(cp)));
    }
  }
  return decl;
}

// parameter-declaration-clause:
//   parameter-declaration-list [opt] ... [opt]
//   parameter-declaration-list , ...
PT::List *Parser::parameter_declaration_clause(PT::Encoding &encoding)
{
  Trace trace("Parser::parameter_declaration_clause", Trace::PARSING);
  
  encoding.start_func_args();
  switch (my_lexer.look_ahead())
  {
    case Token::Ellipsis:
      encoding.ellipsis_arg();
      encoding.end_func_args();
      return PT::cons(new PT::Atom(my_lexer.get_token()));
    case ')':
      encoding.void_();
      encoding.end_func_args();
      return PT::cons(0);
    case Token::VOID:
      if (my_lexer.look_ahead(1) == ')')
      {
	encoding.void_();
	encoding.end_func_args();
	return PT::cons(new PT::Kwd::Void(my_lexer.get_token()));
      }
      // Fall through.
    default:
    {
      PT::List *parameters = parameter_declaration_list(encoding);
      if (!require(parameters, "parameter-declaration-list")) return 0;
      PT::Node *comma = 0;
      if (my_lexer.look_ahead() == ',')
      {
	comma = new PT::Atom(my_lexer.get_token());
	parameters = PT::snoc(parameters, comma);
      }
      if (my_lexer.look_ahead() == Token::Ellipsis)
      {
	encoding.ellipsis_arg();
	parameters = PT::snoc(parameters, new PT::Atom(my_lexer.get_token()));
      }
      else if (comma)
      {
	error("expected '...'");
	return 0;
      }
      encoding.end_func_args();
      return parameters;
    }
  }
}

// parameter-declaration-list:
//   parameter-declaration
//   parameter-declaration-list , parameter-declaration
PT::List *Parser::parameter_declaration_list(PT::Encoding &encoding)
{
  Trace trace("Parser::parameter_declaration_list", Trace::PARSING);

  PT::List *parameters = 0;
  while (true)
  {
    PT::Encoding penc;
    PT::Node *parameter = parameter_declaration(penc);
    if (!require(parameter, "parameter-declaration")) return 0;
    else
    {
      encoding.append(penc);
      parameters = PT::snoc(parameters, parameter);
    }
    switch (my_lexer.look_ahead())
    {
      case ')':
      case Token::Ellipsis:
	return parameters;
      case ',':
	if (my_lexer.look_ahead(1) == Token::Ellipsis)
	  return parameters;
	else
	{
	  parameters = PT::snoc(parameters, new PT::Atom(my_lexer.get_token()));
	  break;
	}
      default:
	error("expected ',' or ')'");
	return 0;
    }
  }
}

// parameter-declaration:
//   decl-specifier-seq declarator
//   decl-specifier-seq declarator = assignment-expression
//   decl-specifier-seq abstract-declarator [opt]
//   decl-specifier-seq abstract-declarator [opt] = assignment-expression
PT::ParameterDeclaration *Parser::parameter_declaration(PT::Encoding &type)
{
  Trace trace("Parser::parameter_declaration", Trace::PARSING);
  PGuard<ST::Scope *> pguard(*this, &Parser::my_qualifying_scope, 0);
  PT::DeclSpec *spec = decl_specifier_seq(type);
  if (!spec) return 0;
  PT::Declarator *decl = 0;
  Token::Type token = my_lexer.look_ahead();
  if (token == ')' || token == ',' || token == '=' || token == '>' ||
      token == Token::Ellipsis)
  {
    // no declarator
    decl = 0;
  }
  else
  {
    PT::Encoding dummy_name; // We are only interested into the type.
    decl = declarator(type, dummy_name, EITHER);
    if (!require(decl, "declarator")) return 0;
  }
  if (my_lexer.look_ahead() == '=')
  {
    PT::Atom *equal = new PT::Atom(my_lexer.get_token());
    PT::Node *init = assignment_expression();
    if (!require(init, "assignment-expression")) return 0;
    else return new PT::ParameterDeclaration(spec, decl, equal, init);
  }
  else return new PT::ParameterDeclaration(spec, decl);
}

// simple-declaration:
//   decl-specifier-seq [opt] init-declarator-list [opt] ;
//
// init-declarator-list:
//   init-declarator
//   init-declarator-list , init-declarator
//
// Since a function-definition starts like a simple-declaration,
// we handle it here, too, to avoid an unnecessary tentative / rollback.
//
// function-definition:
//   decl-specifier-seq [opt] declarator ctor-initializer [opt]
//     function-body
//   decl-specifier-seq [opt] declarator function-try-block
PT::Declaration *Parser::simple_declaration(bool function_definition_allowed)
{
  Trace trace("Parser::simple_declaration", Trace::PARSING);

  PT::Encoding encoding;
  PT::DeclSpec *spec = decl_specifier_seq(encoding);

  PT::List *comments = wrap_comments(my_lexer.get_comments());

  // [dcl.dcl]
  //
  // Only in function declarations for constructors, destructors, and
  // type conversions can the decl-specifier-seq be omitted.
  //
  // If this is not a functional cast, it is definitely a declaration.
  if (spec && my_lexer.look_ahead() != '(') commit();

  // [dcl.dcl]
  //
  // In a simple-declaration, the optional init-declarator-list can be
  // omitted only when declaring a class or enumeration, that is when
  // the decl-specifier-seq contains either a class-specifier, an
  // elaborated-type-specifier with a class-key, or an enum-specifier.
  PT::List *declarators = 0;
  while (my_lexer.look_ahead() != ';')
  {
    PT::Encoding type = encoding;
    PT::Declarator *decl = init_declarator(type);
    if (type.is_function() &&
	function_definition_allowed &&
	starts_function_definition(my_lexer.look_ahead()))
    {
      PT::FunctionDefinition *func = 
	new PT::FunctionDefinition(spec, PT::cons(decl));
      {
	trace << "qualifying scope " << my_qualifying_scope;
	declare(func);
	if (my_in_class)
	{
	  // FIXME: For now skip the whole function body (incl. ctor-initializer).
	  //        Enhance the Lexer so it can be nested, e.g. to
	  //        scan chunks of the buffer such as member function bodies.
	  Token op = my_lexer.get_token();
	  size_t nesting = op.type == '{' ? 1 : 0;
	  Token cp = skip_statement(nesting);
	  if (cp.type == Token::BadToken)
	  {
	    error("encountering EOF while parsing function body");
	    return 0;
	  }
	  PT::List *ob_comments = wrap_comments(my_lexer.get_comments());
	  PT::Block *body = new PT::Block(new PT::CommentedAtom(op, ob_comments),
					  PT::cons(new PT::Atom(cp)));
	  func = PT::snoc(func, body);
	}
	else
	{
	  // enter the qualifying scope
	  ScopeGuard scope_guard1(*this, my_qualifying_scope);
	  {
	    // make lookup non-qualified
	    PGuard<ST::Scope *> guard(*this, &Parser::my_qualifying_scope, 0);
	    PT::List *ctor_init = opt_ctor_initializer();
	    if (ctor_init) func = PT::snoc(func, ctor_init);
	  }
	  // now enter the function scope
	  ScopeGuard scope_guard2(*this, func);
	  PT::Block *body = compound_statement();
	  if (!require(body, "compound-statement")) return 0;
	  func = PT::snoc(func, body);
	}
      }
      trace << "qualifying scope after " << my_qualifying_scope;
      return func;
    }
    // the decl-specifier-seq may be ommitted only for functions (C) or
    // constructors, destructors, and conversion operators (C++).
    else if (!type.is_function() && (!spec || spec->type().empty()))
    {
      error("expected type");
      return 0;
    }
    declarators = PT::snoc(declarators, decl);
    Token::Type token = my_lexer.look_ahead();
    if (token == ',')
      declarators = PT::snoc(declarators, new PT::Atom(my_lexer.get_token()));
    else if (token == ';') break;
    else
    {
      error("expected ',' or ';'");
      return 0;
    }
    // A function-definition only has a single declarator, so
    // disallow function-definition once we'v already encountered a declarator.
    function_definition_allowed = false;
  }
  Token semic = my_lexer.get_token();
  PT::SimpleDeclaration *decl =
    new PT::SimpleDeclaration(spec, PT::list(declarators, new PT::Atom(semic)));
  decl->set_comments(comments);
  if (spec && spec->is_typedef())
    my_symbols.declare_typedef(decl, static_cast<ST::TypeName const *>(my_symbol));
  else declare(decl);
  return decl;
}

// linkage-specification:
//   extern string-literal { declaration-seq [opt] }
//   extern string-literal declaration
PT::LinkageSpec *Parser::linkage_specification()
{
  Trace trace("Parser::linkage_specification", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::EXTERN &&
      my_lexer.look_ahead(1) != Token::StringL) return 0;

  Token extern_ = my_lexer.get_token();
  Token token = my_lexer.get_token();

  PT::LinkageSpec *spec = new PT::LinkageSpec(new PT::Kwd::Extern(extern_),
					      PT::cons(new PT::Atom(token)));
  if(my_lexer.look_ahead() == '{')
  {
    Token ob = my_lexer.get_token();
    PT::List *body = opt_declaration_seq();
    if (!require('}')) return 0;
    Token cb = my_lexer.get_token();
    PT::List *comment = wrap_comments(my_lexer.get_comments());
    PT::Brace *brace = new PT::Brace(new PT::Atom(ob), body,
				     new PT::CommentedAtom(cb, comment));
    return PT::snoc(spec, brace);
  }
  else
  {
    PT::Node *decl = declaration();
    if (!require(decl, "declaration")) return 0;
    return PT::snoc(spec, decl);
  }
}

// block-declaration:
//   simple-declaration
//   asm-definition
//   namespace-alias-definition
//   using-declaration
//   using-directive
PT::Declaration *Parser::block_declaration()
{
  Trace trace("Parser::block_declaration", Trace::PARSING);

  switch (my_lexer.look_ahead())
  {
    case Token::ASM:
      error("sorry, asm-definition not yet implemented");
      return 0;
    case Token::NAMESPACE:
      return namespace_alias_definition();
    case Token::USING:
      if (my_lexer.look_ahead(1) == Token::NAMESPACE)
	return using_directive();
      else
	return using_declaration();
    default:
      // C++ does not allow function definitions inside block declarations,
      // while C does.
      if (my_ruleset & CXX)
	return simple_declaration(!my_in_declaration_statement);
      else
	return simple_declaration(true);
  }
}

// declaration:
//   block-declaration
//   function-definition
//   template-declaration
//   explicit-instantiation
//   explicit-specialization
//   linkage-specification
//   namespace-definition
PT::Declaration *Parser::declaration()
{
  Trace trace("Parser::declaration", Trace::PARSING);
  my_qualifying_scope = 0;
  // TODO:
  // We may be inside a template declaration, in which case only
  // a subset of the above productions is active.

  Token::Type token = my_lexer.look_ahead();
  if (token == Token::EXTERN && my_lexer.look_ahead(1) == Token::StringL)
    return linkage_specification();

  else if (token == Token::TEMPLATE)
  {
    if (my_lexer.look_ahead(1) == '<')
    {
      if (my_lexer.look_ahead(2) == '>') return explicit_specialization();
      else return template_declaration();
    }
    else return explicit_instantiation();
  }
  else if (token == Token::EXPORT) return template_declaration();
  else if (my_ruleset & GCC &&
	   (token == Token::EXTERN ||
	    token == Token::STATIC ||
	    token == Token::INLINE) &&
	   my_lexer.look_ahead(1) == Token::TEMPLATE)
    return explicit_instantiation();
  else if (token == Token::NAMESPACE &&
	   ((my_lexer.look_ahead(1) == Token::Identifier &&
	     my_lexer.look_ahead(2) == '{') ||
	    my_lexer.look_ahead(1) == '{'))
    return namespace_definition();
  else return block_declaration();
}

// declaration-seq:
//   declaration
//   declaration-seq declaration
PT::List *Parser::opt_declaration_seq()
{
  Trace trace("Parser::opt_declaration_seq", Trace::PARSING);
  PT::List *declarations = 0;
  while(true)
  {
    Token::Type type = my_lexer.look_ahead();
    if (type == '}' || type == '\0') break;
    PT::Node *decl = declaration();
    if (!require(decl, "declaration")) break;
    else declarations = PT::snoc(declarations, decl);
  }
  return declarations;
}

// namespace-definition:
//   named-namespace-definition
//   unnamed-namespace-definition
//
// named-namespace-definition:
//   original-namespace-definition
//   extension-namespace-definition
//
// original-namespace-definition:
//   namespace identifier { namespace-body }
//
// extension-namespace-definition:
//   namespace original-namespace-name { namespace-body }
//
// unnamed-namespace-definition:
//   namespace { namespace-body }
//
// namespace-body:
//   declaration-seq [opt]
PT::NamespaceSpec *Parser::namespace_definition()
{
  Trace trace("Parser::namespace_definition", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::NAMESPACE) return 0;

  PT::Kwd::Namespace *namespace_ = new PT::Kwd::Namespace(my_lexer.get_token());
  PT::List *comments = wrap_comments(my_lexer.get_comments());

  PT::Encoding encoding;
  PT::Node *name = 0;
  if(my_lexer.look_ahead() != '{')
  {
    name = identifier(encoding);
    if (!name) return 0;
  }
  PT::NamespaceSpec *spec = new PT::NamespaceSpec(namespace_, PT::list(name, 0));
  spec->set_comments(comments);

  PT::Node *body;
  if(!require('{')) return 0;
  PT::Atom *ob = new PT::Atom(my_lexer.get_token());
  declare(spec);
  ScopeGuard scope_guard(*this, spec);
  PT::List *decl_seq = opt_declaration_seq();
  if(!require('}')) return 0;
  PT::Atom *cb = new PT::Atom(my_lexer.get_token());
  PT::Brace *brace = new PT::Brace(ob, decl_seq, cb);
  PT::tail(spec, 2)->set_car(brace);
  return spec;
}

// namespace-alias-definition:
//   namespace identifier = qualified-namespace-specifier ;
//
// qualified-namespace-specifier:
//   :: [opt] nested-name-specifier [opt] namespace-name
PT::NamespaceAlias *Parser::namespace_alias_definition()
{
  Trace trace("Parser::namespace_alias_definition", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::NAMESPACE) return 0;
  Token namespace_ = my_lexer.get_token();
  PT::Encoding encoding;
  PT::Identifier *alias = identifier(encoding);
  if (!require(alias, "identifier") || !require('=')) return 0;
  Token eq = my_lexer.get_token();
  PT::Atom *scope = opt_scope(encoding);
  Tentative tentative(*this, encoding);
  PT::List *name = nested_name_specifier(encoding);
  if (!name) tentative.rollback();
  PT::Node *ns = namespace_name(encoding);
  if (!require(ns, "namespace-name") || !require(';')) return 0;
  if (scope) name = PT::cons(scope, name);
  name = PT::snoc(name, ns);
  return new PT::NamespaceAlias(new PT::Kwd::Namespace(namespace_),
				PT::list(alias,
					 new PT::Atom(eq),
					 name,
					 new PT::Atom(my_lexer.get_token())));
}

// using-directive:
//   using namespace :: [opt] nested-name-specifier [opt] namespace-name ;
PT::UsingDirective *Parser::using_directive()
{
  Trace trace("Parser::using_directive", Trace::PARSING);

  if(my_lexer.look_ahead() != Token::USING ||
     my_lexer.look_ahead(1) != Token::NAMESPACE)
    return 0;

  PT::UsingDirective *udir = 0;

  udir = new PT::UsingDirective(new PT::Kwd::Using(my_lexer.get_token()));
  udir = PT::snoc(udir, new PT::Kwd::Namespace(my_lexer.get_token()));

  PT::Encoding encoding;
  PT::Atom *scope = opt_scope(encoding);
  PT::List *name = 0;
  if (scope) name = PT::cons(scope);
  {
    Tentative tentative(*this, encoding);
    PT::Node *name_spec = nested_name_specifier(encoding);
    if (name_spec) name = PT::snoc(name, name_spec);
    else tentative.rollback();
  }
  PT::Node *ns = namespace_name(encoding);
  if (!require(ns, "namespace-name")) return 0;
  udir = PT::snoc(udir, new PT::Name(PT::snoc(name, ns), encoding));
  if (!require(';')) return 0;
  udir = PT::snoc(udir, new PT::Atom(my_lexer.get_token()));
  declare(udir);
  return udir;
}

// using-declaration:
//   using typename [opt] :: [opt] nested-name-specifier unqualified-id ;
//   using :: unqualified-id ;
PT::UsingDeclaration *Parser::using_declaration()
{
  Trace trace("Parser::using_declaration", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::USING) return 0;
  PT::Kwd::Using *using_ = new PT::Kwd::Using(my_lexer.get_token());
  PT::Kwd::Typename *typename_ = 0;
  if(my_lexer.look_ahead() == Token::TYPENAME)
    typename_ = new PT::Kwd::Typename(my_lexer.get_token());
  PT::Encoding encoding;
  PT::Atom *scope = opt_scope(encoding);
  Tentative tentative(*this, encoding);
  PT::List *name = nested_name_specifier(encoding);
  if (!name)
  {
    if (!typename_ && !scope)
    {
      error("expected nested-name-secifier");
      return 0;
    }
    else tentative.rollback();
  }
  PT::Node *id = unqualified_id(encoding, /*template*/false);
  if (!require(id, "unqualified-id")) return 0;
  name = PT::snoc(name, id);
  if (scope) name = PT::cons(scope, name);
  PT::UsingDeclaration *udecl =
    new PT::UsingDeclaration(using_, typename_, new PT::Name(name, encoding));
  if (!require(';')) return 0;
  udecl = PT::snoc(udecl, new PT::Atom(my_lexer.get_token()));
  // TODO: At this point we have both, the qualifying scope, as well as the id.
  //       Thus we ought to use a simple qualified lookup to find the symbol
  //       we want to set up an alias for...
  declare(udecl);
  return udecl;
}

// type-parameter:
//   class identifier [opt]
//   class identifier [opt] = type-id
//   typename identifier [opt]
//   typename identifier [opt] = type-id
//   template < template-parameter-list > class identifier [opt]
//   template < template-parameter-list > class identifier [opt] = id-expression
PT::Node *Parser::type_parameter(PT::Encoding &encoding)
{
  Trace trace("Parser::type_parameter", Trace::PARSING);

  PT::Keyword *kwd = 0;
  switch (my_lexer.look_ahead())
  {
    case Token::CLASS:
      kwd = new PT::Kwd::Class(my_lexer.get_token());
    case Token::TYPENAME:
    {
      if (!kwd) kwd = new PT::Kwd::Typename(my_lexer.get_token());
      PT::Identifier *id = 0;
      if (my_lexer.look_ahead() == Token::Identifier)
      {
	id = new PT::Identifier(my_lexer.get_token());
	encoding.simple_name(id);
      }
      PT::TypeParameter *tparam = new PT::TypeParameter(kwd, PT::cons(id));
      if (id) declare(tparam);

      if (my_lexer.look_ahead() == '=')
      {
	Token eq = my_lexer.get_token();
	PT::Encoding encoding;
	PT::Node *initializer = type_id(encoding);
	return PT::conc(tparam, PT::list(new PT::Atom(eq), initializer));
      }
      else return tparam;
    }
    case Token::TEMPLATE:
    {
      kwd = new PT::Kwd::Template(my_lexer.get_token());
      if (!require('<')) return 0;
      Token ob = my_lexer.get_token();
      PT::Encoding encoding;
      PT::TemplateParameterList *params = template_parameter_list(encoding);
      if (!require(params, "template-parameter-list") || !require('>')) return 0;
      Token cb = my_lexer.get_token();
      if (!require(Token::CLASS, "class")) return 0;
      PT::TypeParameter *tparam = 
	new PT::TypeParameter(kwd,
			      PT::list(new PT::Atom(ob),
				       params,
				       new PT::Atom(cb),
				       new PT::Kwd::Class(my_lexer.get_token())));
      PT::Identifier *id = 0;
      Token::Type token = my_lexer.look_ahead();
      if (token != '=' && token != '>' && token != ',')
      {
	PT::Encoding dummy;
	id = identifier(dummy);
      }
      tparam = PT::snoc(tparam, id);
      if (id) declare(tparam);
      
      if(my_lexer.look_ahead() == '=')
      {
	Token eq = my_lexer.get_token();
	PT::Encoding encoding;
	PT::Node *initializer = id_expression(encoding, false);
	if (!require(initializer, "id-expression")) return 0;
	return PT::conc(tparam, PT::list(new PT::Atom(eq), initializer));
      }
      else return tparam;
    }
    default:
      return 0;
  }
}

// template-parameter:
//   type-parameter
//   parameter-declaration
PT::Node *Parser::template_parameter(PT::Encoding &encoding)
{
  Trace trace("Parser::template_parameter", Trace::PARSING);
  PGuard<bool> pguard(*this, &Parser::my_gt_is_operator, false);
  switch (my_lexer.look_ahead())
  {
    case Token::TEMPLATE:
      return type_parameter(encoding);
    case Token::CLASS:
    case Token::TYPENAME:
    {
      // We don't know yet whether we are looking at a type.
      // 'class' could start an elaborated class specifier,
      // and 'typename' could be part of a dependent type specifier.
      Token::Type token = my_lexer.look_ahead(1);
      if (token == Token::Identifier)
	token = my_lexer.look_ahead(2);

      if (token == ',' || token == '=' || token == '>')
	return type_parameter(encoding);
      // else fall through.
    }
    default:
      return parameter_declaration(encoding);
  }
}

// template-parameter-list:
//   template-parameter
//   template-parameter-list , template-parameter
PT::TemplateParameterList *Parser::template_parameter_list(PT::Encoding &encoding)
{
  Trace trace("Parser::template_parameter_list", Trace::PARSING);
  PT::TemplateParameterList *tpl = new PT::TemplateParameterList;
  ScopeGuard scope_guard(*this, tpl);
  while (true)
  {
    PT::Node *parameter = template_parameter(encoding);
    if (!require(parameter, "template-parameter")) return 0;
    if (tpl->car()) tpl = PT::snoc(tpl, parameter);
    else tpl->set_car(parameter);
    if (my_lexer.look_ahead() != ',') break;
    tpl = PT::snoc(tpl, new PT::Atom(my_lexer.get_token()));
  }
  return tpl;
}

// template-declaration:
//   export [opt] template < template-parameter-list > declaration
PT::TemplateDeclaration *Parser::template_declaration()
{
  Trace trace("Parser::template_declaration", Trace::PARSING);

  PT::Kwd::Export *export_ = 0;
  if (my_lexer.look_ahead() == Token::EXPORT)
    export_ = new PT::Kwd::Export(my_lexer.get_token());

  if (!require(Token::TEMPLATE, "'template'")) return 0;
  Token templ = my_lexer.get_token();
  if (!require('<')) return 0;

  PT::TemplateDeclaration *tdecl = 
    new PT::TemplateDeclaration(new PT::Kwd::Template(templ));
  tdecl = PT::snoc(tdecl, new PT::Atom(my_lexer.get_token()));
  PT::Encoding encoding;
  {
    PT::TemplateParameterList *params = template_parameter_list(encoding);
    if (!require(params, "template-parameter-list") || !require('>')) return 0;
    tdecl = PT::conc(tdecl, PT::list(params, new PT::Atom(my_lexer.get_token())));
    my_template_parameter_list_seq.push_back(params);
  }
  PT::List *decl = declaration();
  my_template_parameter_list_seq.erase(my_template_parameter_list_seq.end() - 1);
  my_template_parameter_list_cursor = 0;
  if (!require(decl, "declaration")) return 0;
  tdecl = PT::snoc(tdecl, decl);
  return tdecl;
}

// explicit-specialization:
//   template < > declaration
PT::TemplateDeclaration *Parser::explicit_specialization()
{
  Trace trace("Parser::explicit_specialization", Trace::PARSING);

  // we are really looking for one of these:
  //   template < > decl-specifier [opt] init-declarator [opt] ;
  //   template < > function-definition
  //   template < > explicit-specialization
  //   template < > template-declaration

  if (my_lexer.look_ahead() != Token::TEMPLATE ||
      my_lexer.look_ahead(1) != '<' ||
      my_lexer.look_ahead(2) != '>')
    return 0;

  // FIXME: For now we tell the symbol factory about the following
  //        [class-specifier, function-definition] being a template
  //        by allocating a (dummy) template parameter scope.
  {
    PT::TemplateParameterList *tpl = new PT::TemplateParameterList;
    ScopeGuard scope_guard(*this, tpl);
  }

  Token template_ = my_lexer.get_token();
  // TODO: Don't abuse TemplateDeclaration here.
  PT::TemplateDeclaration *tdecl = 
    new PT::TemplateDeclaration(new PT::Kwd::Template(template_));
  tdecl = PT::snoc(tdecl, new PT::Atom(my_lexer.get_token()));
  tdecl = PT::snoc(tdecl, new PT::Atom(my_lexer.get_token()));

  if (my_lexer.look_ahead() == Token::TEMPLATE)
  {
    PT::Node *decl = 0;
    if (my_lexer.look_ahead(1) == '<' && my_lexer.look_ahead(2) != '>')
      decl = template_declaration();
    else
      decl = explicit_specialization();
    if (!require(decl, "template-declaration")) return 0;
    tdecl = PT::snoc(tdecl, decl);
  }
  else
  {
    Tentative tentative(*this);
    PT::Encoding type;
    PT::DeclSpec *spec = decl_specifier_seq(type);
    if (!spec) tentative.rollback();
    PT::List *declarators = 0;
    if (my_lexer.look_ahead() != ';')
    {
      PT::Declarator *decl = init_declarator(type);
      if (type.is_function() &&
	  starts_function_definition(my_lexer.look_ahead()))
      {
	PT::FunctionDefinition *func = new PT::FunctionDefinition(spec, PT::cons(decl));
	ScopeGuard scope_guard(*this, func);
	// TODO: parse function-try-block
	PT::Block *body = compound_statement();
	if (!require(body, "compound-statement")) return 0;
	func = PT::snoc(func, body);
	return PT::snoc(tdecl, func);
      }
      declarators = PT::cons(decl);
    }
    if (!require(';')) return 0;
    Token semic = my_lexer.get_token();
    tdecl = PT::snoc(tdecl,
		     new PT::SimpleDeclaration(spec, PT::list(declarators,
							      new PT::Atom(semic))));
  }
  return tdecl;
}

// explicit-instantiation:
//   template declaration
PT::TemplateInstantiation *Parser::explicit_instantiation()
{
  Trace trace("Parser::explicit_instantiation", Trace::PARSING);

  // We are really looking for:
  //   template decl-specifier-seq [opt] declarator [opt] ;
  //
  // Additionally, GCC allows an initial storage-class-specifier or
  // function-specifier.

  PT::Keyword *kwd = 0;
  if (my_ruleset & GCC)
  {
    PT::DeclSpec::StorageClass storage = PT::DeclSpec::UNDEF;
    kwd = opt_storage_class_specifier(storage);
    if (!kwd) kwd = opt_function_specifier();
  }
  if (my_lexer.look_ahead() != Token::TEMPLATE) return 0;
  PT::Atom *template_ = new PT::Atom(my_lexer.get_token());
  Tentative tentative(*this);
  PT::Encoding type;
  PT::DeclSpec *spec = decl_specifier_seq(type);
  if (!spec) tentative.rollback(true);
  PT::Declarator *decl = 0;
  {
    Tentative tentative(*this);
    PT::Encoding dummy_name;
    if (my_lexer.look_ahead() != ';') decl = declarator(type, dummy_name, NAMED);
    if (!decl) tentative.rollback();
  }
  if (!require(';')) return 0;
  PT::Atom *semic = new PT::Atom(my_lexer.get_token());
  return new PT::TemplateInstantiation(kwd, template_, spec, decl, semic);
}

// operator-name:
//   new delete new[] delete[] + - * / % ^ & | ~ ! = < >
//   += -= *= /= %= ^= &= |= << >> >>= <<= == != <= >= &&
//   || ++ -- , ->* -> () []
PT::Node *Parser::operator_name(PT::Encoding &encoding)
{
  Trace trace("Parser::operator_name", Trace::PARSING);

  switch (my_lexer.look_ahead())
  {
    case Token::NEW:
    case Token::DELETE:
    {
      Token op = my_lexer.get_token();
      if (my_lexer.look_ahead() != '[')
      {
	PT::Atom *name = 0;
	if (op.type == Token::NEW) name = new PT::Kwd::New(op);
	else name = new PT::Kwd::Delete(op);
	encoding.simple_name(name);
	return name;
      }
      else
      {
	PT::List *name = 0;
	if (op.type == Token::NEW)
	{
	  encoding.append_with_length("new[]", 5);
	  name = PT::cons(new PT::Kwd::New(op));
	}
	else
	{
	  encoding.append_with_length("delete[]", 8);
	  name = PT::cons(new PT::Kwd::Delete(op));
	}
	Token os = my_lexer.get_token();
	name = PT::snoc(name, new PT::Atom(os));
	if (!require(']')) return 0;
	Token cs = my_lexer.get_token();
	return PT::snoc(name, new PT::Atom(cs));
      }
    }
    case '(':
    {
      PT::List *name = PT::cons(new PT::Atom(my_lexer.get_token()));
      if (!require(')')) return 0;
      Token cp = my_lexer.get_token();
      encoding.append_with_length("()", 2);
      return PT::snoc(name, new PT::Atom(cp));
    }
    case '[':
    {
      PT::List *name = PT::cons(new PT::Atom(my_lexer.get_token()));
      if (!require(']')) return 0;
      Token cs = my_lexer.get_token();
      encoding.append_with_length("[]", 2);
      return PT::snoc(name, new PT::Atom(cs));
    }
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '^':
    case '&':
    case '|':
    case '~':
    case '!':
    case '=':
    case '<':
    case '>':
    case Token::AssignOp:
    case Token::ShiftOp:
    case Token::EqualOp:
    case Token::RelOp:
    case Token::LogAndOp:
    case Token::LogOrOp:
    case Token::IncOp:
    case ',':
    case Token::PmOp:
    case Token::ArrowOp:
    {
      PT::Atom *name = new PT::Atom(my_lexer.get_token());
      encoding.simple_name(name);
      return name;
    }
    default:
      error("expected operator-name");
      return 0;
  }
}

// operator-function-id:
//   operator op
PT::List *Parser::operator_function_id(PT::Encoding &encoding)
{
  Trace trace("Parser::operator_function_id", Trace::PARSING);
  if (!require(Token::OPERATOR, "'operator'")) return 0;
  Token token = my_lexer.get_token();
  PT::Node *name = operator_name(encoding);
  if (!require(name, "operator-name")) return 0;
  return PT::cons(new PT::Kwd::Operator(token), PT::cons(name));
}

// conversion-function-id:
//   operator conversion-type-id
//
// conversion-type-id:
//   type-specifier-seq conversion-declarator [opt]
//
// conversion-declarator:
//   ptr-operator conversion-declarator [opt]
PT::List *Parser::conversion_function_id(PT::Encoding &encoding)
{
  Trace trace("Parser::operator_conversion_id", Trace::PARSING);
  if (!require(Token::OPERATOR, "'operator'")) return 0;
  PT::Kwd::Operator *op = new PT::Kwd::Operator(my_lexer.get_token());
  PT::Encoding type;
  PT::List *spec = type_specifier_seq(type);
  if (!require(spec, "type-specifier-seq")) return 0;
  PT::List *decl = 0;
  while (true)
  {
    Tentative tentative(*this, type);
    PT::Node *ptr = ptr_operator(type);
    if (!ptr) break;
    decl = PT::snoc(decl, ptr);
  }
  PT::List *id = PT::cons(op, spec);
  if (decl) id = PT::conc(id, decl);
  encoding.cast_operator(type);
  return id;
}

// template-name:
//   identifier
//   operator-function-id [GCC]
PT::Node *Parser::template_name(PT::Encoding &encoding,
				bool do_lookup)
{
  Trace trace("Parser::template_name", Trace::PARSING);
  if (my_lexer.look_ahead() == Token::OPERATOR)
  {
    Token token = my_lexer.get_token();
    PT::Kwd::Operator *operator_ = new PT::Kwd::Operator(token);
    PT::Node *name = operator_function_id(encoding);
    if (name) return PT::list(operator_, name);
    else return 0;
  }
  else
  {
    PT::Identifier *id = identifier(encoding);
    if (id && (!do_lookup || lookup_template_name(encoding))) return id;
    else return 0;
  }
}

// template-argument:
//   assignment-expression
//   type-id
//   id-expression
PT::Node *Parser::template_argument(PT::Encoding &encoding)
{
  Trace trace("Parser::template_argument", Trace::PARSING);
  PGuard<bool> pguard(*this, &Parser::my_gt_is_operator, false);

  // [temp.arg]
  //
  // In a template-argument, an ambiguity between a type-id and an
  // expression is resolved to a type-id, regardless of the form of
  // the corresponding template-parameter.

  Tentative tentative(*this, encoding);
  PT::Node *argument = type_id(encoding);
  if (argument)
  {
    // Did we see the end of the argument ?
    if (my_lexer.look_ahead() != ',' && my_lexer.look_ahead() != '>') return 0;
    else return argument;
  }
  tentative.rollback();
  argument = conditional_expression();
  if (argument)
  {
    encoding.value_temp_param();
    return argument;
  }
  tentative.rollback();
  return id_expression(encoding, false);
}

// template-id:
//   template-name < template-argument-list [opt] >
//
// template-argument-list:
//   template-argument
//   template-argument-list , template-argument
PT::List *Parser::template_id(PT::Encoding &encoding, bool is_template)
{
  Trace trace("Parser::template_id", Trace::PARSING);
  // Parse a template-name, but don't look it up if a 'template'
  // token preceeded it.
  PT::Encoding name_encoding;
  PT::Node *name = template_name(name_encoding, !is_template);
  if (!name) return 0;
  Token ob = my_lexer.get_token();
  if (ob.type != '<') return 0;
  if (my_lexer.look_ahead() == '>')
  {
    Token cb = my_lexer.get_token();
    if (name->is_atom())
      encoding.template_(static_cast<PT::Identifier *>(name), PT::Encoding());
    else
    {
      // name is [operator operator-name]
      PT::List *list = static_cast<PT::List *>(name);
      PT::Identifier *oname = static_cast<PT::Identifier *>(PT::nth<1>(list));
      encoding.template_(oname, PT::Encoding());
    }
    return PT::list(name, new PT::Atom(ob), new::PT::Atom(cb));
  }
  else
  {
    PGuard<ST::Scope *> guard1(*this, &Parser::my_qualifying_scope, 0);
    PGuard<bool> guard2(*this, &Parser::my_in_nested_name_specifier, true);
    PT::List *arguments = 0;
    PT::Encoding template_encoding;
    while (true)
    {
      PT::Encoding encoding;
      PT::Node *argument = template_argument(encoding);
      if (!argument) return 0;
      arguments = PT::snoc(arguments, argument);
      template_encoding.append(encoding);
      if (my_lexer.look_ahead() != ',') break;
      Token comma =  my_lexer.get_token();
      arguments = PT::snoc(arguments, new PT::Atom(comma));
    }
    if (name->is_atom())
      encoding.template_(static_cast<PT::Atom *>(name), template_encoding);
    else
    {
      // name is [operator operator-name]
      PT::List *list = static_cast<PT::List *>(name);
      PT::Identifier *oname = static_cast<PT::Identifier *>(PT::nth<1>(list));
      encoding.template_(oname, template_encoding);
    }
    Token cb = my_lexer.get_token();
    switch(cb.type)
    {
      case '>':
	return PT::cons(name, PT::list(new PT::Atom(ob), arguments, new PT::Atom(cb)));
      case Token::ShiftOp: // parse error !
      default: return 0;
    }
  }
}

// expression:
//   assignment-expression
//   expression , assignment-expression
PT::Node *Parser::expression()
{
  Trace trace("Parser::expression", Trace::PARSING);

  PT::Node *expr = assignment_expression();
  if(!expr)
    return 0;
  while(my_lexer.look_ahead() == ',')
  {
    Token comma = my_lexer.get_token();
    PT::Node *right = assignment_expression();
    if (!right)
      return 0;
    else
      expr = new PT::Expression(expr, PT::list(new PT::Atom(comma), right));
  }
  return expr;
}

// assignment-expression:
//   conditional-expression
//   logical-or-expression assignment-operator assignment-expression
//   throw-expression
PT::Node *Parser::assignment_expression()
{
  Trace trace("Parser::assignment_expression", Trace::PARSING);
  if (my_lexer.look_ahead() == Token::THROW) return throw_expression();

  PT::Node *expr;
  PT::Node *left = conditional_expression();
  if (!left) return 0;
  Token::Type type = my_lexer.look_ahead();
  if(type != '=' && type != Token::AssignOp) expr = left;
  else
  {
    Token op = my_lexer.get_token();
    PT::Node *right = assignment_expression();
    if (!right) return 0;
    
    expr = new PT::AssignExpr(left, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// constant-expression:
//   conditional-expression
PT::Node *Parser::constant_expression()
{
  Trace trace("Parser::constant_expression", Trace::PARSING);
  PGuard<bool> pguard(*this, &Parser::my_in_constant_expression, true);
  PT::Node *expression = conditional_expression();
  if (!expression) return 0;
  // TODO: evaluate constant
  else return expression;
}

// conditional-expression:
//   logical-or-expression
//   logical-or-expression ? expression : assignment-expression
PT::Node *Parser::conditional_expression()
{
  Trace trace("Parser::conditional_expression", Trace::PARSING);

  PT::Node *cond = logical_or_expression();
  if (!cond) return 0;
  if(my_lexer.look_ahead() == '?')
  {
    Token quest = my_lexer.get_token();
    PT::Node *then = expression();
    if (!then) return 0;
    Token colon = my_lexer.get_token();
    if (colon.type != ':') return 0;
    PT::Node *otherwise = assignment_expression();
    if (!otherwise) return 0;

    return new PT::CondExpr(cond, PT::list(new PT::Atom(quest), then,
					   new PT::Atom(colon), otherwise));
  }
  return cond;
}

// logical-or-expression:
//   logical-and-expression
//   logical-or-expression || logical-and-expression
PT::Node *Parser::logical_or_expression()
{
  Trace trace("Parser::logical_or_expression", Trace::PARSING);

  PT::Node *expr = logical_and_expression();
  if (!expr) return 0;

  while(my_lexer.look_ahead() == Token::LogOrOp)
  {
    Token lor = my_lexer.get_token();
    PT::Node *right = logical_and_expression();
    if (!right) return 0;
    
    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(lor), right));
  }
  return expr;
}

// logical-and-expression:
//   inclusive-or-expression
//   logical-and-expr && inclusive-or-expression
PT::Node *Parser::logical_and_expression()
{
  Trace trace("Parser::logical_and_expression", Trace::PARSING);

  PT::Node *expr = inclusive_or_expression();
  if (!expr) return 0;

  while(my_lexer.look_ahead() == Token::LogAndOp)
  {
    Token land = my_lexer.get_token();
    PT::Node *right = inclusive_or_expression();
    if (!right)
      return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(land), right));
  }
  return expr;
}

// inclusive-or-expression:
//   exclusive-or-expression
//   inclusive-or-expression | exclusive-or-expression
PT::Node *Parser::inclusive_or_expression()
{
  Trace trace("Parser::inclusive_or_expression", Trace::PARSING);

  PT::Node *expr = exclusive_or_expression();
  if (!expr)
    return 0;

  while(my_lexer.look_ahead() == '|')
  {
    Token bor = my_lexer.get_token();
    PT::Node *right = exclusive_or_expression();
    if (!right)
      return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(bor), right));
  }
  return expr;
}

// exclusive-or-expression:
//   and-expression
//   exclusive-or-expression ^ and-expression
PT::Node *Parser::exclusive_or_expression()
{
  Trace trace("Parser::exclusive_or_expression", Trace::PARSING);

  PT::Node *expr = and_expression();
  if (!expr) return 0;

  while(my_lexer.look_ahead() == '^')
  {
    Token op = my_lexer.get_token();
    PT::Node *right = and_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// and-expression:
//   equality-expression
//   and-expression & equality-expression
PT::Node *Parser::and_expression()
{
  Trace trace("Parser::and_expression", Trace::PARSING);

  PT::Node *expr = equality_expression();
  if (!expr) return 0;

  while(my_lexer.look_ahead() == '&')
  {
    Token op = my_lexer.get_token();
    PT::Node *right = equality_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// equality-expression:
//   relational-expression
//   equality-expression == relational-expression
//   equality-expression != relational-expression
PT::Node *Parser::equality_expression()
{
  Trace trace("Parser::equality_expression", Trace::PARSING);

  PT::Node *expr = relational_expression();
  if (!expr) return 0;
  while(my_lexer.look_ahead() == Token::EqualOp)
  {
    Token op = my_lexer.get_token();
    PT::Node *right = relational_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// relational-expression:
//   shift-expression
//   relational-expression < shift-expression
//   relational-expression > shift-expression
//   relational-expression <= shift-expression
//   relational-expression >= shift-expression
PT::Node *Parser::relational_expression()
{
  Trace trace("Parser::relational_expression", Trace::PARSING);

  PT::Node *expr = shift_expression();
  if (!expr) return 0;

  Token::Type t;
  while(t = my_lexer.look_ahead(),
	(t == Token::RelOp || t == '<' || (t == '>' && my_gt_is_operator)))
  {
    Token op = my_lexer.get_token();
    PT::Node *right = shift_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// shift-expression:
//   additive-expression
//   shift-expression << additive-expression
//   shift-expression >> additive-expression
PT::Node *Parser::shift_expression()
{
  Trace trace("Parser::shift_expression", Trace::PARSING);

  PT::Node *expr = additive_expression();
  if (!expr) return 0;

  while(my_lexer.look_ahead() == Token::ShiftOp)
  {
    Token op = my_lexer.get_token();
    PT::Node *right = additive_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// additive-expression:
//   multiplicative-expression
//   additive-expression + multiplicative-expression
//   additive-expression - multiplicative-expression
PT::Node *Parser::additive_expression()
{
  Trace trace("Parser::additive_expression", Trace::PARSING);

  PT::Node *expr = multiplicative_expression();
  if (!expr) return 0;

  Token::Type t;
  while(t = my_lexer.look_ahead(), (t == '+' || t == '-'))
  {
    Token op = my_lexer.get_token();
    PT::Node *right = multiplicative_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// multiplicative-expression:
//   pm-expression
//   multiplicative-expression * pm-expression
//   multiplicative-expression / pm-expression
//   multiplicative-expression % pm-expression
PT::Node *Parser::multiplicative_expression()
{
  Trace trace("Parser::multiplicative_expression", Trace::PARSING);

  PT::Node *expr = pm_expression();
  if (!expr) return 0;

  Token::Type t;
  while(t = my_lexer.look_ahead(), (t == '*' || t == '/' || t == '%'))
  {
    Token op = my_lexer.get_token();
    PT::Node *right = pm_expression();
    if (!right) return 0;

    expr = new PT::InfixExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// pm-expression:
//   cast-expression
//   pm-expression .* cast-expression
//   pm-expression ->* cast-expression
PT::Node *Parser::pm_expression()
{
  Trace trace("Parser::pm_expression", Trace::PARSING);

  PT::Node *expr = cast_expression();
  if (!expr) return 0;
  while(my_lexer.look_ahead() == Token::PmOp)
  {
    Token op = my_lexer.get_token();
    PT::Node *right = cast_expression();
    if (!right) return 0;
    expr = new PT::PmExpr(expr, PT::list(new PT::Atom(op), right));
  }
  return expr;
}

// cast-expression:
//   unary-expression
//   ( type-id ) cast-expression
PT::Node *Parser::cast_expression()
{
  Trace trace("Parser::cast_expression", Trace::PARSING);
  if(my_lexer.look_ahead() != '(')
  {
    return unary_expression();
  }
  else
  {
    Tentative tentative(*this);
    Token op = my_lexer.get_token();
    PT::Encoding encoding;
    PT::Node *name = type_id(encoding);
    if (name && my_lexer.look_ahead() == ')')
    {
      Token cp = my_lexer.get_token();
      PT::Node *expr = cast_expression();
      if(expr)
	return new PT::CastExpr(new PT::Atom(op),
				PT::list(name, new PT::Atom(cp), expr));
    }
    tentative.rollback();
    return unary_expression();
  }
}

// type-id:
//   type-specifier-seq abstract-declarator [opt]
PT::Node *Parser::type_id(PT::Encoding &encoding)
{
  Trace trace("Parser::type_id", Trace::PARSING);
  
  PT::Node *type_spec = type_specifier_seq(encoding);
  if (!require(type_spec, "type-specifier-seq")) return 0;
  Tentative tentative(*this, encoding);
  PT::Encoding dummy_name;
  PT::Declarator *decl = declarator(encoding, dummy_name, ABSTRACT);
  if (!decl) tentative.rollback();
  // FIXME: what's the right node here ?
  return PT::list(type_spec, decl);
}

// type-id-list:
//   type-id
//   type-id-list , type-id
PT::List *Parser::type_id_list()
{
  Trace trace("Parser::type_id_list", Trace::PARSING);
  PT::List *types = 0;
  while (true)
  {
    PT::Encoding encoding;
    PT::Node *type = type_id(encoding);
    if (!require(type, "type-id")) return 0;
    if (!types) types = PT::cons(type);
    else types = PT::snoc(types, type);
    if (my_lexer.look_ahead() != ',') break;
    types = PT::snoc(types, new PT::Atom(my_lexer.get_token()));
  }
  return types;
}

// unary-expression:
//   postfix-expression
//   ++ cast-expression
//   -- cast-expression
//   unary-operator cast-expression
//   sizeof unary-expression
//   sizeof ( type-id )
//   new-expression
//   delete-expression
//
// unary-operator:
//   *
//   &
//   +
//   -
//   !
//   ~
PT::Node *Parser::unary_expression()
{
  Trace trace("Parser::unary_expression", Trace::PARSING);
  switch (my_lexer.look_ahead())
  {
    case '*':
    case '&': 
    case '+':
    case '-':
    case '!':
    case '~':
    case Token::IncOp:
    {
      Token op = my_lexer.get_token();
      PT::Node *right = cast_expression();
      if (!right) return 0;
      else return new PT::UnaryExpr(new PT::Atom(op), PT::cons(right));
    }
    case Token::SIZEOF: return sizeof_expression();
    case Token::NEW: return new_expression();
    case Token::DELETE: return delete_expression();
    case Token::Scope:
      if (my_lexer.look_ahead(1) == Token::NEW) return new_expression();
      else if (my_lexer.look_ahead(1) == Token::DELETE) return delete_expression();
      // Fall through.
    default: return postfix_expression();
  }
}

// throw-expression:
//   throw assignment-expression [opt]
PT::Node *Parser::throw_expression()
{
  Trace trace("Parser::throw_expression", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::THROW)
    return 0;

  PT::Kwd::Throw *throw_ = new PT::Kwd::Throw(my_lexer.get_token());

  Token::Type type = my_lexer.look_ahead();
  if (type == ',' ||
      type == ';' ||
      type == ')' ||
      type == ']' ||
      type == '}' ||
      type == ':')
    return new PT::ThrowExpr(throw_, PT::cons(0));
  else
  {
    PT::Node *expr = assignment_expression();
    if (!require(expr, "assignment-expression")) return 0;
    return new PT::ThrowExpr(throw_, PT::cons(expr));
  }
}

// sizeof-expression:
//   sizeof unary-expression
//   sizeof ( type-id )
PT::Node *Parser::sizeof_expression()
{
  Trace trace("Parser::sizeof_expression", Trace::PARSING);

  Token kwd = my_lexer.get_token();
  if (kwd.type != Token::SIZEOF) return 0;
  if(my_lexer.look_ahead() == '(')
  {
    Tentative tentative(*this);
    Token op = my_lexer.get_token();
    PT::Encoding encoding;
    PT::Node *name = type_id(encoding);
    if (name && my_lexer.look_ahead() == ')')
    {
      Token cp = my_lexer.get_token();
      return new PT::SizeofExpr(new PT::Atom(kwd),
				PT::list(new PT::Atom(op), name, new PT::Atom(cp)));
    }
    else tentative.rollback();
  }
  PT::Node *unary = unary_expression();
  trace << "got " << unary;
  if (!unary) return 0;
  else return new PT::SizeofExpr(new PT::Atom(kwd), PT::cons(unary));
}

// new-expression:
//   :: [opt] new new-placement [opt] new-type-id new-initializer [opt]
//   :: [opt] new new-placement [opt] ( type-id ) new-initializer [opt]
//
// new-placement:
//   ( expression-list )
//
// new-type-id:
//   type-specifier-seq new-declarator [opt]
//
// new-declarator:
//   ptr-operator new-declarator [opt]
//   direct-new-declarator
//
// direct-new-declarator:
//   [ expression ]
//   direct-new-declarator [constant-expression]
PT::NewExpr *Parser::new_expression()
{
  Trace trace("Parser::new_expression", Trace::PARSING);
  PT::Encoding encoding;
  PT::Atom *scope = opt_scope(encoding);
  if (!require(Token::NEW, "'new'")) return 0;
  PT::Kwd::New *new_ = new PT::Kwd::New(my_lexer.get_token());
  PT::List *placement = 0;
  if (my_lexer.look_ahead() == '(')
  {
    // try a new-placement
    Tentative tentative(*this);
    Token op = my_lexer.get_token();
    PT::List *expr = expression_list(false);
    if (expr && my_lexer.look_ahead() == ')')
      placement = PT::list(new PT::Atom(op), expr, new PT::Atom(my_lexer.get_token()));
    else tentative.rollback();
  }
  PT::Node *type = 0;
  // FIXME: complete new-expression 
  while (my_lexer.look_ahead() != ';') my_lexer.get_token();
  return new PT::NewExpr(scope, PT::list(new_, placement, type));
}

// delete-expression:
//   :: [opt] delete cast-expression
//   :: [opt] delete [ ] cast-expression
PT::Node *Parser::delete_expression()
{
  Trace trace("Parser::delete_expression", Trace::PARSING);
  PT::Encoding encoding;
  PT::Atom *scope = opt_scope(encoding);
  if (!require(Token::DELETE, "'delete'")) return 0;
  PT::Kwd::Delete *delete_ = new PT::Kwd::Delete(my_lexer.get_token());

  PT::List *dexp = scope 
    ? new PT::DeleteExpr(scope, PT::cons(delete_))
    : new PT::DeleteExpr(delete_, 0);

  if (my_lexer.look_ahead() == '[')
  {
    dexp = PTree::snoc(dexp, new PT::Atom(my_lexer.get_token()));
    if (!require(']')) return 0;
    dexp = PTree::snoc(dexp, new PT::Atom(my_lexer.get_token()));
  }
  PT::Node *exp = cast_expression();
  if(!require(exp, "cast-expression")) return 0;
  return PTree::snoc(dexp, exp);
}

// expression-list:
//   assignment-expression
//   expression-list, assignment-expression
PT::List *Parser::expression_list(bool is_const)
{
  Trace trace("Parser::expression_list", Trace::PARSING);
  PT::List *exprs = 0;
  while (true)
  {
    PT::Node *expr = is_const ? constant_expression() : assignment_expression();
    if (!expr)
    {
      if (is_const)
	error("expected constant-expression");
      else
	error("expected assignment-expression");
      return 0;
    }
    exprs = PT::snoc(exprs, expr);
    if (my_lexer.look_ahead() != ',') break;
    else exprs = PT::snoc(exprs, new PT::Atom(my_lexer.get_token()));
  }
  return exprs;
}

PT::List *Parser::functional_cast()
{
  Trace trace("Parser::functional_cast", Trace::PARSING);
  
  if (my_lexer.look_ahead() != '(')
  {
    error("expecting ')'");
    return 0;
  }
  PT::List *args = PT::cons(new PT::Atom(my_lexer.get_token()));
  if (my_lexer.look_ahead() != ')')
  {
    PGuard<bool> pguard(*this, &Parser::my_in_functional_cast, true);
    PT::List *expr = expression_list(/*is_const=*/false);
    if (!require(expr, "expression-list")) return 0;
    args = PT::snoc(args, expr);
  }
  if (!require(')')) return 0;
  return PT::snoc(args, new PTree::Atom(my_lexer.get_token()));
}

// pseudo-destructor-name:
//   :: [opt] nested-name-specifier [opt] type-name :: ~ type-name
//   :: [opt] nested-name-specifier template template-id :: ~ type-name
//   :: [opt] nested-name-specifier [opt] ~ type-name
PT::List *Parser::pseudo_destructor_name()
{
  Trace trace("Parser::pseudo_destructor_name", Trace::PARSING);
  PT::Encoding encoding;
  PT::Atom *scope = opt_scope(encoding);
  Tentative tentative(*this);
  PT::List *nested_name = nested_name_specifier(encoding);
  if (!nested_name) tentative.rollback();
  if (nested_name && my_lexer.look_ahead() == Token::TEMPLATE)
  {
    PT::Atom *template_ = new PT::Kwd::Template(my_lexer.get_token());
    nested_name = PT::snoc(nested_name, template_);
    PT::List *id = template_id(encoding, true);
    if (!require(id, "template-id") || !require(Token::Scope, "::")) return 0;
    PT::Atom *scope = new PT::Atom(my_lexer.get_token());
    nested_name = PT::snoc(nested_name, id);
    nested_name = PT::snoc(nested_name, scope);
  }
  else if (my_lexer.look_ahead() != '~')
  {
    PT::Node *name = type_name(encoding);
    if (!require(name, "type-name") || !require(Token::Scope, "::")) return 0;
    PT::Atom *scope = new PT::Atom(my_lexer.get_token());
    nested_name = PT::snoc(nested_name, name);
    nested_name = PT::snoc(nested_name, scope);
  }
  if (!require('~')) return 0;
  nested_name = PT::snoc(nested_name, new PT::Atom(my_lexer.get_token()));
  PT::Node *name = type_name(encoding);
  if (!require(name, "type-name")) return 0;
  nested_name = PT::snoc(nested_name, name);
  return new PT::Name(nested_name, encoding);
}

// postfix-expression:
//   primary-expression
//   postfix-expression [ expression ]
//   postfix-expression ( expression-list [opt] )
//   simple-type-specifier ( expression-list [opt] )
//   typename :: [opt] nested-name-specifier identifier
//     ( expression-list [opt] )
//   typename :: [opt] nested-name-specifier template [opt] template-id
//     ( expression-list [opt] )
//   postfix-expression . template [opt] id-expression
//   postfix-expression -> template [opt] id-expression
//   postfix-expression . pseudo-destructor-name
//   postfix-expression -> pseudo-destructor-name
//   postfix-expression ++
//   postfix-expression --
//   dynamic_cast < type-id > ( expression )
//   static_cast < type-id > ( expression )
//   reinterpret_cast < type-id > ( expression )
//   const_cast < type-id > ( expression )
//   typeid ( expression )
//   typeid ( type-id )
PT::Node *Parser::postfix_expression()
{
  Trace trace("Parser::postfix_expression", Trace::PARSING);

  PT::Node *expr = 0;

  switch (my_lexer.look_ahead())
  {
    case Token::TYPEID:
    {
      PT::Node *expr = 0;
      error("sorry, typeid not supported yet");
      return 0;
      //       return typeid_expression(expr) ? expr : 0;
    }
    case Token::TYPENAME:
    {
      PT::Kwd::Typename *typename_ = new PT::Kwd::Typename(my_lexer.get_token());
      PT::Encoding encoding;
      PT::Atom *scope = opt_scope(encoding);
      PT::List *qname = nested_name_specifier(encoding);
      if (!require(qname, "nested-name-specifier")) return 0;
      if (scope) qname = PT::snoc(PT::cons(scope), qname);
      if (my_lexer.look_ahead() == Token::TEMPLATE)
      {
	// We are looking at a template-id
	PT::Node *id = template_id(encoding, /*is_template=*/true);
	if (!require(id, "template-id")) return 0;
	qname = PT::snoc(qname, id);
      }
      else
      {
	// We may be looking at an identifier or a template-id.
	// The second-next token will tell us which:
	if (my_lexer.look_ahead(1) == '<')
	{
	  PT::Node *id = template_id(encoding, /*is_template*/false);
	  if (!id) return 0;
	  else qname = PT::snoc(qname, id);
	}
	else
	{
	  PT::Atom *id = identifier(encoding);
	  if (!require(id, "identifier")) return 0;
	  qname = PT::snoc(qname, id);
	}
      }
      // functional-cast
      PT::List *args = functional_cast();
      if (!args) return 0;
      else expr = new PTree::FstyleCastExpr(encoding, qname, args);
      break;
    }
    case Token::CONST_CAST:
    case Token::DYNAMIC_CAST:
    case Token::STATIC_CAST:
    case Token::REINTERPRET_CAST:
    {
      // FIXME: This should become a PT::Keyword
      PT::Atom *kwd = new PT::Atom(my_lexer.get_token());
      if (!require('<')) return 0;
      Token ob = my_lexer.get_token();
      PT::Encoding type;
      PT::Node *id = type_id(type);
      if (!require(id, "type-id") || !require('>')) return 0;
      Token cb = my_lexer.get_token();
      if (!require('(')) return 0;
      Token op = my_lexer.get_token();
      PT::Node *expr = expression();
      if (!require(expr, "expression") || !require(')')) return 0;
      Token cp = my_lexer.get_token();
      return new PT::FstyleCastExpr(type, kwd,
				    PT::list(new PT::Atom(ob),
					     id,
					     new PT::Atom(cb),
					     new PT::Atom(op),
					     expr,
					     new PT::Atom(cp)));
    }
    default:
    {
      Tentative tentative(*this);
      PT::Encoding encoding;
      bool user_defined = true;
      PT::Node *type = simple_type_specifier(encoding, user_defined);
      if (type)
      {
	PT::List *args = functional_cast();
	if (args)
	{
	  expr = new PTree::FstyleCastExpr(encoding, type, args);	
	  break;
	}
      }
      tentative.rollback();
      expr = primary_expression();
      if (!require(expr, "primary-expression")) return 0;
      break;
    }
  }

  while(true)
  {
    switch(my_lexer.look_ahead())
    {
      case '[':
      {
	Token os = my_lexer.get_token();
	PT::Node *e = expression();
	if (!require(e, "expression") ||
	    !require(']'))
	  return 0;
	Token cs = my_lexer.get_token();
	expr = new PT::ArrayExpr(expr, PT::list(new PT::Atom(os),
						e,
						new PT::Atom(cs)));
	break;
      }
      case '(':
      {
	Token op = my_lexer.get_token();
	PT::Node *args = 0;
	if (my_lexer.look_ahead() != ')')
	{
	  args = expression_list(/*is_const*/false);
	  if (!require(args, "expression-list")) return 0;
	}
	if (!require(')')) return 0;
	Token cp = my_lexer.get_token();
	expr = new PT::FuncallExpr(expr, PT::list(new PT::Atom(op),
						  args,
						  new PT::Atom(cp)));
	break;
      }
      case Token::IncOp:
      {
	Token op = my_lexer.get_token();
	expr = new PT::PostfixExpr(expr, PT::cons(new PT::Atom(op)));
	break;
      }
      case '.':
      case Token::ArrowOp:
      {
	Token op = my_lexer.get_token();
	PT::List *lhs = 0;
	PT::Keyword *template_ = 0;
	if (my_lexer.look_ahead() == Token::TEMPLATE)
	  template_ = new PT::Kwd::Template(my_lexer.get_token());
	if (!template_)
	{
	  Tentative tentative(*this);
	  lhs = pseudo_destructor_name();
	  if (!lhs) tentative.rollback();
	}
	if (!lhs)
	{
	  PT::Encoding encoding;
	  PT::Node *id = id_expression(encoding, template_);
	  if (!require(id, "id-expression")) return 0;
	  if (template_) lhs = PT::cons(template_, PT::cons(id));
	  else lhs = PT::cons(id);
	}
 	if(op.type == '.')
 	  expr = new PT::DotMemberExpr(expr, PT::list(new PT::Atom(op), lhs));
 	else
 	  expr = new PT::ArrowMemberExpr(expr, PT::list(new PT::Atom(op), lhs));
	break;
      }
      default:
	return expr;
    }
  }
}

// primary-expression:
//   literal
//   this
//   ( expression )
//   id-expression
PT::Node *Parser::primary_expression()
{
  Trace trace("Parser::primary_expression", Trace::PARSING);

  switch(my_lexer.look_ahead())
  {
    case Token::Constant:
    case Token::CharConst:
    case Token::WideCharConst:
    case Token::StringL:
    case Token::WideStringL:
      return new PT::Literal(my_lexer.get_token());
    case Token::THIS:
      return new PT::Kwd::This(my_lexer.get_token());
    case '(':
    {
      PGuard<bool> pguard(*this, &Parser::my_gt_is_operator, true);
      Token op = my_lexer.get_token();
      PT::Node *expr = expression();
      if (!expr || !require(')')) return 0;
      Token cp = my_lexer.get_token();
      return new PT::ParenExpr(new PT::Atom(op),
			       PT::list(expr, new PT::Atom(cp)));
    }
    default:
    {
      PT::Encoding encoding;
      return id_expression(encoding, false);
    }
  }
}

// id-expression:
//   unqualified-id
//   qualified-id
//
// qualified-id:
//   :: [opt] nested-name-specifier template [opt] unqualified-id
//   :: identifier
//   :: operator-function-id
//   :: template-id
PT::Node *Parser::id_expression(PT::Encoding &encoding, bool is_template)
{
  Trace trace("Parser::id_expression", Trace::PARSING);
  PT::Atom *scope = opt_scope(encoding);
  Tentative tentative(*this, encoding);
  PT::List *name = nested_name_specifier(encoding, is_template);
  if (name)
  {
    bool is_template = false;
    if (my_lexer.look_ahead() == Token::TEMPLATE)
    {
      Token token = my_lexer.get_token();
      name = PT::snoc(name, new PT::Kwd::Template(token));
      is_template = true;
    }
    PT::Node *rest = unqualified_id(encoding, is_template);
    if (!require(rest, "unqualified-id")) return 0;
    else return new PT::Name(PT::snoc(name, rest), encoding);
  }
  tentative.rollback(true);

  if (!scope)
  {
    PT::Node *id = unqualified_id(encoding, is_template);
    return id ? id->is_atom() ? id : new PT::Name(id, encoding) : 0;
  }
  if (my_lexer.look_ahead() == Token::Identifier)
  {
    PT::List *id = PT::cons(scope, PT::cons(identifier(encoding)));
    return new PT::Name(id, encoding);
  }
  name = operator_function_id(encoding);
  if (name) return name;
  tentative.rollback();
  PT::List *id = template_id(encoding, is_template);
  return id ? new PT::Name(id, encoding) : 0;
}

// typeof-expression:
//   __typeof__ unary-expression
//   __typeof__ ( type-id )
PT::Node *Parser::typeof_expression(PT::Encoding &encoding)
{
  Trace trace("Parser::typeof_expression", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::TYPEOF) return 0;
  Token typeof_ = my_lexer.get_token();
  if (my_lexer.look_ahead(1) == '(')
  {
    // Try to parse this as a type-id.
    Tentative tentative(*this, encoding);
    Token op = my_lexer.get_token();
    PT::Node *type = type_id(encoding);
    if (type && require(')')) 
      return new PT::TypeofExpr(new PT::Atom(typeof_), 
				PT::list(new PT::Atom(op),
					 type, 
					 new PT::Atom(my_lexer.get_token())));
    else tentative.rollback();
  }
  PT::Node *type = unary_expression();
  if (!require(type, "unary-expression")) return 0;
  // FIXME: To be able to defer type analysis, we need to be able to express
  //        a 'to-be-evaluated' state in the encoding. For now use anonymous...
  encoding.anonymous();
  return new PT::TypeofExpr(new PT::Atom(typeof_), PT::cons(type));
}

// condition:
//   expression
//   type-specifier-seq declarator = assignment-expression
PT::Node *Parser::condition()
{
  Trace trace("Parser::condition", Trace::PARSING);

  Tentative tentative(*this);
  PT::Encoding type;
  PT::List *cond = type_specifier_seq(type);
  if (cond)
  {
    PT::Encoding dummy_name;
    PT::Declarator *decl = declarator(type, dummy_name, NAMED);
    if (my_lexer.look_ahead() == '=')
    {
      Token eq = my_lexer.get_token();
      PT::Node *expr = assignment_expression();
      if (!require(expr, "assignment-expression")) return 0;
      cond = PT::cons(cond);
      cond = PT::snoc(cond, decl);
      cond = PT::snoc(cond, new PT::Atom(eq));
      return PT::snoc(cond, expr);
    }
  }
  tentative.rollback();
  PT::Node *expr = expression();
  if (!require(expr, "expression")) return 0;
  return PT::cons(expr);
}

////////////////////////////////////////////////////////////////////////////////
// statements [gram.stmt.stmt]
////////////////////////////////////////////////////////////////////////////////

// labeled-statement:
//   identifier : statement
//   case constant-expression : statement
//   default : statement
PT::List *Parser::labeled_statement()
{
  Trace trace("Parser::labeled_statement", Trace::PARSING);
  Token kwd = my_lexer.get_token();
  assert(kwd.type == Token::CASE ||
	 kwd.type == Token::DEFAULT ||
	 kwd.type == Token::Identifier);

  switch (kwd.type)
  {
    case Token::CASE:
    {
      PT::Node *expr = constant_expression();
      if (!require(expr, "constant-expression")) return 0;
      if (!require(':')) return 0;
      Token colon = my_lexer.get_token();
      PT::Node *stmt = statement();
      if (!require(stmt, "statement")) return 0;

      return new PT::CaseStatement(new PT::Kwd::Case(kwd),
				   PT::list(expr, new PT::Atom(colon), stmt));
    }
    case Token::DEFAULT:
    {
      if (!require(':')) return 0;
      Token colon = my_lexer.get_token();
      PT::Node *stmt = statement();
      if (!require(stmt, "statement")) return 0;

      return new PT::DefaultStatement(new PT::Kwd::Default(kwd),
				      PT::list(new PT::Atom(colon), stmt));
    }
    case Token::Identifier:
    {
      Token colon = my_lexer.get_token();
      assert(colon.type == ':');
      PT::Node *stmt = statement();
      if (!require(stmt, "statement")) return 0;

      return new PT::LabelStatement(new PT::Identifier(kwd),
				    PT::list(new PT::Atom(colon), stmt));
    }
    default:
      // Never get here.
      return 0;
  }
}

// expression-statement:
//   expression [opt] ;
PT::ExprStatement *Parser::expression_statement()
{
  Trace trace("Parser::expression_statement", Trace::PARSING);

  if(my_lexer.look_ahead() == ';')
  {
    return new PT::ExprStatement(0, PT::cons(new PT::Atom(my_lexer.get_token())));
  }
  else
  {
    PT::Node *expr = expression();
    if (!require(expr, "expression") || !require(';')) return 0;
    return new PT::ExprStatement(expr, PT::cons(new PT::Atom(my_lexer.get_token())));
  }
}

// compound-statement:
//   { statement [opt] }
PT::Block *Parser::compound_statement(bool create_scope)
{
  Trace trace("Parser::compound_statement", Trace::PARSING);

  Token ob = my_lexer.get_token();
  if(ob.type != '{') return 0;

  PT::List *ob_comments = wrap_comments(my_lexer.get_comments());
  PT::Block *body = new PT::Block(new PT::CommentedAtom(ob, ob_comments), 0);

  ScopeGuard scope_guard(*this, create_scope ? body : 0);

  PT::List *stmts = 0;
  while(my_lexer.look_ahead() != '}')
  {
    PT::Node *stmt = statement();
    if (!require(stmt, "expression-statement")) return 0;
    else stmts = PT::snoc(stmts, stmt);
  }
  body = PT::snoc(body, stmts);

  Token cb = my_lexer.get_token();
  if(cb.type != '}') return 0;

  PT::List *cb_comments = wrap_comments(my_lexer.get_comments());
  return PT::snoc(body, new PT::CommentedAtom(cb, cb_comments));
}

// selection-statement:
//   if ( condition ) statement
//   if ( condition ) statement else statement
//   switch ( condition ) statement
PT::List *Parser::selection_statement()
{
  Trace trace("Parser::selection_statement", Trace::PARSING);

  Token kwd = my_lexer.get_token();
  assert(kwd.type == Token::IF || kwd.type == Token::SWITCH);
  
  if (!require('(')) return 0;
  Token op = my_lexer.get_token();
  PT::Node *cond = condition();
  if (!require(cond, "condition")) return 0;
  if (!require(')')) return 0;
  Token cp = my_lexer.get_token();
  PT::List *stmt = statement();
  if (!require(stmt, "statement")) return 0;

  if (kwd.type == Token::SWITCH)
    return new PT::SwitchStatement(new PT::Kwd::Switch(kwd),
				   PT::list(new PT::Atom(op), cond,
					    new PT::Atom(cp), stmt));
  else
  {
    stmt = new PT::IfStatement(new PT::Kwd::If(kwd),
			       PT::list(new PT::Atom(op),
					cond,
					new PT::Atom(cp),
					stmt));
    if(my_lexer.look_ahead() == Token::ELSE)
    {
      Token else_ = my_lexer.get_token();
      PT::Node *otherwise = statement();
      if (!require(otherwise, "statement")) return 0;
      stmt = PT::snoc(stmt, PT::list(new PT::Kwd::Else(else_), otherwise));
    }
    return stmt;
  }
}

// iteration-statement:
//   while ( condition ) statement
//   do statement while ( expression ) ;
//   for ( for-init-statement condition [opt] ; expression [opt] )
//     statement
//
// for-init-statement:
//   expression-statement
//   simple-declaration
PT::List *Parser::iteration_statement()
{
  Trace trace("Parser::iteration_statement", Trace::PARSING);
  switch (my_lexer.look_ahead())
  {
    case Token::WHILE:
    {
      Token while_ = my_lexer.get_token();
      if (!require('(')) return 0;
      Token op = my_lexer.get_token();
      PT::Node *cond = condition();
      if (!require(cond, "condition")) return 0;
      if (!require(')')) return 0;
      Token cp = my_lexer.get_token();
      PT::Node *stmt = statement();
      if (!require(stmt, "statement")) return 0;
      
      return new PT::WhileStatement(new PT::Kwd::While(while_),
				    PT::list(new PT::Atom(op), cond,
					     new PT::Atom(cp), stmt));
    }
    case Token::DO:
    {
      Token do_ = my_lexer.get_token();
      PT::Node *stmt = statement();
      if (!require(stmt, "statement")) return 0;
      if (!require(Token::WHILE, "while")) return 0;
      Token while_ = my_lexer.get_token();
      if (!require('(')) return 0;
      Token op = my_lexer.get_token();
      PT::Node *cond = condition();
      if (!require(cond, "condition")) return 0;
      if (!require(')')) return 0;
      Token cp = my_lexer.get_token();
      if (!require(';')) return 0;
      Token semic = my_lexer.get_token();

      return new PT::DoStatement(new PT::Kwd::Do(do_),
				 PT::list(stmt,
					  new PT::Kwd::While(while_),
					  new PT::Atom(op),
					  cond,
					  new PT::Atom(cp),
					  new PT::Atom(semic)));
    }
    case Token::FOR:
    {
      Token for_ = my_lexer.get_token();
      if (!require('(')) return 0;
      Token op = my_lexer.get_token();
      // for-init-statement
      PT::Node *init = 0;
      if (my_lexer.look_ahead() != ';')
      {
	// try simple-declaration first
	Tentative tentative(*this);
	init = simple_declaration(/*function_definition_allowed=*/false);
	if (!init) tentative.rollback();
      }
      if (!init) init = expression_statement();
      if (!require(init, "for-init-statement")) return 0;
      PT::Node *cond = 0;
      if (my_lexer.look_ahead() != ';')
      {
	cond = condition();
	if (!require(cond, "condition")) return 0;
      }
      if (!require(';')) return 0;
      Token semic = my_lexer.get_token();
      PT::Node *expr = 0;
      if (my_lexer.look_ahead() != ')')
      {
	expr = expression();
	if (!require(expr, "expression")) return 0;
      }
      if (!require(')')) return 0;
      Token cp = my_lexer.get_token();
      PT::Node *stmt = statement();
      if (!require(stmt, "statement")) return 0;

      return new PT::ForStatement(new PT::Kwd::For(op),
				  PT::list(new PT::Atom(for_), init, cond,
					   new PT::Atom(semic), expr,
					   new PT::Atom(cp), stmt));
    }
    default:
      return 0;
  }
}

// declaration-statement:
//   block-declaration
PT::List *Parser::declaration_statement()
{
  Trace trace("Parser::declaration_statement", Trace::PARSING);
  PGuard<bool> pguard(*this, &Parser::my_in_declaration_statement, true);
  return block_declaration();
}

// statement:
//   labeled-statement
//   expression-statement
//   compound-statement
//   selection-statement
//   iteration-statement
//   jump-statement
//   declaration-statement
//   try-block
PT::List *Parser::statement()
{
  Trace trace("Parser::statement", Trace::PARSING);

  PT::List *comments = wrap_comments(my_lexer.get_comments());
  PT::List *stmt = 0;
  switch(my_lexer.look_ahead())
  {
    case '{':
    {
      PT::Block *block = compound_statement(true);
      if (!block) return 0;
      stmt = block;
      break;
    }
    case Token::IF:
    case Token::SWITCH:
      stmt = selection_statement();
      break;
    case Token::WHILE:
    case Token::DO:
    case Token::FOR:
      stmt = iteration_statement();
      break;
    case Token::TRY:
      stmt = try_block();
      break;
    case Token::BREAK:
    case Token::CONTINUE:
    {
      Token kwd = my_lexer.get_token();
      Token semic = my_lexer.get_token();
      if(semic.type != ';') return 0;
      if(kwd.type == Token::BREAK)
	stmt = new PT::BreakStatement(new PT::Kwd::Break(kwd),
				      PT::cons(new PT::Atom(semic)));
      else
	stmt = new PT::ContinueStatement(new PT::Kwd::Continue(kwd),
					 PT::cons(new PT::Atom(semic)));
      break;
    }
    case Token::RETURN:
    {
      Token kwd = my_lexer.get_token();
      if(my_lexer.look_ahead() == ';')
      {
	Token semic = my_lexer.get_token();
	stmt = new PT::ReturnStatement(new PT::Kwd::Return(kwd),
				       PT::cons(new PT::Atom(semic)));
      } 
      else
      {
	PT::Node *expr = expression();
	if (!expr) return 0;
	Token semic = my_lexer.get_token();
	if(semic.type != ';') return 0;

	stmt = new PT::ReturnStatement(new PT::Kwd::Return(kwd),
				       PT::list(expr, new PT::Atom(semic)));
      }
      break;
    }
    case Token::GOTO:
    {
      Token kwd = my_lexer.get_token();
      Token id = my_lexer.get_token();
      if (id.type != Token::Identifier) return 0;
      Token semic = my_lexer.get_token();
      if (semic.type != ';') return 0;

      stmt = new PT::GotoStatement(new PT::Kwd::Goto(kwd),
				   PT::list(new PT::Atom(id), new PT::Atom(semic)));
      break;
    }
    case Token::CASE:
    case Token::DEFAULT:
      stmt = labeled_statement();
      break;
    case Token::Identifier:
      if(my_lexer.look_ahead(1) == ':')
      {
	stmt = labeled_statement();
	break;
      }
      // Fall through
    default:
      if (my_lexer.look_ahead() != ';')
      {
	Tentative tentative(*this);
	stmt = declaration_statement();
	if (!stmt) tentative.rollback();
      }
      if (!stmt) stmt = expression_statement();
      break;
  }
  return stmt;
}

// try-block:
//   try compound-statement handler-seq
//
// handler-seq:
//   handler handler-seq [opt]
//
// handler:
//   catch ( exception-declaration ) compound-statement
//
// exception-declaration:
//   type-specifier-seq declarator
//   type-specifier-seq abstract-declarator
//   type-specifier-seq
//   ...
PT::List *Parser::try_block()
{
  Trace trace("Parser::try_block", Trace::PARSING);

  if (my_lexer.look_ahead() != Token::TRY ||
      my_lexer.look_ahead(1) != '{') return 0;

  Token try_ = my_lexer.get_token();
  PT::Block *body = compound_statement();
  if (!body) return 0;

  PT::List *stmt = new PT::TryStatement(new PT::Kwd::Try(try_), PT::cons(body));

  do
  {
    Token catch_ = my_lexer.get_token();
    if(catch_.type != Token::CATCH) return 0;
    if (!require('(')) return 0;
    Token op = my_lexer.get_token();
    // TODO: handler should become a ParameterDeclaration
    PT::Node *handler;
    if(my_lexer.look_ahead() == Token::Ellipsis)
    {
      Token ellipsis = my_lexer.get_token();
      handler = new PT::Atom(ellipsis);
    }
    else
    {
      PT::Encoding encode;
      PT::ParameterDeclaration *parameter = 0;//parameter_declaration(encode);
      if (!parameter) return 0;
      handler = parameter;
    }
    if (!require(')')) return 0;
    Token cp = my_lexer.get_token();
    PT::Block *body = compound_statement();
    if (!body) return 0;

    stmt = PT::snoc(stmt, PT::list(new PT::Kwd::Catch(catch_),
				   new PT::Atom(op), handler, new PT::Atom(cp),
				   body));
  } while(my_lexer.look_ahead() == Token::CATCH);
  return stmt;
}

// exception-specification:
//   throw ( type-id-list [opt] )
PT::Node *Parser::opt_exception_specification()
{
  Trace trace("Parser::opt_exception_specification", Trace::PARSING);
  if (my_lexer.look_ahead() != Token::THROW) 
    return 0;

  Token throw_ = my_lexer.get_token();
  if (!require('(')) return 0;
  Token op = my_lexer.get_token();
  PT::Node *type_ids = 0;
  if (my_lexer.look_ahead() != ')') type_ids = type_id_list();
  if (!require(')')) return 0;
  Token cp = my_lexer.get_token();
  // TODO: define new PT::List type 'ExceptionSpec'
  return PT::list(new PT::Kwd::Throw(throw_),
		  new PT::Atom(op), type_ids, new PT::Atom(cp));
}

// Skip to the end of the (possibly compound) statement.
// Return the last consumed token.
Token Parser::skip_statement(size_t nesting)
{
  Trace trace("Parser::skip_statement", Trace::PARSING);
  while (true)
  {
    Token::Type token = my_lexer.look_ahead();
    if (token == Token::BadToken) break;
    else if (token == ';' && !nesting) break;
    else if (token == '}' && (!nesting || --nesting == 0)) break;
    else if (token == '{') ++nesting;
    my_lexer.get_token();
  }
  return my_lexer.get_token();
}

}
