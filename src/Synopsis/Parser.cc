//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include "Synopsis/Parser.hh"
#include "Synopsis/Lexer.hh"
#include <Synopsis/Trace.hh>
#include <iostream>

using namespace Synopsis;

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
  UndefinedSymbol(PTree::Encoding const &name) : my_name(name) {}
  virtual void write(std::ostream &os) const
  {
    os << "Undefined symbol : " << my_name << std::endl;
  }
private:
  PTree::Encoding my_name;
};

class SymbolAlreadyDefined : public Parser::Error
{
public:
  SymbolAlreadyDefined(PTree::Encoding const &name, 
		       std::string const & f1, unsigned long l1,
		       std::string const & f2, unsigned long l2)
    : my_name(name), my_file1(f1), my_line1(l1), my_file2(f2), my_line2(l2) {}
  virtual void write(std::ostream &os) const
  {
    os << "Symbol already defined : definition of " << my_name
       << " at " << my_file1 << ':' << my_line1 << '\n'
       << "previously defined at " << my_file2 << ':' << my_line2 << std::endl;
  }
private:
  PTree::Encoding my_name;
  std::string     my_file1;
  unsigned long   my_line1;
  std::string     my_file2;
  unsigned long   my_line2;
};

class SymbolTypeMismatch : public Parser::Error
{
public:
  SymbolTypeMismatch(PTree::Encoding const &name, PTree::Encoding const &type)
    : my_name(name), my_type(type) {}
  virtual void write(std::ostream &os) const
  {
    os << "Symbol type mismatch : " << my_name 
       << " has unexpected type " << my_type << std::endl;
  }
private:
  PTree::Encoding my_name;
  PTree::Encoding my_type;
};

namespace
{

const int max_errors = 10;

PTree::Node *wrap_comments(const Lexer::Comments &c)
{
  PTree::Node *head = 0;
  for (Lexer::Comments::const_iterator i = c.begin(); i != c.end(); ++i)
    head = PTree::snoc(head, new PTree::Atom(*i));
  return head;
}

PTree::Node *nth_declarator(PTree::Node *decl, size_t n)
{
  decl = PTree::third(decl);
  if(!decl || decl->is_atom()) return 0;

  if(PTree::is_a(decl, Token::ntDeclarator))
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

void set_declarator_comments(PTree::Declaration *decl, PTree::Node *comments)
{
  if (!decl) return;

  PTree::Node *declarator;
  size_t n = 0;
  while (true)
  {
    size_t i = n++;
    declarator = nth_declarator(decl, i);
    if (!declarator) break;
    else if (PTree::is_a(declarator, Token::ntDeclarator))
      ((PTree::Declarator*)declarator)->set_comments(comments);
  }
}

//. Helper function to recursively find the first left-most leaf node
PTree::Node *leftmost_leaf(PTree::Node *node, PTree::Node *& parent)
{
  if (!node || node->is_atom()) return node;
  // Non-leaf node. So find first leafy child
  PTree::Node *leaf;
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
void set_leaf_comments(PTree::Node *node, PTree::Node *comments)
{
  PTree::Node *parent, *leaf;
  PTree::CommentedAtom* cleaf;

  // Find leaf
  leaf = leftmost_leaf(node, parent);

  // Sanity
  if (!leaf)
  {
    std::cerr << "Warning: Failed to find leaf when trying to add comments." << std::endl;
    PTree::display(parent, std::cerr, false);
    return; 
  }

  if (!(cleaf = dynamic_cast<PTree::CommentedAtom *>(leaf)))
  {
    // Must change first child of parent to be a commented leaf
    Token tk(leaf->position(), leaf->length(), Token::Comment);
    cleaf = new (PTree::GC) PTree::CommentedAtom(tk, comments);
    parent->set_car(cleaf);
  }
  else
  {
    // Already is a commented leaf, so add the comments to it
    comments = PTree::snoc(cleaf->get_comments(), comments);
    cleaf->set_comments(comments);
  }
}

}

Parser::Parser(Lexer &lexer, SymbolLookup::Table &symbols, int ruleset)
  : my_lexer(lexer),
    my_ruleset(ruleset),
    my_symbols(symbols),
    my_comments(0),
    my_in_template_decl(false)
{
}

Parser::~Parser()
{
}

bool Parser::mark_error()
{
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
  try
  {
    my_symbols.declare(t);
  }
  catch (SymbolLookup::Undefined const &e)
  {
    my_errors.push_back(new UndefinedSymbol(e.name));
  }
  catch (SymbolLookup::MultiplyDefined const &e)
  {
    std::string file1;
    unsigned long line1 = my_lexer.origin(e.declaration->begin(), file1);
    std::string file2;
    unsigned long line2 = my_lexer.origin(e.original->begin(), file2);
    my_errors.push_back(new SymbolAlreadyDefined(e.name,
						 file1, line1,
						 file2, line2));
  }
  catch (SymbolLookup::TypeError const &e)
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

PTree::Node *Parser::parse()
{
  Trace trace("Parser::parse");
  PTree::Node *statements = 0;
  while(my_lexer.look_ahead(0) != '\0')
  {
    PTree::Node *def;
    if(definition(def))
    {
      statements = PTree::nconc(statements, PTree::list(def));
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
  if (statements)
    (void) PTree::nconc(PTree::last(statements)->car(), wrap_comments(my_lexer.get_comments()));
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
bool Parser::definition(PTree::Node *&p)
{
  Trace trace("Parser::definition");
  bool res;
  int t = my_lexer.look_ahead(0);
  if(t == ';')
    res = null_declaration(p);
  else if(t == Token::TYPEDEF)
  {
    PTree::Typedef *td;
    res = typedef_(td);
    p = td;
  }
  else if(t == Token::TEMPLATE)
    res = template_decl(p);
  else if(t == Token::METACLASS)
    res = metaclass_decl(p);
  else if(t == Token::EXTERN && my_lexer.look_ahead(1) == Token::StringL)
    res = linkage_spec(p);
  else if(t == Token::EXTERN && my_lexer.look_ahead(1) == Token::TEMPLATE)
    res = extern_template_decl(p);
  else if(t == Token::NAMESPACE && my_lexer.look_ahead(2) == '=')
    res = namespace_alias(p);
  else if(t == Token::NAMESPACE)
  {
    PTree::NamespaceSpec *spec;
    res = namespace_spec(spec);
    p = spec;
  }
  else if(t == Token::USING)
  {
    PTree::Using *udecl;
    res = using_(udecl);
    declare(udecl);
    p = udecl;
  }
  else 
  {
    PTree::Declaration *decl;
    if (!declaration(decl)) return false;
    PTree::Node *c = wrap_comments(my_lexer.get_comments());
    if (c) set_declarator_comments(decl, c);
    p = decl;
    declare(decl);
    return true;
  }
  my_lexer.get_comments();
  return res;
}

bool Parser::null_declaration(PTree::Node *&decl)
{
  Trace trace("Parser::null_declaration");
  Token tk;

  if(my_lexer.get_token(tk) != ';') return false;
  decl = new PTree::Declaration(0, PTree::list(0, new PTree::Atom(tk)));
  return true;
}

/*
  typedef
  : TYPEDEF type.specifier declarators ';'
*/
bool Parser::typedef_(PTree::Typedef *&def)
{
  Trace trace("Parser::typedef_");
  Token tk;
  PTree::Node *type_name, *decl;
  PTree::Encoding type_encode;

  if(my_lexer.get_token(tk) != Token::TYPEDEF) return false;

  def = new PTree::Typedef(new PTree::Reserved(tk));
  if(!type_specifier(type_name, false, type_encode)) return false;

  def = PTree::snoc(def, type_name);
  if(!declarators(decl, type_encode, true)) return false;
  if(my_lexer.get_token(tk) != ';') return false;

  def = PTree::nconc(def, PTree::list(decl, new PTree::Atom(tk)));
  declare(def);
  return true;
}

/*
  type.specifier
  : {cv.qualify} (integral.or.class.spec | name) {cv.qualify}
*/
bool Parser::type_specifier(PTree::Node *&tspec, bool check, PTree::Encoding &encode)
{
  Trace trace("Parser::type_specifier");
  PTree::Node *cv_q, *cv_q2;

  if(!opt_cv_qualify(cv_q) || !opt_integral_type_or_class_spec(tspec, encode))
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

  if(!opt_cv_qualify(cv_q2))
    return false;

  if(cv_q)
  {
    tspec = PTree::snoc(cv_q, tspec);
    if(cv_q2)
      tspec = PTree::nconc(tspec, cv_q2);
  }
  else if(cv_q2)
    tspec = PTree::cons(tspec, cv_q2);

  encode.cv_qualify(cv_q, cv_q2);
  return true;
}

// is_type_specifier() returns true if the next is probably a type specifier.
bool Parser::is_type_specifier()
{
  int t = my_lexer.look_ahead(0);
  if(t == Token::Identifier || t == Token::Scope
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
bool Parser::metaclass_decl(PTree::Node *&decl)
{
  int t;
  Token tk1, tk2, tk3, tk4;
  PTree::Node *metaclass_name;

  if(my_lexer.get_token(tk1) != Token::METACLASS)
    return false;

  if(my_lexer.get_token(tk2) != Token::Identifier)
    return false;

  t = my_lexer.get_token(tk3);
  if(t == Token::Identifier)
  {
    metaclass_name = new PTree::Atom(tk2);
    decl = new PTree::MetaclassDecl(new PTree::Reserved(tk1),
				    PTree::list(metaclass_name,
						new PTree::Atom(tk3)));
  }
  else if(t == ':')
  {
    if(my_lexer.get_token(tk4) != Token::Identifier)
      return false;

    metaclass_name = new PTree::Atom(tk4);
    decl = new PTree::MetaclassDecl(new PTree::Reserved(tk1),
				    PTree::list(metaclass_name,
						new PTree::Atom(tk2)));
  }
  else if(t == ';')
  {
    metaclass_name = new PTree::Atom(tk2);
    decl = new PTree::MetaclassDecl(new PTree::Reserved(tk1),
				    PTree::list(metaclass_name, 0,
						new PTree::Atom(tk3)));
    return true;
  }
  else
    return false;

  t = my_lexer.get_token(tk1);
  if(t == '(')
  {
    PTree::Node *args;
    if(!meta_arguments(args))
      return false;

    if(my_lexer.get_token(tk2) != ')')
      return false;

    decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1), args,
					  new PTree::Atom(tk2)));
    t = my_lexer.get_token(tk1);
  }

  if(t == ';')
  {
    decl = PTree::snoc(decl, new PTree::Atom(tk1));
    return true;
  }
  else
    return false;
}

/*
  meta.arguments : (anything but ')')*
*/
bool Parser::meta_arguments(PTree::Node *&args)
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
    args = PTree::snoc(args, new PTree::Atom(tk));
  }
}

/*
  linkage.spec
  : EXTERN StringL definition
  |  EXTERN StringL linkage.body
*/
bool Parser::linkage_spec(PTree::Node *&spec)
{
  Trace trace("Parser::linkage_spec");
  Token tk1, tk2;
  PTree::Node *body;

  if(my_lexer.get_token(tk1) != Token::EXTERN) return false;
  if(my_lexer.get_token(tk2) != Token::StringL) return false;

  spec = new PTree::LinkageSpec(new PTree::AtomEXTERN(tk1),
				PTree::list(new PTree::Atom(tk2)));
  if(my_lexer.look_ahead(0) == '{')
  {
    if(!linkage_body(body)) return false;
  }
  else
    if(!definition(body)) return false;

  spec = PTree::snoc(spec, body);
  return true;
}

/*
  namespace.spec
  : NAMESPACE Identifier definition
  | NAMESPACE { Identifier } linkage.body
*/
bool Parser::namespace_spec(PTree::NamespaceSpec *&spec)
{
  Trace trace("Parser::namespace_spec");
  Token tk1, tk2;

  if(my_lexer.get_token(tk1) != Token::NAMESPACE) return false;

  PTree::Node *comments = wrap_comments(my_lexer.get_comments());

  PTree::Node *name;
  if(my_lexer.look_ahead(0) == '{') name = 0;
  else
    if(my_lexer.get_token(tk2) == Token::Identifier)
      name = new PTree::Atom(tk2);
    else return false;

  spec = new PTree::NamespaceSpec(new PTree::AtomNAMESPACE(tk1),
				  PTree::list(name, 0));
  spec->set_comments(comments);

  PTree::Node *body;
  if(my_lexer.look_ahead(0) == '{')
  {
    declare(spec);
    SymbolLookup::Table::Guard guard(&my_symbols.enter_namespace(spec));
    if(!linkage_body(body)) return false;
  }
  else if(!definition(body)) return false;

  PTree::tail(spec, 2)->set_car(body);

  return true;
}

/*
  namespace.alias : NAMESPACE Identifier '=' Identifier ';'
*/
bool Parser::namespace_alias(PTree::Node *&exp)
{
  Trace trace("Parser::namespace_alias");
  Token tk;

  if(my_lexer.get_token(tk) != Token::NAMESPACE) return false;
  PTree::Node *ns = new PTree::AtomNAMESPACE(tk);

  if (my_lexer.get_token(tk) != Token::Identifier) return false;
  PTree::Node *alias = new PTree::Atom(tk);

  if (my_lexer.get_token(tk) != '=') return false;
  PTree::Node *eq = new PTree::Atom(tk);

  PTree::Node *name;
  PTree::Encoding encode;
  int length = 0;
  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    name = PTree::list(new PTree::Atom(tk));
    encode.global_scope();
    ++length;
  }
  else name = 0;

  while (true)
  {
    if (my_lexer.get_token(tk) != Token::Identifier) return false;
    PTree::Node *n = new PTree::Atom(tk);
    encode.simple_name(n);
    ++length;
    
    if(my_lexer.look_ahead(0) == Token::Scope)
    {
      my_lexer.get_token(tk);
      name = PTree::nconc(name, PTree::list(n, new PTree::Atom(tk)));
    }
    else
    {
      if(name == 0) name = n;
      else name = PTree::snoc(name, n);

      if(length > 1) encode.qualified(length);

      break;
    }
  }

  if (my_lexer.get_token(tk) != ';') return false;

  exp = new PTree::NamespaceAlias(ns, PTree::list(alias, eq, 
						  name, new PTree::Atom(tk)));
  return true;
}

/*
  using.declaration : USING ... ';'
*/
/*
  using.declaration
  : USING NAMESPACE name
  | USING name
*/
bool Parser::using_(PTree::Using *&decl)
{
  Trace trace("Parser::user_");
  Token tk;

  if(my_lexer.get_token(tk) != Token::USING)
    return false;

  decl = new PTree::Using(new PTree::AtomUSING(tk));

  if (my_lexer.look_ahead(0) == Token::NAMESPACE) // using.dir
  {
    my_lexer.get_token(tk);
    decl = PTree::snoc(decl, new PTree::AtomNAMESPACE(tk));
  }
  PTree::Node *id;
  PTree::Encoding name_encode;
  if (!name(id, name_encode)) return false;
  if (!id->is_atom())
    id = new PTree::Name(id, name_encode);
  else
    id = new PTree::Name(PTree::list(id), name_encode);
  decl = PTree::snoc(decl, id);
  if (!my_lexer.look_ahead(0) == ';') return false;
  else return true;
}


/*
  linkage.body : '{' (definition)* '}'

  Note: this is also used to construct namespace.spec
*/
bool Parser::linkage_body(PTree::Node *&body)
{
  Trace trace("Parser::linkage_body");
  Token op, cp;
  PTree::Node *def;

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
      body = PTree::list(new PTree::Atom(op), 0, new PTree::Atom(cp));
      return true;		// error recovery
    }

    body = PTree::snoc(body, def);
  }

