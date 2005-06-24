//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis.hh>
#include "Synopsis/Parser.hh"
#include "Synopsis/Lexer.hh"
#include <Synopsis/Trace.hh>
#include <iostream>

namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;

namespace Synopsis
{

class SyntaxError : public Parser::Error
{
public:
  SyntaxError(const std::string &f, unsigned long l, const std::string &c)
    : my_filename(f), my_line(l), my_context(c) {}
  virtual void write(std::ostream &) const
  {
    std::cerr << "Syntax error : " << my_filename << ':' << my_line 
	      << ": Error before '" << my_context << '\'' << std::endl;
  }
private:
  std::string   my_filename;
  unsigned long my_line;
  std::string   my_context;
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
  SymbolTypeMismatch(PT::Encoding const &name, PTree::Encoding const &type)
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

template <typename T>
struct PGuard
{
  PGuard(Parser &p, T Parser::*m) : parser(p), member(m), saved(p.*m) {}
  ~PGuard() { parser.*member = saved;}
  Parser &    parser;
  T Parser::* member;
  T           saved;
};

const unsigned int max_errors = 10;

PT::Node *wrap_comments(const Lexer::Comments &c)
{
  PT::Node *head = 0;
  for (Lexer::Comments::const_iterator i = c.begin(); i != c.end(); ++i)
    head = PT::snoc(head, new PT::Atom(*i));
  return head;
}

PT::Node *nth_declarator(PT::Node *decl, size_t n)
{
  decl = PT::third(decl);
  if(!decl || decl->is_atom()) return 0;

  if(PT::is_a(decl, Token::ntDeclarator))
  {	// if it is a function
    if(n-- == 0) return decl;
  }
  else
    while(decl && !decl->is_atom())
    {
      if(n-- == 0) return decl->car();
      if((decl = decl->cdr())) decl = decl->cdr(); // skip ,
    }
  return 0;
}

void set_declarator_comments(PT::Declaration *decl, PT::Node *comments)
{
  if (!decl) return;

  PT::Node *declarator;
  size_t n = 0;
  while (true)
  {
    size_t i = n++;
    declarator = nth_declarator(decl, i);
    if (!declarator) break;
    else if (PT::is_a(declarator, Token::ntDeclarator))
      ((PT::Declarator*)declarator)->set_comments(comments);
  }
}

//. Helper function to recursively find the first left-most leaf node
PT::Node *leftmost_leaf(PT::Node *node, PT::Node *& parent)
{
  if (!node || node->is_atom()) return node;
  // Non-leaf node. So find first leafy child
  PT::Node *leaf;
  while (node)
  {
    if (node->car())
    {
      // There is a child here..
      if (node->car()->is_atom())
      {
	// And this child is a leaf! return it and set parent
	parent = node;
	return node->car();
      }
      if ((leaf = leftmost_leaf(node->car(), parent)))
	// Not a leaf so try recursing on it
	return leaf;
    }
    // No leaves from car of this node, so try next cdr
    node = node->cdr();
  }
  return 0;
}

//. Node is never the leaf. Instead we traverse the left side of the tree
//. until we find a leaf, and change the leaf to be a CommentedLeaf.
void set_leaf_comments(PT::Node *node, PT::Node *comments)
{
  PT::Node *parent, *leaf;
  PT::CommentedAtom* cleaf;

  // Find leaf
  leaf = leftmost_leaf(node, parent);

  // Sanity
  if (!leaf)
  {
    std::cerr << "Warning: Failed to find leaf when trying to add comments." << std::endl;
    PT::display(parent, std::cerr, false);
    return; 
  }

  if (!(cleaf = dynamic_cast<PT::CommentedAtom *>(leaf)))
  {
    // Must change first child of parent to be a commented leaf
    Token tk(leaf->position(), leaf->length(), Token::Comment);
    cleaf = new (PT::GC) PT::CommentedAtom(tk, comments);
    parent->set_car(cleaf);
  }
  else
  {
    // Already is a commented leaf, so add the comments to it
    comments = PT::snoc(cleaf->get_comments(), comments);
    cleaf->set_comments(comments);
  }
}

}

struct Parser::ScopeGuard
{
  template <typename T>
  ScopeGuard(Parser &p, T const *s)
    : parser(p), noop(s == 0), scope_was_valid(p.my_scope_is_valid)
  {
    // If s contains a qualified name this may fail, as the name
    // has to be declared before. We record the error but continune
    // processing.
    try { if (!noop) parser.my_symbols.enter_scope(s);}
    catch (ST::Undefined const &e)
    {
      std::string filename;
      unsigned long line = 0;
      if (e.ptree)
	line = parser.my_lexer.origin(e.ptree->begin(), filename);

      parser.my_errors.push_back(new UndefinedSymbol(e.name, filename, line));
      parser.my_scope_is_valid = false;
      noop = true;
    }
  }
  ~ScopeGuard() 
  {
    if (!noop) parser.my_symbols.leave_scope();
    parser.my_scope_is_valid = scope_was_valid;
  }
  Parser & parser;
  bool     noop;
  bool     scope_was_valid;
};

Parser::StatusGuard::StatusGuard(Parser &p)
  : my_lexer(p.my_lexer),
    my_token_mark(my_lexer.save()),
    my_errors(p.my_errors),
    my_error_mark(my_errors.size()),
    my_committed(false)
{
}

Parser::StatusGuard::~StatusGuard() 
{
  if (!my_committed)
  {
    my_lexer.restore(my_token_mark);
    my_errors.resize(my_error_mark);
  }
}

Parser::Parser(Lexer &lexer, SymbolFactory &symbols, int ruleset)
  : my_lexer(lexer),
    my_ruleset(ruleset),
    my_symbols(symbols),
    my_scope_is_valid(true),
    my_comments(0),
    my_gt_is_operator(true),
    my_in_template_decl(false)
{
}

Parser::~Parser()
{
}

bool Parser::mark_error()
{
  Trace trace("Parser::mark_error", Trace::PARSING);
  Token t1, t2;
  my_lexer.look_ahead(0, t1);
  my_lexer.look_ahead(1, t2);

  std::string filename;
  unsigned long line = my_lexer.origin(t1.ptr, filename);

  const char *end = t1.ptr;
  if(t2.type != '\0') end = t2.ptr + t2.length;
  else if(t1.type != '\0') end = t1.ptr + t1.length;
  my_errors.push_back(new SyntaxError(filename, line, std::string(t1.ptr, end - t1.ptr)));
  return my_errors.size() < max_errors;
}

template <typename T>
bool Parser::declare(T *t)
{
  // If the scope isn't valid, do nothing.
  if (!my_scope_is_valid) return true;
  try
  {
    my_symbols.declare(t);
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

unsigned long Parser::origin(const char *ptr,
			     std::string &filename) const
{
  return my_lexer.origin(ptr, filename);
}

void Parser::show_message_head(const char *pos)
{
  std::string filename;
  unsigned long line = origin(pos, filename);
  std::cerr << filename << ':' << line << ": ";
}

PT::Node *Parser::parse()
{
  Trace trace("Parser::parse", Trace::PARSING);
  PT::Node *statements = 0;
  while(my_lexer.look_ahead(0) != '\0')
  {
    PT::Node *def;
    if(definition(def))
    {
      statements = PT::nconc(statements, PT::list(def));
    }
    else
    {
      if(!mark_error()) return 0; // too many errors
      skip_to(';');
      Token tk;
      my_lexer.get_token(tk);	// ignore ';'
    }
  }
  // Retrieve trailing comments
  PT::Node *c = wrap_comments(my_lexer.get_comments());
  if (c)
  {
    // Use zero-length CommentedAtom as special marker.
    // Should we define a 'PT::Comment' atom for those comments that
    // don't clash with the grammar ? At least that seems less hackish than this:
    c = new PT::CommentedAtom(c->begin(), 0, c);
    statements = PT::nconc(statements, PT::list(c));
  }
  return statements;
}

/*
  definition
  : null.declaration
  | typedef
  | template.decl
  | metaclass.decl
  | linkage.spec
  | namespace.spec
  | namespace.alias
  | using.declaration
  | extern.template.decl
  | declaration
*/
bool Parser::definition(PT::Node *&p)
{
  Trace trace("Parser::definition", Trace::PARSING);
  bool res;
  int t = my_lexer.look_ahead(0);
  if(t == ';')
    res = null_declaration(p);
  else if(t == Token::TYPEDEF)
  {
    PT::Typedef *td;
    res = typedef_(td);
    p = td;
  }
  else if(t == Token::TEMPLATE)
    res = template_decl(p);
  else if(t == Token::EXPORT && my_lexer.look_ahead(1) == Token::TEMPLATE)
    res = template_decl(p);
  else if(t == Token::METACLASS)
    res = metaclass_decl(p);
  else if(t == Token::EXTERN && my_lexer.look_ahead(1) == Token::StringL)
    res = linkage_spec(p);
  else if(t == Token::EXTERN && my_lexer.look_ahead(1) == Token::TEMPLATE &&
	  my_ruleset & GCC)
    res = extern_template_decl(p);
  else if(t == Token::NAMESPACE && my_lexer.look_ahead(2) == '=')
  {
    PT::NamespaceAlias *alias;
    res = namespace_alias(alias);
    p = alias;
  }
  else if(t == Token::NAMESPACE)
  {
    PT::NamespaceSpec *spec;
    res = namespace_spec(spec);
    p = spec;
  }
  else if(t == Token::USING)
  {
    if (my_lexer.look_ahead(1) == Token::NAMESPACE)
    {
      PT::UsingDirective *udir;
      res = using_directive(udir);
      if (res)
      {
	declare(udir);
	p = udir;
      }
    }
    else
    {
      PT::UsingDeclaration *udecl;
      res = using_declaration(udecl);
      if (res)
      {
	declare(udecl);
	p = udecl;
      }
    }
  }
  else 
  {
    PT::Declaration *decl;
    if (!declaration(decl)) return false;
    PT::Node *c = wrap_comments(my_lexer.get_comments());
    if (c) set_declarator_comments(decl, c);
    p = decl;
    declare(decl);
    return true;
  }
  my_lexer.get_comments();
  return res;
}

bool Parser::null_declaration(PT::Node *&decl)
{
  Trace trace("Parser::null_declaration", Trace::PARSING);
  Token tk;

  if(my_lexer.get_token(tk) != ';') return false;
  decl = new PT::Declaration(0, PT::list(0, new PT::Atom(tk)));
  return true;
}

/*
  typedef
  : TYPEDEF type.specifier init_declarator_list ';'
*/
bool Parser::typedef_(PT::Typedef *&def)
{
  Trace trace("Parser::typedef_", Trace::PARSING);
  Token tk;
  PT::Node *type_name, *decl;
  PT::Encoding type_encode;

  if(my_lexer.get_token(tk) != Token::TYPEDEF) return false;

  def = new PT::Typedef(new PT::Kwd::Typedef(tk));
  if(!type_specifier(type_name, false, type_encode)) return false;

  def = PT::snoc(def, type_name);
  if(!init_declarator_list(decl, type_encode, true)) return false;
  if(my_lexer.get_token(tk) != ';') return false;

  def = PT::nconc(def, PT::list(decl, new PT::Atom(tk)));
  declare(def);
  return true;
}

/*
  type.specifier
  : {cv.qualify} (integral.or.class.spec | name) {cv.qualify}
*/
bool Parser::type_specifier(PT::Node *&tspec, bool check, PT::Encoding &encode)
{
  Trace trace("Parser::type_specifier", Trace::PARSING);
  PT::Node *cv_q, *cv_q2;

  // FIXME: Need to rewrite this to correctly reflect the grammar, in particular
  //        'typename' ...
  //        Do we need a new node type ('Typename') ?
  if(!opt_cv_qualifier(cv_q) || !opt_integral_type_or_class_spec(tspec, encode))
    return false;
  
  if(!tspec)
  {
    if(check)
    {
      Token tk;
      my_lexer.look_ahead(0, tk);
      if(!maybe_typename_or_class_template(tk))
	return false;
    }

    if(!name(tspec, encode))
      return false;
  }

  if(!opt_cv_qualifier(cv_q2))
    return false;

  if(cv_q)
  {
    tspec = PT::snoc(cv_q, tspec);
    if(cv_q2)
      tspec = PT::nconc(tspec, cv_q2);
  }
  else if(cv_q2)
    tspec = PT::cons(tspec, cv_q2);

  encode.cv_qualify(cv_q, cv_q2);
  return true;
}

// is_type_specifier() returns true if the next is probably a type specifier.
bool Parser::is_type_specifier()
{
  int t = my_lexer.look_ahead(0);
  if(t == Token::TYPENAME || t == Token::Identifier || t == Token::Scope
     || t == Token::CONST || t == Token::VOLATILE
     || t == Token::CHAR || t == Token::WCHAR 
     || t == Token::INT || t == Token::SHORT || t == Token::LONG
     || t == Token::SIGNED || t == Token::UNSIGNED || t == Token::FLOAT || t == Token::DOUBLE
     || t == Token::VOID || t == Token::BOOLEAN
     || t == Token::CLASS || t == Token::STRUCT || t == Token::UNION || t == Token::ENUM)
    return true;
  else if (my_ruleset & MSVC && t == Token::INT64)
    return true;
  else
    return false;
}

/*
  metaclass.decl
  : METACLASS Identifier {{':'} Identifier {'(' meta.arguments ')'}} ';'

  We allow two kinds of syntax:

  metaclass <metaclass> <class>(...);
  metaclass <metaclass>;
  metaclass <class> : <metaclass>(...);		// for backward compatibility
*/
bool Parser::metaclass_decl(PT::Node *&decl)
{
  int t;
  Token tk1, tk2, tk3, tk4;
  PT::Node *metaclass_name;

  if(my_lexer.get_token(tk1) != Token::METACLASS)
    return false;

  if(my_lexer.get_token(tk2) != Token::Identifier)
    return false;

  t = my_lexer.get_token(tk3);
  if(t == Token::Identifier)
  {
    metaclass_name = new PT::Identifier(tk2);
    decl = new PT::MetaclassDecl(new PT::UserKeyword(tk1),
				 PT::list(metaclass_name, new PT::Identifier(tk3)));
  }
  else if(t == ':')
  {
    if(my_lexer.get_token(tk4) != Token::Identifier)
      return false;

    metaclass_name = new PT::Identifier(tk4);
    decl = new PT::MetaclassDecl(new PT::UserKeyword(tk1),
				 PT::list(metaclass_name, new PT::Identifier(tk2)));
  }
  else if(t == ';')
  {
    metaclass_name = new PT::Identifier(tk2);
    decl = new PT::MetaclassDecl(new PT::UserKeyword(tk1),
				 PT::list(metaclass_name, 0, new PT::Atom(tk3)));
    return true;
  }
  else
    return false;

  t = my_lexer.get_token(tk1);
  if(t == '(')
  {
    PT::Node *args;
    if(!meta_arguments(args))
      return false;

    if(my_lexer.get_token(tk2) != ')')
      return false;

    decl = PT::nconc(decl, PT::list(new PT::Atom(tk1), args, new PT::Atom(tk2)));
    t = my_lexer.get_token(tk1);
  }

  if(t == ';')
  {
    decl = PT::snoc(decl, new PT::Atom(tk1));
    return true;
  }
  else
    return false;
}

/*
  meta.arguments : (anything but ')')*
*/
bool Parser::meta_arguments(PT::Node *&args)
{
  int t;
  Token tk;

  int n = 1;
  args = 0;
  while(true)
  {
    t = my_lexer.look_ahead(0);
    if(t == '\0')
      return false;
    else if(t == '(')
      ++n;
    else if(t == ')')
      if(--n <= 0)
	return true;

    my_lexer.get_token(tk);
    args = PT::snoc(args, new PT::Atom(tk));
  }
}

/*
  linkage.spec
  : EXTERN StringL definition
  |  EXTERN StringL linkage.body
*/
bool Parser::linkage_spec(PT::Node *&spec)
{
  Trace trace("Parser::linkage_spec", Trace::PARSING);
  Token tk1, tk2;
  PT::Node *body;

  if(my_lexer.get_token(tk1) != Token::EXTERN) return false;
  if(my_lexer.get_token(tk2) != Token::StringL) return false;

  spec = new PT::LinkageSpec(new PT::Kwd::Extern(tk1), PT::list(new PT::Atom(tk2)));
  if(my_lexer.look_ahead(0) == '{')
  {
    if(!linkage_body(body)) return false;
  }
  else
    if(!definition(body)) return false;

  spec = PT::snoc(spec, body);
  return true;
}

/*
  namespace.spec
  : NAMESPACE Identifier definition
  | NAMESPACE { Identifier } linkage.body
*/
bool Parser::namespace_spec(PT::NamespaceSpec *&spec)
{
  Trace trace("Parser::namespace_spec", Trace::PARSING);

  Token tk;
  if(my_lexer.get_token(tk) != Token::NAMESPACE) return false;
  PT::Kwd::Namespace *namespace_ = new PT::Kwd::Namespace(tk);

  PT::Node *comments = wrap_comments(my_lexer.get_comments());

  PT::Node *name;
  if(my_lexer.look_ahead(0) == '{') name = 0;
  else
    if(my_lexer.get_token(tk) == Token::Identifier)
      name = new PT::Identifier(tk);
    else return false;

  spec = new PT::NamespaceSpec(namespace_, PT::list(name, 0));
  spec->set_comments(comments);

  PT::Node *body;
  if(my_lexer.look_ahead(0) == '{')
  {
    declare(spec);
    ScopeGuard guard(*this, spec);
    if(!linkage_body(body)) return false;
  }
  else if(!definition(body)) return false;

  PT::tail(spec, 2)->set_car(body);

  return true;
}

/*
  namespace.alias : NAMESPACE Identifier '=' Identifier ';'
*/
bool Parser::namespace_alias(PT::NamespaceAlias *&exp)
{
  Trace trace("Parser::namespace_alias", Trace::PARSING);
  Token tk;

  if(my_lexer.get_token(tk) != Token::NAMESPACE) return false;
  PT::Node *ns = new PT::Kwd::Namespace(tk);

  if (my_lexer.get_token(tk) != Token::Identifier) return false;
  PT::Node *alias = new PT::Identifier(tk);

  if (my_lexer.get_token(tk) != '=') return false;
  PT::Node *eq = new PT::Atom(tk);

  PT::Node *name;
  PT::Encoding encode;
  int length = 0;
  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    name = PT::list(new PT::Atom(tk));
    encode.global_scope();
    ++length;
  }
  else name = 0;

  while (true)
  {
    if (my_lexer.get_token(tk) != Token::Identifier) return false;
    PT::Node *n = new PT::Identifier(tk);
    encode.simple_name(n);
    ++length;
    
    if(my_lexer.look_ahead(0) == Token::Scope)
    {
      my_lexer.get_token(tk);
      name = PT::nconc(name, PT::list(n, new PT::Atom(tk)));
    }
    else
    {
      if(name == 0) name = n;
      else name = PT::snoc(name, n);

      if(length > 1) encode.qualified(length);

      break;
    }
  }

  if (my_lexer.get_token(tk) != ';') return false;

  exp = new PT::NamespaceAlias(ns, PT::list(alias, eq, name, new PT::Atom(tk)));
  return true;
}

/*
  using.directive
  : USING NAMESPACE name
*/
bool Parser::using_directive(PT::UsingDirective *&udir)
{
  Trace trace("Parser::using_directive", Trace::PARSING);
  Token tk;
  if(my_lexer.get_token(tk) != Token::USING)
    return false;

  udir = new PT::UsingDirective(new PT::Kwd::Using(tk));

  if(my_lexer.get_token(tk) != Token::NAMESPACE)
    return false;
  udir = PT::snoc(udir, new PT::Kwd::Namespace(tk));

  PT::Node *id;
  PT::Encoding name_encode;
  if (!name(id, name_encode)) return false;
  if (!id->is_atom())
    id = new PT::Name(id, name_encode);
  else
    id = new PT::Name(PT::list(id), name_encode);
  udir = PT::snoc(udir, id);
  if (my_lexer.get_token(tk) != ';') return false;

  udir = PT::snoc(udir, new PT::Atom(tk));
  return true;
}


/*
  using.declaration
  : USING name
*/
bool Parser::using_declaration(PT::UsingDeclaration *&udecl)
{
  Trace trace("Parser::user_declaration", Trace::PARSING);
  Token tk;
  
  if(my_lexer.get_token(tk) != Token::USING)
    return false;

  PT::Node *id;
  PT::Encoding name_encode;
  if (!name(id, name_encode)) return false;
  if (!id->is_atom())
    id = new PT::Name(id, name_encode);
  else
    id = new PT::Name(PT::list(id), name_encode);

  udecl = new PT::UsingDeclaration(new PT::Kwd::Using(tk), id);

  if (my_lexer.get_token(tk) != ';') return false;

  udecl = PT::snoc(udecl, new PT::Atom(tk));
  return true;
}


/*
  linkage.body : '{' (definition)* '}'

  Note: this is also used to construct namespace.spec
*/
bool Parser::linkage_body(PT::Node *&body)
{
  Trace trace("Parser::linkage_body", Trace::PARSING);
  Token op, cp;
  PT::Node *def;

  if(my_lexer.get_token(op) != '{')
    return false;

  body = 0;
  while(my_lexer.look_ahead(0) != '}')
  {
    if(!definition(def))
    {
      if(!mark_error())
	return false;		// too many errors

      skip_to('}');
      my_lexer.get_token(cp);
      body = PT::list(new PT::Atom(op), 0, new PT::Atom(cp));
      return true;		// error recovery
    }

    body = PT::snoc(body, def);
  }

  my_lexer.get_token(cp);
  body = new PT::Brace(new PT::Atom(op), body,
		       new PT::CommentedAtom(cp, wrap_comments(my_lexer.get_comments())));
  return true;
}

/*
  template.decl
  : TEMPLATE '<' temp.arg.list '>' declaration
  | TEMPLATE declaration
  | TEMPLATE '<' '>' declaration

  The second case is an explicit template instantiation.  declaration must
  be a class declaration.  For example,

      template class Foo<int, char>;

  explicitly instantiates the template Foo with int and char.

  The third case is a specialization of a template function.  declaration
  must be a function template.  For example,

      template <> int count(String x) { return x.length; }
*/
bool Parser::template_decl(PT::Node *&decl)
{
  Trace trace("Parser::template_decl", Trace::PARSING);
  PT::Declaration *body;
  PT::TemplateDecl *tdecl;
  TemplateDeclKind kind = tdk_unknown;

  my_comments = wrap_comments(my_lexer.get_comments());
  if(!template_decl2(tdecl, kind)) return false;
  if (kind == tdk_decl) my_in_template_decl = true;
  bool success = declaration(body);
  my_in_template_decl = false;
  if (!success) return false;
  // Repackage the decl and body depending upon what kind of template
  // declaration was observed.
  switch (kind)
  {
    case tdk_instantiation:
      // Repackage the decl as a PtreeTemplateInstantiation
      decl = body;
      // assumes that decl has the form: [0 [class ...] ;]
      if (PT::length(decl) != 3) return false;
      if (PT::first(decl) != 0) return false;
      if (PT::type_of(PT::second(decl)) != Token::ntClassSpec) return false;
      if (*PT::third(decl) != ';') return false;
      decl = new PT::TemplateInstantiation(PT::second(decl));
      break;
    case tdk_decl:
    {
      tdecl = PT::snoc(tdecl, body);
      declare(tdecl);
      decl = tdecl;
      break;
    }
    case tdk_specialization:
    {
      tdecl = PT::snoc(tdecl, body);
      decl = tdecl;
      break;
    }
    default:
      throw std::runtime_error("Parser::template_decl(): fatal");
  }
  return true;
}

bool Parser::template_decl2(PT::TemplateDecl *&decl, TemplateDeclKind &kind)
{
  Trace trace("Parser::template_decl2", Trace::PARSING);
  Token tk;
  PT::List *params;

  my_lexer.get_token(tk);
  if (tk.type == Token::EXPORT)
    my_lexer.get_token(tk); // FIXME: ignored for now.
  if(tk.type != Token::TEMPLATE) return false;
  if(my_lexer.look_ahead(0) != '<') 
  {
    // explicit-instantiation
    decl = 0;
    kind = tdk_instantiation;
    return true;
  }

  decl = new PT::TemplateDecl(new PT::Kwd::Template(tk));
  if(my_lexer.get_token(tk) != '<') return false;

  decl = PT::snoc(decl, new PT::Atom(tk));
  {
    ScopeGuard guard(*this, decl);
    if(!template_parameter_list(params)) return false;
  }
  if(my_lexer.get_token(tk) != '>') return false;

  // FIXME: Flush any dangling comments, or else they will be attached to
  //        the declared class / function template itself.
  my_lexer.get_comments();
  decl = PT::nconc(decl, PT::list(params, new PT::Atom(tk)));

  // FIXME: nested TEMPLATE is ignored
  while (my_lexer.look_ahead(0) == Token::TEMPLATE) 
  {
    my_lexer.get_token(tk);
    if(my_lexer.look_ahead(0) != '<') break;

    my_lexer.get_token(tk);
    PT::List *nested = PT::list(0, 0);
    ScopeGuard guard(*this, nested); // template parameter list
    if(!template_parameter_list(params)) return false;
    if(my_lexer.get_token(tk) != '>') return false;
  }

  if (params == 0) kind = tdk_specialization; // template < > declaration
  else kind = tdk_decl;                       // template < ... > declaration
  return true;
}

//. template-parameter-list:
//.   template-parameter
//.   template-parameter-list , template-parameter
bool Parser::template_parameter_list(PT::List *&params)
{
  Trace trace("Parser::template_parameter_list", Trace::PARSING);
  Token tk;
  PT::Node *a;

  // FIXME: '<>' is invalid in this context. This probably belongs into
  //        the production of an explicit specialization.
  if(my_lexer.look_ahead(0) == '>')
  {
    params = 0;
    return true;
  }

  if(!template_parameter(a))
    return false;
  params = PT::list(a);
  while(my_lexer.look_ahead(0) == ',')
  {
    my_lexer.get_token(tk);
    params = PT::snoc(params, new PT::Atom(tk));
    if(!template_parameter(a))
      return false;

    params = PT::snoc(params, a);
  }
  return true;
}

//. template-parameter:
//.   type-parameter
//.   parameter-declaration
bool Parser::template_parameter(PT::Node *&decl)
{
  Trace trace("Parser::template_parameter", Trace::PARSING);

  PGuard<bool> guard(*this, &Parser::my_gt_is_operator);
  my_gt_is_operator = false;

  Token::Type type = my_lexer.look_ahead(0);
  // template template parameter
  if (type == Token::TEMPLATE) return type_parameter(decl);
  // possibly a type parameter
  else if (type == Token::TYPENAME || type == Token::CLASS)
  {
    // If the next token is an identifier, and the following
    // one is ',', '=', or '>', it's a type parameter.
    type = my_lexer.look_ahead(1);
    if (type == Token::Identifier) type = my_lexer.look_ahead(2);
    if (type == ',' || type == '=' || type == '>')
      return type_parameter(decl);
  }
  // It's a non-type parameter.
  PT::Encoding encoding; // unused
  PT::ParameterDeclaration *pdecl;
  if (!parameter_declaration(pdecl, encoding)) return false;
  decl = pdecl;
  return true;
}

//. type-parameter:
//.   class identifier [opt]
//.   class identifier [opt] = type-id
//.   typename identifier [opt]
//.   typename identifier [opt] = type-id
//.   template  < template-parameter-list > class identifier [opt]
//.   template  < template-parameter-list > class identifier [opt] = id-expression
bool Parser::type_parameter(PT::Node *&decl)
{
  Trace trace("Parser::type_parameter", Trace::PARSING);

  Token::Type type = my_lexer.look_ahead(0);
  if(type == Token::TYPENAME || type == Token::CLASS)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Keyword *kwd;
    if (type == Token::TYPENAME) kwd = new PT::Kwd::Typename(tk);
    else kwd = new PT::Kwd::Class(tk);

    PT::Identifier *name = 0;
    if (my_lexer.look_ahead(0) == Token::Identifier)
    {
      my_lexer.get_token(tk);
      name = new PT::Identifier(tk);
    }
    PT::TypeParameter *tparam = new PT::TypeParameter(kwd, PT::list(name));
    if (name) declare(tparam);

    decl = tparam;
    type = my_lexer.look_ahead(0);
    if (type == '=')
    {
      my_lexer.get_token(tk);
      PT::Encoding name;
      PT::Node *default_type;
      if(!type_id(default_type, name)) return false;
      default_type = new PT::Name(default_type, name);
      decl = PT::nconc(decl, PT::list(new PT::Atom(tk), default_type));
    }
  }
  else if (type == Token::TEMPLATE) 
  {
    TemplateDeclKind kind;
    PT::TemplateDecl *tdecl;
    if(!template_decl2(tdecl, kind)) return false;

    Token tk;
    if (my_lexer.get_token(tk) != Token::CLASS) return false;
    PT::Kwd::Class *class_ = new PT::Kwd::Class(tk);
    PT::Identifier *name = 0;
    if (my_lexer.look_ahead(0) == Token::Identifier)
    {
      my_lexer.get_token(tk);
      name = new PT::Identifier(tk);
    }
    PT::ClassSpec *cspec = new PT::ClassSpec(class_, PT::cons(name, 0), 0);
    tdecl = PT::nconc(tdecl, cspec);
    PT::TypeParameter *tparam = new PT::TypeParameter(tdecl, 0);
    if (name) declare(tparam);
    decl = tparam;

    if(my_lexer.look_ahead(0) == '=')
    {
      my_lexer.get_token(tk);
      PT::Encoding name;
      PT::Node *default_type;
      if(!type_id(default_type, name)) return false;
      default_type = new PT::Name(default_type, name);
      decl = PT::nconc(decl, PT::list(new PT::Atom(tk), default_type));
    }
  }
  return true;
}

//. extern-template-decl:
//.   extern template declaration
 bool Parser::extern_template_decl(PT::Node *&decl) 	 
 { 	 
   Trace trace("Parser::extern_template_decl", Trace::PARSING); 	 
   Token tk1, tk2; 	 
   PT::Declaration *body; 	 
  	 
   if(my_lexer.get_token(tk1) != Token::EXTERN) return false; 	 
   if(my_lexer.get_token(tk2) != Token::TEMPLATE) return false; 	 
   if(!declaration(body)) return false; 	 
  	 
   decl = new PT::ExternTemplate(new PT::Atom(tk1),PT::list(new PT::Atom(tk2), body));
   return true;
 } 	 
  	 
/*
  declaration
  : integral.declaration
  | const.declaration
  | other.declaration

  decl.head
  : {member.spec} {storage.spec} {member.spec} {cv.qualify}

  integral.declaration
  : integral.decl.head init_declarator_list (';' | function.body)
  | integral.decl.head ';'
  | integral.decl.head ':' expression ';'

  integral.decl.head
  : decl.head integral.or.class.spec {cv.qualify}

  other.declaration
  : decl.head name {cv.qualify} init_declarator_list (';' | function.body)
  | decl.head name constructor.decl (';' | function.body)
  | FRIEND name ';'

  const.declaration
  : cv.qualify {'*'} Identifier '=' expression {',' init_declarator_list} ';'

  Note: if you modify this function, look at declaration.statement, too.
  Note: this regards a statement like "T (a);" as a constructor
        declaration.  See is_constructor_decl().
*/
bool Parser::declaration(PT::Declaration *&statement)
{
  Trace trace("Parser::declaration", Trace::PARSING);
  PT::Node *mem_s, *storage_s, *cv_q, *integral, *head;
  PT::Encoding type_encode;
  int res;

  if (!my_in_template_decl)
    my_comments = wrap_comments(my_lexer.get_comments());

  if(!opt_member_spec(mem_s) || !opt_storage_spec(storage_s))
    return false;

  if(mem_s == 0)
    head = 0;
  else
    head = mem_s;	// mem_s is a list.

  if(storage_s != 0)
    head = PT::snoc(head, storage_s);

  if(mem_s == 0)
    if(opt_member_spec(mem_s))
      head = PT::nconc(head, mem_s);
    else
      return false;

  if(!opt_cv_qualifier(cv_q)
     || !opt_integral_type_or_class_spec(integral, type_encode))
    return false;

  if(integral)
    res = integral_declaration(statement, type_encode, head, integral, cv_q);
  else
  {
    type_encode.clear();
    int t = my_lexer.look_ahead(0);
    if(cv_q != 0 && ((t == Token::Identifier && my_lexer.look_ahead(1) == '=')
		     || t == '*'))
      res = const_declaration(statement, type_encode, head, cv_q);
    else
      res = other_declaration(statement, type_encode, mem_s, cv_q, head);
  }
  if (res && statement)
  {
    statement->set_comments(my_comments);
    my_comments = 0;
  }
  return res;
}

bool Parser::integral_declaration(PT::Declaration *&statement,
				  PT::Encoding &type_encode,
				  PT::Node *head, PT::Node *integral,
				  PT::Node *cv_q)
{
  Trace trace("Parser::integral_declaration", Trace::PARSING);
  Token tk;
  PT::Node *cv_q2, *decl;

  if(!opt_cv_qualifier(cv_q2))
    return false;

  if(cv_q)
    if(cv_q2 == 0)
      integral = PT::snoc(cv_q, integral);
    else
      integral = PT::nconc(cv_q, PT::cons(integral, cv_q2));
  else if(cv_q2 != 0)
    integral = PT::cons(integral, cv_q2);

  type_encode.cv_qualify(cv_q, cv_q2);
  switch(my_lexer.look_ahead(0))
  {
    case ';' :
      my_lexer.get_token(tk);
      statement = new PT::Declaration(head, PT::list(integral, new PT::Atom(tk)));
      return true;
    case ':' : // bit field
      my_lexer.get_token(tk);
      if(!assign_expr(decl)) return false;

      decl = PT::list(PT::list(new PT::Atom(tk), decl));
      if(my_lexer.get_token(tk) != ';') return false;

      statement = new PT::Declaration(head,
				      PT::list(integral, decl, new PT::Atom(tk)));
      return true;
    default :
      if(!init_declarator_list(decl, type_encode, true)) return false;

      if(my_lexer.look_ahead(0) == ';')
      {
	my_lexer.get_token(tk);
	statement = new PT::Declaration(head, 
					PT::list(integral, decl, new PT::Atom(tk)));
	return true;
      }
      else
      {
	PT::FunctionDefinition *def;
  	def = new PT::FunctionDefinition(head, PT::list(integral, decl->car()));
	ScopeGuard guard(*this, def);
	PT::Block *body;
	if(!function_body(body)) return false;
	if(PT::length(decl) != 1) return false;

  	statement = PT::snoc(def, body);
	return true;
      }
  }
}

bool Parser::const_declaration(PT::Declaration *&statement, PT::Encoding&,
			       PT::Node *head, PT::Node *cv_q)
{
  Trace trace("Parser::const_declaration", Trace::PARSING);
  PT::Node *decl;
  Token tk;
  PT::Encoding type_encode;

  type_encode.simple_const();
  if(!init_declarator_list(decl, type_encode, false))
    return false;

  if(my_lexer.look_ahead(0) != ';')
    return false;

  my_lexer.get_token(tk);
  statement = new PT::Declaration(head, PT::list(cv_q, decl, new PT::Atom(tk)));
  return true;
}

bool Parser::other_declaration(PT::Declaration *&statement, PT::Encoding &type_encode,
			       PT::Node *mem_s, PT::Node *cv_q,
			       PT::Node *head)
{
  Trace trace("Parser::other_declaration", Trace::PARSING);
  PT::Node *type_name, *decl, *cv_q2;
  Token tk;

  if(!name(type_name, type_encode))
    return false;

  if(cv_q == 0 && is_constructor_decl())
  {
    PT::Encoding ftype_encode;
    if(!constructor_decl(decl, ftype_encode))
      return false;

    decl = PT::list(new PT::Declarator(type_name, decl,
				       ftype_encode, type_encode,
				       type_name));
    type_name = 0;
  }
  else if(mem_s != 0 && my_lexer.look_ahead(0) == ';')
  {
    // FRIEND name ';'
    if(PT::length(mem_s) == 1 && PT::type_of(mem_s->car()) == Token::FRIEND)
    {
      my_lexer.get_token(tk);
      statement = new PT::Declaration(head, PT::list(type_name, new PT::Atom(tk)));
      return true;
    }
    else
      return false;
  }
  else
  {
    if(!opt_cv_qualifier(cv_q2))
      return false;

    if(cv_q)
      if(cv_q2 == 0)
	type_name = PT::snoc(cv_q, type_name);
      else
	type_name = PT::nconc(cv_q, PT::cons(type_name, cv_q2));
    else if(cv_q2)
      type_name = PT::cons(type_name, cv_q2);

    type_encode.cv_qualify(cv_q, cv_q2);
    if(!init_declarator_list(decl, type_encode, false))
      return false;
  }

  if(my_lexer.look_ahead(0) == ';')
  {
    my_lexer.get_token(tk);
    statement = new PT::Declaration(head,
				    PT::list(type_name, decl, new PT::Atom(tk)));
  }
  else
  {
    PT::Block *body;
    if(!function_body(body))
      return false;

    if(PT::length(decl) != 1)
      return false;

    statement = new PT::Declaration(head, PT::list(type_name, decl->car(), body));
  }
  return true;
}

/*
  This returns true for an declaration like:
	T (a);
  even if a is not a type name.  This is a bug according to the ANSI
  specification, but I believe none says "T (a);" for a variable
  declaration.
*/
bool Parser::is_constructor_decl()
{
  Trace trace("Parser::is_constructor_decl", Trace::PARSING);
  if(my_lexer.look_ahead(0) != '(')
    return false;
  else
  {
    int t = my_lexer.look_ahead(1);
    if(t == '*' || t == '&' || t == '(')
      return false;	// declarator
    else if(t == Token::CONST || t == Token::VOLATILE)
      return true;	// constructor or declarator
    else if(is_ptr_to_member(1))
      return false;	// declarator (::*)
    else
      return true;	// maybe constructor
  }
}

/*
  ptr.to.member
  : {'::'} (identifier {'<' any* '>'} '::')+ '*'
*/
bool Parser::is_ptr_to_member(int i)
{
  int t0 = my_lexer.look_ahead(i++);
  
  if(t0 == Token::Scope)
    t0 = my_lexer.look_ahead(i++);

  while(t0 == Token::Identifier)
  {
    int t = my_lexer.look_ahead(i++);
    if(t == '<')
    {
      int n = 1;
      while(n > 0)
      {
	int u = my_lexer.look_ahead(i++);
	if(u == '<') ++n;
	else if(u == '>') --n;
	else if(u == '(')
	{
	  int m = 1;
	  while(m > 0)
	  {
	    int v = my_lexer.look_ahead(i++);
	    if(v == '(') ++m;
	    else if(v == ')') --m;
	    else if(v == '\0' || v == ';' || v == '}')
	      return false;
	  }
	}
	else if(u == '\0' || u == ';' || u == '}')
	  return false;
      }
      t = my_lexer.look_ahead(i++);
    }
    if(t != Token::Scope)
      return false;
    t0 = my_lexer.look_ahead(i++);
    if(t0 == '*')
      return true;
  }
  return false;
}

/*
  member.spec
  : (FRIEND | INLINE | VIRTUAL | EXPLICIT | userdef.keyword)+
*/
bool Parser::opt_member_spec(PT::Node *&p)
{
  Trace trace("Parser::opt_member_spec", Trace::PARSING);
  Token tk;
  PT::Node *lf;
  int t = my_lexer.look_ahead(0);

  p = 0;
  while(t == Token::FRIEND ||
	t == Token::INLINE ||
	t == Token::VIRTUAL ||
	t == Token::EXPLICIT ||
	t == Token::UserKeyword5)
  {
    if(t == Token::UserKeyword5)
    {
      if(!userdef_keyword(lf))
	return false;
    }
    else
    {
      my_lexer.get_token(tk);
      if(t == Token::INLINE)
	lf = new PT::Kwd::Inline(tk);
      else if(t == Token::VIRTUAL)
	lf = new PT::Kwd::Virtual(tk);
      else if(t == Token::EXPLICIT)
	lf = new PT::Kwd::Explicit(tk);
      else
	lf = new PT::Kwd::Friend(tk);
    }
    p = PT::snoc(p, lf);
    t = my_lexer.look_ahead(0);
  }
  return true;
}

//. storage-spec:
//.   empty
//.   static
//.   extern
//.   auto
//.   register
//.   mutable
bool Parser::opt_storage_spec(PT::Node *&p)
{
  Trace trace("Parser::opt_storage_spec", Trace::PARSING);
  int t = my_lexer.look_ahead(0);
  if(t == Token::STATIC || t == Token::EXTERN || t == Token::AUTO ||
     t == Token::REGISTER || t == Token::MUTABLE)
  {
    Token tk;
    my_lexer.get_token(tk);
    switch(t)
    {
      case Token::STATIC :
	p = new PT::Kwd::Static(tk);
	break;
      case Token::EXTERN :
	p = new PT::Kwd::Extern(tk);
	break;
      case Token::AUTO :
	p = new PT::Kwd::Auto(tk);
	break;
      case Token::REGISTER :
	p = new PT::Kwd::Register(tk);
	break;
      case Token::MUTABLE :
	p = new PT::Kwd::Mutable(tk);
	break;
      default :
	throw std::runtime_error("opt_storage_spec: fatal");
    }
  }
  else
    p = 0;	// no storage specifier
  return true;
}

//. cv-qualifier:
//.   empty
//.   const
//.   volatile
bool Parser::opt_cv_qualifier(PT::Node *&cv)
{
  Trace trace("Parser::opt_cv_qualifier", Trace::PARSING);
  PT::Node *p = 0;
  while(true)
  {
    int t = my_lexer.look_ahead(0);
    if(t == Token::CONST || t == Token::VOLATILE)
    {
      Token tk;
      my_lexer.get_token(tk);
      switch(t)
      {
        case Token::CONST :
	  p = PT::snoc(p, new PT::Kwd::Const(tk));
	  break;
        case Token::VOLATILE :
	  p = PT::snoc(p, new PT::Kwd::Volatile(tk));
	  break;
        default :
	  throw std::runtime_error("opt_cv_qualifier: fatal");
      }
    }
    else
      break;
  }
  cv = p;
  return true;
}

/*
  integral.or.class.spec
  : (CHAR | WCHAR | INT | SHORT | LONG | SIGNED | UNSIGNED | FLOAT | DOUBLE
     | VOID | BOOLEAN)+
  | class.spec
  | enum.spec

  Note: if editing this, see also is_type_specifier().
*/
bool Parser::opt_integral_type_or_class_spec(PT::Node *&p, PT::Encoding& encode)
{
  Trace trace("Parser::opt_integral_type_or_class_spec", Trace::PARSING);
  bool is_integral;
  int t;
  char type = ' ', flag = ' ';

  is_integral = false;
  p = 0;
  while(true)
  {
    t = my_lexer.look_ahead(0);
    if(t == Token::CHAR || t == Token::WCHAR ||
       t == Token::INT || t == Token::SHORT || t == Token::LONG ||
       t == Token::SIGNED || t == Token::UNSIGNED ||
       t == Token::FLOAT || t == Token::DOUBLE ||
       t == Token::VOID ||
       t == Token::BOOLEAN ||
       (my_ruleset & MSVC && t == Token::INT64))
    {
      Token tk;
      PT::Node *kw;
      my_lexer.get_token(tk);
      switch(t)
      {
        case Token::CHAR:
	  type = 'c';
	  kw = new PT::Kwd::Char(tk);
	  break;
        case Token::WCHAR:
	  type = 'w';
	  kw = new PT::Kwd::WChar(tk);
	  break;
        case Token::INT:
        case Token::INT64: // an int64 is *NOT* an int but...
	  if(type != 's' && type != 'l' && type != 'j' && type != 'r')
	    type = 'i';
	  kw = new PT::Kwd::Int(tk);
	  break;
        case Token::SHORT:
	  type = 's';
	  kw = new PT::Kwd::Short(tk);
	  break;
        case Token::LONG:
	  if(type == 'l') type = 'j';      // long long
	  else if(type == 'd') type = 'r'; // double long
	  else type = 'l';
	  kw = new PT::Kwd::Long(tk);
	  break;
        case Token::SIGNED:
	  flag = 'S';
	  kw = new PT::Kwd::Signed(tk);
	  break;
        case Token::UNSIGNED:
	  flag = 'U';
	  kw = new PT::Kwd::Unsigned(tk);
	  break;
        case Token::FLOAT:
	  type = 'f';
	  kw = new PT::Kwd::Float(tk);
	  break;
        case Token::DOUBLE:
	  if(type == 'l') type = 'r'; // long double
	  else type = 'd';
	  kw = new PT::Kwd::Double(tk);
	  break;
        case Token::VOID:
	  type = 'v';
	  kw = new PT::Kwd::Void(tk);
	  break;
        case Token::BOOLEAN:
	  type = 'b';
	  kw = new PT::Kwd::Bool(tk);
	  break;
        default :
	  throw std::runtime_error("Parser::opt_integral_type_or_class_spec(): fatal");
      }
      p = PT::snoc(p, kw);
      is_integral = true;
    }
    else
      break;
  }
  if(is_integral)
  {
    if(flag == 'S' && type != 'c') flag = ' ';
    if(flag != ' ') encode.append(flag);
    if(type == ' ') type = 'i';		// signed, unsigned
    encode.append(type);
    return true;
  }
  if(t == Token::TYPENAME || // FIXME: 'typename' doesn't imply a class spec !
     t == Token::CLASS || t == Token::STRUCT || t == Token::UNION ||
     t == Token::UserKeyword)
  {
    PT::ClassSpec *spec;
    bool success = class_spec(spec, encode);
    p = spec;
    return success;
  }
  else if(t == Token::ENUM)
  {
    PT::EnumSpec *spec;
    bool success = enum_spec(spec, encode);
    p = spec;
    return success;
  }
  else
  {
    p = 0;
    return true;
  }
}

/*
  constructor.decl
  : '(' {arg.decl.list} ')' {cv.qualify} {throw.decl}
  {member.initializers} {'=' Constant}
*/
bool Parser::constructor_decl(PT::Node *&constructor, PT::Encoding& encode)
{
  Trace trace("Parser::constructor_decl", Trace::PARSING);
  Token op, cp;
  PT::Node *args, *cv, *throw_decl, *mi;

  if(my_lexer.get_token(op) != '(')
    return false;

  if(my_lexer.look_ahead(0) == ')')
  {
    args = 0;
    encode.start_func_args();
    encode.void_();
    encode.end_func_args();
  }
  else
    if(!parameter_declaration_list(args, encode))
      return false;

  my_lexer.get_token(cp);
  constructor = PT::list(new PT::Atom(op), args, new PT::Atom(cp));
  opt_cv_qualifier(cv);
  if(cv)
  {
    encode.cv_qualify(cv);
    constructor = PT::nconc(constructor, cv);
  }

  opt_throw_decl(throw_decl);	// ignore in this version

  if(my_lexer.look_ahead(0) == ':')
    if(member_initializers(mi))
      constructor = PT::snoc(constructor, mi);
    else
      return false;

  if(my_lexer.look_ahead(0) == '=')
  {
    Token eq, zero;
    my_lexer.get_token(eq);
    if(my_lexer.get_token(zero) != Token::Constant)
      return false;

    constructor = PT::nconc(constructor,
			       PT::list(new PT::Atom(eq), new PT::Atom(zero)));
  }

  encode.no_return_type();
  return true;
}

/*
  throw.decl : THROW '(' (name {','})* {name} ')'
*/
bool Parser::opt_throw_decl(PT::Node *&throw_decl)
{
  Trace trace("Parser::opt_throw_decl", Trace::PARSING);
  Token tk;
  int t;
  PT::Node *p = 0;

  if(my_lexer.look_ahead(0) == Token::THROW)
  {
    my_lexer.get_token(tk);
    p = PT::snoc(p, new PT::Kwd::Throw(tk));

    if(my_lexer.get_token(tk) != '(')
      return false;

    p = PT::snoc(p, new PT::Atom(tk));

    while(true)
    {
      PT::Node *q;
      PT::Encoding encode;
      t = my_lexer.look_ahead(0);
      if(t == '\0')
	return false;
      else if(t == ')')
	break;
      else if(my_ruleset & MSVC && t == Token::Ellipsis)
      {
	// for MSVC compatibility we accept 'throw(...)' declarations
	my_lexer.get_token(tk);
	p = PT::snoc(p, new PT::Atom(tk));
      }
      else if(name(q, encode))
	p = PT::snoc(p, q);
      else
	return false;

      if(my_lexer.look_ahead(0) == ','){
	my_lexer.get_token(tk);
	p = PT::snoc(p, new PT::Atom(tk));
      }
      else
	break;
    }
    if(my_lexer.get_token(tk) != ')')
      return false;

    p = PT::snoc(p, new PT::Atom(tk));
  }
  throw_decl = p;
  return true;
}

/*
  init-declarator-list : init-declarator (',' init-declarator)*

  is_statement changes the behavior of rArgDeclListOrInit().
*/
bool Parser::init_declarator_list(PT::Node *&decls, PT::Encoding& type_encode,
				  bool should_be_declarator, bool is_statement)
{
  Trace trace("Parser::init_declarator_list", Trace::PARSING);
  PT::Node *d;
  Token tk;
  PT::Encoding encode;

  decls = 0;
  while(true)
  {
    my_lexer.look_ahead(0); // force comment finding
    PT::Node *comments = wrap_comments(my_lexer.get_comments());

    encode = type_encode;
    if(!init_declarator(d, encode, should_be_declarator, is_statement))
      return false;
	
    if (d && (PT::type_of(d) == Token::ntDeclarator))
      static_cast<PT::Declarator*>(d)->set_comments(comments);

    decls = PT::snoc(decls, d);
    if(my_lexer.look_ahead(0) == ',')
    {
      my_lexer.get_token(tk);
      decls = PT::snoc(decls, new PT::Atom(tk));
    }
    else
      return true;
  };
}

/*
  init-declarator
  : ':' expression
  | declarator {'=' initialize.expr | ':' expression}
*/
bool Parser::init_declarator(PT::Node *&dw, PT::Encoding& type_encode,
			     bool should_be_declarator,
			     bool is_statement)
{
  Trace trace("Parser::init_declarator", Trace::PARSING);
  Token tk;
  PT::Encoding name_encode;

  // FIXME: This is only valid for a member-declarator.
  //        Put it into a separate method.
  if(my_lexer.look_ahead(0) == ':')
  {	// bit field
    my_lexer.get_token(tk);
    PT::Node *expr;
    if(!assign_expr(expr)) return false;
    // FIXME: why is this a list and not a PT::Declarator ?
    dw = PT::list(new PT::Atom(tk), expr);
    return true;
  }
  else
  {
    PT::Node *decl;
    if(!declarator(decl, kDeclarator, false, type_encode, name_encode,
		   should_be_declarator, is_statement))
      return false;

    int t = my_lexer.look_ahead(0);
    if(t == '=')
    {
      my_lexer.get_token(tk);
      PT::Node *expr;
      if(!initialize_expr(expr)) return false;

      dw = PT::nconc(decl, PT::list(new PT::Atom(tk), expr));
      return true;
    }
    else if(t == ':')
    {		// bit field
      my_lexer.get_token(tk);
      PT::Node *expr;
      if(!assign_expr(expr)) return false;

      dw = PT::nconc(decl, PT::list(new PT::Atom(tk), expr));
      return true;
    }
    else
    {
      dw = decl;
      return true;
    }
  }
}

/*
  declarator
  : (ptr.operator)* (name | '(' declarator ')')
	('[' expression ']')* {func.args.or.init}

  func.args.or.init
  : '(' arg.decl.list.or.init ')' {cv.qualify} {throw.decl}
  {member.initializers}

  Note: We assume that '(' declarator ')' is followed by '(' or '['.
	This is to avoid accepting a function call F(x) as a pair of
	a type F and a declarator x.  This assumption is ignored
	if should_be_declarator is true.

  Note: An argument declaration list and a function-style initializer
	take a different Ptree structure.
	e.g.
	    int f(char) ==> .. [f ( [[[char] 0]] )]
	    Point f(1)  ==> .. [f [( [1] )]]

  Note: is_statement changes the behavior of rArgDeclListOrInit().
*/
bool Parser::declarator(PT::Node *&decl, DeclKind kind, bool recursive,
			PT::Encoding& type_encode, PT::Encoding& name_encode,
			bool should_be_declarator, bool is_statement)
{
  Trace trace("Parser::declarator", Trace::PARSING);
  return declarator2(decl, kind, recursive, type_encode, name_encode,
		     should_be_declarator, is_statement, 0);
}

bool Parser::declarator2(PT::Node *&decl, DeclKind kind, bool recursive,
			 PT::Encoding& type_encode, PT::Encoding& name_encode,
			 bool should_be_declarator, bool is_statement,
			 PT::Node **declared_name)
{
  Trace trace("Parser::declarator2", Trace::PARSING);
  PT::Encoding recursive_encode;
  int t;
  bool recursive_decl = false;
  PT::Node *declared_name0 = 0;

  if(declared_name == 0)
    declared_name = &declared_name0;

  PT::Node *d;
  if(!opt_ptr_operator(d, type_encode))
    return false;

  t = my_lexer.look_ahead(0);
  if(t == '(')
  {
    char const * lex_save = my_lexer.save();
    Token op;
    my_lexer.get_token(op);
    recursive_decl = true;
    PT::Node *decl2;
    if(!declarator2(decl2, kind, true, recursive_encode, name_encode,
		    true, false, declared_name))
      return false;

    Token cp;
    if(my_lexer.get_token(cp) != ')')
    {
      if (kind != kCastDeclarator) 
	return false;
      my_lexer.restore(lex_save);
      name_encode.clear();
    }
    else
    {
      if(!should_be_declarator)
	if(kind == kDeclarator && d == 0)
	{
	  t = my_lexer.look_ahead(0);
	  if(t != '[' && t != '(')
	    return false;
	}
      d = PT::snoc(d, PT::list(new PT::Atom(op), decl2, new PT::Atom(cp)));
    }
  }
  else if(kind != kCastDeclarator)
  {
    if (t == Token::INLINE)
    {
      // TODO: store inline somehow
      Token i;
      my_lexer.get_token(i);
      t = my_lexer.look_ahead(0);
    }
    if (kind == kDeclarator || t == Token::Identifier || t == Token::Scope)
    {
      // if this is an argument declarator, "int (*)()" is valid.
      PT::Node *id;
      if(name(id, name_encode)) d = PT::snoc(d, id);
      else return false;
      *declared_name = id;
    }
  }
  else
    name_encode.clear();	// empty

  while(true)
  {
    t = my_lexer.look_ahead(0);
    if(t == '(')
    {		// function
      PT::Encoding args_encode;
      PT::Node *args, *cv, *throw_decl, *mi;
      bool is_args = true;

      Token op;
      my_lexer.get_token(op);
      ScopeGuard guard(*this, d);
      if(my_lexer.look_ahead(0) == ')')
      {
	args = 0;
	args_encode.start_func_args();
	args_encode.void_();
	args_encode.end_func_args();
      }
      else
	if(!parameter_declaration_list_or_init(args, is_args,
					       args_encode, is_statement))
	  return false;
      Token cp;
      if(my_lexer.get_token(cp) != ')')
	return false;

      if(is_args)
      {
	d = PT::nconc(d, PT::list(new PT::Atom(op), args, new PT::Atom(cp)));
	opt_cv_qualifier(cv);
	if(cv)
	{
	  args_encode.cv_qualify(cv);
	  d = PT::nconc(d, cv);
	}
      }
      else
	d = PT::snoc(d, PT::list(new PT::Atom(op), args, new PT::Atom(cp)));

      if(!args_encode.empty())
	type_encode.function(args_encode);
      
      opt_throw_decl(throw_decl);	// ignore in this version

      if(my_lexer.look_ahead(0) == ':')
	if(member_initializers(mi)) d = PT::snoc(d, mi);
	else return false;
      
      break;		// "T f(int)(char)" is invalid.
    }
    else if(t == '[')
    {	// array
      Token ob, cb;
      PT::Node *expr;
      my_lexer.get_token(ob);
      if(my_lexer.look_ahead(0) == ']') expr = 0;
      else if(!expression(expr)) return false;

      if(my_lexer.get_token(cb) != ']') return false;

      if (expr)
      {
	long size;
	if (TypeAnalysis::evaluate_const(my_symbols.current_scope(), expr, size))
	  type_encode.array(size);
	else 
	  type_encode.array();
      }
      d = PT::nconc(d, PT::list(new PT::Atom(ob), expr, new PT::Atom(cb)));
    }
    else break;
  }

  if(recursive_decl) type_encode.recursion(recursive_encode);
  if(recursive) decl = d;
  else
    if(d == 0) decl = new PT::Declarator(type_encode, name_encode, *declared_name);
    else decl = new PT::Declarator(d, type_encode, name_encode, *declared_name);

  return true;
}

/*
  ptr.operator
  : (('*' | '&' | ptr.to.member) {cv.qualify})+
*/
bool Parser::opt_ptr_operator(PT::Node *&ptrs, PT::Encoding& encode)
{
  Trace trace("Parser::opt_ptr_operator", Trace::PARSING);
  ptrs = 0;
  while(true)
  {
    int t = my_lexer.look_ahead(0);
    if(t != '*' && t != '&' && !is_ptr_to_member(0)) break;
    else
    {
      PT::Node *op, *cv;
      if(t == '*' || t == '&')
      {
	Token tk;
	my_lexer.get_token(tk);
	op = new PT::Atom(tk);
	encode.ptr_operator(t);
      }
      else
	if(!ptr_to_member(op, encode)) return false;

      ptrs = PT::snoc(ptrs, op);
      opt_cv_qualifier(cv);
      if(cv)
      {
	ptrs = PT::nconc(ptrs, cv);
	encode.cv_qualify(cv);
      }
    }
  }
  return true;
}

/*
  member.initializers
  : ':' member.init (',' member.init)*
*/
bool Parser::member_initializers(PT::Node *&init)
{
  Trace trace("Parser::member_initializer", Trace::PARSING);
  Token tk;
  PT::Node *m;

  if(my_lexer.get_token(tk) != ':')
    return false;

  init = PT::list(new PT::Atom(tk));
  if(!member_init(m))
    return false;

  init = PT::snoc(init, m);
  while(my_lexer.look_ahead(0) == ',')
  {
    my_lexer.get_token(tk);
    init = PT::snoc(init, new PT::Atom(tk));
    if(!member_init(m))
      return false;

    init = PT::snoc(init, m);
  }
  return true;
}

/*
  member.init
  : name '(' function.arguments ')'
*/
bool Parser::member_init(PT::Node *&init)
{
  Trace trace("Parser::member_init", Trace::PARSING);
  PT::Node *name, *args;
  Token tk1, tk2;
  PT::Encoding encode;

  if(!this->name(name, encode)) return false;
  if(!name->is_atom()) name = new PT::Name(name, encode);
  if(my_lexer.get_token(tk1) != '(') return false;
  if(!function_arguments(args)) return false;
  if(my_lexer.get_token(tk2) != ')') return false;

  init = PT::list(name, new PT::Atom(tk1), args, new PT::Atom(tk2));
  return true;
}

/*
  name : {'::'} name2 ('::' name2)*

  name2
  : Identifier {template.args}
  | '~' Identifier
  | OPERATOR operator.name {template.args}

  Don't use this function for parsing an expression
  It always regards '<' as the beginning of template arguments.
*/
bool Parser::name(PT::Node *&id, PT::Encoding& encode)
{
  Trace trace("Parser::name", Trace::PARSING);
  Token tk, tk2;
  int t;
  int length = 0;

  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    id = PT::list(new PT::Atom(tk));
    encode.global_scope();
    ++length;
  }
  else
  {
    id = 0;

    // gcc keyword typeof(name) means type of the given name
    if(my_lexer.look_ahead(0) == Token::TYPEOF)
    {
      // TODO: Do proper type analysis.
      encode.anonymous();
      return typeof_expr(id);
    }
  }
  while(true)
  {
    t = my_lexer.get_token(tk);
    if(t == Token::TEMPLATE)
    {
      // Skip template token, next will be identifier
      t = my_lexer.get_token(tk);
    }
    if(t == Token::Identifier)
    {
      PT::Node *n = new PT::Identifier(tk);
      t = my_lexer.look_ahead(0);
      if(t == '<')
      {
	PT::Node *args;
	PT::Encoding args_encode;
	if(!template_args(args, args_encode))
	  return false;

	encode.template_(n, args_encode);
	++length;
	n = PT::list(n, args);
	t = my_lexer.look_ahead(0);
      }
      else
      {
	encode.simple_name(n);
	++length;
      }
      if(t == Token::Scope)
      {
	my_lexer.get_token(tk);
	id = PT::nconc(id, PT::list(n, new PT::Atom(tk)));
      }
      else
      {
	id = id ? PT::snoc(id, n) : n;
	if(length > 1) encode.qualified(length);
	return true;
      }
    }
    else if(t == '~')
    {
      if(my_lexer.look_ahead(0) != Token::Identifier)
	return false;

      my_lexer.get_token(tk2);
      PT::Node *class_name = new PT::Atom(tk2);
      PT::Node *dt = PT::list(new PT::Atom(tk), class_name);
      id = id ? PT::snoc(id, dt) : dt;
      encode.destructor(class_name);
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else if(t == Token::OPERATOR)
    {
      PT::Node *op;
      PT::Node *opf;
      if(!operator_name(op, encode)) return false;
      
      t = my_lexer.look_ahead(0);
      if(t != '<') opf = PT::list(new PT::Kwd::Operator(tk), op);
      else
      {
	PT::Node *args;
	PT::Encoding args_encode;
	if(!template_args(args, args_encode)) return false;

	// here, I must merge args_encode into encode.
	// I'll do it in future. :p

	opf = PT::list(new PT::Kwd::Operator(tk), op, args);
      }
      id = id ? PT::snoc(id, opf) : opf;
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else return false;
  }
}

/*
  operator.name
  : '+' | '-' | '*' | '/' | '%' | '^' | '&' | '|' | '~'
  | '!' | '=' | '<' | '>' | AssignOp | ShiftOp | EqualOp
  | RelOp | LogAndOp | LogOrOp | IncOp | ',' | PmOp | ArrowOp
  | NEW {'[' ']'}
  | DELETE {'[' ']'}
  | '(' ')'
  | '[' ']'
  | cast.operator.name
*/
bool Parser::operator_name(PT::Node *&name, PT::Encoding &encode)
{
  Trace trace("Parser::operator_name", Trace::PARSING);
    Token tk;

    int t = my_lexer.look_ahead(0);
    if(t == '+' || t == '-' || t == '*' || t == '/' || t == '%' || t == '^'
       || t == '&' || t == '|' || t == '~' || t == '!' || t == '=' || t == '<'
       || t == '>' || t == Token::AssignOp || t == Token::ShiftOp || t == Token::EqualOp
       || t == Token::RelOp || t == Token::LogAndOp || t == Token::LogOrOp || t == Token::IncOp
       || t == ',' || t == Token::PmOp || t == Token::ArrowOp)
    {
      my_lexer.get_token(tk);
      name = new PT::Atom(tk);
      encode.simple_name(name);
      return true;
    }
    else if(t == Token::NEW || t == Token::DELETE)
    {
      my_lexer.get_token(tk);
      if(my_lexer.look_ahead(0) != '[')
      {
	if (t == Token::NEW) name = new PT::Kwd::New(tk);
	else name = new PT::Kwd::Delete(tk);
	encode.simple_name(name);
	return true;
      }
      else
      {
	if (t == Token::NEW) name = PT::list(new PT::Kwd::New(tk));
	else name = PT::list(new PT::Kwd::Delete(tk));
	my_lexer.get_token(tk);
	name = PT::snoc(name, new PT::Atom(tk));
	if(my_lexer.get_token(tk) != ']') return false;

	name = PT::snoc(name, new PT::Atom(tk));
	if(t == Token::NEW) encode.append_with_length("new[]", 5);
	else encode.append_with_length("delete[]", 8);
	return true;
      }
    }
    else if(t == '(')
    {
      my_lexer.get_token(tk);
      name = PT::list(new PT::Atom(tk));
      if(my_lexer.get_token(tk) != ')') return false;

      encode.append_with_length("()", 2);
      name = PT::snoc(name, new PT::Atom(tk));
      return true;
    }
    else if(t == '[')
    {
      my_lexer.get_token(tk);
      name = PT::list(new PT::Atom(tk));
      if(my_lexer.get_token(tk) != ']') return false;

      encode.append_with_length("[]", 2);
      name = PT::snoc(name, new PT::Atom(tk));
      return true;
    }
    else return cast_operator_name(name, encode);
}

/*
  cast.operator.name
  : {cv.qualify} (integral.or.class.spec | name) {cv.qualify}
    {(ptr.operator)*}
*/
bool Parser::cast_operator_name(PT::Node *&name, PT::Encoding &encode)
{
  Trace trace("Parser::cast_operator_name", Trace::PARSING);
  PT::Node *cv1, *cv2, *type_name, *ptr;
  PT::Encoding type_encode;

  if(!opt_cv_qualifier(cv1)) return false;
  if(!opt_integral_type_or_class_spec(type_name, type_encode)) return false;
  if(type_name == 0)
  {
    type_encode.clear();
    if(!this->name(type_name, type_encode)) return false;
  }

  if(!opt_cv_qualifier(cv2)) return false;
  if(cv1)
    if(cv2 == 0) type_name = PT::snoc(cv1, type_name);
    else type_name = PT::nconc(cv1, PT::cons(type_name, cv2));
  else if(cv2) type_name = PT::cons(type_name, cv2);
  type_encode.cv_qualify(cv1, cv2);
  if(!opt_ptr_operator(ptr, type_encode)) return false;

  encode.cast_operator(type_encode);
  if(ptr == 0)
  {
    name = type_name;
    return true;
  }
  else
  {
    name = PT::list(type_name, ptr);
    return true;
  }
}

/*
  ptr.to.member
  : {'::'} (identifier {template.args} '::')+ '*'
*/
bool Parser::ptr_to_member(PT::Node *&ptr_to_mem, PT::Encoding &encode)
{
  Trace trace("Parser::ptr_to_member", Trace::PARSING);
  Token tk;
  PT::Node *p, *n;
  PT::Encoding pm_encode;
  int length = 0;

  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    p = PT::list(new PT::Atom(tk));
    pm_encode.global_scope();
    ++length;
  }
  else p = 0;

  while(true)
  {
    if(my_lexer.get_token(tk) == Token::Identifier)
      n = new PT::Atom(tk);
    else
      return false;

    int t = my_lexer.look_ahead(0);
    if(t == '<')
    {
      PT::Node *args;
      PT::Encoding args_encode;
      if(!template_args(args, args_encode)) return false;

      pm_encode.template_(n, args_encode);
      ++length;
      n = PT::list(n, args);
      t = my_lexer.look_ahead(0);
    }
    else
    {
      pm_encode.simple_name(n);
      ++length;
    }

    if(my_lexer.get_token(tk) != Token::Scope) return false;

    p = PT::nconc(p, PT::list(n, new PT::Atom(tk)));
    if(my_lexer.look_ahead(0) == '*')
    {
      my_lexer.get_token(tk);
      p = PT::snoc(p, new PT::Atom(tk));
      break;
    }
  }
  ptr_to_mem = p;
  encode.ptr_to_member(pm_encode, length);
  return true;
}

/*
  template.args
  : '<' '>'
  | '<' template.argument {',' template.argument} '>'

  template.argument
  : type.name
  | conditional.expr
*/
bool Parser::template_args(PT::Node *&temp_args, PT::Encoding &encode)
{
  Trace trace("Parser::template_args", Trace::PARSING);
  Token tk1, tk2;
  PT::Encoding type_encode;

  if(my_lexer.get_token(tk1) != '<') return false;

  // in case of Foo<>
  if(my_lexer.look_ahead(0) == '>')
  {
    my_lexer.get_token(tk2);
    temp_args = PT::list(new PT::Atom(tk1), new PT::Atom(tk2));
    return true;
  }

  PT::Node *args = 0;
  while(true)
  {
    PT::Node *a;
    const char* pos = my_lexer.save();
    type_encode.clear();

    // Prefer type name, but if not ',' or '>' then must be expression
    if(type_id(a, type_encode) && 
       (my_lexer.look_ahead(0) == ',' || my_lexer.look_ahead(0) == '>'))
      encode.append(type_encode);
    else
    {
      // FIXME: find the right place to put this.
      PGuard<bool> guard(*this, &Parser::my_gt_is_operator);
      my_gt_is_operator = false;
      my_lexer.restore(pos);	
      if(!conditional_expr(a)) return false;
      encode.value_temp_param();
    }
    args = PT::snoc(args, a);
    switch(my_lexer.get_token(tk2))
    {
      case '>' :
	temp_args = PT::list(new PT::Atom(tk1), args, new PT::Atom(tk2));
	return true;
      case ',' :
	args = PT::snoc(args, new PT::Atom(tk2));
	break;
      case Token::ShiftOp :
	// parse error !
	return false;
      default :
	return false;
    }
  }
}

/*
  arg.decl.list.or.init
    : para.decl.list
    | function.arguments

  This rule accepts function.arguments to parse declarations like:
	Point p(1, 3);
  "(1, 3)" is arg.decl.list.or.init.

  If maybe_init is true, we first examine whether tokens construct
  function.arguments.  This ordering is significant if tokens are
	Point p(s, t);
  s and t can be type names or variable names.
*/
bool Parser::parameter_declaration_list_or_init(PT::Node *&arglist, bool &is_args,
						PT::Encoding &encode, bool maybe_init)
{
  Trace trace("Parser::parameter_declaration_list_or_init", Trace::PARSING);
  const char* pos = my_lexer.save();
  if(maybe_init)
  {
    if(function_arguments(arglist))
      if(my_lexer.look_ahead(0) == ')')
      {
	is_args = false;
	encode.clear();
	return true;
      }

    my_lexer.restore(pos);
    return(is_args = parameter_declaration_list(arglist, encode));
  }
  else 
    if(is_args = parameter_declaration_list(arglist, encode)) return true;
    else
    {
      my_lexer.restore(pos);
      encode.clear();
      return function_arguments(arglist);
    }
}

/*
  para.decl.list
    : empty
    | arg.declaration ( ',' arg.declaration )* {{ ',' } Ellipses}
*/
bool Parser::parameter_declaration_list(PT::Node *&arglist,
					PT::Encoding& encode)
{
  Trace trace("Parser::parameter_declaration_list", Trace::PARSING);
  PT::Node *list = 0;
  PT::Encoding arg_encode;

  encode.start_func_args();
  while(true)
  {
    PT::ParameterDeclaration *pdecl;
    arg_encode.clear();
    int t = my_lexer.look_ahead(0);
    if(t == ')')
    {
      if(list == 0) encode.void_();
      arglist = list;
      break;
    }
    else if(t == Token::Ellipsis)
    {
      Token tk;
      my_lexer.get_token(tk);
      encode.ellipsis_arg();
      arglist = PT::snoc(list, new PT::Atom(tk));
      break;
    }
    else if(parameter_declaration(pdecl, arg_encode))
    {
      encode.append(arg_encode);
      list = PT::snoc(list, pdecl);
      t = my_lexer.look_ahead(0);
      if(t == ',')
      {
	Token tk;
	my_lexer.get_token(tk);
	list = PT::snoc(list, new PT::Atom(tk));
      }
      else if(t != ')' && t != Token::Ellipsis) return false;
    }
    else
    {
      arglist = 0;
      return false;
    }
  }
  encode.end_func_args();
  return true;
}

//. parameter-declaration:
//.   decl-specifier-seq declarator
//.   decl-specifier-seq declarator = assignment-expression
//.   decl-specifier-seq abstract-declarator [opt]
//.   decl-specifier-seq abstract-declarator [opt] = assignment-expression
bool Parser::parameter_declaration(PT::ParameterDeclaration *&para,
				   PT::Encoding &encode)
{
  Trace trace("Parser::parameter_declaration", Trace::PARSING);
  PT::Node *header;
  Token tk;
  PT::Encoding name_encode;

  switch(my_lexer.look_ahead(0))
  {
    case Token::REGISTER:
      my_lexer.get_token(tk);
      header = new PT::Kwd::Register(tk);
      break;
    case Token::UserKeyword:
      if(!userdef_keyword(header)) return false;
      break;
    default:
      header = 0;
      break;
  }

  PT::Node *type_name;
  if(!type_specifier(type_name, true, encode)) return false;

  PT::Node *decl;
  if(!declarator(decl, kArgDeclarator, false, encode, name_encode, true)) return false;
  para = new PT::ParameterDeclaration(header, type_name, decl);
  declare(para);
  Token::Type type = my_lexer.look_ahead(0);
  if(type == '=')
  {
    my_lexer.get_token(tk);
    PT::Node *init;
    if(!initialize_expr(init)) return false;    
    decl = PT::nconc(decl, PT::list(new PT::Atom(tk), init));
  }
  return true;
}

/*
  initialize.expr
  : expression
  | '{' initialize.expr (',' initialize.expr)* {','} '}'
*/
bool Parser::initialize_expr(PT::Node *&exp)
{
  Trace trace("Parser::initialize_expr", Trace::PARSING);
  Token tk;
  PT::Node *e, *elist;

  if(my_lexer.look_ahead(0) != '{') return assign_expr(exp);
  else
  {
    my_lexer.get_token(tk);
    PT::Node *ob = new PT::Atom(tk);
    elist = 0;
    int t = my_lexer.look_ahead(0);
    while(t != '}')
    {
      if(!initialize_expr(e))
      {
	if(!mark_error()) return false; // too many errors

	skip_to('}');
	my_lexer.get_token(tk);
	exp = PT::list(ob, 0, new PT::Atom(tk));
	return true;		// error recovery
      }

      elist = PT::snoc(elist, e);
      t = my_lexer.look_ahead(0);
      if(t == '}') break;
      else if(t == ',')
      {
	my_lexer.get_token(tk);
	elist = PT::snoc(elist, new PT::Atom(tk));
	t = my_lexer.look_ahead(0);
      }
      else
      {
	if(!mark_error()) return false; // too many errors

	skip_to('}');
	my_lexer.get_token(tk);
	exp = PT::list(ob, 0, new PT::Atom(tk));
	return true;		// error recovery
      }
    }
    my_lexer.get_token(tk);
    exp = new PT::Brace(ob, elist, new PT::Atom(tk));
    return true;
  }
}

/*
  function.arguments
  : empty
  | expression (',' expression)*

  This assumes that the next token following function.arguments is ')'.
*/
bool Parser::function_arguments(PT::Node *&args)
{
  Trace trace("Parser::function_arguments", Trace::PARSING);
  PT::Node *exp;
  Token tk;

  args = 0;
  if(my_lexer.look_ahead(0) == ')') return true;
  
  while(true)
  {
    if(!assign_expr(exp)) return false;

    args = PT::snoc(args, exp);
    if(my_lexer.look_ahead(0) != ',') return true;
    else
    {
      my_lexer.get_token(tk);
      args = PT::snoc(args, new PT::Atom(tk));
    }
  }
}

/*
  enum.spec
  : ENUM Identifier
  | ENUM {Identifier} '{' {enum.body} '}'
*/
bool Parser::enum_spec(PT::EnumSpec *&spec, PT::Encoding &encode)
{
  Trace trace("Parser::enum_spec", Trace::PARSING);
  Token tk, tk2;
  PT::Node *body;

  if(my_lexer.get_token(tk) != Token::ENUM) return false;

  spec = new PT::EnumSpec(new PT::Atom(tk));
  int t = my_lexer.get_token(tk);
  if(t == Token::Identifier)
  {
    PT::Node *name = new PT::Atom(tk);
    encode.simple_name(name);
    spec->set_encoded_name(encode);
    spec = PT::snoc(spec, name);
    if(my_lexer.look_ahead(0) == '{') t = my_lexer.get_token(tk);
    else return true;
  }
  else
  {
    encode.anonymous();
    spec->set_encoded_name(encode);
    spec = PT::snoc(spec, 0);
  }
  if(t != '{') return false;
  
  if(my_lexer.look_ahead(0) == '}') body = 0;
  else if(!enum_body(body)) return false;

  if(my_lexer.get_token(tk2) != '}') return false;

  spec = PT::snoc(spec, 
		  new PT::Brace(new PT::Atom(tk), body,
				new PT::CommentedAtom(tk2, wrap_comments(my_lexer.get_comments()))));
  declare(spec);
  return true;
}

/*
  enum.body
  : Identifier {'=' expression} (',' Identifier {'=' expression})* {','}
*/
bool Parser::enum_body(PT::Node *&body)
{
  Trace trace("Parser::enum_body", Trace::PARSING);
  Token tk, tk2;
  PT::Node *name, *exp;

  body = 0;
  while(true)
  {
    if(my_lexer.look_ahead(0) == '}') return true;
    if(my_lexer.get_token(tk) != Token::Identifier) return false;

    PT::Node *comments = wrap_comments(my_lexer.get_comments());
    
    if(my_lexer.look_ahead(0, tk2) != '=')
      name = new PT::CommentedAtom(tk, comments);
    else
    {
      my_lexer.get_token(tk2);
      if(!assign_expr(exp))
      {
	if(!mark_error()) return false; // too many errors

	skip_to('}');
	body = 0; // empty
	return true;		// error recovery
      }
      name = PT::list(new PT::CommentedAtom(tk, comments), new PT::Atom(tk2), exp);
    }

    if(my_lexer.look_ahead(0) != ',')
    {
      body = PT::snoc(body, name);
      return true;
    }
    else
    {
      my_lexer.get_token(tk);
      body = PT::nconc(body, PT::list(name, new PT::Atom(tk)));
    }
  }
}

/*
  class.spec
  : {userdef.keyword} class.key class.body
  | {userdef.keyword} class.key name {class.body}
  | {userdef.keyword} class.key name ':' base.specifiers class.body

  class.key
  : CLASS | STRUCT | UNION
*/
bool Parser::class_spec(PT::ClassSpec *&spec, PT::Encoding &encode)
{
  Trace trace("Parser::class_spec", Trace::PARSING);
  PT::Node *head, *bases, *name;
  PT::ClassBody *body;
  Token tk;

  head = 0;
  if(my_lexer.look_ahead(0) == Token::UserKeyword)
    if(!userdef_keyword(head)) return false;

  int t = my_lexer.get_token(tk);
  PT::Keyword *kwd;
  switch (t)
  {
    case Token::CLASS: kwd = new PT::Kwd::Class(tk); break;
    case Token::STRUCT: kwd = new PT::Kwd::Struct(tk); break;
    case Token::UNION: kwd = new PT::Kwd::Union(tk); break;
      // FIXME: The following shouldn't be here.
      //        See opt_integral_type_or_class_spec for why this is needed.
    case Token::TYPENAME: kwd = new PT::Kwd::Typename(tk); break;
    default: return false;
  }
  spec = new PT::ClassSpec(kwd, 0, my_comments);
  my_comments = 0;
  if(head != 0) spec = new PT::ClassSpec(head, spec, 0);

  if(my_lexer.look_ahead(0) == '{') // anonymous class
  {
    encode.anonymous();
    // FIXME: why is the absense of a name marked by a list(0, 0) ?
    spec = PT::snoc(spec, PT::list(0, 0));
  }
  else
  {
    if(!this->name(name, encode)) return false;
    spec = PT::snoc(spec, name);
    t = my_lexer.look_ahead(0);
    if(t == ':')
    {
      if(!base_clause(bases)) return false;

      spec = PT::snoc(spec, bases);
    }
    else if(t == '{') spec = PT::snoc(spec, 0);
    else
    {
      spec->set_encoded_name(encode);
      if (!my_in_template_decl) declare(spec);
      return true;	// class.key Identifier
    }
  }
  spec->set_encoded_name(encode);
  {
    ScopeGuard guard(*this, spec);
    if(!class_body(body)) return false;
  }
  spec = PT::snoc(spec, body);
  if (!my_in_template_decl) declare(spec);

  return true;
}

//. base-clause:
//.   : base-specifier-list
//. base-specifier-list:
//.   base-specifier
//.   base-specifier-list , base-specifier
//. base-specifier:
//.   virtual access-specifier [opt] :: [opt] nested-name-specifier [opt] class-name
//.   access-specifier virtual [opt] :: [opt] nested-name-specifier [opt] class-name
bool Parser::base_clause(PT::Node *&bases)
{
  Trace trace("Parser::base_clause", Trace::PARSING);
  Token tk;
  int t;
  PT::Node *name;
  PT::Encoding encode;

  if(my_lexer.get_token(tk) != ':') return false;

  bases = PT::list(new PT::Atom(tk));
  while(true)
  {
    PT::Node *super = 0;
    t = my_lexer.look_ahead(0);
    if(t == Token::VIRTUAL)
    {
      my_lexer.get_token(tk);
      super = PT::snoc(super, new PT::Kwd::Virtual(tk));
      t = my_lexer.look_ahead(0);
    }

    if(t == Token::PUBLIC | t == Token::PROTECTED | t == Token::PRIVATE)
    {
      PT::Node *lf;
      switch(my_lexer.get_token(tk))
      {
        case Token::PUBLIC:
	  lf = new PT::Kwd::Public(tk);
	  break;
        case Token::PROTECTED:
	  lf = new PT::Kwd::Protected(tk);
	  break;
        case Token::PRIVATE:
	  lf = new PT::Kwd::Private(tk);
	  break;
        default :
	  throw std::runtime_error("Parser::base_clause(): fatal");
      }
      
      super = PT::snoc(super, lf);
      t = my_lexer.look_ahead(0);
    }

    // FIXME: test whether 'virtual' has already been encountered above.
    if(t == Token::VIRTUAL)
    {
      my_lexer.get_token(tk);
      super = PT::snoc(super, new PT::Kwd::Virtual(tk));
    }

    encode.clear();
    if(!this->name(name, encode)) return false;

    if(!name->is_atom()) name = new PT::Name(name, encode);

    super = PT::snoc(super, name);
    bases = PT::snoc(bases, super);
    if(my_lexer.look_ahead(0) != ',') return true;
    else
    {
      my_lexer.get_token(tk);
      bases = PT::snoc(bases, new PT::Atom(tk));
    }
  }
}

/*
  class.body : '{' (class.members)* '}'
*/
bool Parser::class_body(PT::ClassBody *&body)
{
  Trace trace("Parser::class_body", Trace::PARSING);
  Token tk;
  PT::Node *mems, *m;

  if(my_lexer.get_token(tk) != '{') return false;

  PT::Node *ob = new PT::Atom(tk);
  mems = 0;
  while(my_lexer.look_ahead(0) != '}')
  {
    if(!class_member(m))
    {
      if(!mark_error()) return false;	// too many errors

      skip_to('}');
      my_lexer.get_token(tk);
      body = new PT::ClassBody(ob, 0, new PT::Atom(tk));
      return true;	// error recovery
    }

    my_lexer.get_comments();
    mems = PT::snoc(mems, m);
  }

  my_lexer.get_token(tk);
  body = new PT::ClassBody(ob, mems, 
			   new PT::CommentedAtom(tk, wrap_comments(my_lexer.get_comments())));
  return true;
}

/*
  class.member
  : (PUBLIC | PROTECTED | PRIVATE) ':'
  | user.access.spec
  | ';'
  | type.def
  | template.decl
  | using.declaration
  | metaclass.decl
  | declaration
  | access.decl

  Note: if you modify this function, see ClassWalker::TranslateClassSpec()
  as well.
*/
bool Parser::class_member(PT::Node *&mem)
{
  Trace trace("Parser::class_member", Trace::PARSING);
  Token tk1, tk2;

  int t = my_lexer.look_ahead(0);
  if(t == Token::PUBLIC || t == Token::PROTECTED || t == Token::PRIVATE)
  {
    PT::Node *lf;
    switch(my_lexer.get_token(tk1))
    {
      case Token::PUBLIC :
	lf = new PT::Kwd::Public(tk1);
	break;
      case Token::PROTECTED :
	lf = new PT::Kwd::Protected(tk1);
	break;
      case Token::PRIVATE :
	lf = new PT::Kwd::Private(tk1);
	break;
      default :
	throw std::runtime_error("Parser::class_member(): fatal");
    }

    PT::Node *comments = wrap_comments(my_lexer.get_comments());
    if(my_lexer.get_token(tk2) != ':') return false;

    mem = new PT::AccessSpec(lf, PT::list(new PT::Atom(tk2)), comments);
    return true;
  }
  else if(t == Token::UserKeyword4) return user_access_spec(mem);
  else if(t == ';') return null_declaration(mem);
  else if(t == Token::TYPEDEF)
  {
    PT::Typedef *td;
    bool result = typedef_(td);
    mem = td;
    return result;
  }
  else if(t == Token::TEMPLATE) return template_decl(mem);
  else if(t == Token::USING)
  {
    if (my_lexer.look_ahead(1) == Token::NAMESPACE)
    {
      PT::UsingDirective *udir;
      bool result = using_directive(udir);
      declare(udir);
      mem = udir;
      return result;
    }
    else
    {
      PT::UsingDeclaration *udecl;
      bool result = using_declaration(udecl);
      declare(udecl);
      mem = udecl;
      return result;
    }
  }
  else if(t == Token::METACLASS) return metaclass_decl(mem);
  else
  {
    const char *pos = my_lexer.save();
    PT::Declaration *decl;
    if(declaration(decl))
    {
      PT::Node *comments = wrap_comments(my_lexer.get_comments());
      if (comments) set_declarator_comments(decl, comments);
      declare(decl);
      mem = decl;
      return true;
    }
    my_lexer.restore(pos);
    return access_decl(mem);
  }
}

/*
  access.decl
  : name ';'		e.g. <qualified class>::<member name>;
*/
bool Parser::access_decl(PT::Node *&mem)
{
  Trace trace("Parser::access_decl", Trace::PARSING);
  PT::Node *name;
  PT::Encoding encode;
  Token tk;

  if(!this->name(name, encode)) return false;

  if(my_lexer.get_token(tk) != ';') return false;

  mem = new PT::AccessDecl(new PT::Name(name, encode), PT::list(new PT::Atom(tk)));
  return true;
}

/*
  user.access.spec
  : UserKeyword4 ':'
  | UserKeyword4 '(' function.arguments ')' ':'
*/
bool Parser::user_access_spec(PT::Node *&mem)
{
  Trace trace("Parser::user_access_spec", Trace::PARSING);
  Token tk1, tk2, tk3, tk4;
  PT::Node *args;

  if(my_lexer.get_token(tk1) != Token::UserKeyword4) return false;

  int t = my_lexer.get_token(tk2);
  if(t == ':')
  {
    mem = new PT::UserAccessSpec(new PT::Atom(tk1), PT::list(new PT::Atom(tk2)));
    return true;
  }
  else if(t == '(')
  {
    if(!function_arguments(args)) return false;
    if(my_lexer.get_token(tk3) != ')') return false;
    if(my_lexer.get_token(tk4) != ':') return false;

    mem = new PT::UserAccessSpec(new PT::Atom(tk1),
				 PT::list(new PT::Atom(tk2), args,
					  new PT::Atom(tk3),
					  new PT::Atom(tk4)));
    return true;
  }
  else return false;
}

//. expression:
//.   assignment-expression
//.   expression , assignment-expression
bool Parser::expression(PT::Node *&exp)
{
  Trace trace("Parser::expression", Trace::PARSING);

  if(!assign_expr(exp)) return false;
  while(my_lexer.look_ahead(0) == ',')
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!assign_expr(right)) return false;