  my_lexer.get_token(cp);
  body = new PTree::Brace(new PTree::Atom(op), body,
			  new PTree::CommentedAtom(cp, wrap_comments(my_lexer.get_comments())));
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
bool Parser::template_decl(PTree::Node *&decl)
{
  Trace trace("Parser::template_decl");
  PTree::Declaration *body;
  PTree::TemplateDecl *tdecl;
  TemplateDeclKind kind = tdk_unknown;

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
      if (PTree::length(decl) != 3) return false;
      if (PTree::first(decl) != 0) return false;
      if (PTree::type_of(PTree::second(decl)) != Token::ntClassSpec) return false;
      if (*PTree::third(decl) != ';') return false;
      decl = new PTree::TemplateInstantiation(PTree::second(decl));
      break;
    case tdk_decl:
    {
      tdecl = PTree::snoc(tdecl, body);
      declare(tdecl);
      decl = tdecl;
      break;
    }
    case tdk_specialization:
    {
      tdecl = PTree::snoc(tdecl, body);
      decl = tdecl;
      break;
    }
    default:
      throw std::runtime_error("Parser::template_decl(): fatal");
  }
  return true;
}

bool Parser::template_decl2(PTree::TemplateDecl *&decl, TemplateDeclKind &kind)
{
  Trace trace("Parser::template_decl2");
  Token tk;
  PTree::Node *args;

  if(my_lexer.get_token(tk) != Token::TEMPLATE) return false;
  if(my_lexer.look_ahead(0) != '<') 
  {
    // template instantiation
    decl = 0;
    kind = tdk_instantiation;
    return true;	// ignore TEMPLATE
  }

  decl = new PTree::TemplateDecl(new PTree::Reserved(tk));
  if(my_lexer.get_token(tk) != '<') return false;

  decl = PTree::snoc(decl, new PTree::Atom(tk));
  if(!template_arg_list(args)) return false;
  if(my_lexer.get_token(tk) != '>') return false;

  decl = PTree::nconc(decl, PTree::list(args, new PTree::Atom(tk)));

  // ignore nested TEMPLATE
  while (my_lexer.look_ahead(0) == Token::TEMPLATE) 
  {
    my_lexer.get_token(tk);
    if(my_lexer.look_ahead(0) != '<') break;

    my_lexer.get_token(tk);
    if(!template_arg_list(args)) return false;
    if(my_lexer.get_token(tk) != '>') return false;
  }

  if (args == 0) kind = tdk_specialization; // template < > declaration
  else kind = tdk_decl;                     // template < ... > declaration
  return true;
}

/*
  temp.arg.list
  : empty
  | temp.arg.declaration (',' temp.arg.declaration)*
*/
bool Parser::template_arg_list(PTree::Node *&args)
{
  Trace trace("Parser::template_arg_list");
  Token tk;
  PTree::Node *a;

  if(my_lexer.look_ahead(0) == '>')
  {
    args = 0;
    return true;
  }

  if(!template_arg_declaration(a))
    return false;

  args = PTree::list(a);
  while(my_lexer.look_ahead(0) == ',')
  {
    my_lexer.get_token(tk);
    args = PTree::snoc(args, new PTree::Atom(tk));
    if(!template_arg_declaration(a))
      return false;

    args = PTree::snoc(args, a);
  }
  return true;
}

/*
  temp.arg.declaration
  : CLASS Identifier {'=' type.name}
  | type.specifier arg.declarator {'=' additive.expr}
  | template.decl2 CLASS Identifier {'=' type.name}
*/
bool Parser::template_arg_declaration(PTree::Node *&decl)
{
  Trace trace("Parser::template_arg_declaration");
  Token tk1, tk2;

  int t0 = my_lexer.look_ahead(0);
  int t1 = my_lexer.look_ahead(1);
  int t2 = my_lexer.look_ahead(2);
  if(t0 == Token::CLASS && t1 == Token::Identifier &&
     (t2 == '=' || t2 == '>' || t2 == ',')) 
  {
    my_lexer.get_token(tk1);
    my_lexer.get_token(tk2);
    PTree::Node *name = new PTree::Atom(tk2);
    decl = PTree::list(new PTree::Atom(tk1), name);

    if(t2 == '=')
    {
      PTree::Node *default_type;

      my_lexer.get_token(tk1);
      if(!typename_(default_type))
	return false;

      decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1), default_type));
      return true;
    }
    else if (t2 == '>' || t2 == ',')
      return true;
  }
  else if (t0 == Token::CLASS && (t1 == '=' || t1 == '>' || t1 == ',')) 
  {
    // class without the identifier
    my_lexer.get_token(tk1);
    decl = PTree::list(new PTree::Atom(tk1));

    if(my_lexer.look_ahead(0) == '=')
    {
      PTree::Node *default_type;

      my_lexer.get_token(tk1);
      if(!typename_(default_type))
	return false;

      decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1), default_type));
    }
  }
  else if (t0 == Token::TEMPLATE) 
  {
    TemplateDeclKind kind;
    PTree::TemplateDecl *tdecl;
    if(!template_decl2(tdecl, kind)) return false;

    if (my_lexer.get_token(tk1) != Token::CLASS || 
	my_lexer.get_token(tk2) != Token::Identifier)
      return false;

    PTree::ClassSpec *cspec = new PTree::ClassSpec(new PTree::Reserved(tk1),
						   PTree::cons(new PTree::Atom(tk2), 0),
						   0);
    tdecl = PTree::snoc(tdecl, cspec);
    declare(tdecl);
    decl = tdecl;
    if(my_lexer.look_ahead(0) == '=')
    {
      PTree::Node *default_type;
      my_lexer.get_token(tk1);
      if(!typename_(default_type))
	return false;

      decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1), default_type));
    }
  }
  else
  {
    PTree::Node *type_name, *arg;
    PTree::Encoding type_encode, name_encode;
    if(!type_specifier(type_name, true, type_encode))
      return false;

    if(!declarator(arg, kArgDeclarator, false, type_encode, name_encode, true))
      return false;

    decl = PTree::list(type_name, arg);
    if(my_lexer.look_ahead(0) == '=')
    {
      PTree::Node *exp;
      my_lexer.get_token(tk1);
      if(!additive_expr(exp))
	return false;

      decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1), exp));
    }
  }
  return true;
}

/*
   extern.template.decl
   : EXTERN TEMPLATE declaration
*/
bool Parser::extern_template_decl(PTree::Node *&decl)
{
  Trace trace("Parser::extern_template_decl");
  Token tk1, tk2;
  PTree::Declaration *body;

  if(my_lexer.get_token(tk1) != Token::EXTERN) return false;
  if(my_lexer.get_token(tk2) != Token::TEMPLATE) return false;
  if(!declaration(body)) return false;

  decl = new PTree::ExternTemplate(new PTree::Atom(tk1),
				   PTree::list(new PTree::Atom(tk2), body));
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
  : integral.decl.head declarators (';' | function.body)
  | integral.decl.head ';'
  | integral.decl.head ':' expression ';'

  integral.decl.head
  : decl.head integral.or.class.spec {cv.qualify}

  other.declaration
  : decl.head name {cv.qualify} declarators (';' | function.body)
  | decl.head name constructor.decl (';' | function.body)
  | FRIEND name ';'

  const.declaration
  : cv.qualify {'*'} Identifier '=' expression {',' declarators} ';'

  Note: if you modify this function, look at declaration.statement, too.
  Note: this regards a statement like "T (a);" as a constructor
        declaration.  See is_constructor_decl().
*/
bool Parser::declaration(PTree::Declaration *&statement)
{
  Trace trace("Parser::declaration");
  PTree::Node *mem_s, *storage_s, *cv_q, *integral, *head;
  PTree::Encoding type_encode;
  int res;

  my_lexer.look_ahead(0);
  my_comments = wrap_comments(my_lexer.get_comments());

  if(!opt_member_spec(mem_s) || !opt_storage_spec(storage_s))
    return false;

  if(mem_s == 0)
    head = 0;
  else
    head = mem_s;	// mem_s is a list.

  if(storage_s != 0)
    head = PTree::snoc(head, storage_s);

  if(mem_s == 0)
    if(opt_member_spec(mem_s))
      head = PTree::nconc(head, mem_s);
    else
      return false;

  if(!opt_cv_qualify(cv_q)
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

/* single declaration, for use in a condition (controlling
   expression of switch/while/if) */
bool Parser::simple_declaration(PTree::Declaration *&statement)
{
  Trace trace("Parser::simple_declaration");
  PTree::Node *cv_q, *integral;
  PTree::Encoding type_encode, name_encode;

  /* no member specification permitted here, and no
     storage specifier:
     type-specifier ::=
     simple-type-specifier
     class-specifier
     enum-specifier
     elaborated-type-specifier
     cv-qualifier */
  
  if(!opt_cv_qualify(cv_q) || !opt_integral_type_or_class_spec(integral, type_encode))
    return false;
  if (integral == 0 && !name(integral, type_encode))
    return false;

  if (cv_q && integral)
    integral = PTree::snoc(cv_q, integral);
  else if (cv_q && integral == 0)
    integral = cv_q, cv_q = 0;

  /* no type-specifier so far -> can't be a declaration */
  if (integral == 0)
    return false;
    
  PTree::Node *d;
  if(!declarator(d, kDeclarator, false, type_encode, name_encode, true, true))
    return false;

  if (my_lexer.look_ahead(0) != '=')
    return false;

  Token eqs;
  my_lexer.get_token(eqs);
  int t = my_lexer.look_ahead(0);
  PTree::Node *e;
  if(!expression(e))
    return false;

  PTree::nconc(d, PTree::list(new PTree::Atom(eqs), e));

  statement = new PTree::Declaration(0, PTree::list(integral, PTree::list(d)));
  return true;
}

bool Parser::integral_declaration(PTree::Declaration *&statement,
				  PTree::Encoding &type_encode,
				  PTree::Node *head, PTree::Node *integral,
				  PTree::Node *cv_q)
{
  Trace trace("Parser::integral_declaration");
  Token tk;
  PTree::Node *cv_q2, *decl;

  if(!opt_cv_qualify(cv_q2))
    return false;

  if(cv_q)
    if(cv_q2 == 0)
      integral = PTree::snoc(cv_q, integral);
    else
      integral = PTree::nconc(cv_q, PTree::cons(integral, cv_q2));
  else if(cv_q2 != 0)
    integral = PTree::cons(integral, cv_q2);

  type_encode.cv_qualify(cv_q, cv_q2);
  switch(my_lexer.look_ahead(0))
  {
    case ';' :
      my_lexer.get_token(tk);
      statement = new PTree::Declaration(head, PTree::list(integral,
							   new PTree::Atom(tk)));
      return true;
    case ':' : // bit field
      my_lexer.get_token(tk);
      if(!expression(decl)) return false;

      decl = PTree::list(PTree::list(new PTree::Atom(tk), decl));
      if(my_lexer.get_token(tk) != ';') return false;

      statement = new PTree::Declaration(head, PTree::list(integral, decl,
							   new PTree::Atom(tk)));
      return true;
    default :
      if(!declarators(decl, type_encode, true)) return false;

      if(my_lexer.look_ahead(0) == ';')
      {
	my_lexer.get_token(tk);
	statement = new PTree::Declaration(head, PTree::list(integral, decl,
							     new PTree::Atom(tk)));
	return true;
      }
      else
      {
  	statement = new PTree::Declaration(head,
  					   PTree::list(integral, decl->car()));
 	SymbolLookup::Table::Guard guard(&my_symbols.enter_function(statement));
	PTree::Block *body;
	if(!function_body(body)) return false;
	if(PTree::length(decl) != 1) return false;

  	PTree::snoc(statement, body);
	return true;
      }
  }
}

bool Parser::const_declaration(PTree::Declaration *&statement, PTree::Encoding&,
			       PTree::Node *head, PTree::Node *cv_q)
{
  Trace trace("Parser::const_declaration");
  PTree::Node *decl;
  Token tk;
  PTree::Encoding type_encode;

  type_encode.simple_const();
  if(!declarators(decl, type_encode, false))
    return false;

  if(my_lexer.look_ahead(0) != ';')
    return false;

  my_lexer.get_token(tk);
  statement = new PTree::Declaration(head, PTree::list(cv_q, decl,
						       new PTree::Atom(tk)));
  return true;
}

bool Parser::other_declaration(PTree::Declaration *&statement, PTree::Encoding &type_encode,
			       PTree::Node *mem_s, PTree::Node *cv_q,
			       PTree::Node *head)
{
  Trace trace("Parser::other_declaration");
  PTree::Node *type_name, *decl, *cv_q2;
  Token tk;

  if(!name(type_name, type_encode))
    return false;

  if(cv_q == 0 && is_constructor_decl())
  {
    PTree::Encoding ftype_encode;
    if(!constructor_decl(decl, ftype_encode))
      return false;

    decl = PTree::list(new PTree::Declarator(type_name, decl,
					     ftype_encode, type_encode,
					     type_name));
    type_name = 0;
  }
  else if(mem_s != 0 && my_lexer.look_ahead(0) == ';')
  {
    // FRIEND name ';'
    if(PTree::length(mem_s) == 1 && PTree::type_of(mem_s->car()) == Token::FRIEND)
    {
      my_lexer.get_token(tk);
      statement = new PTree::Declaration(head, PTree::list(type_name,
							   new PTree::Atom(tk)));
      return true;
    }
    else
      return false;
  }
  else
  {
    if(!opt_cv_qualify(cv_q2))
      return false;

    if(cv_q)
      if(cv_q2 == 0)
	type_name = PTree::snoc(cv_q, type_name);
      else
	type_name = PTree::nconc(cv_q, PTree::cons(type_name, cv_q2));
    else if(cv_q2)
      type_name = PTree::cons(type_name, cv_q2);

    type_encode.cv_qualify(cv_q, cv_q2);
    if(!declarators(decl, type_encode, false))
      return false;
  }

  if(my_lexer.look_ahead(0) == ';')
  {
    my_lexer.get_token(tk);
    statement = new PTree::Declaration(head, PTree::list(type_name, decl,
							 new PTree::Atom(tk)));
  }
  else
  {
    PTree::Block *body;
    if(!function_body(body))
      return false;

    if(PTree::length(decl) != 1)
      return false;

    statement = new PTree::Declaration(head, PTree::list(type_name,
							 decl->car(), body));
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
  Trace trace("Parser::is_constructor_decl");
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
  : (FRIEND | INLINE | VIRTUAL | userdef.keyword)+
*/
bool Parser::opt_member_spec(PTree::Node *&p)
{
  Trace trace("Parser::opt_member_spec");
  Token tk;
  PTree::Node *lf;
  int t = my_lexer.look_ahead(0);

  p = 0;
  while(t == Token::FRIEND || t == Token::INLINE || t == Token::VIRTUAL ||
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
	lf = new PTree::AtomINLINE(tk);
      else if(t == Token::VIRTUAL)
	lf = new PTree::AtomVIRTUAL(tk);
      else
	lf = new PTree::AtomFRIEND(tk);
    }
    p = PTree::snoc(p, lf);
    t = my_lexer.look_ahead(0);
  }
  return true;
}

/*
  storage.spec : STATIC | EXTERN | AUTO | REGISTER | MUTABLE
*/
bool Parser::opt_storage_spec(PTree::Node *&p)
{
  Trace trace("Parser::opt_storage_spec");
  int t = my_lexer.look_ahead(0);
  if(t == Token::STATIC || t == Token::EXTERN || t == Token::AUTO ||
     t == Token::REGISTER || t == Token::MUTABLE)
  {
    Token tk;
    my_lexer.get_token(tk);
    switch(t)
    {
      case Token::STATIC :
	p = new PTree::AtomSTATIC(tk);
	break;
      case Token::EXTERN :
	p = new PTree::AtomEXTERN(tk);
	break;
      case Token::AUTO :
	p = new PTree::AtomAUTO(tk);
	break;
      case Token::REGISTER :
	p = new PTree::AtomREGISTER(tk);
	break;
      case Token::MUTABLE :
	p = new PTree::AtomMUTABLE(tk);
	break;
      default :
	throw std::runtime_error("optStorageSpec(): fatal");
    }
  }
  else
    p = 0;	// no storage specifier
  return true;
}

/*
  cv.qualify : (CONST | VOLATILE)+
*/
bool Parser::opt_cv_qualify(PTree::Node *&cv)
{
  Trace trace("Parser::opt_cv_qualify");
  PTree::Node *p = 0;
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
	  p = PTree::snoc(p, new PTree::AtomCONST(tk));
	  break;
        case Token::VOLATILE :
	  p = PTree::snoc(p, new PTree::AtomVOLATILE(tk));
	  break;
        default :
	  throw std::runtime_error("optCvQualify(): fatal");
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
bool Parser::opt_integral_type_or_class_spec(PTree::Node *&p, PTree::Encoding& encode)
{
  Trace trace("Parser::opt_integral_type_or_class_spec");
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
      PTree::Node *kw;
      my_lexer.get_token(tk);
      switch(t)
      {
        case Token::CHAR :
	  type = 'c';
	  kw = new PTree::AtomCHAR(tk);
	  break;
        case Token::WCHAR :
	  type = 'w';
	  kw = new PTree::AtomWCHAR(tk);
	  break;
        case Token::INT :
        case Token::INT64 : // an int64 is *NOT* an int but...
	  if(type != 's' && type != 'l' && type != 'j' && type != 'r')
	    type = 'i';
	  kw = new PTree::AtomINT(tk);
	  break;
        case Token::SHORT :
	  type = 's';
	  kw = new PTree::AtomSHORT(tk);
	  break;
        case Token::LONG :
	  if(type == 'l') type = 'j';      // long long
	  else if(type == 'd') type = 'r'; // double long
	  else type = 'l';
	  kw = new PTree::AtomLONG(tk);
	  break;
        case Token::SIGNED :
	  flag = 'S';
	  kw = new PTree::AtomSIGNED(tk);
	  break;
        case Token::UNSIGNED :
	  flag = 'U';
	  kw = new PTree::AtomUNSIGNED(tk);
	  break;
        case Token::FLOAT :
	  type = 'f';
	  kw = new PTree::AtomFLOAT(tk);
	  break;
        case Token::DOUBLE :
	  if(type == 'l') type = 'r'; // long double
	  else type = 'd';
	  kw = new PTree::AtomDOUBLE(tk);
	  break;
        case Token::VOID :
	  type = 'v';
	  kw = new PTree::AtomVOID(tk);
	  break;
        case Token::BOOLEAN :
	  type = 'b';
	  kw = new PTree::AtomBOOLEAN(tk);
	  break;
        default :
	  throw std::runtime_error("Parser::opt_integral_type_or_class_spec(): fatal");
      }
      p = PTree::snoc(p, kw);
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

  if(t == Token::CLASS || t == Token::STRUCT || t == Token::UNION ||
     t == Token::UserKeyword)
  {
    PTree::ClassSpec *spec;
    bool success = class_spec(spec, encode);
    p = spec;
    return success;
  }
  else if(t == Token::ENUM)
  {
    PTree::EnumSpec *spec;
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
bool Parser::constructor_decl(PTree::Node *&constructor, PTree::Encoding& encode)
{
  Trace trace("Parser::constructor_decl");
  Token op, cp;
  PTree::Node *args, *cv, *throw_decl, *mi;

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
    if(!arg_decl_list(args, encode))
      return false;

  my_lexer.get_token(cp);
  constructor = PTree::list(new PTree::Atom(op), args, new PTree::Atom(cp));
  opt_cv_qualify(cv);
  if(cv)
  {
    encode.cv_qualify(cv);
    constructor = PTree::nconc(constructor, cv);
  }

  opt_throw_decl(throw_decl);	// ignore in this version

  if(my_lexer.look_ahead(0) == ':')
    if(member_initializers(mi))
      constructor = PTree::snoc(constructor, mi);
    else
      return false;

  if(my_lexer.look_ahead(0) == '=')
  {
    Token eq, zero;
    my_lexer.get_token(eq);
    if(my_lexer.get_token(zero) != Token::Constant)
      return false;

    constructor = PTree::nconc(constructor,
			       PTree::list(new PTree::Atom(eq), new PTree::Atom(zero)));
  }

  encode.no_return_type();
  return true;
}

/*
  throw.decl : THROW '(' (name {','})* {name} ')'
*/
bool Parser::opt_throw_decl(PTree::Node *&throw_decl)
{
  Trace trace("Parser::opt_throw_decl");
  Token tk;
  int t;
  PTree::Node *p = 0;

  if(my_lexer.look_ahead(0) == Token::THROW){
    my_lexer.get_token(tk);
    p = PTree::snoc(p, new PTree::Reserved(tk));

    if(my_lexer.get_token(tk) != '(')
      return false;

    p = PTree::snoc(p, new PTree::Atom(tk));

    while(true)
    {
      PTree::Node *q;
      PTree::Encoding encode;
      t = my_lexer.look_ahead(0);
      if(t == '\0')
	return false;
      else if(t == ')')
	break;
      else if(my_ruleset & MSVC && t == Token::Ellipsis)
      {
	// for MSVC compatibility we accept 'throw(...)' declarations
	my_lexer.get_token(tk);
	p = PTree::snoc(p, new PTree::Atom(tk));
      }
      else if(name(q, encode))
	p = PTree::snoc(p, q);
      else
	return false;

      if(my_lexer.look_ahead(0) == ','){
	my_lexer.get_token(tk);
	p = PTree::snoc(p, new PTree::Atom(tk));
      }
      else
	break;
    }
    if(my_lexer.get_token(tk) != ')')
      return false;

    p = PTree::snoc(p, new PTree::Atom(tk));
  }
  throw_decl = p;
  return true;
}

/*
  declarators : declarator.with.init (',' declarator.with.init)*

  is_statement changes the behavior of rArgDeclListOrInit().
*/
bool Parser::declarators(PTree::Node *&decls, PTree::Encoding& type_encode,
			 bool should_be_declarator, bool is_statement)
{
  Trace trace("Parser::declarators");
  PTree::Node *d;
  Token tk;
  PTree::Encoding encode;

  decls = 0;
  while(true)
  {
    my_lexer.look_ahead(0); // force comment finding
    PTree::Node *comments = wrap_comments(my_lexer.get_comments());

    encode = type_encode;
    if(!declarator_with_init(d, encode, should_be_declarator, is_statement))
      return false;
	
    if (d && (PTree::type_of(d) == Token::ntDeclarator))
      static_cast<PTree::Declarator*>(d)->set_comments(comments);

    decls = PTree::snoc(decls, d);
    if(my_lexer.look_ahead(0) == ',')
    {
      my_lexer.get_token(tk);
      decls = PTree::snoc(decls, new PTree::Atom(tk));
    }
    else
      return true;
  };
}

/*
  declarator.with.init
  : ':' expression
  | declarator {'=' initialize.expr | ':' expression}
*/
bool Parser::declarator_with_init(PTree::Node *&dw, PTree::Encoding& type_encode,
				  bool should_be_declarator,
				  bool is_statement)
{
  Trace trace("Parser::declarator_with_init");
  PTree::Node *d, *e;
  Token tk;
  PTree::Encoding name_encode;

  if(my_lexer.look_ahead(0) == ':')
  {	// bit field
    my_lexer.get_token(tk);
    if(!expression(e))
      return false;

    dw = PTree::list(new PTree::Atom(tk), e);
    return true;
  }
  else
  {
    if(!declarator(d, kDeclarator, false, type_encode, name_encode,
		   should_be_declarator, is_statement))
      return false;

    int t = my_lexer.look_ahead(0);
    if(t == '=')
    {
      my_lexer.get_token(tk);
      if(!initialize_expr(e))
	return false;

      dw = PTree::nconc(d, PTree::list(new PTree::Atom(tk), e));
      return true;
    }
    else if(t == ':')
    {		// bit field
      my_lexer.get_token(tk);
      if(!expression(e))
	return false;

      dw = PTree::nconc(d, PTree::list(new PTree::Atom(tk), e));
      return true;
    }
    else
    {
      dw = d;
      return true;
    }
  }
}

/*
  declarator
  : (ptr.operator)* (name | '(' declarator ')')
	('[' comma.expression ']')* {func.args.or.init}

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
bool Parser::declarator(PTree::Node *&decl, DeclKind kind, bool recursive,
			PTree::Encoding& type_encode, PTree::Encoding& name_encode,
			bool should_be_declarator, bool is_statement)
{
  Trace trace("Parser::declarator");
  return declarator2(decl, kind, recursive, type_encode, name_encode,
		     should_be_declarator, is_statement, 0);
}

bool Parser::declarator2(PTree::Node *&decl, DeclKind kind, bool recursive,
			 PTree::Encoding& type_encode, PTree::Encoding& name_encode,
			 bool should_be_declarator, bool is_statement,
			 PTree::Node **declared_name)
{
  Trace trace("Parser::declarator2");
  PTree::Encoding recursive_encode;
  PTree::Node *d;
  int t;
  bool recursive_decl = false;
  PTree::Node *declared_name0 = 0;

  if(declared_name == 0)
    declared_name = &declared_name0;

  if(!opt_ptr_operator(d, type_encode))
    return false;

  const char* lex_save = my_lexer.save();
  t = my_lexer.look_ahead(0);
  if(t == '(')
  {
    Token op, cp;
    PTree::Node *decl2;
    my_lexer.get_token(op);
    recursive_decl = true;
    if(!declarator2(decl2, kind, true, recursive_encode, name_encode,
		    true, false, declared_name))
      return false;

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
	if(kind == kDeclarator && d == 0){
	  t = my_lexer.look_ahead(0);
	  if(t != '[' && t != '(')
	    return false;
	}
      d = PTree::snoc(d, PTree::list(new PTree::Atom(op), decl2, new PTree::Atom(cp)));
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
      PTree::Node *id;
      if(name(id, name_encode)) d = PTree::snoc(d, id);
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
      PTree::Encoding args_encode;
      Token op, cp;
      PTree::Node *args, *cv, *throw_decl, *mi;
      bool is_args = true;

      my_lexer.get_token(op);
      if(my_lexer.look_ahead(0) == ')')
      {
	args = 0;
	args_encode.start_func_args();
	args_encode.void_();
	args_encode.end_func_args();
      }
      else
	if(!arg_decl_list_or_init(args, is_args, args_encode, is_statement))
	  return false;
      if(my_lexer.get_token(cp) != ')')
	return false;

      if(is_args)
      {
	d = PTree::nconc(d, PTree::list(new PTree::Atom(op), args,
					new PTree::Atom(cp)));
	opt_cv_qualify(cv);
	if(cv)
	{
	  args_encode.cv_qualify(cv);
	  d = PTree::nconc(d, cv);
	}
      }
      else
	d = PTree::snoc(d, PTree::list(new PTree::Atom(op), args,
				       new PTree::Atom(cp)));

      if(!args_encode.empty())
	type_encode.function(args_encode);
      
      opt_throw_decl(throw_decl);	// ignore in this version

      if(my_lexer.look_ahead(0) == ':')
	if(member_initializers(mi)) d = PTree::snoc(d, mi);
	else return false;
      
      break;		// "T f(int)(char)" is invalid.
    }
    else if(t == '[')
    {	// array
      Token ob, cb;
      PTree::Node *expr;
      my_lexer.get_token(ob);
      if(my_lexer.look_ahead(0) == ']') expr = 0;
      else if(!comma_expression(expr)) return false;

      if(my_lexer.get_token(cb) != ']') return false;

      if (expr)
      {
	long size;
	if (my_symbols.evaluate_const(expr, size))
	  type_encode.array(size);
	else 
	  type_encode.array();
      }
      d = PTree::nconc(d, PTree::list(new PTree::Atom(ob), expr,
				      new PTree::Atom(cb)));
    }
    else break;
  }

  if(recursive_decl) type_encode.recursion(recursive_encode);
  if(recursive) decl = d;
  else
    if(d == 0) decl = new PTree::Declarator(type_encode, name_encode, *declared_name);
    else decl = new PTree::Declarator(d, type_encode, name_encode, *declared_name);

  return true;
}

/*
  ptr.operator
  : (('*' | '&' | ptr.to.member) {cv.qualify})+
*/
bool Parser::opt_ptr_operator(PTree::Node *&ptrs, PTree::Encoding& encode)
{
  Trace trace("Parser::opt_ptr_operator");
  ptrs = 0;
  while(true)
  {
    int t = my_lexer.look_ahead(0);
    if(t != '*' && t != '&' && !is_ptr_to_member(0)) break;
    else
    {
      PTree::Node *op, *cv;
      if(t == '*' || t == '&')
      {
	Token tk;
	my_lexer.get_token(tk);
	op = new PTree::Atom(tk);
	encode.ptr_operator(t);
      }
      else
	if(!ptr_to_member(op, encode)) return false;

      ptrs = PTree::snoc(ptrs, op);
      opt_cv_qualify(cv);
      if(cv)
      {
	ptrs = PTree::nconc(ptrs, cv);
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
bool Parser::member_initializers(PTree::Node *&init)
{
  Trace trace("Parser::member_initializer");
  Token tk;
  PTree::Node *m;

  if(my_lexer.get_token(tk) != ':')
    return false;

  init = PTree::list(new PTree::Atom(tk));
  if(!member_init(m))
    return false;

  init = PTree::snoc(init, m);
  while(my_lexer.look_ahead(0) == ',')
  {
    my_lexer.get_token(tk);
    init = PTree::snoc(init, new PTree::Atom(tk));
    if(!member_init(m))
      return false;

    init = PTree::snoc(init, m);
  }
  return true;
}

/*
  member.init
  : name '(' function.arguments ')'
*/
bool Parser::member_init(PTree::Node *&init)
{
  Trace trace("Parser::member_init");
  PTree::Node *name, *args;
  Token tk1, tk2;
  PTree::Encoding encode;

  if(!this->name(name, encode)) return false;
  if(!name->is_atom()) name = new PTree::Name(name, encode);
  if(my_lexer.get_token(tk1) != '(') return false;
  if(!function_arguments(args)) return false;
  if(my_lexer.get_token(tk2) != ')') return false;

  init = PTree::list(name, new PTree::Atom(tk1), args, new PTree::Atom(tk2));
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
bool Parser::name(PTree::Node *&id, PTree::Encoding& encode)
{
  Trace trace("Parser::name");
  Token tk, tk2;
  int t;
  int length = 0;

  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    id = PTree::list(new PTree::Atom(tk));
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
      PTree::Node *n = new PTree::Atom(tk);
      t = my_lexer.look_ahead(0);
      if(t == '<')
      {
	PTree::Node *args;
	PTree::Encoding args_encode;
	if(!template_args(args, args_encode))
	  return false;

	encode.template_(n, args_encode);
	++length;
	n = PTree::list(n, args);
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
	id = PTree::nconc(id, PTree::list(n, new PTree::Atom(tk)));
      }
      else
      {
	id = id ? PTree::snoc(id, n) : n;
	if(length > 1) encode.qualified(length);
	return true;
      }
    }
    else if(t == '~')
    {
      if(my_lexer.look_ahead(0) != Token::Identifier)
	return false;

      my_lexer.get_token(tk2);
      PTree::Node *class_name = new PTree::Atom(tk2);
      PTree::Node *dt = PTree::list(new PTree::Atom(tk), class_name);
      id = id ? PTree::snoc(id, dt) : dt;
      encode.destructor(class_name);
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else if(t == Token::OPERATOR)
    {
      PTree::Node *op;
      PTree::Node *opf;
      if(!operator_name(op, encode)) return false;
      
      t = my_lexer.look_ahead(0);
      if(t != '<') opf = PTree::list(new PTree::Reserved(tk), op);
      else
      {
	PTree::Node *args;
	PTree::Encoding args_encode;
	if(!template_args(args, args_encode)) return false;

	// here, I must merge args_encode into encode.
	// I'll do it in future. :p

	opf = PTree::list(new PTree::Reserved(tk), op, args);
      }
      id = id ? PTree::snoc(id, opf) : opf;
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
bool Parser::operator_name(PTree::Node *&name, PTree::Encoding &encode)
{
  Trace trace("Parser::operator_name");
    Token tk;

    int t = my_lexer.look_ahead(0);
    if(t == '+' || t == '-' || t == '*' || t == '/' || t == '%' || t == '^'
       || t == '&' || t == '|' || t == '~' || t == '!' || t == '=' || t == '<'
       || t == '>' || t == Token::AssignOp || t == Token::ShiftOp || t == Token::EqualOp
       || t == Token::RelOp || t == Token::LogAndOp || t == Token::LogOrOp || t == Token::IncOp
       || t == ',' || t == Token::PmOp || t == Token::ArrowOp){
	my_lexer.get_token(tk);
	name = new PTree::Atom(tk);
	encode.simple_name(name);
	return true;
    }
    else if(t == Token::NEW || t == Token::DELETE)
    {
      my_lexer.get_token(tk);
      if(my_lexer.look_ahead(0) != '[')
      {
	name = new PTree::Reserved(tk);
	encode.simple_name(name);
	return true;
      }
      else
      {
	name = PTree::list(new PTree::Reserved(tk));
	my_lexer.get_token(tk);
	name = PTree::snoc(name, new PTree::Atom(tk));
	if(my_lexer.get_token(tk) != ']') return false;

	name = PTree::snoc(name, new PTree::Atom(tk));
	if(t == Token::NEW) encode.append_with_length("new[]", 5);
	else encode.append_with_length("delete[]", 8);
	return true;
      }
    }
    else if(t == '(')
    {
      my_lexer.get_token(tk);
      name = PTree::list(new PTree::Atom(tk));
      if(my_lexer.get_token(tk) != ')') return false;

      encode.append_with_length("()", 2);
      name = PTree::snoc(name, new PTree::Atom(tk));
      return true;
    }
    else if(t == '[')
    {
      my_lexer.get_token(tk);
      name = PTree::list(new PTree::Atom(tk));
      if(my_lexer.get_token(tk) != ']') return false;

      encode.append_with_length("[]", 2);
      name = PTree::snoc(name, new PTree::Atom(tk));
      return true;
    }
    else return cast_operator_name(name, encode);
}

/*
  cast.operator.name
  : {cv.qualify} (integral.or.class.spec | name) {cv.qualify}
    {(ptr.operator)*}
*/
bool Parser::cast_operator_name(PTree::Node *&name, PTree::Encoding &encode)
{
  Trace trace("Parser::cast_operator_name");
  PTree::Node *cv1, *cv2, *type_name, *ptr;
  PTree::Encoding type_encode;

  if(!opt_cv_qualify(cv1)) return false;
  if(!opt_integral_type_or_class_spec(type_name, type_encode)) return false;
  if(type_name == 0)
  {
    type_encode.clear();
    if(!this->name(type_name, type_encode)) return false;
  }

  if(!opt_cv_qualify(cv2)) return false;
  if(cv1)
    if(cv2 == 0) type_name = PTree::snoc(cv1, type_name);
    else type_name = PTree::nconc(cv1, PTree::cons(type_name, cv2));
  else if(cv2) type_name = PTree::cons(type_name, cv2);
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
    name = PTree::list(type_name, ptr);
    return true;
  }
}

/*
  ptr.to.member
  : {'::'} (identifier {template.args} '::')+ '*'
*/
bool Parser::ptr_to_member(PTree::Node *&ptr_to_mem, PTree::Encoding &encode)
{
  Trace trace("Parser::ptr_to_member");
  Token tk;
  PTree::Node *p, *n;
  PTree::Encoding pm_encode;
  int length = 0;

  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    p = PTree::list(new PTree::Atom(tk));
    pm_encode.global_scope();
    ++length;
  }
  else p = 0;

  while(true)
  {
    if(my_lexer.get_token(tk) == Token::Identifier)
      n = new PTree::Atom(tk);
    else
      return false;

    int t = my_lexer.look_ahead(0);
    if(t == '<')
    {
      PTree::Node *args;
      PTree::Encoding args_encode;
      if(!template_args(args, args_encode)) return false;

      pm_encode.template_(n, args_encode);
      ++length;
      n = PTree::list(n, args);
      t = my_lexer.look_ahead(0);
    }
    else
    {
      pm_encode.simple_name(n);
      ++length;
    }

    if(my_lexer.get_token(tk) != Token::Scope) return false;

    p = PTree::nconc(p, PTree::list(n, new PTree::Atom(tk)));
    if(my_lexer.look_ahead(0) == '*')
    {
      my_lexer.get_token(tk);
      p = PTree::snoc(p, new PTree::Atom(tk));
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
bool Parser::template_args(PTree::Node *&temp_args, PTree::Encoding &encode)
{
  Trace trace("Parser::template_args");
  Token tk1, tk2;
  PTree::Encoding type_encode;

  if(my_lexer.get_token(tk1) != '<') return false;

  // in case of Foo<>
  if(my_lexer.look_ahead(0) == '>')
  {
    my_lexer.get_token(tk2);
    temp_args = PTree::list(new PTree::Atom(tk1), new PTree::Atom(tk2));
    return true;
  }

  PTree::Node *args = 0;
  while(true)
  {
    PTree::Node *a;
    const char* pos = my_lexer.save();
    type_encode.clear();

    // Prefer type name, but if not ',' or '>' then must be expression
    if(typename_(a, type_encode) && 
       (my_lexer.look_ahead(0) == ',' || my_lexer.look_ahead(0) == '>'))
      encode.append(type_encode);
    else
    {
      my_lexer.restore(pos);	
      if(!conditional_expr(a, true)) return false;
      encode.value_temp_param();
    }
    args = PTree::snoc(args, a);
    switch(my_lexer.get_token(tk2))
    {
      case '>' :
	temp_args = PTree::list(new PTree::Atom(tk1), args, new PTree::Atom(tk2));
	return true;
      case ',' :
	args = PTree::snoc(args, new PTree::Atom(tk2));
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
    : arg.decl.list
    | function.arguments

  This rule accepts function.arguments to parse declarations like:
	Point p(1, 3);
  "(1, 3)" is arg.decl.list.or.init.

  If maybe_init is true, we first examine whether tokens construct
  function.arguments.  This ordering is significant if tokens are
	Point p(s, t);
  s and t can be type names or variable names.
*/
bool Parser::arg_decl_list_or_init(PTree::Node *&arglist, bool &is_args,
				   PTree::Encoding &encode, bool maybe_init)
{
  Trace trace("Parser::arg_decl_list_or_init");
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
    return(is_args = arg_decl_list(arglist, encode));
  }
  else 
    if(is_args = arg_decl_list(arglist, encode)) return true;
    else
    {
      my_lexer.restore(pos);
      encode.clear();
      return function_arguments(arglist);
    }
}

/*
  arg.decl.list
    : empty
    | arg.declaration ( ',' arg.declaration )* {{ ',' } Ellipses}
*/
bool Parser::arg_decl_list(PTree::Node *&arglist, PTree::Encoding& encode)
{
  Trace trace("Parser::arg_decl_list");
  PTree::Node *list;
  PTree::Node *d;
  int t;
  Token tk;
  PTree::Encoding arg_encode;

  encode.start_func_args();
  list = 0;
  while(true)
  {
    arg_encode.clear();
    t = my_lexer.look_ahead(0);
    if(t == ')')
    {
      if(list == 0) encode.void_();
      arglist = list;
      break;
    }
    else if(t == Token::Ellipsis)
    {
      my_lexer.get_token(tk);
      encode.ellipsis_arg();
      arglist = PTree::snoc(list, new PTree::Atom(tk));
      break;
    }
    else if(arg_declaration(d, arg_encode))
    {
      encode.append(arg_encode);
      list = PTree::snoc(list, d);
      t = my_lexer.look_ahead(0);
      if(t == ',')
      {
	my_lexer.get_token(tk);
	list = PTree::snoc(list, new PTree::Atom(tk));
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

/*
  arg.declaration
    : {userdef.keyword | REGISTER} type.specifier arg.declarator
      {'=' expression}
*/
bool Parser::arg_declaration(PTree::Node *&decl, PTree::Encoding &encode)
{
  Trace trace("Parser::arg_declaration");
  PTree::Node *header, *type_name, *arg, *e;
  Token tk;
  PTree::Encoding name_encode;

  switch(my_lexer.look_ahead(0))
  {
    case Token::REGISTER :
      my_lexer.get_token(tk);
      header = new PTree::AtomREGISTER(tk);
      break;
    case Token::UserKeyword :
      if(!userdef_keyword(header)) return false;
      break;
    default :
      header = 0;
      break;
  }

  if(!type_specifier(type_name, true, encode)) return false;

  if(!declarator(arg, kArgDeclarator, false, encode, name_encode, true)) return false;

  decl = header ? PTree::list(header, type_name, arg) : PTree::list(type_name, arg);
  int t = my_lexer.look_ahead(0);
  if(t == '=')
  {
    my_lexer.get_token(tk);
    if(!initialize_expr(e)) return false;
    
    decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk), e));
  }
  return true;
}

/*
  initialize.expr
  : expression
  | '{' initialize.expr (',' initialize.expr)* {','} '}'
*/
bool Parser::initialize_expr(PTree::Node *&exp)
{
  Trace trace("Parser::initialize_expr");
  Token tk;
  PTree::Node *e, *elist;

  if(my_lexer.look_ahead(0) != '{') return expression(exp);
  else
  {
    my_lexer.get_token(tk);
    PTree::Node *ob = new PTree::Atom(tk);
    elist = 0;
    int t = my_lexer.look_ahead(0);
    while(t != '}')
    {
      if(!initialize_expr(e))
      {
	if(!mark_error()) return false; // too many errors

	skip_to('}');
	my_lexer.get_token(tk);
	exp = PTree::list(ob, 0, new PTree::Atom(tk));
	return true;		// error recovery
      }

      elist = PTree::snoc(elist, e);
      t = my_lexer.look_ahead(0);
      if(t == '}') break;
      else if(t == ',')
      {
	my_lexer.get_token(tk);
	elist = PTree::snoc(elist, new PTree::Atom(tk));
	t = my_lexer.look_ahead(0);
      }
      else
      {
	if(!mark_error()) return false; // too many errors

	skip_to('}');
	my_lexer.get_token(tk);
	exp = PTree::list(ob, 0, new PTree::Atom(tk));
	return true;		// error recovery
      }
    }
    my_lexer.get_token(tk);
    exp = new PTree::Brace(ob, elist, new PTree::Atom(tk));
    return true;
  }
}

/*
  function.arguments
  : empty
  | expression (',' expression)*

  This assumes that the next token following function.arguments is ')'.
*/
bool Parser::function_arguments(PTree::Node *&args)
{
  Trace trace("Parser::function_arguments");
  PTree::Node *exp;
  Token tk;

  args = 0;
  if(my_lexer.look_ahead(0) == ')') return true;
  
  while(true)
  {
    if(!expression(exp)) return false;

    args = PTree::snoc(args, exp);
    if(my_lexer.look_ahead(0) != ',') return true;
    else
    {
      my_lexer.get_token(tk);
      args = PTree::snoc(args, new PTree::Atom(tk));
    }
  }
}

/*
  enum.spec
  : ENUM Identifier
  | ENUM {Identifier} '{' {enum.body} '}'
*/
bool Parser::enum_spec(PTree::EnumSpec *&spec, PTree::Encoding &encode)
{
  Trace trace("Parser::enum_spec");
  Token tk, tk2;
  PTree::Node *body;

  if(my_lexer.get_token(tk) != Token::ENUM) return false;

  spec = new PTree::EnumSpec(new PTree::Atom(tk));
  int t = my_lexer.get_token(tk);
  if(t == Token::Identifier)
  {
    PTree::Node *name = new PTree::Atom(tk);
    encode.simple_name(name);
    spec->set_encoded_name(encode);
    spec = PTree::snoc(spec, name);
    if(my_lexer.look_ahead(0) == '{') t = my_lexer.get_token(tk);
    else return true;
  }
  else
  {
    encode.anonymous();
    spec->set_encoded_name(encode);
    spec = PTree::snoc(spec, 0);
  }
  if(t != '{') return false;
  
  if(my_lexer.look_ahead(0) == '}') body = 0;
  else if(!enum_body(body)) return false;

  if(my_lexer.get_token(tk2) != '}') return false;

  spec = PTree::snoc(spec, 
		     new PTree::Brace(new PTree::Atom(tk), body,
				      new PTree::CommentedAtom(tk2, wrap_comments(my_lexer.get_comments()))));
  declare(spec);
  return true;
}

/*
  enum.body
  : Identifier {'=' expression} (',' Identifier {'=' expression})* {','}
*/
bool Parser::enum_body(PTree::Node *&body)
{
  Trace trace("Parser::enum_body");
  Token tk, tk2;
  PTree::Node *name, *exp;

  body = 0;
  while(true)
  {
    if(my_lexer.look_ahead(0) == '}') return true;
    if(my_lexer.get_token(tk) != Token::Identifier) return false;

    PTree::Node *comments = wrap_comments(my_lexer.get_comments());
    
    if(my_lexer.look_ahead(0, tk2) != '=')
      name = new PTree::CommentedAtom(tk, comments);
    else
    {
      my_lexer.get_token(tk2);
      if(!expression(exp))
      {
	if(!mark_error()) return false; // too many errors

	skip_to('}');
	body = 0; // empty
	return true;		// error recovery
      }
      name = PTree::list(new PTree::CommentedAtom(tk, comments),
			 new PTree::Atom(tk2), exp);
    }

    if(my_lexer.look_ahead(0) != ',')
    {
      body = PTree::snoc(body, name);
      return true;
    }
    else
    {
      my_lexer.get_token(tk);
      body = PTree::nconc(body, PTree::list(name, new PTree::Atom(tk)));
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
bool Parser::class_spec(PTree::ClassSpec *&spec, PTree::Encoding &encode)
{
  Trace trace("Parser::class_spec");
  PTree::Node *head, *bases, *body, *name;
  Token tk;

  head = 0;
  if(my_lexer.look_ahead(0) == Token::UserKeyword)
    if(!userdef_keyword(head)) return false;

  int t = my_lexer.get_token(tk);
  if(t != Token::CLASS && t != Token::STRUCT && t != Token::UNION) return false;

  spec = new PTree::ClassSpec(new PTree::Reserved(tk), 0, my_comments);
  my_comments = 0;
  if(head != 0) spec = new PTree::ClassSpec(head, spec, 0);

  if(my_lexer.look_ahead(0) == '{')
  {
    encode.anonymous();
    // FIXME: why is the absense of a name marked by a list(0, 0) ?
    spec = PTree::snoc(spec, PTree::list(0, 0));
  }
  else
  {
    if(!this->name(name, encode)) return false;
    spec = PTree::snoc(spec, name);
    t = my_lexer.look_ahead(0);
    if(t == ':')
    {
      if(!base_specifiers(bases)) return false;

      spec = PTree::snoc(spec, bases);
    }
    else if(t == '{') spec = PTree::snoc(spec, 0);
    else
    {
      spec->set_encoded_name(encode);
      if (!my_in_template_decl) declare(spec);
      return true;	// class.key Identifier
    }
  }
  spec->set_encoded_name(encode);
  if (!my_in_template_decl) declare(spec);

  SymbolLookup::Table::Guard guard(&my_symbols.enter_class(spec));

  if(!class_body(body)) return false;

  spec = PTree::snoc(spec, body);

  return true;
}

/*
  base.specifiers
  : ':' base.specifier (',' base.specifier)*

  base.specifier
  : {{VIRTUAL} (PUBLIC | PROTECTED | PRIVATE) {VIRTUAL}} name
*/
bool Parser::base_specifiers(PTree::Node *&bases)
{
  Trace trace("Parser::base_specifiers");
  Token tk;
  int t;
  PTree::Node *name;
  PTree::Encoding encode;

  if(my_lexer.get_token(tk) != ':') return false;

  bases = PTree::list(new PTree::Atom(tk));
  while(true)
  {
    PTree::Node *super = 0;
    t = my_lexer.look_ahead(0);
    if(t == Token::VIRTUAL)
    {
      my_lexer.get_token(tk);
      super = PTree::snoc(super, new PTree::AtomVIRTUAL(tk));
      t = my_lexer.look_ahead(0);
    }

    if(t == Token::PUBLIC | t == Token::PROTECTED | t == Token::PRIVATE){
      PTree::Node *lf;
      switch(my_lexer.get_token(tk))
      {
        case Token::PUBLIC :
	  lf = new PTree::AtomPUBLIC(tk);
	  break;
        case Token::PROTECTED :
	  lf = new PTree::AtomPROTECTED(tk);
	  break;
        case Token::PRIVATE :
	  lf = new PTree::AtomPRIVATE(tk);
	  break;
        default :
	  throw std::runtime_error("Parser::base_specifiers(): fatal");
      }
      
      super = PTree::snoc(super, lf);
      t = my_lexer.look_ahead(0);
    }

    if(t == Token::VIRTUAL)
    {
      my_lexer.get_token(tk);
      super = PTree::snoc(super, new PTree::AtomVIRTUAL(tk));
    }

    encode.clear();
    if(!this->name(name, encode)) return false;

    if(!name->is_atom()) name = new PTree::Name(name, encode);

    super = PTree::snoc(super, name);
    bases = PTree::snoc(bases, super);
    if(my_lexer.look_ahead(0) != ',') return true;
    else
    {
      my_lexer.get_token(tk);
      bases = PTree::snoc(bases, new PTree::Atom(tk));
    }
  }
}

/*
  class.body : '{' (class.members)* '}'
*/
bool Parser::class_body(PTree::Node *&body)
{
  Trace trace("Parser::class_body");
  Token tk;
  PTree::Node *mems, *m;

  if(my_lexer.get_token(tk) != '{') return false;

  PTree::Node *ob = new PTree::Atom(tk);
  mems = 0;
  while(my_lexer.look_ahead(0) != '}')
  {
    if(!class_member(m))
    {
      if(!mark_error()) return false;	// too many errors

      skip_to('}');
      my_lexer.get_token(tk);
      body = PTree::list(ob, 0, new PTree::Atom(tk));
      return true;	// error recovery
    }

    my_lexer.get_comments();
    mems = PTree::snoc(mems, m);
  }

  my_lexer.get_token(tk);
  body = new PTree::ClassBody(ob, mems, 
			      new PTree::CommentedAtom(tk, wrap_comments(my_lexer.get_comments())));
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
bool Parser::class_member(PTree::Node *&mem)
{
  Trace trace("Parser::class_member");
  Token tk1, tk2;

  int t = my_lexer.look_ahead(0);
  if(t == Token::PUBLIC || t == Token::PROTECTED || t == Token::PRIVATE)
  {
    PTree::Node *lf;
    switch(my_lexer.get_token(tk1))
    {
      case Token::PUBLIC :
	lf = new PTree::AtomPUBLIC(tk1);
	break;
      case Token::PROTECTED :
	lf = new PTree::AtomPROTECTED(tk1);
	break;
      case Token::PRIVATE :
	lf = new PTree::AtomPRIVATE(tk1);
	break;
      default :
	throw std::runtime_error("Parser::class_member(): fatal");
    }

    if(my_lexer.get_token(tk2) != ':') return false;

    mem = new PTree::AccessSpec(lf, PTree::list(new PTree::Atom(tk2))); return true;
  }
  else if(t == Token::UserKeyword4) return user_access_spec(mem);
  else if(t == ';') return null_declaration(mem);
  else if(t == Token::TYPEDEF)
  {
    PTree::Typedef *td;
    bool result = typedef_(td);
    mem = td;
    return result;
  }
  else if(t == Token::TEMPLATE) return template_decl(mem);
  else if(t == Token::USING)
  {
    PTree::Using *udecl;
    bool result = using_(udecl);
    declare(udecl);
    mem = udecl;
    return result;
  }
  else if(t == Token::METACLASS) return metaclass_decl(mem);
  else
  {
    const char *pos = my_lexer.save();
    PTree::Declaration *decl;
    if(declaration(decl))
    {
      PTree::Node *comments = wrap_comments(my_lexer.get_comments());
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
bool Parser::access_decl(PTree::Node *&mem)
{
  Trace trace("Parser::access_decl");
  PTree::Node *name;
  PTree::Encoding encode;
  Token tk;

  if(!this->name(name, encode)) return false;

  if(my_lexer.get_token(tk) != ';') return false;

  mem = new PTree::AccessDecl(new PTree::Name(name, encode),
			      PTree::list(new PTree::Atom(tk)));
  return true;
}

/*
  user.access.spec
  : UserKeyword4 ':'
  | UserKeyword4 '(' function.arguments ')' ':'
*/
bool Parser::user_access_spec(PTree::Node *&mem)
{
  Trace trace("Parser::user_access_spec");
  Token tk1, tk2, tk3, tk4;
  PTree::Node *args;

  if(my_lexer.get_token(tk1) != Token::UserKeyword4) return false;

  int t = my_lexer.get_token(tk2);
  if(t == ':')
  {
    mem = new PTree::UserAccessSpec(new PTree::Atom(tk1),
				    PTree::list(new PTree::Atom(tk2)));
    return true;
  }
  else if(t == '(')
  {
    if(!function_arguments(args)) return false;
    if(my_lexer.get_token(tk3) != ')') return false;
    if(my_lexer.get_token(tk4) != ':') return false;

    mem = new PTree::UserAccessSpec(new PTree::Atom(tk1),
				    PTree::list(new PTree::Atom(tk2), args,
						new PTree::Atom(tk3),
						new PTree::Atom(tk4)));
    return true;
  }
  else return false;
}

/*
  comma.expression
  : expression
  | comma.expression ',' expression	(left-to-right)
*/
bool Parser::comma_expression(PTree::Node *&exp)
{
  Trace trace("Parser::comma_expression");
  Token tk;
  PTree::Node *right;

  if(!expression(exp)) return false;

  while(my_lexer.look_ahead(0) == ',')
  {
    my_lexer.get_token(tk);
    if(!expression(right)) return false;

    exp = new PTree::CommaExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  expression
  : conditional.expr {(AssignOp | '=') expression}	right-to-left
*/
bool Parser::expression(PTree::Node *&exp)
{
  Trace trace("Parser::expression");
  Token tk;
  PTree::Node *left, *right;

  if(!conditional_expr(left, false)) return false;

  int t = my_lexer.look_ahead(0);
  if(t != '=' && t != Token::AssignOp) exp = left;
  else
  {
    my_lexer.get_token(tk);
    if(!expression(right)) return false;
    
    exp = new PTree::AssignExpr(left, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  conditional.expr
  : logical.or.expr {'?' comma.expression ':' conditional.expr}  right-to-left
*/
bool Parser::conditional_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::conditional_expr");
  Token tk1, tk2;
  PTree::Node *then, *otherwise;

  if(!logical_or_expr(exp, temp_args)) return false;

  if(my_lexer.look_ahead(0) == '?')
  {
    my_lexer.get_token(tk1);
    if(!comma_expression(then)) return false;
    if(my_lexer.get_token(tk2) != ':') return false;
    if(!conditional_expr(otherwise, temp_args)) return false;

    exp = new PTree::CondExpr(exp, PTree::list(new PTree::Atom(tk1), then,
					       new PTree::Atom(tk2), otherwise));
  }
  return true;
}

/*
  logical.or.expr
  : logical.and.expr
  | logical.or.expr LogOrOp logical.and.expr		left-to-right
*/
bool Parser::logical_or_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::logical_or_expr");
  Token tk;
  PTree::Node *right;

  if(!logical_and_expr(exp, temp_args)) return false;

  while(my_lexer.look_ahead(0) == Token::LogOrOp)
  {
    my_lexer.get_token(tk);
    if(!logical_and_expr(right, temp_args)) return false;
    
    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  logical.and.expr
  : inclusive.or.expr
  | logical.and.expr LogAndOp inclusive.or.expr
*/
bool Parser::logical_and_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::logical_and_expr");
  Token tk;
  PTree::Node *right;

  if(!inclusive_or_expr(exp, temp_args)) return false;

  while(my_lexer.look_ahead(0) == Token::LogAndOp)
  {
    my_lexer.get_token(tk);
    if(!inclusive_or_expr(right, temp_args)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  inclusive.or.expr
  : exclusive.or.expr
  | inclusive.or.expr '|' exclusive.or.expr
*/
bool Parser::inclusive_or_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::inclusive_or_expr");
  Token tk;
  PTree::Node *right;

  if(!exclusive_or_expr(exp, temp_args)) return false;

  while(my_lexer.look_ahead(0) == '|')
  {
    my_lexer.get_token(tk);
    if(!exclusive_or_expr(right, temp_args)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  exclusive.or.expr
  : and.expr
  | exclusive.or.expr '^' and.expr
*/
bool Parser::exclusive_or_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::exclusive_or_expr");
  Token tk;
  PTree::Node *right;

  if(!and_expr(exp, temp_args)) return false;

  while(my_lexer.look_ahead(0) == '^')
  {
    my_lexer.get_token(tk);
    if(!and_expr(right, temp_args)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  and.expr
  : equality.expr
  | and.expr '&' equality.expr
*/
bool Parser::and_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::and_expr");
  Token tk;
  PTree::Node *right;

  if(!equality_expr(exp, temp_args)) return false;

  while(my_lexer.look_ahead(0) == '&')
  {
    my_lexer.get_token(tk);
    if(!equality_expr(right, temp_args)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  equality.expr
  : relational.expr
  | equality.expr EqualOp relational.expr
*/
bool Parser::equality_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::equality_expr");
  Token tk;
  PTree::Node *right;

  if(!relational_expr(exp, temp_args)) return false;

  while(my_lexer.look_ahead(0) == Token::EqualOp)
  {
    my_lexer.get_token(tk);
    if(!relational_expr(right, temp_args)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  relational.expr
  : shift.expr
  | relational.expr (RelOp | '<' | '>') shift.expr
*/
bool Parser::relational_expr(PTree::Node *&exp, bool temp_args)
{
  Trace trace("Parser::relational_expr");
  int t;
  Token tk;
  PTree::Node *right;

  if(!shift_expr(exp)) return false;

  while(t = my_lexer.look_ahead(0),
	(t == Token::RelOp || t == '<' || (t == '>' && !temp_args)))
  {
    my_lexer.get_token(tk);
    if(!shift_expr(right)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  shift.expr
  : additive.expr
  | shift.expr ShiftOp additive.expr
*/
bool Parser::shift_expr(PTree::Node *&exp)
{
  Trace trace("Parser::shift_expr");
  Token tk;
  PTree::Node *right;

  if(!additive_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == Token::ShiftOp)
  {
    my_lexer.get_token(tk);
    if(!additive_expr(right)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  additive.expr
  : multiply.expr
  | additive.expr ('+' | '-') multiply.expr
*/
bool Parser::additive_expr(PTree::Node *&exp)
{
  Trace trace("Parser::additive_expr");
  int t;
  Token tk;
  PTree::Node *right;

  if(!multiply_expr(exp)) return false;

  while(t = my_lexer.look_ahead(0), (t == '+' || t == '-'))
  {
    my_lexer.get_token(tk);
    if(!multiply_expr(right)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  multiply.expr
  : pm.expr
  | multiply.expr ('*' | '/' | '%') pm.expr
*/
bool Parser::multiply_expr(PTree::Node *&exp)
{
  Trace trace("Parser::multiply_expr");
  int t;
  Token tk;
  PTree::Node *right;

  if(!pm_expr(exp)) return false;

  while(t = my_lexer.look_ahead(0), (t == '*' || t == '/' || t == '%'))
  {
    my_lexer.get_token(tk);
    if(!pm_expr(right)) return false;

    exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  pm.expr	(pointer to member .*, ->*)
  : cast.expr
  | pm.expr PmOp cast.expr
*/
bool Parser::pm_expr(PTree::Node *&exp)
{
  Trace trace("Parser::pm_expr");
  Token tk;
  PTree::Node *right;

  if(!cast_expr(exp)) return false;

  while(my_lexer.look_ahead(0) == Token::PmOp)
  {
    my_lexer.get_token(tk);
    if(!cast_expr(right)) return false;

    exp = new PTree::PmExpr(exp, PTree::list(new PTree::Atom(tk), right));
  }
  return true;
}

/*
  cast.expr
  : unary.expr
  | '(' type.name ')' cast.expr
*/
bool Parser::cast_expr(PTree::Node *&exp)
{
  Trace trace("Parser::cast_expr");
  if(my_lexer.look_ahead(0) != '(') return unary_expr(exp);
  else
  {
    Token tk1, tk2;
    PTree::Node *tname;
    const char* pos = my_lexer.save();
    my_lexer.get_token(tk1);
    if(typename_(tname))
      if(my_lexer.get_token(tk2) == ')')
	if(cast_expr(exp))
	{
	  exp = new PTree::CastExpr(new PTree::Atom(tk1),
				    PTree::list(tname, new PTree::Atom(tk2), exp));
	  return true;
	}

    my_lexer.restore(pos);
    return unary_expr(exp);
  }
}

/*
  type.name
  : type.specifier cast.declarator
*/
bool Parser::typename_(PTree::Node *&tname)
{
  Trace trace("Parser::typename_");
  PTree::Encoding type_encode;
  return typename_(tname, type_encode);
}

bool Parser::typename_(PTree::Node *&tname, PTree::Encoding &type_encode)
{
  Trace trace("Parser::typename_");
  PTree::Node *type_name, *arg;
  PTree::Encoding name_encode;

  if(!type_specifier(type_name, true, type_encode)) return false;
  if(!declarator(arg, kCastDeclarator, false, type_encode, name_encode, false))
    return false;

  tname = PTree::list(type_name, arg); 
  return true;
}

/*
  unary.expr
  : postfix.expr
  | ('*' | '&' | '+' | '-' | '!' | '~' | IncOp) cast.expr
  | sizeof.expr
  | allocate.expr
  | throw.expression
*/
bool Parser::unary_expr(PTree::Node *&exp)
{
  Trace trace("Parser::unary_expr");
  int t = my_lexer.look_ahead(0);
  if(t == '*' || t == '&' || t == '+' || t == '-' || t == '!' || t == '~' ||
     t == Token::IncOp)
  {
    Token tk;
    PTree::Node *right;

    my_lexer.get_token(tk);
    if(!cast_expr(right)) return false;

    exp = new PTree::UnaryExpr(new PTree::Atom(tk), PTree::list(right));
    return true;
  }
  else if(t == Token::SIZEOF) return sizeof_expr(exp);
  else if(t == Token::THROW) return throw_expr(exp);
  else if(is_allocate_expr(t)) return allocate_expr(exp);
  else return postfix_expr(exp);
}

/*
  throw.expression
  : THROW {expression}
*/
bool Parser::throw_expr(PTree::Node *&exp)
{
  Trace trace("Parser::throw_expr");
  Token tk;
  PTree::Node *e;

  if(my_lexer.get_token(tk) != Token::THROW) return false;

  int t = my_lexer.look_ahead(0);
  if(t == ':' || t == ';') e = 0;
  else if(!expression(e)) return false;

  exp = new PTree::ThrowExpr(new PTree::Reserved(tk), PTree::list(e));
  return true;
}

/*
  sizeof.expr
  : SIZEOF unary.expr
  | SIZEOF '(' type.name ')'
*/
bool Parser::sizeof_expr(PTree::Node *&exp)
{
  Trace trace("Parser::sizeof_expr");
  Token tk;
  PTree::Node *unary;

  if(my_lexer.get_token(tk) != Token::SIZEOF) return false;
  if(my_lexer.look_ahead(0) == '(')
  {
    PTree::Node *tname;
    Token op, cp;

    const char* pos = my_lexer.save();
    my_lexer.get_token(op);
    if(typename_(tname))
      if(my_lexer.get_token(cp) == ')')
      {
	exp = new PTree::SizeofExpr(new PTree::Atom(tk),
				    PTree::list(new PTree::Atom(op), tname,
						new PTree::Atom(cp)));
	return true;
      }
    my_lexer.restore(pos);
  }
  if(!unary_expr(unary)) return false;

  exp = new PTree::SizeofExpr(new PTree::Atom(tk), PTree::list(unary));
  return true;
}

/*
  typeid.expr
  : TYPEID unary.expr
  | TYPEID '(' type.name ')'
*/
bool Parser::typeid_expr(PTree::Node *&exp)
{
  Trace trace("Parser::typeid_expr");
  Token tk;
  PTree::Node *unary;

  if(my_lexer.get_token(tk) != Token::TYPEID) return false;

  if(my_lexer.look_ahead(0) == '(')
  {
    PTree::Node *tname;
    Token op, cp;

    const char* pos = my_lexer.save();
    my_lexer.get_token(op);
    if(typename_(tname))
      if(my_lexer.get_token(cp) == ')')
      {
	exp = new PTree::TypeidExpr(new PTree::Atom(tk),
				    PTree::list(new PTree::Atom(op), tname,
						new PTree::Atom(cp)));
	return true;
      }
    my_lexer.restore(pos);
  }
  if(!unary_expr(unary)) return false;

  exp = new PTree::TypeidExpr(new PTree::Atom(tk), PTree::list(unary));
  return true;
}

bool Parser::is_allocate_expr(int t)
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
bool Parser::allocate_expr(PTree::Node *&exp)
{
  Trace trace("Parser::allocate_expr");
  Token tk;
  PTree::Node *head = 0;

  bool ukey = false;
  int t = my_lexer.look_ahead(0);
  if(t == Token::Scope)
  {
    my_lexer.get_token(tk);
    head = new PTree::Atom(tk);
  }
  else if(t == Token::UserKeyword)
  {
    if(!userdef_keyword(head)) return false;
    ukey = true;
  }

  t = my_lexer.get_token(tk);
  if(t == Token::DELETE)
  {
    PTree::Node *obj;
    if(ukey) return false;

    if(head == 0) exp = new PTree::DeleteExpr(new PTree::Reserved(tk), 0);
    else exp = new PTree::DeleteExpr(head, PTree::list(new PTree::Reserved(tk)));

    if(my_lexer.look_ahead(0) == '[')
    {
      my_lexer.get_token(tk);
      exp = PTree::snoc(exp, new PTree::Atom(tk));
      if(my_lexer.get_token(tk) != ']') return false;

      exp = PTree::snoc(exp, new PTree::Atom(tk));
    }
    if(!cast_expr(obj)) return false;

    exp = PTree::snoc(exp, obj);
    return true;
  }
  else if(t == Token::NEW)
  {
    PTree::Node *atype;
    if(head == 0) exp = new PTree::NewExpr(new PTree::Reserved(tk), 0);
    else exp = new PTree::NewExpr(head, PTree::list(new PTree::Reserved(tk)));

    if(!allocate_type(atype)) return false;

    exp = PTree::nconc(exp, atype);
    return true;
  }
  else return false;
}

/*
  userdef.keyword
  : [UserKeyword | UserKeyword5] {'(' function.arguments ')'}
*/
bool Parser::userdef_keyword(PTree::Node *&ukey)
{
  Token tk;

  int t = my_lexer.get_token(tk);
  if(t != Token::UserKeyword && t != Token::UserKeyword5) return false;

  if(my_lexer.look_ahead(0) != '(')
    ukey = new PTree::UserdefKeyword(new PTree::Atom(tk), 0);
  else
  {
    PTree::Node *args;
    Token op, cp;
    my_lexer.get_token(op);
    if(!function_arguments(args)) return false;

    if(my_lexer.get_token(cp) != ')') return false;

    ukey = new PTree::UserdefKeyword(new PTree::Atom(tk),
				     PTree::list(new PTree::Atom(op), args,
						 new PTree::Atom(cp)));
  }
  return true;
}

/*
  allocate.type
  : {'(' function.arguments ')'} type.specifier new.declarator
    {allocate.initializer}
  | {'(' function.arguments ')'} '(' type.name ')' {allocate.initializer}
*/
bool Parser::allocate_type(PTree::Node *&atype)
{
  Trace trace("Parser::allocate_type");
  Token op, cp;
  PTree::Node *tname, *init, *exp;

  if(my_lexer.look_ahead(0) != '(') atype = PTree::list(0);
  else
  {
    my_lexer.get_token(op);

    const char* pos = my_lexer.save();
    if(typename_(tname))
      if(my_lexer.get_token(cp) == ')')
	if(my_lexer.look_ahead(0) != '(')
	{
	  atype = PTree::list(0, PTree::list(new PTree::Atom(op), tname,
					     new PTree::Atom(cp)));
	  if(!is_type_specifier()) return true;
	}
	else if(allocate_initializer(init))
	{
	  atype = PTree::list(0, PTree::list(new PTree::Atom(op), tname,
					     new PTree::Atom(cp)),
			      init);
	  // the next token cannot be '('
	  if(my_lexer.look_ahead(0) != '(') return true;
	}

    // if we reach here, we have to process '(' function.arguments ')'.
    my_lexer.restore(pos);
    if(!function_arguments(exp)) return false;

    if(my_lexer.get_token(cp) != ')') return false;

    atype = PTree::list(PTree::list(new PTree::Atom(op), exp, new PTree::Atom(cp)));
  }
  if(my_lexer.look_ahead(0) == '(')
  {
    my_lexer.get_token(op);
    if(!typename_(tname)) return false;
    if(my_lexer.get_token(cp) != ')') return false;

    atype = PTree::snoc(atype, PTree::list(new PTree::Atom(op), tname,
					   new PTree::Atom(cp)));
  }
  else
  {
    PTree::Node *decl;
    PTree::Encoding type_encode;
    if(!type_specifier(tname, false, type_encode)) return false;
    if(!new_declarator(decl, type_encode)) return false;

    atype = PTree::snoc(atype, PTree::list(tname, decl));
  }

  if(my_lexer.look_ahead(0) == '(')
  {
    if(!allocate_initializer(init)) return false;

    atype = PTree::snoc(atype, init);
  }
  return true;
}

/*
  new.declarator
  : empty
  | ptr.operator
  | {ptr.operator} ('[' comma.expression ']')+
*/
bool Parser::new_declarator(PTree::Node *&decl, PTree::Encoding& encode)
{
  Trace trace("Parser::new_declarator");
  decl = 0;
  if(my_lexer.look_ahead(0) != '[')
    if(!opt_ptr_operator(decl, encode)) return false;

  while(my_lexer.look_ahead(0) == '[')
  {
    Token ob, cb;
    PTree::Node *exp;
    my_lexer.get_token(ob);
    if(!comma_expression(exp)) return false;
    if(my_lexer.get_token(cb) != ']') return false;

    encode.array();
    decl = PTree::nconc(decl, PTree::list(new PTree::Atom(ob), exp,
					  new PTree::Atom(cb)));
  }

  decl = decl ? new PTree::Declarator(decl, encode) : new PTree::Declarator(encode);
  return true;
}

/*
  allocate.initializer
  : '(' {initialize.expr (',' initialize.expr)* } ')'
*/
bool Parser::allocate_initializer(PTree::Node *&init)
{
  Trace trace("Parser::allocate_initializer");
  Token op, cp;

  if(my_lexer.get_token(op) != '(') return false;

  if(my_lexer.look_ahead(0) == ')')
  {
    my_lexer.get_token(cp);
    init = PTree::list(new PTree::Atom(op), 0, new PTree::Atom(cp));
    return true;
  }

  init = 0;
  while(true)
  {
    PTree::Node *exp;
    if(!initialize_expr(exp)) return false;

    init = PTree::snoc(init, exp);
    if(my_lexer.look_ahead(0) != ',') break;
    else
    {
      Token tk;
      my_lexer.get_token(tk);
      init = PTree::snoc(init, new PTree::Atom(tk));
    }
  }
  my_lexer.get_token(cp);
  init = PTree::list(new PTree::Atom(op), init, new PTree::Atom(cp));
  return true;
}

/*
  postfix.exp
  : primary.exp
  | postfix.expr '[' comma.expression ']'
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
bool Parser::postfix_expr(PTree::Node *&exp)
{
  Trace trace("Parser::postfix_expr");
  PTree::Node *e;
  Token cp, op;
  int t, t2;

  if(!primary_expr(exp)) return false;

  while(true)
  {
    switch(my_lexer.look_ahead(0))
    {
      case '[' :
	my_lexer.get_token(op);
	if(!comma_expression(e)) return false;
	if(my_lexer.get_token(cp) != ']') return false;

	exp = new PTree::ArrayExpr(exp, PTree::list(new PTree::Atom(op),
						    e, new PTree::Atom(cp)));
	break;
      case '(' :
	my_lexer.get_token(op);
	if(!function_arguments(e)) return false;
	if(my_lexer.get_token(cp) != ')') return false;

	exp = new PTree::FuncallExpr(exp, PTree::list(new PTree::Atom(op),
						      e, new PTree::Atom(cp)));
	break;
      case Token::IncOp :
	my_lexer.get_token(op);
	exp = new PTree::PostfixExpr(exp, PTree::list(new PTree::Atom(op)));
	break;
      case '.' :
      case Token::ArrowOp :
	t2 = my_lexer.get_token(op);
	t = my_lexer.look_ahead(0);
	if(t == Token::UserKeyword || t == Token::UserKeyword2 || t == Token::UserKeyword3)
	{
	  if(!userdef_statement(e)) return false;

	  exp = new PTree::UserStatementExpr(exp, PTree::cons(new PTree::Atom(op), e));
	  break;
	}
	else
	{
	  if(!var_name(e)) return false;
	  if(t2 == '.')
	    exp = new PTree::DotMemberExpr(exp, PTree::list(new PTree::Atom(op), e));
	  else
	    exp = new PTree::ArrowMemberExpr(exp, PTree::list(new PTree::Atom(op), e));
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
  | '(' comma.expression ')'
  | integral.or.class.spec '(' function.arguments ')'
  | openc++.primary.exp
  | typeid '(' typething ')'

  openc++.primary.exp
  : var.name '::' userdef.statement
*/
bool Parser::primary_expr(PTree::Node *&exp)
{
  Trace trace("Parser::primary_expr");
  Token tk, tk2;
  PTree::Node *exp2;
  PTree::Encoding cast_type_encode;

  switch(my_lexer.look_ahead(0))
  {
    case Token::Constant :
    case Token::CharConst :
    case Token::WideCharConst :
    case Token::StringL :
    case Token::WideStringL :
      my_lexer.get_token(tk);
      exp = new PTree::Literal(tk);
      return true;
    case Token::THIS :
      my_lexer.get_token(tk);
      exp = new PTree::This(tk);
      return true;
    case Token::TYPEID :
      return typeid_expr(exp);
    case '(' :
      my_lexer.get_token(tk);
      if(!comma_expression(exp2)) return false;
      if(my_lexer.get_token(tk2) != ')') return false;

      exp = new PTree::ParenExpr(new PTree::Atom(tk),
				 PTree::list(exp2, new PTree::Atom(tk2)));
      return true;
    default :
      if(!opt_integral_type_or_class_spec(exp, cast_type_encode)) return false;

      if(exp)
      { // if integral.or.class.spec
	if(my_lexer.get_token(tk) != '(') return false;
	if(!function_arguments(exp2)) return false;
	if(my_lexer.get_token(tk2) != ')') return false;

	exp = new PTree::FstyleCastExpr(cast_type_encode, exp,
					PTree::list(new PTree::Atom(tk), exp2,
						    new PTree::Atom(tk2)));
	return true;
      }
      else
      {
	if(!var_name(exp)) return false;
	if(my_lexer.look_ahead(0) == Token::Scope)
	{
	  my_lexer.get_token(tk);
	  if(!userdef_statement(exp2)) return false;

	  exp = new PTree::StaticUserStatementExpr(exp,
						   PTree::cons(new PTree::Atom(tk),
							       exp2));
	}
	return true;
      }
  }
}

bool Parser::typeof_expr(PTree::Node *&node)
{
  Trace trace("Parser::typeof_expr");
  Token tk, tk2;

  Token::Type t = my_lexer.get_token(tk);
  if (t != Token::TYPEOF)
    return false;
  if ((t = my_lexer.get_token(tk2)) != '(')
    return false;
  PTree::Node *type = PTree::list(new PTree::Atom(tk2));
#if 1
  if (!expression(node)) return false;
#else
  PTree::Encoding name_encode;
  if (!name(node, name_encode)) return false; 	 
  if (!node->is_atom())
    node = new PTree::Name(node, name_encode);
  else
    node = new PTree::Name(PTree::list(node), name_encode);
#endif
  type = PTree::snoc(type, node);
  if ((t = my_lexer.get_token(tk2)) != ')') return false;
  type = PTree::snoc(type, new PTree::Atom(tk2));
  node = new PTree::TypeofExpr(new PTree::Atom(tk), type);
  return true;
}

/*
  userdef.statement
  : UserKeyword '(' function.arguments ')' compound.statement
  | UserKeyword2 '(' arg.decl.list ')' compound.statement
  | UserKeyword3 '(' expr.statement {comma.expression} ';'
			{comma.expression} ')' compound.statement
*/
bool Parser::userdef_statement(PTree::Node *&st)
{
  Token tk, tk2, tk3, tk4;
  PTree::Node *keyword, *exp, *exp2, *exp3;
  PTree::Encoding dummy_encode;

  int t = my_lexer.get_token(tk);
  if(my_lexer.get_token(tk2) != '(') return false;

  switch(t)
  {
    case Token::UserKeyword :
      keyword = new PTree::Reserved(tk);
      if(!function_arguments(exp)) return false;
      goto rest;
    case Token::UserKeyword2 :
      keyword = new PTree::AtomUserKeyword2(tk);
      if(!arg_decl_list(exp, dummy_encode)) return false;
    rest:
    {
      if(my_lexer.get_token(tk3) != ')') return false;
      PTree::Block *body;
      if(!compound_statement(body)) return false;
      st = PTree::list(keyword, new PTree::Atom(tk2), exp, new PTree::Atom(tk3), body);
      return true;
    }
    case Token::UserKeyword3 :
    {
      if(!expr_statement(exp)) return false;

      if(my_lexer.look_ahead(0) == ';') exp2 = 0;
      else if(!comma_expression(exp2)) return false;
      if(my_lexer.get_token(tk3) != ';') return false;
      if(my_lexer.look_ahead(0) == ')') exp3 = 0;
      else if(!comma_expression(exp3)) return false;
      if(my_lexer.get_token(tk4) != ')') return false;
      PTree::Block *body;
      if(!compound_statement(body)) return false;

      st = PTree::list(new PTree::Atom(tk), new PTree::Atom(tk2), exp, exp2,
		       new PTree::Atom(tk3), exp3, new PTree::Atom(tk4), body);
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
bool Parser::var_name(PTree::Node *&name)
{
  Trace trace("Parser::var_name");
  PTree::Encoding encode;

  if(var_name_core(name, encode))
  {
    if(!name->is_atom()) name = new PTree::Name(name, encode);
    return true;
  }
  else return false;
}

bool Parser::var_name_core(PTree::Node *&name, PTree::Encoding &encode)
{
  Trace trace("Parser::var_name_core");
  Token tk;
  int length = 0;
  
  if(my_lexer.look_ahead(0) == Token::Scope)
  {
    my_lexer.get_token(tk);
    name = PTree::list(new PTree::Atom(tk));
    encode.global_scope();
    ++length;
  }
  else name = 0;

  while(true)
  {
    int t = my_lexer.get_token(tk);
    if(t == Token::TEMPLATE) 
    {
      // Skip template token, next will be identifier
      t = my_lexer.get_token(tk);
    }
    if(t == Token::Identifier)
    {
      PTree::Node *n = new PTree::Identifier(tk.ptr, tk.length);
      if(is_template_args())
      {
	PTree::Node *args;
	PTree::Encoding args_encode;
	if(!template_args(args, args_encode)) return false;

	encode.template_(n, args_encode);
	++length;
	n = PTree::list(n, args);
      }
      else
      {
	encode.simple_name(n);
	++length;
      }
      if(more_var_name())
      {
	my_lexer.get_token(tk);
	name = PTree::nconc(name, PTree::list(n, new PTree::Atom(tk)));
      }
      else
      {
	if(name == 0) name = n;
	else name = PTree::snoc(name, n);
	if(length > 1) encode.qualified(length);
	return true;
      }
    }
    else if(t == '~')
    {
      Token tk2;
      if(my_lexer.look_ahead(0) != Token::Identifier) return false;
      my_lexer.get_token(tk2);
      PTree::Node *class_name = new PTree::Atom(tk2);
      PTree::Node *dt = PTree::list(new PTree::Atom(tk), class_name);
      if(name == 0) name = dt;
      else name = PTree::snoc(name, dt);
      encode.destructor(class_name);
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else if(t == Token::OPERATOR)
    {
      PTree::Node *op;
      if(!operator_name(op, encode)) return false;

      PTree::Node *opf = PTree::list(new PTree::Reserved(tk), op);
      name = name ? PTree::snoc(name, opf) : opf;
      if(length > 0) encode.qualified(length + 1);
      return true;
    }
    else return false;
  }
}

bool Parser::more_var_name()
{
  Trace trace("Parser::more_var_name");
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

/*
  condition
  : comma.expression
  | declarator.with.init

  Condition is used inside if, switch statements where a declarator can be
  used if it is initialised.
*/
bool Parser::condition(PTree::Node *&exp)
{
  Trace trace("Parser::condition");
  PTree::Encoding type_encode;

  // Do declarator first, otherwise "T*foo = blah" gets matched as a
  // multiplication infix expression inside an assignment expression!
  const char *save = my_lexer.save();
  do 
  {
    PTree::Node *storage_s, *cv_q, *cv_q2, *integral, *head, *decl;

    if (!opt_storage_spec(storage_s)) break;

    head = storage_s;
	
    if (!opt_cv_qualify(cv_q) ||
	!opt_integral_type_or_class_spec(integral, type_encode))
      break;

    if (integral)
    {
      // Integral Declaration
      // Find const after type
      if (!opt_cv_qualify(cv_q2)) break;
      // Make type ptree with pre and post const ptrees
      if (cv_q)
	if (cv_q2 == 0)
	  integral = PTree::snoc(cv_q, integral);
	else
	  integral = PTree::nconc(cv_q, PTree::cons(integral, cv_q2));
      else if (cv_q2) integral = PTree::cons(integral, cv_q2);
      // Store type of CV's
      type_encode.cv_qualify(cv_q, cv_q2);
      // Parse declarator
      if (!declarator_with_init(decl, type_encode, true, false)) break;
      // *must* be end of condition, condition is in a () pair
      if (my_lexer.look_ahead(0) != ')') break;
      exp = new PTree::Declaration(head, PTree::list(integral, decl));
    }
    else
    {
      // Other Declaration
      PTree::Node *type_name;
      // Find name of type
      if (!name(type_name, type_encode)) break;
      // Find const after type
      if (!opt_cv_qualify(cv_q2)) break;
      // Make type ptree with pre and post const ptrees
      if (cv_q)
	if (cv_q2 == 0) type_name = PTree::snoc(cv_q, type_name);
	else type_name = PTree::nconc(cv_q, PTree::cons(type_name, cv_q2));
      else if (cv_q2) type_name = PTree::cons(type_name, cv_q2);
      // Store type of CV's
      type_encode.cv_qualify(cv_q, cv_q2);
      // Parse declarator
      if (!declarator_with_init(decl, type_encode, true, false)) break;
      // *must* be end of condition, condition is in a () pair
      if (my_lexer.look_ahead(0) != ')') break;
      exp = new PTree::Declaration(head, PTree::list(type_name, decl));
    }
    return true;
  } while(false);

  // Must be a comma expression
  my_lexer.restore(save);
  return comma_expression(exp);
}

/*
  function.body  : compound.statement
*/
bool Parser::function_body(PTree::Block *&body)
{
  Trace trace("Parser::function_body");
  return compound_statement(body);
}

/*
  compound.statement
  : '{' (statement)* '}'
*/
bool Parser::compound_statement(PTree::Block *&body, bool create_scope)
{
  Trace trace("Parser::compound_statement");

  Token ob;
  if(my_lexer.get_token(ob) != '{') return false;

  PTree::Node *ob_comments = wrap_comments(my_lexer.get_comments());
  body = new PTree::Block(new PTree::CommentedAtom(ob, ob_comments), 0);

  SymbolLookup::Table::Guard guard(create_scope ? 
				   &my_symbols.enter_block(body) :
				   0);

  PTree::Node *sts = 0;
  while(my_lexer.look_ahead(0) != '}')
  {
    PTree::Node *st;
    if(!statement(st))
    {
      if(!mark_error()) return false; // too many errors
      skip_to('}');
      Token cb;
      my_lexer.get_token(cb);
      body = new PTree::Block(new PTree::Atom(ob), 0, new PTree::Atom(cb));
      return true;	// error recovery
    }

    sts = PTree::snoc(sts, st);
  }
  Token cb;
  if(my_lexer.get_token(cb) != '}') return false;

  PTree::Node *cb_comments = wrap_comments(my_lexer.get_comments());
  body = PTree::nconc(body, PTree::list(sts, new PTree::CommentedAtom(cb, cb_comments)));
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
  | RETURN { comma.expression } ';'
  | GOTO Identifier ';'
  | CASE expression ':' statement
  | DEFAULT ':' statement
  | Identifier ':' statement
  | expr.statement
*/
bool Parser::statement(PTree::Node *&st)
{
  Trace trace("Parser::statement");
  Token tk1, tk2, tk3;
  PTree::Node *st2, *exp, *comments;
  int k;

  // Get the comments - if we dont make it past the switch then it is a
  // parse error anyway!
  comments = wrap_comments(my_lexer.get_comments());

  // Whichever case we get, it must succeed
  switch(k = my_lexer.look_ahead(0))
  {
    case '{' :
    {
      PTree::Block *block;
      if (!compound_statement(block, true)) return false;
      st = block;
      break;
    }
    case Token::USING :
    {
      PTree::Using *udecl;
      if (!using_(udecl)) return false;
      declare(udecl);
      st = udecl;
      break;
    }
    case Token::TYPEDEF :
    {
      PTree::Typedef *td;
      if (!typedef_(td)) return false;
      st = td;
      break;
    }
    case Token::IF :
      if (!if_statement(st)) return false;
      break;
    case Token::SWITCH :
      if (!switch_statement(st)) return false;
      break;
    case Token::WHILE :
      if (!while_statement(st)) return false;
      break;
    case Token::DO :
      if (!do_statement(st)) return false;
      break;
    case Token::FOR :
      if (!for_statement(st)) return false;
      break;
    case Token::TRY :
      if (!try_statement(st)) return false;
      break;
    case Token::BREAK :
    case Token::CONTINUE :
      my_lexer.get_token(tk1);
      if(my_lexer.get_token(tk2) != ';') return false;
      if(k == Token::BREAK)
	st = new PTree::BreakStatement(new PTree::Reserved(tk1),
				       PTree::list(new PTree::Atom(tk2)));
      else
	st = new PTree::ContinueStatement(new PTree::Reserved(tk1),
					  PTree::list(new PTree::Atom(tk2)));
      break;
    case Token::RETURN :
      my_lexer.get_token(tk1);
      if(my_lexer.look_ahead(0) == ';')
      {
	my_lexer.get_token(tk2);
	st = new PTree::ReturnStatement(new PTree::Reserved(tk1),
					PTree::list(new PTree::Atom(tk2)));
      } 
      else 
      {
	if(!comma_expression(exp)) return false;
	if(my_lexer.get_token(tk2) != ';') return false;

	st = new PTree::ReturnStatement(new PTree::Reserved(tk1),
					PTree::list(exp, new PTree::Atom(tk2)));
      }
      break;
    case Token::GOTO :
      my_lexer.get_token(tk1);
      if(my_lexer.get_token(tk2) != Token::Identifier) return false;
      if(my_lexer.get_token(tk3) != ';') return false;

      st = new PTree::GotoStatement(new PTree::Reserved(tk1),
				    PTree::list(new PTree::Atom(tk2), new PTree::Atom(tk3)));
      break;
    case Token::CASE :
      my_lexer.get_token(tk1);
      if(!expression(exp)) return false;
      if(my_lexer.get_token(tk2) != ':') return false;
      if(!statement(st2)) return false;

      st = new PTree::CaseStatement(new PTree::Reserved(tk1),
				    PTree::list(exp, new PTree::Atom(tk2), st2));
      break;
    case Token::DEFAULT :
      my_lexer.get_token(tk1);
      if(my_lexer.get_token(tk2) != ':') return false;
      if(!statement(st2)) return false;

      st = new PTree::DefaultStatement(new PTree::Reserved(tk1),
				       PTree::list(new PTree::Atom(tk2), st2));
      break;
    case Token::Identifier :
      if(my_lexer.look_ahead(1) == ':')
      { // label statement
	my_lexer.get_token(tk1);
	my_lexer.get_token(tk2);
	if(!statement(st2)) return false;

	st = new PTree::LabelStatement(new PTree::Atom(tk1),
				       PTree::list(new PTree::Atom(tk2), st2));
	return true;
      }
      // don't break here!
    default :
      if (!expr_statement(st)) return false;
  }

  // No parse error, attach comment to whatever was returned
  set_leaf_comments(st, comments);
  return true;
}

/*
  if.statement
  : IF '(' declaration.statement ')' statement { ELSE statement }
  : IF '(' comma.expression ')' statement { ELSE statement }
*/
bool Parser::if_statement(PTree::Node *&st)
{
  Trace trace("Parser::if_statement");
  Token tk1, tk2, tk3, tk4;
  PTree::Node *exp, *then, *otherwise;

  if(my_lexer.get_token(tk1) != Token::IF) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!condition(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(!statement(then)) return false;

  st = new PTree::IfStatement(new PTree::Reserved(tk1),
			      PTree::list(new PTree::Atom(tk2), exp, new PTree::Atom(tk3),
					  then));
  if(my_lexer.look_ahead(0) == Token::ELSE)
  {
    my_lexer.get_token(tk4);
    if(!statement(otherwise)) return false;

    st = PTree::nconc(st, PTree::list(new PTree::Atom(tk4), otherwise));
  }
  return true;
}

/*
  switch.statement
  : SWITCH '(' comma.expression ')' statement
*/
bool Parser::switch_statement(PTree::Node *&st)
{
  Trace trace("Parser::switch_statement");
  Token tk1, tk2, tk3;
  PTree::Node *exp, *body;

  if(my_lexer.get_token(tk1) != Token::SWITCH) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!condition(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(!statement(body)) return false;

  st = new PTree::SwitchStatement(new PTree::Reserved(tk1),
				  PTree::list(new PTree::Atom(tk2), exp,
					      new PTree::Atom(tk3), body));
  return true;
}

/*
  while.statement
  : WHILE '(' comma.expression ')' statement
*/
bool Parser::while_statement(PTree::Node *&st)
{
  Trace trace("Parser::which_statement");
  Token tk1, tk2, tk3;
  PTree::Node *exp, *body;

  if(my_lexer.get_token(tk1) != Token::WHILE) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!comma_expression(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(!statement(body)) return false;

  st = new PTree::WhileStatement(new PTree::Reserved(tk1),
				 PTree::list(new PTree::Atom(tk2), exp,
					     new PTree::Atom(tk3), body));
  return true;
}

/*
  do.statement
  : DO statement WHILE '(' comma.expression ')' ';'
*/
bool Parser::do_statement(PTree::Node *&st)
{
  Trace trace("Parser::do_statement");
  Token tk0, tk1, tk2, tk3, tk4;
  PTree::Node *exp, *body;

  if(my_lexer.get_token(tk0) != Token::DO) return false;
  if(!statement(body)) return false;
  if(my_lexer.get_token(tk1) != Token::WHILE) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!comma_expression(exp)) return false;
  if(my_lexer.get_token(tk3) != ')') return false;
  if(my_lexer.get_token(tk4) != ';') return false;

  st = new PTree::DoStatement(new PTree::Reserved(tk0),
			      PTree::list(body, new PTree::Reserved(tk1),
					  new PTree::Atom(tk2), exp,
					  new PTree::Atom(tk3), new PTree::Atom(tk4)));
  return true;
}

/*
  for.statement
  : FOR '(' expr.statement {comma.expression} ';' {comma.expression} ')'
    statement
*/
bool Parser::for_statement(PTree::Node *&st)
{
  Trace trace("Parser::for_statement");
  Token tk1, tk2, tk3, tk4;
  PTree::Node *exp1, *exp2, *exp3, *body;

  if(my_lexer.get_token(tk1) != Token::FOR) return false;
  if(my_lexer.get_token(tk2) != '(') return false;
  if(!expr_statement(exp1)) return false;
  if(my_lexer.look_ahead(0) == ';') exp2 = 0;
  else if(!comma_expression(exp2)) return false;
  if(my_lexer.get_token(tk3) != ';') return false;
  if(my_lexer.look_ahead(0) == ')') exp3 = 0;
  else if(!comma_expression(exp3)) return false;
  if(my_lexer.get_token(tk4) != ')') return false;
  if(!statement(body)) return false;

  st = new PTree::ForStatement(new PTree::Reserved(tk1),
			       PTree::list(new PTree::Atom(tk2), exp1, exp2,
					   new PTree::Atom(tk3), exp3,
					   new PTree::Atom(tk4), body));
  return true;
}

/*
  try.statement
  : TRY compound.statement (exception.handler)+ ';'

  exception.handler
  : CATCH '(' (arg.declaration | Ellipsis) ')' compound.statement
*/
bool Parser::try_statement(PTree::Node *&st)
{
  Trace trace("Parser::try_statement");
  Token tk, op, cp;

  if(my_lexer.get_token(tk) != Token::TRY) return false;
  PTree::Block *body;
  if(!compound_statement(body)) return false;

  st = new PTree::TryStatement(new PTree::Reserved(tk), PTree::list(body));

  do
  {
    if(my_lexer.get_token(tk) != Token::CATCH) return false;
    if(my_lexer.get_token(op) != '(') return false;
    PTree::Node *handler;
    if(my_lexer.look_ahead(0) == Token::Ellipsis)
    {
      my_lexer.get_token(cp);
      handler = new PTree::Atom(cp);
    }
    else
    {
      PTree::Encoding encode;
      if(!arg_declaration(handler, encode)) return false;
    }
    if(my_lexer.get_token(cp) != ')') return false;
    PTree::Block *body;
    if(!compound_statement(body)) return false;

    st = PTree::snoc(st, PTree::list(new PTree::Reserved(tk),
				     new PTree::Atom(op), handler, new PTree::Atom(cp),
				     body));
  } while(my_lexer.look_ahead(0) == Token::CATCH);
  return true;
}

/*
  expr.statement
  : ';'
  | declaration.statement
  | comma.expression ';'
  | openc++.postfix.expr
  | openc++.primary.exp
*/
bool Parser::expr_statement(PTree::Node *&st)
{
  Trace trace("Parser::expr_statement");
  Token tk;

  if(my_lexer.look_ahead(0) == ';')
  {
    my_lexer.get_token(tk);
    st = new PTree::ExprStatement(0, PTree::list(new PTree::Atom(tk)));
    return true;
  }
  else
  {
    const char *pos = my_lexer.save();
    PTree::Declaration *decl;
    if(declaration_statement(decl))
    {
      declare(decl);
      st = decl;
      return true;
    }
    else
    {
      PTree::Node *exp;
      my_lexer.restore(pos);
      if(!comma_expression(exp)) return false;
      if(PTree::is_a(exp, Token::ntUserStatementExpr, Token::ntStaticUserStatementExpr))
      {
	st = exp;
	return true;
      }
      if(my_lexer.get_token(tk) != ';') return false;

      st = new PTree::ExprStatement(exp, PTree::list(new PTree::Atom(tk)));
      return true;
    }
  }
}

/*
  declaration.statement
  : decl.head integral.or.class.spec {cv.qualify} {declarators} ';'
  | decl.head name {cv.qualify} declarators ';'
  | const.declaration

  decl.head
  : {storage.spec} {cv.qualify}

  const.declaration
  : cv.qualify {'*'} Identifier '=' expression {',' declarators} ';'

  Note: if you modify this function, take a look at rDeclaration(), too.
*/
bool Parser::declaration_statement(PTree::Declaration *&statement)
{
  Trace trace("Parser::declaration_statement");
  PTree::Node *storage_s, *cv_q, *integral;
  PTree::Encoding type_encode;

  if(!opt_storage_spec(storage_s) ||
     !opt_cv_qualify(cv_q) ||
     !opt_integral_type_or_class_spec(integral, type_encode))
    return false;

  PTree::Node *head = 0;
  if(storage_s) head = PTree::snoc(head, storage_s);

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
  : decl.head integral.or.class.spec {cv.qualify} {declarators} ';'
*/
bool Parser::integral_decl_statement(PTree::Declaration *&statement, PTree::Encoding& type_encode,
				     PTree::Node *integral, PTree::Node *cv_q, PTree::Node *head)
{
  Trace trace("Parser::integral_decl_statement");
  PTree::Node *cv_q2, *decl;
  Token tk;

  if(!opt_cv_qualify(cv_q2)) return false;

  if(cv_q)
    if(cv_q2 == 0) integral = PTree::snoc(cv_q, integral);
    else integral = PTree::nconc(cv_q, PTree::cons(integral, cv_q2));
  else if(cv_q2) integral = PTree::cons(integral, cv_q2);

  type_encode.cv_qualify(cv_q, cv_q2);
  if(my_lexer.look_ahead(0) == ';')
  {
    my_lexer.get_token(tk);
    statement = new PTree::Declaration(head, PTree::list(integral,
							 new PTree::Atom(tk)));
    return true;
  }
  else
  {
    if(!declarators(decl, type_encode, false, true)) return false;
    if(my_lexer.get_token(tk) != ';') return false;
	    
    statement = new PTree::Declaration(head, PTree::list(integral, decl,
							 new PTree::Atom(tk)));
    return true;
  }
}

/*
   other.decl.statement
   :decl.head name {cv.qualify} declarators ';'
*/
bool Parser::other_decl_statement(PTree::Declaration *&statement,
				  PTree::Encoding& type_encode,
				  PTree::Node *cv_q,
				  PTree::Node *head)
{
  Trace trace("Parser::other_decl_statement");
  PTree::Node *type_name, *cv_q2, *decl;
  Token tk;

  if(!name(type_name, type_encode)) return false;
  if(!opt_cv_qualify(cv_q2)) return false;

  if(cv_q)
    if(cv_q2 == 0) type_name = PTree::snoc(cv_q, type_name);
    else type_name = PTree::nconc(cv_q, PTree::cons(type_name, cv_q2));
  else if(cv_q2) type_name = PTree::cons(type_name, cv_q2);

  type_encode.cv_qualify(cv_q, cv_q2);
  if(!declarators(decl, type_encode, false, true)) return false;
  if(my_lexer.get_token(tk) != ';') return false;

  statement = new PTree::Declaration(head, PTree::list(type_name, decl,
						       new PTree::Atom(tk)));
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