    exp = new PT::Expression(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. assignment-expression:
//.   conditional-expression
//.   logical-or-expression assignment-operator assignment-expression
//.   throw-expression
bool Parser::assign_expr(PT::Node *&exp)
{
  Trace trace("Parser::assign_expr", Trace::PARSING);

  Token::Type t = my_lexer.look_ahead(0);
  
  if (t == Token::THROW) return throw_expr(exp);

  PT::Node *left;
  if(!conditional_expr(left)) return false;
  t = my_lexer.look_ahead(0);
  if(t != '=' && t != Token::AssignOp) exp = left;
  else
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!assign_expr(right)) return false;
    
    exp = new PT::AssignExpr(left, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. conditional-expression:
//.   logical-or-expression
//.   logical-or-expression ? expression : assignment-expression
bool Parser::conditional_expr(PT::Node *&exp)
{
  Trace trace("Parser::conditional_expr", Trace::PARSING);

  if(!logical_or_expr(exp)) return false;
  if(my_lexer.look_ahead(0) == '?')
  {
    Token tk1;
    my_lexer.get_token(tk1);
    PT::Node *then;
    if(!expression(then)) return false;
    Token tk2;
    if(my_lexer.get_token(tk2) != ':') return false;
    PT::Node *otherwise;
    if(!assign_expr(otherwise)) return false;

    exp = new PT::CondExpr(exp, PT::list(new PT::Atom(tk1), then,
					 new PT::Atom(tk2), otherwise));
  }
  return true;
}

//. logical-or-expression:
//.   logical-and-expression
//.   logical-or-expression || logical-and-expression
bool Parser::logical_or_expr(PT::Node *&exp)
{
  Trace trace("Parser::logical_or_expr", Trace::PARSING);

  if(!logical_and_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == Token::LogOrOp)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!logical_and_expr(right)) return false;
    
    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. logical-and-expression:
//.   inclusive-or-expression
//.   logical-and-expr && inclusive-or-expression
bool Parser::logical_and_expr(PT::Node *&exp)
{
  Trace trace("Parser::logical_and_expr", Trace::PARSING);

  if(!inclusive_or_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == Token::LogAndOp)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!inclusive_or_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. inclusive-or-expression:
//.   exclusive-or-expression
//.   inclusive-or-expression | exclusive-or-expression
bool Parser::inclusive_or_expr(PT::Node *&exp)
{
  Trace trace("Parser::inclusive_or_expr", Trace::PARSING);

  if(!exclusive_or_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == '|')
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!exclusive_or_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. exclusive-or-expression:
//.   and-expression
//.   exclusive-or-expression ^ and-expression
bool Parser::exclusive_or_expr(PT::Node *&exp)
{
  Trace trace("Parser::exclusive_or_expr", Trace::PARSING);

  if(!and_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == '^')
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!and_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. and-expression:
//.   equality-expression
//.   and-expression & equality-expression
bool Parser::and_expr(PT::Node *&exp)
{
  Trace trace("Parser::and_expr", Trace::PARSING);

  if(!equality_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == '&')
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!equality_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. equality-expression:
//.   relational-expression
//.   equality-expression == relational-expression
//.   equality-expression != relational-expression
bool Parser::equality_expr(PT::Node *&exp)
{
  Trace trace("Parser::equality_expr", Trace::PARSING);

  if(!relational_expr(exp)) return false;
  while(my_lexer.look_ahead(0) == Token::EqualOp)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!relational_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. relational-expression:
//.   shift-expression
//.   relational-expression < shift-expression
//.   relational-expression > shift-expression
//.   relational-expression <= shift-expression
//.   relational-expression >= shift-expression
bool Parser::relational_expr(PT::Node *&exp)
{
  Trace trace("Parser::relational_expr", Trace::PARSING);

  if(!shift_expr(exp)) return false;

  Token::Type t;
  while(t = my_lexer.look_ahead(0),
	(t == Token::RelOp || t == '<' || (t == '>' && my_gt_is_operator)))
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!shift_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. shift-expression:
//.   additive-expression
//.   shift-expression << additive-expression
//.   shift-expression >> additive-expression
bool Parser::shift_expr(PT::Node *&exp)
{
  Trace trace("Parser::shift_expr", Trace::PARSING);

  if(!additive_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == Token::ShiftOp)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!additive_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. additive-expression:
//.   multiplicative-expression
//.   additive-expression + multiplicative-expression
//.   additive-expression - multiplicative-expression
bool Parser::additive_expr(PT::Node *&exp)
{
  Trace trace("Parser::additive_expr", Trace::PARSING);

  if(!multiplicative_expr(exp)) return false;

  Token::Type t;
  while(t = my_lexer.look_ahead(0), (t == '+' || t == '-'))
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!multiplicative_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. multiplicative-expression:
//.   pm-expression
//.   multiplicative-expression * pm-expression
//.   multiplicative-expression / pm-expression
//.   multiplicative-expression % pm-expression
bool Parser::multiplicative_expr(PT::Node *&exp)
{
  Trace trace("Parser::multiplicative_expr", Trace::PARSING);

  if(!pm_expr(exp)) return false;

  Token::Type t;
  while(t = my_lexer.look_ahead(0), (t == '*' || t == '/' || t == '%'))
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!pm_expr(right)) return false;

    exp = new PT::InfixExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. pm-expression:
//.   cast-expression
//.   pm-expression .* cast-expression
//.   pm-expression ->* cast-expression
bool Parser::pm_expr(PT::Node *&exp)
{
  Trace trace("Parser::pm_expr", Trace::PARSING);

  if(!cast_expr(exp)) return false;
  while(my_lexer.look_ahead(0) == Token::PmOp)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!cast_expr(right)) return false;

    exp = new PT::PmExpr(exp, PT::list(new PT::Atom(tk), right));
  }
  return true;
}

//. cast-expression:
//.   unary-expression
//.   ( type-id ) cast-expression
bool Parser::cast_expr(PT::Node *&exp)
{
  Trace trace("Parser::cast_expr", Trace::PARSING);
  if(my_lexer.look_ahead(0) != '(') return unary_expr(exp);
  else
  {
    Token tk1, tk2;
    PT::Node *tname;
    const char* pos = my_lexer.save();
    my_lexer.get_token(tk1);
    if(type_id(tname))
      if(my_lexer.get_token(tk2) == ')')
	if(cast_expr(exp))
	{
	  exp = new PT::CastExpr(new PT::Atom(tk1),
				 PT::list(tname, new PT::Atom(tk2), exp));
	  return true;
	}

    my_lexer.restore(pos);
    return unary_expr(exp);
  }
}

//. type-id:
//.   type-specifier-seq abstract-declarator [opt]
bool Parser::type_id(PT::Node *&tname)
{
  PT::Encoding type_encode;
  return type_id(tname, type_encode);
}

bool Parser::type_id(PT::Node *&tname, PT::Encoding &type_encode)
{
  Trace trace("Parser::type_id", Trace::PARSING);
  PT::Node *type_name, *arg;
  PT::Encoding name_encode;

  if(!type_specifier(type_name, true, type_encode)) return false;
  if(!declarator(arg, kCastDeclarator, false, type_encode, name_encode, false))
    return false;

  tname = PT::list(type_name, arg); 
  return true;
}

//. unary-expression:
//.   postfix-expression
//.   ++ cast-expression
//.   -- cast-expression
//.   unary-operator cast-expression
//.   sizeof unary-expression
//.   sizeof ( type-id )
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
bool Parser::unary_expr(PT::Node *&exp)
{
  Trace trace("Parser::unary_expr", Trace::PARSING);
  Token::Type t = my_lexer.look_ahead(0);
  if(t == '*' || t == '&' || t == '+' || t == '-' || t == '!' || t == '~' ||
     t == Token::IncOp)
  {
    Token tk;
    my_lexer.get_token(tk);
    PT::Node *right;
    if(!cast_expr(right)) return false;

    exp = new PT::UnaryExpr(new PT::Atom(tk), PT::list(right));
    return true;
  }
  else if(t == Token::SIZEOF) return sizeof_expr(exp);
  // FIXME: can this be removed ?
  //   else if(t == Token::THROW) return throw_expr(exp);
  else if(is_allocate_expr(t)) return allocate_expr(exp);
  else return postfix_expr(exp);
}

//. throw-expression:
//.   throw assignment-expression
bool Parser::throw_expr(PT::Node *&exp)
{
  Trace trace("Parser::throw_expr", Trace::PARSING);
  Token tk;
  if(my_lexer.get_token(tk) != Token::THROW) return false;

  int t = my_lexer.look_ahead(0);
  PT::Node *e = 0;
  // FIXME: what is that ??
  if(t == ':' || t == ';') e = 0;
  else if(!assign_expr(e)) return false;

  exp = new PT::ThrowExpr(new PT::Kwd::Throw(tk), PT::list(e));
  return true;
}

//. sizeof-expression:
//.   sizeof unary-expression
//.   sizeof ( type-id )
bool Parser::sizeof_expr(PT::Node *&exp)
{
  Trace trace("Parser::sizeof_expr", Trace::PARSING);
  Token tk;

  if(my_lexer.get_token(tk) != Token::SIZEOF) return false;
  if(my_lexer.look_ahead(0) == '(')
  {
    Token op, cp;
    PT::Node *tname;
    char const *tag = my_lexer.save();
    my_lexer.get_token(op);
    if(type_id(tname) && my_lexer.get_token(cp) == ')')
    {
      // success !
      exp = new PT::SizeofExpr(new PT::Atom(tk),
			       PT::list(new PT::Atom(op), tname, new PT::Atom(cp)));
      return true;
    }
    else
      my_lexer.restore(tag);
  }
  PT::Node *unary;
  if(!unary_expr(unary)) return false;

  exp = new PT::SizeofExpr(new PT::Atom(tk), PT::list(unary));
  return true;
}

//. typeid-expression:
//.   typeid ( type-id )
//.   typeid ( expression )
bool Parser::typeid_expr(PT::Node *&exp)
{
  Trace trace("Parser::typeid_expr", Trace::PARSING);

  Token tk;
  if(my_lexer.get_token(tk) != Token::TYPEID) return false;
  Token op;
  if(my_lexer.get_token(op) != '(') return false;
  char const *mark = my_lexer.save();
  PT::Node *arg;
  if(!type_id(arg))
  {
    my_lexer.restore(mark);
    if (!expression(arg)) return false;
  }
  Token cp;
  if(my_lexer.get_token(cp) != ')') return false;

  exp = new PT::TypeidExpr(new PT::Atom(tk),
			   PT::list(new PT::Atom(op), arg, new PT::Atom(cp)));
  return true;
}

bool Parser::is_allocate_expr(Token::Type t)
{
  if(t == Token::UserKeyword) return true;
  else
  {
    if(t == Token::Scope) t = my_lexer.look_ahead(1);
    if(t == Token::NEW || t == Token::DELETE) return true;
    else return false;
  }
}

/*
  allocate.expr
  : {Scope | userdef.keyword} NEW allocate.type
  | {Scope} DELETE {'[' ']'} cast.expr
*/
bool Parser::allocate_expr(PT::Node *&exp)
{
  Trace trace("Parser::allocate_expr", Trace::PARSING);
  Token tk;
  PT::Node *head = 0;

  bool ukey = false;
  int t = my_lexer.look_ahead(0);
  if(t == Token::Scope)
  {
    my_lexer.get_token(tk);
    head = new PT::Atom(tk);
  }
  else if(t == Token::UserKeyword)
  {
    if(!userdef_keyword(head)) return false;
    ukey = true;
  }

  t = my_lexer.get_token(tk);
  if(t == Token::DELETE)
  {
    PT::Node *obj;
    if(ukey) return false;

    if(head == 0) exp = new PT::DeleteExpr(new PT::Kwd::Delete(tk), 0);
    else exp = new PT::DeleteExpr(head, PT::list(new PT::Kwd::Delete(tk)));

    if(my_lexer.look_ahead(0) == '[')
    {
      my_lexer.get_token(tk);
      exp = PT::snoc(exp, new PT::Atom(tk));
      if(my_lexer.get_token(tk) != ']') return false;

      exp = PT::snoc(exp, new PT::Atom(tk));
    }
    if(!cast_expr(obj)) return false;

    exp = PT::snoc(exp, obj);
    return true;
  }
  else if(t == Token::NEW)
  {
    PT::Node *atype;
    if(head == 0) exp = new PT::NewExpr(new PT::Kwd::New(tk), 0);
    else exp = new PT::NewExpr(head, PT::list(new PT::Kwd::New(tk)));

    if(!allocate_type(atype)) return false;

    exp = PT::nconc(exp, atype);
    return true;
  }
  else return false;
}

/*
  userdef.keyword
  : [UserKeyword | UserKeyword5] {'(' function.arguments ')'}
*/
bool Parser::userdef_keyword(PT::Node *&ukey)
{
  Token tk;

  int t = my_lexer.get_token(tk);
  if(t != Token::UserKeyword && t != Token::UserKeyword5) return false;

  if(my_lexer.look_ahead(0) != '(')
    ukey = new PT::UserdefKeyword(new PT::Atom(tk), 0);
  else
  {
    PT::Node *args;
    Token op, cp;
    my_lexer.get_token(op);
    if(!function_arguments(args)) return false;

    if(my_lexer.get_token(cp) != ')') return false;

    ukey = new PT::UserdefKeyword(new PT::Atom(tk),
				  PT::list(new PT::Atom(op), args, new PT::Atom(cp)));
  }
  return true;
}

/*
  allocate.type
  : {'(' function.arguments ')'} type.specifier new.declarator
    {allocate.initializer}
  | {'(' function.arguments ')'} '(' type.name ')' {allocate.initializer}
*/
bool Parser::allocate_type(PT::Node *&atype)
{
  Trace trace("Parser::allocate_type", Trace::PARSING);
  Token op, cp;
  PT::Node *tname, *init, *exp;

  if(my_lexer.look_ahead(0) != '(') atype = PT::list(0);
  else
  {
    my_lexer.get_token(op);

    const char* pos = my_lexer.save();
    if(type_id(tname))
      if(my_lexer.get_token(cp) == ')')
	if(my_lexer.look_ahead(0) != '(')
	{
	  atype = PT::list(0, PT::list(new PT::Atom(op), tname, new PT::Atom(cp)));
	  if(!is_type_specifier()) return true;
	}
	else if(allocate_initializer(init))
	{
	  atype = PT::list(0, PT::list(new PT::Atom(op), tname, new PT::Atom(cp)),
			   init);
	  // the next token cannot be '('
	  if(my_lexer.look_ahead(0) != '(') return true;
	}

    // if we reach here, we have to process '(' function.arguments ')'.
    my_lexer.restore(pos);
    if(!function_arguments(exp)) return false;

    if(my_lexer.get_token(cp) != ')') return false;

    atype = PT::list(PT::list(new PT::Atom(op), exp, new PT::Atom(cp)));
  }
  if(my_lexer.look_ahead(0) == '(')
  {
    my_lexer.get_token(op);
    if(!type_id(tname)) return false;
    if(my_lexer.get_token(cp) != ')') return false;

    atype = PT::snoc(atype, PT::list(new PT::Atom(op), tname, new PT::Atom(cp)));
  }
  else
  {
    PT::Declarator *decl;
    PT::Encoding type_encode;
    if(!type_specifier(tname, false, type_encode)) return false;
    if(!new_declarator(decl, type_encode)) return false;

    atype = PT::snoc(atype, PT::list(tname, decl));
  }

  if(my_lexer.look_ahead(0) == '(')
  {
    if(!allocate_initializer(init)) return false;

    atype = PT::snoc(atype, init);
  }
  return true;
}

/*
  new.declarator
  : empty
  | ptr.operator
  | {ptr.operator} ('[' expression ']')+
*/
bool Parser::new_declarator(PT::Declarator *&decl, PT::Encoding& encode)
{
  Trace trace("Parser::new_declarator", Trace::PARSING);
  PT::Node *d = 0;
  if(my_lexer.look_ahead(0) != '[')
    if(!opt_ptr_operator(d, encode)) return false;
  decl = new PT::Declarator(d);
  while(my_lexer.look_ahead(0) == '[')
  {
    Token ob, cb;
    PT::Node *expr;
    my_lexer.get_token(ob);
    if(!expression(expr)) return false;
    if(my_lexer.get_token(cb) != ']') return false;

    if (expr)
    {
      long size;
      if (TypeAnalysis::evaluate_const(my_symbols.current_scope(), expr, size))
	encode.array(size);
      else 
	encode.array();
    }
    decl = PT::nconc(decl, PT::list(new PT::Atom(ob), expr, new PT::Atom(cb)));
  }
  decl->set_encoded_type(encode);
  return true;
}

/*
  allocate.initializer
  : '(' {initialize.expr (',' initialize.expr)* } ')'
*/
bool Parser::allocate_initializer(PT::Node *&init)
{
  Trace trace("Parser::allocate_initializer", Trace::PARSING);
  Token op, cp;

  if(my_lexer.get_token(op) != '(') return false;

  if(my_lexer.look_ahead(0) == ')')
  {
    my_lexer.get_token(cp);
    init = PT::list(new PT::Atom(op), 0, new PT::Atom(cp));
    return true;
  }

  init = 0;
  while(true)
  {
    PT::Node *exp;
    if(!initialize_expr(exp)) return false;

    init = PT::snoc(init, exp);
    if(my_lexer.look_ahead(0) != ',') break;
    else
    {
      Token tk;
      my_lexer.get_token(tk);
      init = PT::snoc(init, new PT::Atom(tk));
    }
  }
  my_lexer.get_token(cp);
  init = PT::list(new PT::Atom(op), init, new PT::Atom(cp));
  return true;
}

/*
  postfix.exp
  : primary.exp
  | postfix.expr '[' expression ']'
  | postfix.expr '(' function.arguments ')'
  | postfix.expr '.' var.name
  | postfix.expr ArrowOp var.name
  | postfix.expr IncOp
  | openc++.postfix.expr

  openc++.postfix.expr
  : postfix.expr '.' userdef.statement
  | postfix.expr ArrowOp userdef.statement

  Note: function-style casts are accepted as function calls.
*/
bool Parser::postfix_expr(PT::Node *&exp)
{
  Trace trace("Parser::postfix_expr", Trace::PARSING);
  PT::Node *e;
  Token cp, op;
  int t, t2;

  if(!primary_expr(exp)) return false;

  while(true)
  {
    switch(my_lexer.look_ahead(0))
    {
      case '[' :
	my_lexer.get_token(op);
	if(!expression(e)) return false;
	if(my_lexer.get_token(cp) != ']') return false;

	exp = new PT::ArrayExpr(exp, PT::list(new PT::Atom(op), e, new PT::Atom(cp)));
	break;
      case '(' :
	my_lexer.get_token(op);
	if(!function_arguments(e)) return false;
	if(my_lexer.get_token(cp) != ')') return false;

	exp = new PT::FuncallExpr(exp, 
				  PT::list(new PT::Atom(op), e, new PT::Atom(cp)));
	break;
      case Token::IncOp :
	my_lexer.get_token(op);
	exp = new PT::PostfixExpr(exp, PT::list(new PT::Atom(op)));
	break;
      case '.' :
      case Token::ArrowOp :
	t2 = my_lexer.get_token(op);
	t = my_lexer.look_ahead(0);
	if(t == Token::UserKeyword ||
	   t == Token::UserKeyword2 ||
	   t == Token::UserKeyword3)
	{
	  if(!userdef_statement(e)) return false;

	  exp = new PT::UserStatementExpr(exp, PT::cons(new PT::Atom(op), e));
	  break;
	}
	else
	{
	  if(!var_name(e)) return false;
	  if(t2 == '.')
	    exp = new PT::DotMemberExpr(exp, PT::list(new PT::Atom(op), e));
	  else
	    exp = new PT::ArrowMemberExpr(exp, PT::list(new PT::Atom(op), e));
	  break;
	}
    default : return true;
    }
  }
}

/*
  primary.exp
  : Constant
  | CharConst
  | WideCharConst
  | StringL
  | WideStringL
  | THIS
  | var.name
  | '(' expression ')'
  | integral.or.class.spec '(' function.arguments ')'
  | openc++.primary.exp
  | typeid '(' typething ')'

  openc++.primary.exp
  : var.name '::' userdef.statement
*/
bool Parser::primary_expr(PT::Node *&exp)
{
  Trace trace("Parser::primary_expr", Trace::PARSING);
  Token tk, tk2;
  PT::Node *exp2;
  PT::Encoding cast_type_encode;

  switch(my_lexer.look_ahead(0))
  {
    case Token::Constant:
    case Token::CharConst:
    case Token::WideCharConst:
    case Token::StringL:
    case Token::WideStringL:
      my_lexer.get_token(tk);
      exp = new PT::Literal(tk);
      return true;
    case Token::THIS:
      my_lexer.get_token(tk);
      exp = new PT::Kwd::This(tk);
      return true;
    case Token::TYPEID:
      return typeid_expr(exp);
    case '(':
    {
      PGuard<bool> guard(*this, &Parser::my_gt_is_operator);
      my_gt_is_operator = true;
      my_lexer.get_token(tk);
      if(!expression(exp2)) return false;
      if(my_lexer.get_token(tk2) != ')') return false;

      exp = new PT::ParenExpr(new PT::Atom(tk), PT::list(exp2, new PT::Atom(tk2)));
      return true;
    }
    default:
      // FIXME: We need symbol lookup here to figure out whether we
      //        are looking at a type or a variable here !
      if(!opt_integral_type_or_class_spec(exp, cast_type_encode)) return false;

      if(exp)
      { // if integral.or.class.spec
	if(my_lexer.get_token(tk) != '(') return false;
	if(!function_arguments(exp2)) return false;
	if(my_lexer.get_token(tk2) != ')') return false;

	exp = new PT::FstyleCastExpr(cast_type_encode, exp,
				     PT::list(new PT::Atom(tk), exp2,
					      new PT::Atom(tk2)));
	return true;
      }
      else
      {
	if(!var_name(exp)) return false;
	if(my_lexer.look_ahead(0) == Token::Scope)
	{
	  my_lexer.get_token(tk);
	  if(!userdef_statement(exp2)) return false;

	  exp = new PT::StaticUserStatementExpr(exp,
						PT::cons(new PT::Atom(tk), exp2));
	}
	return true;
      }
  }
}

bool Parser::typeof_expr(PT::Node *&node)
{
  Trace trace("Parser::typeof_expr", Trace::PARSING);
  Token tk, tk2;

  Token::Type t = my_lexer.get_token(tk);
  if (t != Token::TYPEOF)
    return false;
  if ((t = my_lexer.get_token(tk2)) != '(')
    return false;
  PT::Node *type = PT::list(new PT::Atom(tk2));
#if 1
  if (!assign_expr(node)) return false;
#else
  PT::Encoding name_encode;
  if (!name(node, name_encode)) return false; 	 
  if (!node->is_atom())
    node = new PT::Name(node, name_encode);
  else
    node = new PT::Name(PT::list(node), name_encode);
#endif
  type = PT::snoc(type, node);
  if ((t = my_lexer.get_token(tk2)) != ')') return false;
  type = PT::snoc(type, new PT::Atom(tk2));
  node = new PT::TypeofExpr(new PT::Atom(tk), type);
  return true;
}

/*
  userdef.statement
  : UserKeyword '(' function.arguments ')' compound.statement
  | UserKeyword2 '(' arg.decl.list ')' compound.statement
  | UserKeyword3 '(' expr.statement {expression} ';'
			{expression} ')' compound.statement
*/
bool Parser::userdef_statement(PT::Node *&st)
{
  Token tk, tk2, tk3, tk4;
  PT::Node *keyword, *exp, *exp2, *exp3;
  PT::Encoding dummy_encode;

  int t = my_lexer.get_token(tk);
  if(my_lexer.get_token(tk2) != '(') return false;

  switch(t)
  {
    case Token::UserKeyword :
      keyword = new PT::UserKeyword(tk);
      if(!function_arguments(exp)) return false;
      goto rest;
    case Token::UserKeyword2 :
      keyword = new PT::UserKeyword(tk);
      if(!parameter_declaration_list(exp, dummy_encode)) return false;
    rest:
    {
      if(my_lexer.get_token(tk3) != ')') return false;
      PT::Block *body;
      if(!compound_statement(body)) return false;
      st = PT::list(keyword, new PT::Atom(tk2), exp, new PT::Atom(tk3), body);
      return true;
    }
    case Token::UserKeyword3 :
    {
      if(!expr_statement(exp)) return false;

      if(my_lexer.look_ahead(0) == ';') exp2 = 0;
      else if(!expression(exp2)) return false;
      if(my_lexer.get_token(tk3) != ';') return false;
      if(my_lexer.look_ahead(0) == ')') exp3 = 0;
      else if(!expression(exp3)) return false;
      if(my_lexer.get_token(tk4) != ')') return false;
      PT::Block *body;
      if(!compound_statement(body)) return false;

      st = PT::list(new PT::Atom(tk), new PT::Atom(tk2), exp, exp2,
		    new PT::Atom(tk3), exp3, new PT::Atom(tk4), body);
      return true;
    }
    default :
      return false;
  }
}

/*
  var.name : {'::'} name2 ('::' name2)*

  name2
  : Identifier {template.args}
  | '~' Identifier
  | OPERATOR operator.name

  if var.name ends with a template type, the next token must be '('
*/
bool Parser::var_name(PT::Node *&name)
{
  Trace trace("Parser::var_name", Trace::PARSING);
  PT::Encoding encode;

  if(var_name_core(name, encode))
  {
    if(!name->is_atom()) name = new PT::Name(name, encode);
    return true;
  }
  else return false;
}

bool Parser::var_name_core(PT::Node *&name, PT::Encoding &encode)
{
  Trace trace("Parser::var_name_core", Trace::PARSING);
  Token tk;
  int length = 0;
  
  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    name = PT::list(new PT::Atom(tk));
    encode.global_scope();
    ++length;
  }
  else name = 0;

  while(true)
  {
    Token::Type t = my_lexer.get_token(tk);
    if(t == Token::TEMPLATE) 
    {
      // Skip template token, next will be identifier
      t = my_lexer.get_token(tk);
    }
    if(t == Token::Identifier)
    {
      PT::Node *n = new PT::Identifier(tk.ptr, tk.length);
      if(is_template_args())
      {
	PT::Node *args;
	PT::Encoding args_encode;
	if(!template_args(args, args_encode)) return false;

	encode.template_(n, args_encode);
	++length;
	n = PT::list(n, args);
      }
      else
      {
	encode.simple_name(n);
	++length;
      }
      if(more_var_name())
      {
	my_lexer.get_token(tk);
	name = PT::nconc(name, PT::list(n, new PT::Atom(tk)));
      }
      else
      {
	if(name == 0) name = n;
	else name = PT::snoc(name, n);
	if(length > 1) encode.qualified(length);
	return true;
      }
    }
    else if(t == '~')
    {
      Token tk2;
      if(my_lexer.look_ahead(0) != Token::Identifier) return false;
      my_lexer.get_token(tk2);
      PT::Node *class_name = new PT::Atom(tk2);
      PT::Node *dt = PT::list(new PT::Atom(tk), class_name);
      if(name == 0) name = dt;
      else name = PT::snoc(name, dt);
      encode.destructor(class_name);
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else if(t == Token::OPERATOR)
    {
      PT::Node *op;
      if(!operator_name(op, encode)) return false;

      PT::Node *opf = PT::list(new PT::Kwd::Operator(tk), op);
      name = name ? PT::snoc(name, opf) : opf;
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else return false;
  }
}

bool Parser::more_var_name()
{
  Trace trace("Parser::more_var_name", Trace::PARSING);
  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    int t = my_lexer.look_ahead(1);
    if(t == Token::Identifier || t == '~' || t == Token::OPERATOR ||
       t == Token::TEMPLATE)
      return true;
  }
  return false;
}

/*
  template.args : '<' any* '>'

  template.args must be followed by '(', '::', ';', or ','
*/
bool Parser::is_template_args()
{
  int i = 0;
  int t = my_lexer.look_ahead(i++);
  if(t == '<')
  {
    int n = 1;
    while(n > 0)
    {
      int u = my_lexer.look_ahead(i++);
      if(u == '<') ++n;
      else if(u == '>') --n;
      else if(u == '(')
      {
	int m = 1;
	while(m > 0)
	{
	  int v = my_lexer.look_ahead(i++);
	  if(v == '(') ++m;
	  else if(v == ')') --m;
	  else if(v == '\0' || v == ';' || v == '}') return false;
	}
      }
      else if(u == '\0' || u == ';' || u == '}') return false;
    }
    t = my_lexer.look_ahead(i);
    return bool(t == Token::Scope || t == '(' || t == ';' || t == ',');
  }
  return false;
}

//. condition:
//.   expression
//.   type-specifier-seq declarator = assignment-expression
bool Parser::condition(PT::Node *&exp)
{
  Trace trace("Parser::condition", Trace::PARSING);
  PT::Encoding type_encode;

  // Do declarator first, otherwise "T*foo = blah" gets matched as a
  // multiplication infix expression inside an assignment expression!
  const char *save = my_lexer.save();
  do 
  {
    PT::Node *storage_s, *cv_q, *cv_q2, *integral, *head, *decl;

    if (!opt_storage_spec(storage_s)) break;

    head = storage_s;
	
    if (!opt_cv_qualifier(cv_q) ||
	!opt_integral_type_or_class_spec(integral, type_encode))
      break;

    if (integral)
    {
      // Integral Declaration
      // Find const after type
      if (!opt_cv_qualifier(cv_q2)) break;
      // Make type ptree with pre and post const ptrees
      if (cv_q)
	if (cv_q2 == 0)
	  integral = PT::snoc(cv_q, integral);
	else
	  integral = PT::nconc(cv_q, PT::cons(integral, cv_q2));
      else if (cv_q2) integral = PT::cons(integral, cv_q2);
      // Store type of CV's
      type_encode.cv_qualify(cv_q, cv_q2);
      // Parse declarator
      if (!init_declarator(decl, type_encode, true, false)) break;
      // *must* be end of condition, condition is in a () pair
      if (my_lexer.look_ahead(0) != ')') break;
      exp = new PT::Declaration(head, PT::list(integral, decl));
    }
    else
    {
      // Other Declaration
      PT::Node *type_name;
      // Find name of type
      if (!name(type_name, type_encode)) break;
      // Find const after type
      if (!opt_cv_qualifier(cv_q2)) break;
      // Make type ptree with pre and post const ptrees
      if (cv_q)
	if (cv_q2 == 0) type_name = PT::snoc(cv_q, type_name);
	else type_name = PT::nconc(cv_q, PT::cons(type_name, cv_q2));
      else if (cv_q2) type_name = PT::cons(type_name, cv_q2);
      // Store type of CV's
      type_encode.cv_qualify(cv_q, cv_q2);
      // Parse declarator
      if (!init_declarator(decl, type_encode, true, false)) break;
      // *must* be end of condition, condition is in a () pair
      if (my_lexer.look_ahead(0) != ')') break;
      exp = new PT::Declaration(head, PT::list(type_name, decl));
    }
    return true;
  } while(false);

  // Must be a comma expression
  my_lexer.restore(save);
  return expression(exp);
}

//. function-body:
//.   compound-statement
bool Parser::function_body(PT::Block *&body)
{
  Trace trace("Parser::function_body", Trace::PARSING);
  return compound_statement(body);
}

//. compound-statement:
//.   { statement [opt] }
bool Parser::compound_statement(PT::Block *&body, bool create_scope)
{
  Trace trace("Parser::compound_statement", Trace::PARSING);

  Token ob;
  if(my_lexer.get_token(ob) != '{') return false;

  PT::Node *ob_comments = wrap_comments(my_lexer.get_comments());
  body = new PT::Block(new PT::CommentedAtom(ob, ob_comments), 0);

  ScopeGuard guard(*this, create_scope ? body : 0);

  PT::Node *sts = 0;
  while(my_lexer.look_ahead(0) != '}')
  {
    PT::Node *st;
    if(!statement(st))
    {
      if(!mark_error()) return false; // too many errors
      skip_to('}');
      Token cb;
      my_lexer.get_token(cb);
      body = new PT::Block(new PT::Atom(ob), 0, new PT::Atom(cb));
      return true;	// error recovery
    }

    sts = PT::snoc(sts, st);
  }
  Token cb;
  if(my_lexer.get_token(cb) != '}') return false;

  PT::Node *cb_comments = wrap_comments(my_lexer.get_comments());
  body = PT::nconc(body, PT::list(sts, new PT::CommentedAtom(cb, cb_comments)));
  return true;
}

/*
  statement
  : compound.statement
  | typedef
  | if.statement
  | switch.statement
  | while.statement
  | do.statement
  | for.statement
  | try.statement
  | BREAK ';'
  | CONTINUE ';'
  | RETURN { expression } ';'
  | GOTO Identifier ';'
  | CASE expression ':' statement
  | DEFAULT ':' statement
  | Identifier ':' statement
  | expr.statement
*/
bool Parser::statement(PT::Node *&st)
{
  Trace trace("Parser::statement", Trace::PARSING);
  Token tk1, tk2, tk3;
  PT::Node *st2, *exp, *comments;
  int k;

  // Get the comments - if we dont make it past the switch then it is a
  // parse error anyway!
  comments = wrap_comments(my_lexer.get_comments());

  // Whichever case we get, it must succeed
  switch(k = my_lexer.look_ahead(0))
  {
    case '{':
    {
      PT::Block *block;
      if (!compound_statement(block, true)) return false;
      st = block;
      break;
    }
    case Token::USING:
    {
      if (my_lexer.look_ahead(1) == Token::NAMESPACE)
      {
	PT::UsingDirective *udir;
	if (!using_directive(udir)) return false;
	declare(udir);
	st = udir;
      }
      else
      {
	PT::UsingDeclaration *udecl;
	if (!using_declaration(udecl)) return false;
	declare(udecl);
	st = udecl;
      }
      break;
    }
    case Token::TYPEDEF:
    {
      PT::Typedef *td;
      if (!typedef_(td)) return false;
      st = td;
      break;
    }
    case Token::IF:
      if (!if_statement(st)) return false;
      break;
    case Token::SWITCH:
      if (!switch_statement(st)) return false;
      break;
    case Token::WHILE:
      if (!while_statement(st)) return false;
      break;
    case Token::DO:
      if (!do_statement(st)) return false;
      break;
    case Token::FOR:
      if (!for_statement(st)) return false;
      break;
    case Token::TRY:
      if (!try_block(st)) return false;
      break;
    case Token::BREAK:
    case Token::CONTINUE:
      my_lexer.get_token(tk1);
      if(my_lexer.get_token(tk2) != ';') return false;
      if(k == Token::BREAK)
	st = new PT::BreakStatement(new PT::Kwd::Break(tk1),
				    PT::list(new PT::Atom(tk2)));
      else
	st = new PT::ContinueStatement(new PT::Kwd::Continue(tk1),
				       PT::list(new PT::Atom(tk2)));
      break;
    case Token::RETURN:
      my_lexer.get_token(tk1);
      if(my_lexer.look_ahead(0) == ';')
      {
	my_lexer.get_token(tk2);
	st = new PT::ReturnStatement(new PT::Kwd::Return(tk1),
				     PT::list(new PT::Atom(tk2)));
      } 
      else
      {
	if(!expression(exp)) return false;
	if(my_lexer.get_token(tk2) != ';') return false;

	st = new PT::ReturnStatement(new PT::Kwd::Return(tk1),
				     PT::list(exp, new PT::Atom(tk2)));
      }
      break;
    case Token::GOTO:
      my_lexer.get_token(tk1);
      if(my_lexer.get_token(tk2) != Token::Identifier) return false;
      if(my_lexer.get_token(tk3) != ';') return false;

      st = new PT::GotoStatement(new PT::Kwd::Goto(tk1),
				 PT::list(new PT::Atom(tk2), new PT::Atom(tk3)));
      break;
    case Token::CASE:
      my_lexer.get_token(tk1);
      if(!assign_expr(exp)) return false;
      if(my_lexer.get_token(tk2) != ':') return false;
      if(!statement(st2)) return false;

      st = new PT::CaseStatement(new PT::Kwd::Case(tk1),
				 PT::list(exp, new PT::Atom(tk2), st2));
      break;
    case Token::DEFAULT:
      my_lexer.get_token(tk1);
      if(my_lexer.get_token(tk2) != ':') return false;
      if(!statement(st2)) return false;

      st = new PT::DefaultStatement(new PT::Kwd::Default(tk1),
				    PT::list(new PT::Atom(tk2), st2));
      break;
    case Token::Identifier:
      if(my_lexer.look_ahead(1) == ':')
      { // label statement
	my_lexer.get_token(tk1);
	my_lexer.get_token(tk2);
	if(!statement(st2)) return false;

	st = new PT::LabelStatement(new PT::Atom(tk1),
				    PT::list(new PT::Atom(tk2), st2));
	return true;
      }
      // don't break here!
    default:
      if (!expr_statement(st)) return false;
  }

  // No parse error, attach comment to whatever was returned
  set_leaf_comments(st, comments);
  return true;
}

//. if-statement:
//.   if ( condition ) statement
//.   if ( condition ) statement else statement
bool Parser::if_statement(PT::Node *&st)
{
  Trace trace("Parser::if_statement", Trace::PARSING);
  Token tk1, tk2, tk3, tk4;
  PT::Node *exp, *then, *otherwise;

  if(my_lexer.get_token(tk1) != Token::IF) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!condition(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(!statement(then)) return false;

  st = new PT::IfStatement(new PT::Kwd::If(tk1),
			   PT::list(new PT::Atom(tk2), exp, new PT::Atom(tk3), then));
  if(my_lexer.look_ahead(0) == Token::ELSE)
  {
    my_lexer.get_token(tk4);
    if(!statement(otherwise)) return false;

    st = PT::nconc(st, PT::list(new PT::Kwd::Else(tk4), otherwise));
  }
  return true;
}

//. switch-statement:
//.   switch ( condition ) statement
bool Parser::switch_statement(PT::Node *&st)
{
  Trace trace("Parser::switch_statement", Trace::PARSING);
  Token tk1, tk2, tk3;
  PT::Node *exp, *body;

  if(my_lexer.get_token(tk1) != Token::SWITCH) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!condition(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(!statement(body)) return false;

  st = new PT::SwitchStatement(new PT::Kwd::Switch(tk1),
			       PT::list(new PT::Atom(tk2), exp,
					new PT::Atom(tk3), body));
  return true;
}

//. while-statement:
//.   while ( condition ) statement
bool Parser::while_statement(PT::Node *&st)
{
  Trace trace("Parser::while_statement", Trace::PARSING);
  Token tk1, tk2, tk3;
  PT::Node *exp, *body;

  if(my_lexer.get_token(tk1) != Token::WHILE) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!condition(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(!statement(body)) return false;

  st = new PT::WhileStatement(new PT::Kwd::While(tk1),
			      PT::list(new PT::Atom(tk2), exp,
				       new PT::Atom(tk3), body));
  return true;
}

//. do.statement:
//.   do statement while ( condition ) ;
bool Parser::do_statement(PT::Node *&st)
{
  Trace trace("Parser::do_statement", Trace::PARSING);
  Token tk0, tk1, tk2, tk3, tk4;
  PT::Node *exp, *body;

  if(my_lexer.get_token(tk0) != Token::DO) return false;
  if(!statement(body)) return false;
  if(my_lexer.get_token(tk1) != Token::WHILE) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!condition(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(my_lexer.get_token(tk4) != ';') return false;

  st = new PT::DoStatement(new PT::Kwd::Do(tk0),
			   PT::list(body, new PT::Kwd::While(tk1),
				    new PT::Atom(tk2), exp,
				    new PT::Atom(tk3), new PT::Atom(tk4)));
  return true;
}

/*
  for.statement
  : FOR '(' expr.statement {expression} ';' {expression} ')'
    statement
*/
bool Parser::for_statement(PT::Node *&st)
{
  Trace trace("Parser::for_statement", Trace::PARSING);
  Token tk1, tk2, tk3, tk4;
  PT::Node *exp1, *exp2, *exp3, *body;

  if(my_lexer.get_token(tk1) != Token::FOR) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!expr_statement(exp1)) return false;
  if(my_lexer.look_ahead(0) == ';') exp2 = 0;
  else if(!expression(exp2)) return false;
  if(my_lexer.get_token(tk3) != ';') return false;
  if(my_lexer.look_ahead(0) == ')') exp3 = 0;
  else if(!expression(exp3)) return false;
  if(my_lexer.get_token(tk4) != ')') return false;
  if(!statement(body)) return false;

  st = new PT::ForStatement(new PT::Kwd::For(tk1),
			    PT::list(new PT::Atom(tk2), exp1, exp2,
				     new PT::Atom(tk3), exp3,
				     new PT::Atom(tk4), body));
  return true;
}

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
bool Parser::try_block(PT::Node *&st)
{
  Trace trace("Parser::try_block", Trace::PARSING);
  Token tk, op, cp;

  if(my_lexer.get_token(tk) != Token::TRY) return false;
  PT::Block *body;
  if(!compound_statement(body)) return false;

  st = new PT::TryStatement(new PT::Kwd::Try(tk), PT::list(body));

  do
  {
    if(my_lexer.get_token(tk) != Token::CATCH) return false;
    if(my_lexer.get_token(op) != '(') return false;
    // TODO: handler should become a ParameterDeclaration
    PT::Node *handler;
    if(my_lexer.look_ahead(0) == Token::Ellipsis)
    {
      my_lexer.get_token(cp);
      handler = new PT::Atom(cp);
    }
    else
    {
      PT::Encoding encode;
      PT::ParameterDeclaration *parameter;
      if(!parameter_declaration(parameter, encode)) return false;
      handler = parameter;
    }
    if(my_lexer.get_token(cp) != ')') return false;
    PT::Block *body;
    if(!compound_statement(body)) return false;

    st = PT::snoc(st, PT::list(new PT::Kwd::Catch(tk),
			       new PT::Atom(op), handler, new PT::Atom(cp),
			       body));
  } while(my_lexer.look_ahead(0) == Token::CATCH);
  return true;
}

/*
  expr.statement
  : ';'
  | declaration.statement
  | expression ';'
  | openc++.postfix.expr
  | openc++.primary.exp
*/
bool Parser::expr_statement(PT::Node *&st)
{
  Trace trace("Parser::expr_statement", Trace::PARSING);
  Token tk;

  if(my_lexer.look_ahead(0) == ';')
  {
    my_lexer.get_token(tk);
    st = new PT::ExprStatement(0, PT::list(new PT::Atom(tk)));
    return true;
  }
  else
  {
    const char *pos = my_lexer.save();
    PT::Declaration *decl;
    if(declaration_statement(decl))
    {
      declare(decl);
      st = decl;
      return true;
    }
    else
    {
      PT::Node *exp;
      my_lexer.restore(pos);
      if(!expression(exp)) return false;
      if(PT::is_a(exp, Token::ntUserStatementExpr, Token::ntStaticUserStatementExpr))
      {
	st = exp;
	return true;
      }
      if(my_lexer.get_token(tk) != ';') return false;

      st = new PT::ExprStatement(exp, PT::list(new PT::Atom(tk)));
      return true;
    }
  }
}

/*
  declaration.statement
  : decl.head integral.or.class.spec {cv.qualify} {init-declarator-list} ';'
  | decl.head name {cv.qualify} init-declarator-list ';'
  | const.declaration

  decl.head
  : {storage.spec} {cv.qualify}

  const.declaration
  : cv.qualify {'*'} Identifier '=' expression {',' init-declarator-list} ';'

  Note: if you modify this function, take a look at rDeclaration(), too.
*/
bool Parser::declaration_statement(PT::Declaration *&statement)
{
  Trace trace("Parser::declaration_statement", Trace::PARSING);
  PT::Node *storage_s, *cv_q, *integral;
  PT::Encoding type_encode;

  Token::Type type = my_lexer.look_ahead(0);
  if (type == Token::NAMESPACE)
  {
    PT::NamespaceAlias *alias;
    bool result = namespace_alias(alias);
    statement = alias;
    return result;
  }
  else if (type == Token::USING)
  {
    type = my_lexer.look_ahead(1);
    if (type == Token::NAMESPACE)
    {
      PT::UsingDirective *udir;
      bool result = using_directive(udir);
      statement = udir;
      return result;
    }
    else
    {
      PT::UsingDeclaration *udecl;
      bool result = using_declaration(udecl);
      statement = udecl;
      return result;
    }
  }
  if(!opt_storage_spec(storage_s) ||
     !opt_cv_qualifier(cv_q) ||
     !opt_integral_type_or_class_spec(integral, type_encode))
    return false;

  PT::Node *head = 0;
  if(storage_s) head = PT::snoc(head, storage_s);

  if(integral)
    return integral_decl_statement(statement, type_encode, integral, cv_q, head);
  else
  {
    type_encode.clear();
    int t = my_lexer.look_ahead(0);
    if(cv_q != 0 && ((t == Token::Identifier && my_lexer.look_ahead(1) == '=') ||
		     t == '*'))
      return const_declaration(statement, type_encode, head, cv_q);
    else return other_decl_statement(statement, type_encode, cv_q, head);
  }
}

/*
  integral.decl.statement
  : decl.head integral.or.class.spec {cv.qualify} {init-declarator-list} ';'
*/
bool Parser::integral_decl_statement(PT::Declaration *&statement,
				     PT::Encoding& type_encode,
				     PT::Node *integral,
				     PT::Node *cv_q, PT::Node *head)
{
  Trace trace("Parser::integral_decl_statement", Trace::PARSING);
  PT::Node *cv_q2, *decl;
  Token tk;

  if(!opt_cv_qualifier(cv_q2)) return false;

  if(cv_q)
    if(cv_q2 == 0) integral = PT::snoc(cv_q, integral);
    else integral = PT::nconc(cv_q, PT::cons(integral, cv_q2));
  else if(cv_q2) integral = PT::cons(integral, cv_q2);

  type_encode.cv_qualify(cv_q, cv_q2);
  if(my_lexer.look_ahead(0) == ';')
  {
    my_lexer.get_token(tk);
    statement = new PT::Declaration(head, PT::list(integral, new PT::Atom(tk)));
    return true;
  }
  else
  {
    if(!init_declarator_list(decl, type_encode, false, true)) return false;
    if(my_lexer.get_token(tk) != ';') return false;
	    
    statement = new PT::Declaration(head,
				    PT::list(integral, decl, new PT::Atom(tk)));
    return true;
  }
}

/*
   other.decl.statement
   :decl.head name {cv.qualify} init_declarator_list ';'
*/
bool Parser::other_decl_statement(PT::Declaration *&statement,
				  PT::Encoding& type_encode,
				  PT::Node *cv_q,
				  PT::Node *head)
{
  Trace trace("Parser::other_decl_statement", Trace::PARSING);
  PT::Node *type_name, *cv_q2, *decl;
  Token tk;

  if(!name(type_name, type_encode)) return false;
  if(!opt_cv_qualifier(cv_q2)) return false;

  if(cv_q)
    if(cv_q2 == 0) type_name = PT::snoc(cv_q, type_name);
    else type_name = PT::nconc(cv_q, PT::cons(type_name, cv_q2));
  else if(cv_q2) type_name = PT::cons(type_name, cv_q2);

  type_encode.cv_qualify(cv_q, cv_q2);
  if(!init_declarator_list(decl, type_encode, false, true)) return false;
  if(my_lexer.get_token(tk) != ';') return false;

  statement = new PT::Declaration(head, PT::list(type_name, decl, new PT::Atom(tk)));
  return true;
}

bool Parser::maybe_typename_or_class_template(Token&)
{
  return true;
}

void Parser::skip_to(Token::Type token)
{
  Token tk;

  while(true)
  {
    Token::Type t = my_lexer.look_ahead(0);
    if(t == token || t == '\0') break;
    else my_lexer.get_token(tk);
  }
}

}
