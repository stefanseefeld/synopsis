/*
  Copyright (C) 1997-2001 Shigeru Chiba, Tokyo Institute of Technology.
  Copyright (C) 2001-2002 Stephen Davies

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

/*
   C++ Parser

   This parser is a LL(k) parser with ad hoc rules such as
   backtracking.

   r<name>() is the grammer rule for a non-terminal <name>.
   opt<name>() is the grammer fule for an optional non-terminal <name>.
   is<name>() looks ahead and returns true if the next symbol is <name>.
*/

#include <Synopsis/Trace.hh>
#include <iostream>
#include "Parser.hh"
#include "Lexer.hh"
#include "Environment.hh"
#include "PTree.hh"
#include "Encoding.hh"
#include "MetaClass.hh"
#include "Walker.hh"

using namespace Synopsis;

#if defined(PARSE_MSVC)
#define _MSC_VER	1100
#endif

const int MaxErrors = 10;

namespace
{
PTree::Node *wrap_comments(const Lexer::Comments &c)
{
  PTree::Node *head = 0;
  for (Lexer::Comments::const_iterator i = c.begin(); i != c.end(); ++i)
    head = PTree::snoc(head, new PTree::Atom(*i));
  return head;
}

}

Parser::Parser(Lexer *l)
{
    lex = l;
    nerrors = 0;
    comments = 0;
}

bool Parser::ErrorMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    if(where != 0){
	PTree::Node *head = PTree::ca_ar(where);
	if(head != 0)
	    ShowMessageHead(head->position());
    }

    std::cerr << msg;
    if(name != 0)
	name->Write(std::cerr);

    std::cerr << '\n';
    return bool(++nerrors < MaxErrors);
}

void Parser::WarningMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    if(where != 0){
	PTree::Node *head = PTree::ca_ar(where);
	if(head != 0)
	    ShowMessageHead(head->position());
    }

    std::cerr << "warning: " << msg;
    if(name != 0)
	name->Write(std::cerr);

    std::cerr << '\n';
}

bool Parser::SyntaxError()
{
    Token t, t2;
    int i;

    lex->look_ahead(0, t);
    lex->look_ahead(1, t2);

    ShowMessageHead(t.ptr);
    std::cerr << "parse error before `";
    if(t.type != '\0')
	for(i = 0; i < t.length; ++i)
	    std::cerr << t.ptr[i];

    if(t2.type != '\0'){
	std::cerr << ' ';
	for(i = 0; i < t2.length; ++i)
	    std::cerr << t2.ptr[i];
    }

    std::cerr << "'\n";
    return bool(++nerrors < MaxErrors);
}

unsigned long Parser::origin(const char *ptr,
			     std::string &filename) const
{
  return lex->origin(ptr, filename);
}

void Parser::ShowMessageHead(const char* pos)
{
    std::string filename;
    unsigned long line = origin(pos, filename);
    std::cerr << filename;

#if defined(_MSC_VER)
    std::cerr << '(' << line << ") : ";
#else
    std::cerr << ':' << line << ": ";
#endif
}

bool Parser::rProgram(PTree::Node *&def)
{
  Trace trace("Parser::rProgram");
  while(lex->look_ahead(0) != '\0')
    if(rDefinition(def)) return true;
    else
    {
      Token tk;
      if(!SyntaxError()) return false;		// too many errors
      SkipTo(';');
      lex->get_token(tk);	// ignore ';'
    }

  // Retrieve trailing comments
  def = wrap_comments(lex->get_comments());
  if (def) return true;
  return false;
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
bool Parser::rDefinition(PTree::Node *&p)
{
    bool res;
    int t = lex->look_ahead(0);
    if(t == ';')
	res = rNullDeclaration(p);
    else if(t == Token::TYPEDEF)
	res = rTypedef(p);
    else if(t == Token::TEMPLATE)
	res = rTemplateDecl(p);
    else if(t == Token::METACLASS)
	res = rMetaclassDecl(p);
    else if(t == Token::EXTERN && lex->look_ahead(1) == Token::StringL)
	res = rLinkageSpec(p);
    else if(t == Token::EXTERN && lex->look_ahead(1) == Token::TEMPLATE)
	res = rExternTemplateDecl(p);
    else if(t == Token::NAMESPACE && lex->look_ahead(2) == '=')
	res = rNamespaceAlias(p);
    else if(t == Token::NAMESPACE)
	res = rNamespaceSpec(p);
    else if(t == Token::USING)
	res = rUsing(p);
    else {
	if (!rDeclaration(p))
	    return false;
	PTree::Node *c = wrap_comments(lex->get_comments());
	if (c) {
	    Walker::SetDeclaratorComments(p, c);
	}
	return true;
    }

    lex->get_comments();
    return res;
}

bool Parser::rNullDeclaration(PTree::Node *&decl)
{
    Token tk;

    if(lex->get_token(tk) != ';')
	return false;

    decl = new PTree::Declaration(0, PTree::list(0, new PTree::Atom(tk)));
    return true;
}

/*
  typedef
  : TYPEDEF type.specifier declarators ';'
*/
bool Parser::rTypedef(PTree::Node *&def)
{
    Token tk;
    PTree::Node *type_name, *decl;
    Encoding type_encode;

    if(lex->get_token(tk) != Token::TYPEDEF)
	return false;

    def = new PTree::Typedef(new PTree::Reserved(tk));
    if(!rTypeSpecifier(type_name, false, type_encode))
	return false;

    def = PTree::snoc(def, type_name);
    if(!rDeclarators(decl, type_encode, true))
	return false;

    if(lex->get_token(tk) != ';')
	return false;

    def = PTree::nconc(def, PTree::list(decl, new PTree::Atom(tk)));
    return true;
}

/*
  type.specifier
  : {cv.qualify} (integral.or.class.spec | name) {cv.qualify}
*/
bool Parser::rTypeSpecifier(PTree::Node *&tspec, bool check, Encoding& encode)
{
    PTree::Node *cv_q, *cv_q2;

    if(!optCvQualify(cv_q) || !optIntegralTypeOrClassSpec(tspec, encode))
	return false;

    if(tspec == 0){
	if(check){
	    Token tk;
	    lex->look_ahead(0, tk);
	    if(!MaybeTypeNameOrClassTemplate(tk))
		return false;
	}

	if(!rName(tspec, encode))
	    return false;
    }

    if(!optCvQualify(cv_q2))
	return false;

    if(cv_q != 0){
	tspec = PTree::snoc(cv_q, tspec);
	if(cv_q2 != 0)
	    tspec = PTree::nconc(tspec, cv_q2);
    }
    else if(cv_q2 != 0)
	tspec = PTree::cons(tspec, cv_q2);

    encode.CvQualify(cv_q, cv_q2);
    return true;
}

// isTypeSpecifier() returns true if the next is probably a type specifier.

bool Parser::isTypeSpecifier()
{
    int t = lex->look_ahead(0);
    if(t == Token::Identifier || t == Token::Scope
       || t == Token::CONST || t == Token::VOLATILE
       || t == Token::CHAR || t == Token::WCHAR 
       || t == Token::INT || t == Token::SHORT || t == Token::LONG
       || t == Token::SIGNED || t == Token::UNSIGNED || t == Token::FLOAT || t == Token::DOUBLE
       || t == Token::VOID || t == Token::BOOLEAN
       || t == Token::CLASS || t == Token::STRUCT || t == Token::UNION || t == Token::ENUM
#if defined(_MSC_VER)
       || t == Token::INT64
#endif
       )
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
bool Parser::rMetaclassDecl(PTree::Node *&decl)
{
    int t;
    Token tk1, tk2, tk3, tk4;
    PTree::Node *metaclass_name;

    if(lex->get_token(tk1) != Token::METACLASS)
	return false;

    if(lex->get_token(tk2) != Token::Identifier)
	return false;

    t = lex->get_token(tk3);
    if(t == Token::Identifier){
	metaclass_name = new PTree::Atom(tk2);
	decl = new PTree::MetaclassDecl(new PTree::Reserved(tk1),
				      PTree::list(metaclass_name,
						  new PTree::Atom(tk3)));
    }
    else if(t == ':'){
	if(lex->get_token(tk4) != Token::Identifier)
	    return false;

	metaclass_name = new PTree::Atom(tk4);
	decl = new PTree::MetaclassDecl(new PTree::Reserved(tk1),
				      PTree::list(metaclass_name,
						  new PTree::Atom(tk2)));
    }
    else if(t == ';'){
	metaclass_name = new PTree::Atom(tk2);
	decl = new PTree::MetaclassDecl(new PTree::Reserved(tk1),
				      PTree::list(metaclass_name, 0,
						  new PTree::Atom(tk3)));
	Metaclass::Load(metaclass_name);
	return true;
    }
    else
	return false;

    t = lex->get_token(tk1);
    if(t == '('){
	PTree::Node *args;
	if(!rMetaArguments(args))
	    return false;

	if(lex->get_token(tk2) != ')')
	    return false;

	decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1), args,
					      new PTree::Atom(tk2)));
	t = lex->get_token(tk1);
    }

    if(t == ';'){
	decl = PTree::snoc(decl, new PTree::Atom(tk1));
	Metaclass::Load(metaclass_name);
	return true;
    }
    else
	return false;
}

/*
  meta.arguments : (anything but ')')*
*/
bool Parser::rMetaArguments(PTree::Node *&args)
{
    int t;
    Token tk;

    int n = 1;
    args = 0;
    for(;;){
	t = lex->look_ahead(0);
	if(t == '\0')
	    return false;
	else if(t == '(')
	    ++n;
	else if(t == ')')
	    if(--n <= 0)
		return true;

	lex->get_token(tk);
	args = PTree::snoc(args, new PTree::Atom(tk));
    }
}

/*
  linkage.spec
  : EXTERN StringL definition
  |  EXTERN StringL linkage.body
*/
bool Parser::rLinkageSpec(PTree::Node *&spec)
{
    Token tk1, tk2;
    PTree::Node *body;

    if(lex->get_token(tk1) != Token::EXTERN)
	return false;

    if(lex->get_token(tk2) != Token::StringL)
	return false;

    spec = new PTree::LinkageSpec(new PTree::AtomEXTERN(tk1),
				PTree::list(new PTree::Atom(tk2)));
    if(lex->look_ahead(0) == '{'){
	if(!rLinkageBody(body))
	    return false;
    }
    else
	if(!rDefinition(body))
	    return false;

    spec = PTree::snoc(spec, body);
    return true;
}

/*
  namespace.spec
  : NAMESPACE Identifier definition
  | NAMESPACE { Identifier } linkage.body
*/
bool Parser::rNamespaceSpec(PTree::Node *&spec)
{
    Token tk1, tk2;
    PTree::Node *name;
    PTree::Node *body;

    if(lex->get_token(tk1) != Token::NAMESPACE)
	return false;

    PTree::Node *comments = wrap_comments(lex->get_comments());

    if(lex->look_ahead(0) == '{')
	name = 0;
    else
	if(lex->get_token(tk2) == Token::Identifier)
	    name = new PTree::Atom(tk2);
	else
	    return false;

    if(lex->look_ahead(0) == '{'){
	if(!rLinkageBody(body))
	    return false;
    }
    else
	if(!rDefinition(body))
	    return false;

    PTree::NamespaceSpec *nspec;
    spec = nspec = new PTree::NamespaceSpec(
	new PTree::AtomNAMESPACE(tk1), PTree::list(name, body));
    
    nspec->SetComments(comments);
    return true;
}

/*
  namespace.alias : NAMESPACE Identifier '=' Identifier ';'
*/
bool Parser::rNamespaceAlias(PTree::Node *&exp)
{
  Token tk;

  if(lex->get_token(tk) != Token::NAMESPACE) return false;
  PTree::Node *ns = new PTree::AtomNAMESPACE(tk);

  if (lex->get_token(tk) != Token::Identifier) return false;
  PTree::Node *alias = new PTree::Atom(tk);

  if (lex->get_token(tk) != '=') return false;
  PTree::Node *eq = new PTree::Atom(tk);

  PTree::Node *name;
  Encoding encode;
  int length = 0;
  if(lex->look_ahead(0) == Token::Scope)
  {
    lex->get_token(tk);
    name = PTree::list(new PTree::Atom(tk));
    encode.GlobalScope();
    ++length;
  }
  else name = 0;

  while (true)
  {
    if (lex->get_token(tk) != Token::Identifier) return false;
    PTree::Node *n = new PTree::Atom(tk);
    encode.SimpleName(n);
    ++length;
    
    if(lex->look_ahead(0) == Token::Scope)
    {
      lex->get_token(tk);
      name = PTree::nconc(name, PTree::list(n, new PTree::Atom(tk)));
    }
    else
    {
      if(name == 0) name = n;
      else name = PTree::snoc(name, n);

      if(length > 1) encode.Qualified(length);

      break;
    }
  }

  if (lex->get_token(tk) != ';') return false;

  exp = new PTree::NamespaceAlias(ns, PTree::list(alias, eq, name, new PTree::Atom(tk)));
  return true;
}

/*
  using.declaration : USING ... ';'
*/
bool Parser::rUsing(PTree::Node *&decl)
{
    Token tk;

    if(lex->get_token(tk) != Token::USING)
	return false;

    decl = new PTree::Using(new PTree::AtomUSING(tk));
    do {
	lex->get_token(tk);
	decl = PTree::snoc(decl, new PTree::Atom(tk));
    } while(tk.type != ';' && tk.type != '\0');

    return true;
}


/*
  linkage.body : '{' (definition)* '}'

  Note: this is also used to construct namespace.spec
*/
bool Parser::rLinkageBody(PTree::Node *&body)
{
    Token op, cp;
    PTree::Node *def;

    if(lex->get_token(op) != '{')
	return false;

    body = 0;
    while(lex->look_ahead(0) != '}'){
	if(!rDefinition(def)){
	    if(!SyntaxError())
		return false;		// too many errors

	    SkipTo('}');
	    lex->get_token(cp);
	    body = PTree::list(new PTree::Atom(op), 0, new PTree::Atom(cp));
	    return true;		// error recovery
	}

	body = PTree::snoc(body, def);
    }

    lex->get_token(cp);
    body = new PTree::Brace(new PTree::Atom(op), body,
			  new PTree::CommentedAtom(cp, wrap_comments(lex->get_comments())));
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
bool Parser::rTemplateDecl(PTree::Node *&decl)
{
    PTree::Node *body;
    TemplateDeclKind kind = tdk_unknown;

    if(!rTemplateDecl2(decl, kind))
	return false;

    if(!rDeclaration(body))
	return false;

    // Repackage the decl and body depending upon what kind of template
    // declaration was observed.
    switch (kind) {
    case tdk_instantiation:
	// Repackage the decl as a PtreeTemplateInstantiation
	decl = body;
	// assumes that decl has the form: [0 [class ...] ;]
	if (PTree::length(decl) != 3) return false;
	if (PTree::first(decl) != 0) return false;
	if (PTree::second(decl)->What() != Token::ntClassSpec) return false;
	if (*PTree::third(decl) != ';') return false;
	decl = new PTree::TemplateInstantiation(PTree::second(decl));
	break;
    case tdk_decl:
    case tdk_specialization:
	decl = PTree::snoc(decl, body);
	break;
    default:
	MopErrorMessage("rTemplateDecl()", "fatal");
	break;
    }

    return true;
}

bool Parser::rTemplateDecl2(PTree::Node *&decl, TemplateDeclKind &kind)
{
    Token tk;
    PTree::Node *args;

    if(lex->get_token(tk) != Token::TEMPLATE)
	return false;

    if(lex->look_ahead(0) != '<') {
	// template instantiation
	decl = 0;
	kind = tdk_instantiation;
	return true;	// ignore TEMPLATE
    }

    decl = new PTree::TemplateDecl(new PTree::Reserved(tk));
    if(lex->get_token(tk) != '<')
	return false;

    decl = PTree::snoc(decl, new PTree::Atom(tk));
    if(!rTempArgList(args))
	return false;

    if(lex->get_token(tk) != '>')
	return false;

    decl = PTree::nconc(decl, PTree::list(args, new PTree::Atom(tk)));

    // ignore nested TEMPLATE
    while (lex->look_ahead(0) == Token::TEMPLATE) {
	lex->get_token(tk);
	if(lex->look_ahead(0) != '<')
	    break;

	lex->get_token(tk);
	if(!rTempArgList(args))
	    return false;

	if(lex->get_token(tk) != '>')
	    return false;
    }

    if (args == 0)
	// template < > declaration
	kind = tdk_specialization;
    else
	// template < ... > declaration
	kind = tdk_decl;

    return true;
}

/*
  temp.arg.list
  : empty
  | temp.arg.declaration (',' temp.arg.declaration)*
*/
bool Parser::rTempArgList(PTree::Node *&args)
{
    Token tk;
    PTree::Node *a;

    if(lex->look_ahead(0) == '>'){
	args = 0;
	return true;
    }

    if(!rTempArgDeclaration(a))
	return false;

    args = PTree::list(a);
    while(lex->look_ahead(0) == ','){
	lex->get_token(tk);
	args = PTree::snoc(args, new PTree::Atom(tk));
	if(!rTempArgDeclaration(a))
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
bool Parser::rTempArgDeclaration(PTree::Node *&decl)
{
    Token tk1, tk2;

    int t0 = lex->look_ahead(0);
    int t1 = lex->look_ahead(1);
    int t2 = lex->look_ahead(2);
    if(t0 == Token::CLASS && t1 == Token::Identifier && (t2 == '=' || t2 == '>' || t2 == ',')) {
	lex->get_token(tk1);
	lex->get_token(tk2);
	PTree::Node *name = new PTree::Atom(tk2);
	decl = PTree::list(new PTree::Atom(tk1), name);

	if(t2 == '='){
	    PTree::Node *default_type;

	    lex->get_token(tk1);
	    if(!rTypeName(default_type))
		return false;

	    decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1),
						  default_type));
	    return true;
	}
	else if (t2 == '>' || t2 == ',')
	    return true;
    }
    else if (t0 == Token::CLASS && (t1 == '=' || t1 == '>' || t1 == ',')) {
	// class without the identifier
	lex->get_token(tk1);
	decl = PTree::list(new PTree::Atom(tk1));

	if(lex->look_ahead(0) == '='){
	    PTree::Node *default_type;

	    lex->get_token(tk1);
	    if(!rTypeName(default_type))
		return false;

	    decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1),
						  default_type));
	}
    }
    else if (t0 == Token::TEMPLATE) {
	TemplateDeclKind kind;
	if(!rTemplateDecl2(decl, kind))
	    return false;

	if (lex->get_token(tk1) != Token::CLASS || lex->get_token(tk2) != Token::Identifier)
	    return false;

	PTree::Node *cspec = new PTree::ClassSpec(new PTree::Reserved(tk1),
					  PTree::cons(new PTree::Atom(tk2),0),
					  0);
	decl = PTree::snoc(decl, cspec);
	if(lex->look_ahead(0) == '='){
            PTree::Node *default_type;
	    lex->get_token(tk1);
	    if(!rTypeName(default_type))
		return false;

	    decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk1),
						  default_type));
	}
    }
    else{
	PTree::Node *type_name, *arg;
	Encoding type_encode, name_encode;
	if(!rTypeSpecifier(type_name, true, type_encode))
	    return false;

	if(!rDeclarator(arg, kArgDeclarator, false, type_encode, name_encode,
			true))
	    return false;

	decl = PTree::list(type_name, arg);
	if(lex->look_ahead(0) == '='){
	    PTree::Node *exp;
	    lex->get_token(tk1);
	    if(!rAdditiveExpr(exp))
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
bool Parser::rExternTemplateDecl(PTree::Node *&decl)
{
    Token tk1, tk2;
    PTree::Node *body;

    if(lex->get_token(tk1) != Token::EXTERN)
	return false;

    if(lex->get_token(tk2) != Token::TEMPLATE)
	return false;

    if(!rDeclaration(body))
	return false;

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
        declaration.  See isConstructorDecl().
*/
bool Parser::rDeclaration(PTree::Node *&statement)
{
    PTree::Node *mem_s, *storage_s, *cv_q, *integral, *head;
    Encoding type_encode;
    int res;

    lex->look_ahead(0);
    comments = wrap_comments(lex->get_comments());

    if(!optMemberSpec(mem_s) || !optStorageSpec(storage_s))
	return false;

    if(mem_s == 0)
	head = 0;
    else
	head = mem_s;	// mem_s is a list.

    if(storage_s != 0)
	head = PTree::snoc(head, storage_s);

    if(mem_s == 0)
	if(optMemberSpec(mem_s))
	    head = PTree::nconc(head, mem_s);
	else
	    return false;

    if(!optCvQualify(cv_q)
       || !optIntegralTypeOrClassSpec(integral, type_encode))
	return false;

    if(integral != 0)
	res = rIntegralDeclaration(statement, type_encode,
				   head, integral, cv_q);
    else{
	type_encode.Clear();
	int t = lex->look_ahead(0);
	if(cv_q != 0 && ((t == Token::Identifier && lex->look_ahead(1) == '=')
			   || t == '*'))
	    res = rConstDeclaration(statement, type_encode, head, cv_q);
	else
	    res = rOtherDeclaration(statement, type_encode,
				    mem_s, cv_q, head);
    }
    if (res && statement && (statement->What() == Token::ntDeclaration))
    {
      static_cast<PTree::Declaration*>(statement)->SetComments(comments);
      comments = 0;
    }
    return res;
}

/* single declaration, for use in a condition (controlling
   expression of switch/while/if) */
bool Parser::rSimpleDeclaration(PTree::Node *&statement)
{
    PTree::Node *cv_q, *integral;
    Encoding type_encode, name_encode;

    /* no member specification permitted here, and no
       storage specifier:
          type-specifier ::=
             simple-type-specifier
             class-specifier
             enum-specifier
             elaborated-type-specifier
             cv-qualifier */

    if(!optCvQualify(cv_q) || !optIntegralTypeOrClassSpec(integral, type_encode))
	return false;
    if (integral == 0 && !rName(integral, type_encode))
        return false;

    if (cv_q != 0 && integral != 0)
        integral = PTree::snoc(cv_q, integral);
    else if (cv_q != 0 && integral == 0)
        integral = cv_q, cv_q = 0;

    /* no type-specifier so far -> can't be a declaration */
    if (integral == 0)
        return false;
    
    PTree::Node *d;
    if(!rDeclarator(d, kDeclarator, false, type_encode, name_encode,
                    true, true))
        return false;

    if (lex->look_ahead(0) != '=')
        return false;

    Token eqs;
    lex->get_token(eqs);
    int t = lex->look_ahead(0);
    PTree::Node *e;
    if(!rExpression(e))
        return false;

    PTree::nconc(d, PTree::list(new PTree::Atom(eqs), e));

    statement = new PTree::Declaration(0, PTree::list(integral,
                                                    PTree::list(d)));
    return true;
}

bool Parser::rIntegralDeclaration(PTree::Node *&statement, Encoding& type_encode,
				  PTree::Node *head, PTree::Node *integral, PTree::Node *cv_q)
{
    Token tk;
    PTree::Node *cv_q2, *decl;

    if(!optCvQualify(cv_q2))
	return false;

    if(cv_q != 0)
	if(cv_q2 == 0)
	    integral = PTree::snoc(cv_q, integral);
	else
	    integral = PTree::nconc(cv_q, PTree::cons(integral, cv_q2));
    else if(cv_q2 != 0)
	integral = PTree::cons(integral, cv_q2);

    type_encode.CvQualify(cv_q, cv_q2);
    switch(lex->look_ahead(0)){
    case ';' :
	lex->get_token(tk);
	statement = new PTree::Declaration(head, PTree::list(integral,
							   new PTree::Atom(tk)));
	return true;
    case ':' :	// bit field
	lex->get_token(tk);
	if(!rExpression(decl))
	    return false;

	decl = PTree::list(PTree::list(new PTree::Atom(tk), decl));
	if(lex->get_token(tk) != ';')
	    return false;

	statement = new PTree::Declaration(head, PTree::list(integral, decl,
							   new PTree::Atom(tk)));
	return true;
    default :
	if(!rDeclarators(decl, type_encode, true))
	    return false;

	if(lex->look_ahead(0) == ';'){
	    lex->get_token(tk);
	    statement = new PTree::Declaration(head, PTree::list(integral, decl,
							       new PTree::Atom(tk)));
	    return true;
	}
	else{
	    PTree::Node *body;
	    if(!rFunctionBody(body))
		return false;

	    if(PTree::length(decl) != 1)
		return false;

	    statement = new PTree::Declaration(head,
					     PTree::list(integral,
							 decl->car(), body));
	    return true;
	}
    }
}

bool Parser::rConstDeclaration(PTree::Node *&statement, Encoding&,
			       PTree::Node *head, PTree::Node *cv_q)
{
    PTree::Node *decl;
    Token tk;
    Encoding type_encode;

    type_encode.SimpleConst();
    if(!rDeclarators(decl, type_encode, false))
	return false;

    if(lex->look_ahead(0) != ';')
	return false;

    lex->get_token(tk);
    statement = new PTree::Declaration(head, PTree::list(cv_q, decl,
						       new PTree::Atom(tk)));
    return true;
}

bool Parser::rOtherDeclaration(PTree::Node *&statement, Encoding& type_encode,
			       PTree::Node *mem_s, PTree::Node *cv_q, PTree::Node *head)
{
    PTree::Node *type_name, *decl, *cv_q2;
    Token tk;

    if(!rName(type_name, type_encode))
	return false;

    if(cv_q == 0 && isConstructorDecl()){
	Encoding ftype_encode;
	if(!rConstructorDecl(decl, ftype_encode))
	    return false;

	decl = PTree::list(new PTree::Declarator(type_name, decl,
					       ftype_encode, type_encode,
					       type_name));
	type_name = 0;
    }
    else if(mem_s != 0 && lex->look_ahead(0) == ';'){
	// FRIEND name ';'
	if(PTree::length(mem_s) == 1 && mem_s->car()->What() == Token::FRIEND){
	    lex->get_token(tk);
	    statement = new PTree::Declaration(head, PTree::list(type_name,
							       new PTree::Atom(tk)));
	    return true;
	}
	else
	    return false;
    }
    else{
	if(!optCvQualify(cv_q2))
	    return false;

	if(cv_q != 0)
	    if(cv_q2 == 0)
		type_name = PTree::snoc(cv_q, type_name);
	    else
		type_name = PTree::nconc(cv_q, PTree::cons(type_name, cv_q2));
	else if(cv_q2 != 0)
	    type_name = PTree::cons(type_name, cv_q2);

	type_encode.CvQualify(cv_q, cv_q2);
	if(!rDeclarators(decl, type_encode, false))
	    return false;
    }

    if(lex->look_ahead(0) == ';'){
	lex->get_token(tk);
	statement = new PTree::Declaration(head, PTree::list(type_name, decl,
							   new PTree::Atom(tk)));
    }
    else{
	PTree::Node *body;
	if(!rFunctionBody(body))
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
bool Parser::isConstructorDecl()
{
    if(lex->look_ahead(0) != '(')
	return false;
    else{
	int t = lex->look_ahead(1);
	if(t == '*' || t == '&' || t == '(')
	    return false;	// declarator
	else if(t == Token::CONST || t == Token::VOLATILE)
	    return true;	// constructor or declarator
	else if(isPtrToMember(1))
	    return false;	// declarator (::*)
	else
	    return true;	// maybe constructor
    }
}

/*
  ptr.to.member
  : {'::'} (identifier {'<' any* '>'} '::')+ '*'
*/
bool Parser::isPtrToMember(int i)
{
    int t0 = lex->look_ahead(i++);

    if(t0 == Token::Scope)
	t0 = lex->look_ahead(i++);

    while(t0 == Token::Identifier){
	int t = lex->look_ahead(i++);
	if(t == '<'){
	    int n = 1;
	    while(n > 0){
		int u = lex->look_ahead(i++);
		if(u == '<')
		    ++n;
		else if(u == '>')
		    --n;
		else if(u == '('){
		    int m = 1;
		    while(m > 0){
			int v = lex->look_ahead(i++);
			if(v == '(')
			    ++m;
			else if(v == ')')
			    --m;
			else if(v == '\0' || v == ';' || v == '}')
			    return false;
		    }
		}
		else if(u == '\0' || u == ';' || u == '}')
		    return false;
	    }

	    t = lex->look_ahead(i++);
	}

	if(t != Token::Scope)
	    return false;

	t0 = lex->look_ahead(i++);
	if(t0 == '*')
	    return true;
    }

    return false;
}

/*
  member.spec
  : (FRIEND | INLINE | VIRTUAL | userdef.keyword)+
*/
bool Parser::optMemberSpec(PTree::Node *&p)
{
    Token tk;
    PTree::Node *lf;
    int t = lex->look_ahead(0);

    p = 0;
    while(t == Token::FRIEND || t == Token::INLINE || t == Token::VIRTUAL || t == Token::UserKeyword5){
	if(t == Token::UserKeyword5){
	    if(!rUserdefKeyword(lf))
		return false;
	}
	else{
	    lex->get_token(tk);
	    if(t == Token::INLINE)
		lf = new PTree::AtomINLINE(tk);
	    else if(t == Token::VIRTUAL)
		lf = new PTree::AtomVIRTUAL(tk);
	    else
		lf = new PTree::AtomFRIEND(tk);
	}

	p = PTree::snoc(p, lf);
	t = lex->look_ahead(0);
    }

    return true;
}

/*
  storage.spec : STATIC | EXTERN | AUTO | REGISTER | MUTABLE
*/
bool Parser::optStorageSpec(PTree::Node *&p)
{
    int t = lex->look_ahead(0);
    if(t == Token::STATIC || t == Token::EXTERN || t == Token::AUTO || t == Token::REGISTER
       || t == Token::MUTABLE){
	Token tk;
	lex->get_token(tk);
	switch(t){
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
	    MopErrorMessage("optStorageSpec()", "fatal");
	    break;
	}
    }
    else
	p = 0;	// no storage specifier

    return true;
}

/*
  cv.qualify : (CONST | VOLATILE)+
*/
bool Parser::optCvQualify(PTree::Node *&cv)
{
    PTree::Node *p = 0;
    for(;;){
	int t = lex->look_ahead(0);
	if(t == Token::CONST || t == Token::VOLATILE){
	    Token tk;
	    lex->get_token(tk);
	    switch(t){
	    case Token::CONST :
		p = PTree::snoc(p, new PTree::AtomCONST(tk));
		break;
	    case Token::VOLATILE :
		p = PTree::snoc(p, new PTree::AtomVOLATILE(tk));
		break;
	    default :
		MopErrorMessage("optCvQualify()", "fatal");
		break;
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

  Note: if editing this, see also isTypeSpecifier().
*/
bool Parser::optIntegralTypeOrClassSpec(PTree::Node *&p, Encoding& encode)
{
    bool is_integral;
    int t;
    char type = ' ', flag = ' ';

    is_integral = false;
    p = 0;
    for(;;){
	t = lex->look_ahead(0);
	if(t == Token::CHAR || t == Token::WCHAR || t == Token::INT || t == Token::SHORT || t == Token::LONG || t == Token::SIGNED
	   || t == Token::UNSIGNED || t == Token::FLOAT || t == Token::DOUBLE || t == Token::VOID
	   || t == Token::BOOLEAN
#if defined(_MSC_VER)
	   || t == Token::INT64
#endif
	   ){
	    Token tk;
	    PTree::Node *kw;
	    lex->get_token(tk);
	    switch(t){
	    case Token::CHAR :
		type = 'c';
		kw = new PTree::AtomCHAR(tk);
		break;
	    case Token::WCHAR :
		type = 'w';
		kw = new PTree::AtomWCHAR(tk);
		break;
	    case Token::INT :
#if defined(_MSC_VER)
            case Token::INT64 : // an int64 is *NOT* an int but...
#endif
		if(type != 's' && type != 'l' && type != 'j' && type != 'r')
		    type = 'i';

		kw = new PTree::AtomINT(tk);
		break;
	    case Token::SHORT :
		type = 's';
		kw = new PTree::AtomSHORT(tk);
		break;
	    case Token::LONG :
		if(type == 'l')
		    type = 'j';		// long long
		else if(type == 'd')
		    type = 'r';		// double long
		else
		    type = 'l';

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
		if(type == 'l')
		    type = 'r';		// long double
		else
		    type = 'd';

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
		MopErrorMessage("optIntegralTypeOrClassSpec()", "fatal");
		kw = 0;
		break;
	    }

	    p = PTree::snoc(p, kw);
	    is_integral = true;
	}
	else
	    break;
    }

    if(is_integral){
	if(flag == 'S' && type != 'c')
	    flag = ' ';

	if(flag != ' ')
	    encode.Append(flag);

	if(type == ' ')
	    type = 'i';		// signed, unsigned

	encode.Append(type);
	return true;
    }

    if(t == Token::CLASS || t == Token::STRUCT || t == Token::UNION || t == Token::UserKeyword)
	return rClassSpec(p, encode);
    else if(t == Token::ENUM)
	return rEnumSpec(p, encode);
    else{
	p = 0;
	return true;
    }
}

/*
  constructor.decl
  : '(' {arg.decl.list} ')' {cv.qualify} {throw.decl}
  {member.initializers} {'=' Constant}
*/
bool Parser::rConstructorDecl(PTree::Node *&constructor, Encoding& encode)
{
    Token op, cp;
    PTree::Node *args, *cv, *throw_decl, *mi;

    if(lex->get_token(op) != '(')
	return false;

    if(lex->look_ahead(0) == ')'){
	args = 0;
	encode.StartFuncArgs();
	encode.Void();
	encode.EndFuncArgs();
    }
    else
	if(!rArgDeclList(args, encode))
	    return false;

    lex->get_token(cp);
    constructor = PTree::list(new PTree::Atom(op), args, new PTree::Atom(cp));
    optCvQualify(cv);
    if(cv != 0){
	encode.CvQualify(cv);
	constructor = PTree::nconc(constructor, cv);
    }

    optThrowDecl(throw_decl);	// ignore in this version

    if(lex->look_ahead(0) == ':')
	if(rMemberInitializers(mi))
	    constructor = PTree::snoc(constructor, mi);
	else
	    return false;

    if(lex->look_ahead(0) == '='){
	Token eq, zero;
	lex->get_token(eq);
	if(lex->get_token(zero) != Token::Constant)
	    return false;

	constructor = PTree::nconc(constructor,
				   PTree::list(new PTree::Atom(eq), new PTree::Atom(zero)));
    }

    encode.NoReturnType();
    return true;
}

/*
  throw.decl : THROW '(' (name {','})* {name} ')'
*/
bool Parser::optThrowDecl(PTree::Node *&throw_decl)
{
    Token tk;
    int t;
    PTree::Node *p = 0;

    if(lex->look_ahead(0) == Token::THROW){
	lex->get_token(tk);
	p = PTree::snoc(p, new PTree::Reserved(tk));

	if(lex->get_token(tk) != '(')
	    return false;

	p = PTree::snoc(p, new PTree::Atom(tk));

	for(;;){
	    PTree::Node *q;
	    Encoding encode;
	    t = lex->look_ahead(0);
	    if(t == '\0')
		return false;
	    else if(t == ')')
		break;
#if defined(PARSE_MSVC)
	    // for MSVC compatibility we accept 'throw(...)' declarations
	    else if(t == Token::Ellipsis)
	    {
	      lex->get_token(tk);
	      p = PTree::snoc(p, new PTree::Atom(tk));
	    }
#endif
	    else if(rName(q, encode))
		p = PTree::snoc(p, q);
	    else
		return false;

	    if(lex->look_ahead(0) == ','){
		lex->get_token(tk);
		p = PTree::snoc(p, new PTree::Atom(tk));
	    }
	    else
		break;
	}

	if(lex->get_token(tk) != ')')
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
bool Parser::rDeclarators(PTree::Node *&decls, Encoding& type_encode,
			  bool should_be_declarator, bool is_statement)
{
    PTree::Node *d;
    Token tk;
    Encoding encode;

    decls = 0;
    for(;;){

	lex->look_ahead(0); // force comment finding
	PTree::Node *comments = wrap_comments(lex->get_comments());

	encode.Reset(type_encode);
	if(!rDeclaratorWithInit(d, encode, should_be_declarator, is_statement))
	    return false;
	
	if (d && (d->What() == Token::ntDeclarator))
	    static_cast<PTree::Declarator*>(d)->SetComments(comments);

	decls = PTree::snoc(decls, d);
	if(lex->look_ahead(0) == ','){
	    lex->get_token(tk);
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
bool Parser::rDeclaratorWithInit(PTree::Node *&dw, Encoding& type_encode,
				 bool should_be_declarator,
				 bool is_statement)
{
    PTree::Node *d, *e;
    Token tk;
    Encoding name_encode;

    if(lex->look_ahead(0) == ':'){	// bit field
	lex->get_token(tk);
	if(!rExpression(e))
	    return false;

	dw = PTree::list(new PTree::Atom(tk), e);
	return true;
    }
    else{
	if(!rDeclarator(d, kDeclarator, false, type_encode, name_encode,
			should_be_declarator, is_statement))
	    return false;

	int t = lex->look_ahead(0);
	if(t == '='){
	    lex->get_token(tk);
	    if(!rInitializeExpr(e))
		return false;

	    dw = PTree::nconc(d, PTree::list(new PTree::Atom(tk), e));
	    return true;
	}
	else if(t == ':'){		// bit field
	    lex->get_token(tk);
	    if(!rExpression(e))
		return false;

	    dw = PTree::nconc(d, PTree::list(new PTree::Atom(tk), e));
	    return true;
	}
	else{
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
bool Parser::rDeclarator(PTree::Node *&decl, DeclKind kind, bool recursive,
			 Encoding& type_encode, Encoding& name_encode,
			 bool should_be_declarator, bool is_statement)
{
    return rDeclarator2(decl, kind, recursive, type_encode, name_encode,
			should_be_declarator, is_statement, 0);
}

bool Parser::rDeclarator2(PTree::Node *&decl, DeclKind kind, bool recursive,
			  Encoding& type_encode, Encoding& name_encode,
			  bool should_be_declarator, bool is_statement,
			  PTree::Node **declared_name)
{
    Encoding recursive_encode;
    PTree::Node *d;
    int t;
    bool recursive_decl = false;
    PTree::Node *declared_name0 = 0;

    if(declared_name == 0)
	declared_name = &declared_name0;

    if(!optPtrOperator(d, type_encode))
	return false;

    const char* lex_save = lex->save();
    t = lex->look_ahead(0);
    if(t == '('){
	Token op, cp;
	PTree::Node *decl2;
	lex->get_token(op);
	recursive_decl = true;
	if(!rDeclarator2(decl2, kind, true, recursive_encode, name_encode,
			 true, false, declared_name))
	    return false;

	if(lex->get_token(cp) != ')')
	{
	    if (kind != kCastDeclarator) 
		return false;
	    lex->restore(lex_save);
	    name_encode.Clear();
	}
	else
	{

	    if(!should_be_declarator)
		if(kind == kDeclarator && d == 0){
		    t = lex->look_ahead(0);
		    if(t != '[' && t != '(')
			return false;
		}

	    d = PTree::snoc(d, PTree::list(new PTree::Atom(op), decl2, new PTree::Atom(cp)));
	}
    }
    else if(kind != kCastDeclarator){
	if (t == Token::INLINE){
	    // TODO: store inline somehow
	    Token i;
	    lex->get_token(i);
	    t = lex->look_ahead(0);
	}
	if (kind == kDeclarator || t == Token::Identifier || t == Token::Scope){
	    // if this is an argument declarator, "int (*)()" is valid.
	    PTree::Node *name;
	    if(rName(name, name_encode))
		d = PTree::snoc(d, name);
	    else
		return false;

	    *declared_name = name;
	}
    }
    else
	name_encode.Clear();	// empty

    for(;;){
	t = lex->look_ahead(0);
	if(t == '('){		// function
	    Encoding args_encode;
	    Token op, cp;
	    PTree::Node *args, *cv, *throw_decl, *mi;
	    bool is_args = true;

	    lex->get_token(op);
	    if(lex->look_ahead(0) == ')'){
		args = 0;
		args_encode.StartFuncArgs();
		args_encode.Void();
		args_encode.EndFuncArgs();
	    }
	    else
		if(!rArgDeclListOrInit(args, is_args, args_encode,
				       is_statement))
		    return false;

	    if(lex->get_token(cp) != ')')
		return false;

	    if(is_args){
		d = PTree::nconc(d, PTree::list(new PTree::Atom(op), args,
						new PTree::Atom(cp)));
		optCvQualify(cv);
		if(cv != 0){
		    args_encode.CvQualify(cv);
		    d = PTree::nconc(d, cv);
		}
	    }
	    else
		d = PTree::snoc(d, PTree::list(new PTree::Atom(op), args,
					       new PTree::Atom(cp)));

	    if(!args_encode.IsEmpty())
		type_encode.Function(args_encode);

	    optThrowDecl(throw_decl);	// ignore in this version

	    if(lex->look_ahead(0) == ':')
		if(rMemberInitializers(mi))
		    d = PTree::snoc(d, mi);
		else
		    return false;

	    break;		// "T f(int)(char)" is invalid.
	}
	else if(t == '['){	// array
	    Token ob, cb;
	    PTree::Node *expr;
	    lex->get_token(ob);
	    if(lex->look_ahead(0) == ']')
		expr = 0;
	    else
		if(!rCommaExpression(expr))
		    return false;

	    if(lex->get_token(cb) != ']')
		return false;

	    type_encode.Array();
	    d = PTree::nconc(d, PTree::list(new PTree::Atom(ob), expr,
					    new PTree::Atom(cb)));
	}
	else
	    break;
    }

    if(recursive_decl)
	type_encode.Recursion(recursive_encode);

    if(recursive)
	decl = d;
    else
	if(d == 0)
	    decl = new PTree::Declarator(type_encode, name_encode,
				       *declared_name);
	else
	    decl = new PTree::Declarator(d, type_encode, name_encode,
				       *declared_name);

    return true;
}

/*
  ptr.operator
  : (('*' | '&' | ptr.to.member) {cv.qualify})+
*/
bool Parser::optPtrOperator(PTree::Node *&ptrs, Encoding& encode)
{
    ptrs = 0;
    for(;;){
	int t = lex->look_ahead(0);
	if(t != '*' && t != '&' && !isPtrToMember(0))
	    break;
	else{
	    PTree::Node *op, *cv;
	    if(t == '*' || t == '&'){
		Token tk;
		lex->get_token(tk);
		op = new PTree::Atom(tk);
		encode.PtrOperator(t);
	    }
	    else
		if(!rPtrToMember(op, encode))
		    return false;

	    ptrs = PTree::snoc(ptrs, op);
	    optCvQualify(cv);
	    if(cv != 0){
		ptrs = PTree::nconc(ptrs, cv);
		encode.CvQualify(cv);
	    }
	}
    }

    return true;
}

/*
  member.initializers
  : ':' member.init (',' member.init)*
*/
bool Parser::rMemberInitializers(PTree::Node *&init)
{
    Token tk;
    PTree::Node *m;

    if(lex->get_token(tk) != ':')
	return false;

    init = PTree::list(new PTree::Atom(tk));
    if(!rMemberInit(m))
	return false;

    init = PTree::snoc(init, m);
    while(lex->look_ahead(0) == ','){
	lex->get_token(tk);
	init = PTree::snoc(init, new PTree::Atom(tk));
	if(!rMemberInit(m))
	    return false;

	init = PTree::snoc(init, m);
    }

    return true;
}

/*
  member.init
  : name '(' function.arguments ')'
*/
bool Parser::rMemberInit(PTree::Node *&init)
{
    PTree::Node *name, *args;
    Token tk1, tk2;
    Encoding encode;

    if(!rName(name, encode))
	return false;

    if(!name->is_atom())
	name = new PTree::Name(name, encode);

    if(lex->get_token(tk1) != '(')
	return false;

    if(!rFunctionArguments(args))
	return false;

    if(lex->get_token(tk2) != ')')
	return false;

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
bool Parser::rName(PTree::Node *&name, Encoding& encode)
{
    Token tk, tk2;
    int t;
    int length = 0;

    if(lex->look_ahead(0) == Token::Scope){
	lex->get_token(tk);
	name = PTree::list(new PTree::Atom(tk));
	encode.GlobalScope();
	++length;
    }
    else
    {
	name = 0;

	// gcc keyword typeof(rName) means type of the given name
	if(lex->look_ahead(0) == Token::TYPEOF){
	    t = lex->get_token(tk);
	    if ((t = lex->get_token(tk2)) != '(')
		return false;
	    PTree::Node *type = PTree::list(new PTree::Atom(tk2));
	    Encoding name_encode;
	    if (!rName(name, name_encode))
		return false;
	    if (!name->is_atom())
		name = new PTree::Name(name, name_encode);
	    else
		name = new PTree::Name(PTree::list(name), name_encode);
	    type = PTree::snoc(type, name);
	    if ((t = lex->get_token(tk2)) != ')')
		return false;
	    type = PTree::snoc(type, new PTree::Atom(tk2));
	    name = new PTree::TypeofExpr(new PTree::Atom(tk), type);
	    return true;
	}
    }

    
    for(;;){
	t = lex->get_token(tk);
	if(t == Token::TEMPLATE) {
	    // Skip template token, next will be identifier
	    t = lex->get_token(tk);
	}
	if(t == Token::Identifier){
	    PTree::Node *n = new PTree::Atom(tk);
	    t = lex->look_ahead(0);
	    if(t == '<'){
		PTree::Node *args;
		Encoding args_encode;
		if(!rTemplateArgs(args, args_encode))
		    return false;

		encode.Template(n, args_encode);
		++length;
		n = PTree::list(n, args);
		t = lex->look_ahead(0);
	    }
	    else{
		encode.SimpleName(n);
		++length;
	    }

	    if(t == Token::Scope){
		lex->get_token(tk);
		name = PTree::nconc(name, PTree::list(n, new PTree::Atom(tk)));
	    }
	    else{
		if(name == 0)
		    name = n;
		else
		    name = PTree::snoc(name, n);

		if(length > 1)
		    encode.Qualified(length);

		return true;
	    }
	}
	else if(t == '~'){
	    if(lex->look_ahead(0) != Token::Identifier)
		return false;

	    lex->get_token(tk2);
	    PTree::Node *class_name = new PTree::Atom(tk2);
	    PTree::Node *dt = PTree::list(new PTree::Atom(tk), class_name);
	    if(name == 0)
		name = dt;
	    else
		name = PTree::snoc(name, dt);

	    encode.Destructor(class_name);
	    if(length > 0)
		encode.Qualified(length + 1);

	    return true;
	}
	else if(t == Token::OPERATOR){
	    PTree::Node *op;
	    PTree::Node *opf;
	    if(!rOperatorName(op, encode))
		return false;

	    t = lex->look_ahead(0);
	    if(t != '<')
		opf = PTree::list(new PTree::Reserved(tk), op);
	    else {
		PTree::Node *args;
		Encoding args_encode;
		if(!rTemplateArgs(args, args_encode))
		    return false;

		// here, I must merge args_encode into encode.
		// I'll do it in future. :p

		opf = PTree::list(new PTree::Reserved(tk), op, args);
	    }

	    if(name == 0)
		name = opf;
	    else
		name = PTree::snoc(name, opf);

	    if(length > 0)
		encode.Qualified(length + 1);

	    return true;
	}
	else
	    return false;
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
bool Parser::rOperatorName(PTree::Node *&name, Encoding& encode)
{
    Token tk;

    int t = lex->look_ahead(0);
    if(t == '+' || t == '-' || t == '*' || t == '/' || t == '%' || t == '^'
       || t == '&' || t == '|' || t == '~' || t == '!' || t == '=' || t == '<'
       || t == '>' || t == Token::AssignOp || t == Token::ShiftOp || t == Token::EqualOp
       || t == Token::RelOp || t == Token::LogAndOp || t == Token::LogOrOp || t == Token::IncOp
       || t == ',' || t == Token::PmOp || t == Token::ArrowOp){
	lex->get_token(tk);
	name = new PTree::Atom(tk);
	encode.SimpleName(name);
	return true;
    }
    else if(t == Token::NEW || t == Token::DELETE){
	lex->get_token(tk);
	if(lex->look_ahead(0) != '['){
	    name = new PTree::Reserved(tk);
	    encode.SimpleName(name);
	    return true;
	}
	else{
	    name = PTree::list(new PTree::Reserved(tk));
	    lex->get_token(tk);
	    name = PTree::snoc(name, new PTree::Atom(tk));
	    if(lex->get_token(tk) != ']')
		return false;

	    name = PTree::snoc(name, new PTree::Atom(tk));
	    if(t == Token::NEW)
		encode.AppendWithLen("new[]", 5);
	    else
		encode.AppendWithLen("delete[]", 8);

	    return true;
	}
    }
    else if(t == '('){
	lex->get_token(tk);
	name = PTree::list(new PTree::Atom(tk));
	if(lex->get_token(tk) != ')')
	    return false;

	encode.AppendWithLen("()", 2);
	name = PTree::snoc(name, new PTree::Atom(tk));
	return true;
    }
    else if(t == '['){
	lex->get_token(tk);
	name = PTree::list(new PTree::Atom(tk));
	if(lex->get_token(tk) != ']')
	    return false;

	encode.AppendWithLen("[]", 2);
	name = PTree::snoc(name, new PTree::Atom(tk));
	return true;
    }
    else
	return rCastOperatorName(name, encode);
}

/*
  cast.operator.name
  : {cv.qualify} (integral.or.class.spec | name) {cv.qualify}
    {(ptr.operator)*}
*/
bool Parser::rCastOperatorName(PTree::Node *&name, Encoding& encode)
{
    PTree::Node *cv1, *cv2, *type_name, *ptr;
    Encoding type_encode;

    if(!optCvQualify(cv1))
	return false;

    if(!optIntegralTypeOrClassSpec(type_name, type_encode))
	return false;

    if(type_name == 0){
	type_encode.Clear();
	if(!rName(type_name, type_encode))
	    return false;
    }

    if(!optCvQualify(cv2))
	return false;

    if(cv1 != 0)
	if(cv2 == 0)
	    type_name = PTree::snoc(cv1, type_name);
	else
	    type_name = PTree::nconc(cv1, PTree::cons(type_name, cv2));
    else if(cv2 != 0)
	type_name = PTree::cons(type_name, cv2);

    type_encode.CvQualify(cv1, cv2);

    if(!optPtrOperator(ptr, type_encode))
	return false;

    encode.CastOperator(type_encode);
    if(ptr == 0){
	name = type_name;
	return true;
    }
    else{
	name = PTree::list(type_name, ptr);
	return true;
    }
}

/*
  ptr.to.member
  : {'::'} (identifier {template.args} '::')+ '*'
*/
bool Parser::rPtrToMember(PTree::Node *&ptr_to_mem, Encoding& encode)
{
    Token tk;
    PTree::Node *p, *n;
    Encoding pm_encode;
    int length = 0;

    if(lex->look_ahead(0) == Token::Scope){
	lex->get_token(tk);
	p = PTree::list(new PTree::Atom(tk));
	pm_encode.GlobalScope();
	++length;
    }
    else
	p = 0;

    for(;;){
        if(lex->get_token(tk) == Token::Identifier)
	    n = new PTree::Atom(tk);
	else
	    return false;

	int t = lex->look_ahead(0);
	if(t == '<'){
	    PTree::Node *args;
	    Encoding args_encode;
	    if(!rTemplateArgs(args, args_encode))
		return false;

	    pm_encode.Template(n, args_encode);
	    ++length;
	    n = PTree::list(n, args);
	    t = lex->look_ahead(0);
	}
	else{
	    pm_encode.SimpleName(n);
	    ++length;
	}

	if(lex->get_token(tk) != Token::Scope)
	    return false;

	p = PTree::nconc(p, PTree::list(n, new PTree::Atom(tk)));
	if(lex->look_ahead(0) == '*'){
	    lex->get_token(tk);
	    p = PTree::snoc(p, new PTree::Atom(tk));
	    break;
	}
    }

    ptr_to_mem = p;
    encode.PtrToMember(pm_encode, length);
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
bool Parser::rTemplateArgs(PTree::Node *&temp_args, Encoding& encode)
{
    Token tk1, tk2;
    Encoding type_encode;

    if(lex->get_token(tk1) != '<')
	return false;

    // in case of Foo<>
    if(lex->look_ahead(0) == '>') {
	lex->get_token(tk2);
	temp_args = PTree::list(new PTree::Atom(tk1), new PTree::Atom(tk2));
	return true;
    }

    PTree::Node *args = 0;
    for(;;){
	PTree::Node *a;
	const char* pos = lex->save();
	type_encode.Clear();

	// Prefer type name, but if not ',' or '>' then must be expression
	if(rTypeName(a, type_encode) && (lex->look_ahead(0) == ',' || lex->look_ahead(0) == '>'))
	    encode.Append(type_encode);
	else {
	    lex->restore(pos);	
	    if(!rConditionalExpr(a, true))
	        return false;

	    encode.ValueTempParam();
	}

	args = PTree::snoc(args, a);
	switch(lex->get_token(tk2)){
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
bool Parser::rArgDeclListOrInit(PTree::Node *&arglist, bool& is_args,
				Encoding& encode, bool maybe_init)
{
    const char* pos = lex->save();
    if(maybe_init) {
	if(rFunctionArguments(arglist))
	    if(lex->look_ahead(0) == ')') {
		is_args = false;
		encode.Clear();
		return true;
	    }

	lex->restore(pos);
	return(is_args = rArgDeclList(arglist, encode));
    }
    else
	if(is_args = rArgDeclList(arglist, encode))
	    return true;
	else{
	    lex->restore(pos);
	    encode.Clear();
	    return rFunctionArguments(arglist);
	}
}

/*
  arg.decl.list
    : empty
    | arg.declaration ( ',' arg.declaration )* {{ ',' } Ellipses}
*/
bool Parser::rArgDeclList(PTree::Node *&arglist, Encoding& encode)
{
    PTree::Node *list;
    PTree::Node *d;
    int t;
    Token tk;
    Encoding arg_encode;

    encode.StartFuncArgs();
    list = 0;
    for(;;){
	arg_encode.Clear();
	t = lex->look_ahead(0);
	if(t == ')'){
	    if(list == 0)
		encode.Void();

	    arglist = list;
	    break;
	}
	else if(t == Token::Ellipsis){
	    lex->get_token(tk);
	    encode.EllipsisArg();
	    arglist = PTree::snoc(list, new PTree::Atom(tk));
	    break;
	}
	else if(rArgDeclaration(d, arg_encode)){
	    encode.Append(arg_encode);
	    list = PTree::snoc(list, d);
	    t = lex->look_ahead(0);
	    if(t == ','){
		lex->get_token(tk);
		list = PTree::snoc(list, new PTree::Atom(tk));
	    }
	    else if(t != ')' && t != Token::Ellipsis)
		return false;
	}
	else{
	    arglist = 0;
	    return false;
	}
    }

    encode.EndFuncArgs();
    return true;
}

/*
  arg.declaration
    : {userdef.keyword | REGISTER} type.specifier arg.declarator
      {'=' expression}
*/
bool Parser::rArgDeclaration(PTree::Node *&decl, Encoding& encode)
{
    PTree::Node *header, *type_name, *arg, *e;
    Token tk;
    Encoding name_encode;

    switch(lex->look_ahead(0)){
    case Token::REGISTER :
	lex->get_token(tk);
	header = new PTree::AtomREGISTER(tk);
	break;
    case Token::UserKeyword :
	if(!rUserdefKeyword(header))
	    return false;
        break;
    default :
	header = 0;
        break;
    }

    if(!rTypeSpecifier(type_name, true, encode))
	return false;

    if(!rDeclarator(arg, kArgDeclarator, false, encode, name_encode, true))
	return false;

    if(header == 0)
	decl = PTree::list(type_name, arg);
    else
	decl = PTree::list(header, type_name, arg);

    int t = lex->look_ahead(0);
    if(t == '='){
	lex->get_token(tk);
	if(!rInitializeExpr(e))
	    return false;

	decl = PTree::nconc(decl, PTree::list(new PTree::Atom(tk), e));
    }

    return true;
}

/*
  initialize.expr
  : expression
  | '{' initialize.expr (',' initialize.expr)* {','} '}'
*/
bool Parser::rInitializeExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *e, *elist;

    if(lex->look_ahead(0) != '{')
	return rExpression(exp);
    else{
	lex->get_token(tk);
	PTree::Node *ob = new PTree::Atom(tk);
	elist = 0;
	int t = lex->look_ahead(0);
	while(t != '}'){
	    if(!rInitializeExpr(e)){
		if(!SyntaxError())
		    return false;	// too many errors

		SkipTo('}');
		lex->get_token(tk);
		exp = PTree::list(ob, 0, new PTree::Atom(tk));
		return true;		// error recovery
	    }

	    elist = PTree::snoc(elist, e);
	    t = lex->look_ahead(0);
	    if(t == '}')
		break;
	    else if(t == ','){
		lex->get_token(tk);
		elist = PTree::snoc(elist, new PTree::Atom(tk));
		t = lex->look_ahead(0);
	    }
	    else{
		if(!SyntaxError())
		    return false;	// too many errors

		SkipTo('}');
		lex->get_token(tk);
		exp = PTree::list(ob, 0, new PTree::Atom(tk));
		return true;		// error recovery
	    }
	}

	lex->get_token(tk);
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
bool Parser::rFunctionArguments(PTree::Node *&args)
{
    PTree::Node *exp;
    Token tk;

    args = 0;
    if(lex->look_ahead(0) == ')')
	return true;

    for(;;){
	if(!rExpression(exp))
	    return false;

	args = PTree::snoc(args, exp);
	if(lex->look_ahead(0) != ',')
	    return true;
	else{
	    lex->get_token(tk);
	    args = PTree::snoc(args, new PTree::Atom(tk));
	}
    }
}

/*
  enum.spec
  : ENUM Identifier
  | ENUM {Identifier} '{' {enum.body} '}'
*/
bool Parser::rEnumSpec(PTree::Node *&spec, Encoding& encode)
{
    Token tk, tk2;
    PTree::Node *body;

    if(lex->get_token(tk) != Token::ENUM)
	return false;

    spec = new PTree::EnumSpec(new PTree::Atom(tk));
    int t = lex->get_token(tk);
    if(t == Token::Identifier){
	PTree::Node *name = new PTree::Atom(tk);
	encode.SimpleName(name);
	((PTree::EnumSpec*)spec)->set_encoded_name(encode.Get());
	spec = PTree::snoc(spec, name);
	if(lex->look_ahead(0) == '{')
	    t = lex->get_token(tk);
	else
	    return true;
    }
    else{
	encode.NoName();
	((PTree::EnumSpec*)spec)->set_encoded_name(encode.Get());
	spec = PTree::snoc(spec, 0);
    }

    if(t != '{')
	return false;

    if(lex->look_ahead(0) == '}')
	body = 0;
    else
	if(!rEnumBody(body))
	    return false;

    if(lex->get_token(tk2) != '}')
	return false;

    spec = PTree::snoc(spec, 
	    new PTree::Brace(
		new PTree::Atom(tk), body,
		new PTree::CommentedAtom(tk2, wrap_comments(lex->get_comments()))));
    return true;
}

/*
  enum.body
  : Identifier {'=' expression} (',' Identifier {'=' expression})* {','}
*/
bool Parser::rEnumBody(PTree::Node *&body)
{
    Token tk, tk2;
    PTree::Node *name, *exp;

    body = 0;
    for(;;){
	if(lex->look_ahead(0) == '}')
	    return true;

	if(lex->get_token(tk) != Token::Identifier)
	    return false;

	PTree::Node *comments = wrap_comments(lex->get_comments());

	if(lex->look_ahead(0, tk2) != '=')
	    name = new PTree::CommentedAtom(tk, comments);
	else{
	    lex->get_token(tk2);
	    if(!rExpression(exp)){
		if(!SyntaxError())
		    return false;	// too many errors

		SkipTo('}');
		body = 0;		// empty
		return true;		// error recovery
	    }

	    name = PTree::list(new PTree::CommentedAtom(tk, comments), new PTree::Atom(tk2), exp);
	}

	if(lex->look_ahead(0) != ','){
	    body = PTree::snoc(body, name);
	    return true;
	}
	else{
	    lex->get_token(tk);
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
bool Parser::rClassSpec(PTree::Node *&spec, Encoding& encode)
{
    PTree::Node *head, *bases, *body, *name;
    Token tk;

    head = 0;
    if(lex->look_ahead(0) == Token::UserKeyword)
	if(!rUserdefKeyword(head))
	    return false;

    int t = lex->get_token(tk);
    if(t != Token::CLASS && t != Token::STRUCT && t != Token::UNION)
	return false;

    spec = new PTree::ClassSpec(new PTree::Reserved(tk), 0, comments);
    comments = 0;
    if(head != 0) spec = new PTree::ClassSpec(head, spec, 0);

    if(lex->look_ahead(0) == '{'){
	encode.NoName();
	spec = PTree::snoc(spec, PTree::list(0, 0));
    }
    else{
	if(!rName(name, encode))
	    return false;

	spec = PTree::snoc(spec, name);
	t = lex->look_ahead(0);
	if(t == ':'){
	    if(!rBaseSpecifiers(bases))
		return false;

	    spec = PTree::snoc(spec, bases);
	}
	else if(t == '{')
	    spec = PTree::snoc(spec, 0);
	else{
	  ((PTree::ClassSpec*)spec)->set_encoded_name(encode.Get());
	    return true;	// class.key Identifier
	}
    }

    ((PTree::ClassSpec*)spec)->set_encoded_name(encode.Get());
    if(!rClassBody(body))
	return false;

    spec = PTree::snoc(spec, body);
    return true;
}

/*
  base.specifiers
  : ':' base.specifier (',' base.specifier)*

  base.specifier
  : {{VIRTUAL} (PUBLIC | PROTECTED | PRIVATE) {VIRTUAL}} name
*/
bool Parser::rBaseSpecifiers(PTree::Node *&bases)
{
    Token tk;
    int t;
    PTree::Node *name;
    Encoding encode;

    if(lex->get_token(tk) != ':')
	return false;

    bases = PTree::list(new PTree::Atom(tk));
    for(;;){
	PTree::Node *super = 0;
	t = lex->look_ahead(0);
	if(t == Token::VIRTUAL){
	    lex->get_token(tk);
	    super = PTree::snoc(super, new PTree::AtomVIRTUAL(tk));
	    t = lex->look_ahead(0);
	}

	if(t == Token::PUBLIC | t == Token::PROTECTED | t == Token::PRIVATE){
	    PTree::Node *lf;
	    switch(lex->get_token(tk)){
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
		MopErrorMessage("rBaseSpecifiers()", "fatal");
		lf = 0;
		break;
	    }

	    super = PTree::snoc(super, lf);
	    t = lex->look_ahead(0);
	}

	if(t == Token::VIRTUAL){
	    lex->get_token(tk);
	    super = PTree::snoc(super, new PTree::AtomVIRTUAL(tk));
	}

	encode.Clear();
	if(!rName(name, encode))
	    return false;

	if(!name->is_atom())
	    name = new PTree::Name(name, encode);

	super = PTree::snoc(super, name);
	bases = PTree::snoc(bases, super);
	if(lex->look_ahead(0) != ',')
	    return true;
	else{
	    lex->get_token(tk);
	    bases = PTree::snoc(bases, new PTree::Atom(tk));
	}
    }
}

/*
  class.body : '{' (class.members)* '}'
*/
bool Parser::rClassBody(PTree::Node *&body)
{
    Token tk;
    PTree::Node *mems, *m;

    if(lex->get_token(tk) != '{')
	return false;

    PTree::Node *ob = new PTree::Atom(tk);
    mems = 0;
    while(lex->look_ahead(0) != '}'){
	if(!rClassMember(m)){
	    if(!SyntaxError())
		return false;	// too many errors

	    SkipTo('}');
	    lex->get_token(tk);
	    body = PTree::list(ob, 0, new PTree::Atom(tk));
	    return true;	// error recovery
	}

	lex->get_comments();
	mems = PTree::snoc(mems, m);
    }

    lex->get_token(tk);
    body = new PTree::ClassBody(ob, mems, 
			      new PTree::CommentedAtom(tk, wrap_comments(lex->get_comments())));
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
bool Parser::rClassMember(PTree::Node *&mem)
{
    Token tk1, tk2;

    int t = lex->look_ahead(0);
    if(t == Token::PUBLIC || t == Token::PROTECTED || t == Token::PRIVATE){
	PTree::Node *lf;
	switch(lex->get_token(tk1)){
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
	    MopErrorMessage("rClassMember()", "fatal");
	    lf = 0;
	    break;
	}

	if(lex->get_token(tk2) != ':')
	    return false;

	mem = new PTree::AccessSpec(lf, PTree::list(new PTree::Atom(tk2)));
	return true;
    }
    else if(t == Token::UserKeyword4)
	return rUserAccessSpec(mem);
    else if(t == ';')
	return rNullDeclaration(mem);
    else if(t == Token::TYPEDEF)
	return rTypedef(mem);
    else if(t == Token::TEMPLATE)
	return rTemplateDecl(mem);
    else if(t == Token::USING)
	return rUsing(mem);
    else if(t == Token::METACLASS)
	return rMetaclassDecl(mem);
    else{
	const char* pos = lex->save();
	if(rDeclaration(mem)) {
	  PTree::Node *comments = wrap_comments(lex->get_comments());
	    if (comments) {
		Walker::SetDeclaratorComments(mem, comments);
	    }
	    return true;
	}

	lex->restore(pos);
	return rAccessDecl(mem);
    }
}

/*
  access.decl
  : name ';'		e.g. <qualified class>::<member name>;
*/
bool Parser::rAccessDecl(PTree::Node *&mem)
{
    PTree::Node *name;
    Encoding encode;
    Token tk;

    if(!rName(name, encode))
	return false;

    if(lex->get_token(tk) != ';')
	return false;

    mem = new PTree::AccessDecl(new PTree::Name(name, encode),
			       PTree::list(new PTree::Atom(tk)));
    return true;
}

/*
  user.access.spec
  : UserKeyword4 ':'
  | UserKeyword4 '(' function.arguments ')' ':'
*/
bool Parser::rUserAccessSpec(PTree::Node *&mem)
{
    Token tk1, tk2, tk3, tk4;
    PTree::Node *args;

    if(lex->get_token(tk1) != Token::UserKeyword4)
	return false;

    int t = lex->get_token(tk2);
    if(t == ':'){
	mem = new PTree::UserAccessSpec(new PTree::Atom(tk1),
				      PTree::list(new PTree::Atom(tk2)));
	return true;
    }
    else if(t == '('){
	if(!rFunctionArguments(args))
	    return false;

	if(lex->get_token(tk3) != ')')
	    return false;

	if(lex->get_token(tk4) != ':')
	    return false;

	mem = new PTree::UserAccessSpec(new PTree::Atom(tk1),
				      PTree::list(new PTree::Atom(tk2), args,
						  new PTree::Atom(tk3),
						  new PTree::Atom(tk4)));
	return true;
    }
    else
	return false;
}

/*
  comma.expression
  : expression
  | comma.expression ',' expression	(left-to-right)
*/
bool Parser::rCommaExpression(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *right;

    if(!rExpression(exp))
	return false;

    while(lex->look_ahead(0) == ','){
	lex->get_token(tk);
	if(!rExpression(right))
	    return false;

	exp = new PTree::CommaExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  expression
  : conditional.expr {(AssignOp | '=') expression}	right-to-left
*/
bool Parser::rExpression(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *left, *right;

    if(!rConditionalExpr(left, false))
	return false;

    int t = lex->look_ahead(0);
    if(t != '=' && t != Token::AssignOp)
	exp = left;
    else{
	lex->get_token(tk);
	if(!rExpression(right))
	    return false;

	exp = new PTree::AssignExpr(left, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  conditional.expr
  : logical.or.expr {'?' comma.expression ':' conditional.expr}  right-to-left
*/
bool Parser::rConditionalExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk1, tk2;
    PTree::Node *then, *otherwise;

    if(!rLogicalOrExpr(exp, temp_args))
	return false;

    if(lex->look_ahead(0) == '?'){
	lex->get_token(tk1);
	if(!rCommaExpression(then))
	    return false;

	if(lex->get_token(tk2) != ':')
	    return false;

	if(!rConditionalExpr(otherwise, temp_args))
	    return false;

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
bool Parser::rLogicalOrExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk;
    PTree::Node *right;

    if(!rLogicalAndExpr(exp, temp_args))
	return false;

    while(lex->look_ahead(0) == Token::LogOrOp){
	lex->get_token(tk);
	if(!rLogicalAndExpr(right, temp_args))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  logical.and.expr
  : inclusive.or.expr
  | logical.and.expr LogAndOp inclusive.or.expr
*/
bool Parser::rLogicalAndExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk;
    PTree::Node *right;

    if(!rInclusiveOrExpr(exp, temp_args))
	return false;

    while(lex->look_ahead(0) == Token::LogAndOp){
	lex->get_token(tk);
	if(!rInclusiveOrExpr(right, temp_args))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  inclusive.or.expr
  : exclusive.or.expr
  | inclusive.or.expr '|' exclusive.or.expr
*/
bool Parser::rInclusiveOrExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk;
    PTree::Node *right;

    if(!rExclusiveOrExpr(exp, temp_args))
	return false;

    while(lex->look_ahead(0) == '|'){
	lex->get_token(tk);
	if(!rExclusiveOrExpr(right, temp_args))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  exclusive.or.expr
  : and.expr
  | exclusive.or.expr '^' and.expr
*/
bool Parser::rExclusiveOrExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk;
    PTree::Node *right;

    if(!rAndExpr(exp, temp_args))
	return false;

    while(lex->look_ahead(0) == '^'){
	lex->get_token(tk);
	if(!rAndExpr(right, temp_args))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  and.expr
  : equality.expr
  | and.expr '&' equality.expr
*/
bool Parser::rAndExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk;
    PTree::Node *right;

    if(!rEqualityExpr(exp, temp_args))
	return false;

    while(lex->look_ahead(0) == '&'){
	lex->get_token(tk);
	if(!rEqualityExpr(right, temp_args))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  equality.expr
  : relational.expr
  | equality.expr EqualOp relational.expr
*/
bool Parser::rEqualityExpr(PTree::Node *&exp, bool temp_args)
{
    Token tk;
    PTree::Node *right;

    if(!rRelationalExpr(exp, temp_args))
	return false;

    while(lex->look_ahead(0) == Token::EqualOp){
	lex->get_token(tk);
	if(!rRelationalExpr(right, temp_args))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  relational.expr
  : shift.expr
  | relational.expr (RelOp | '<' | '>') shift.expr
*/
bool Parser::rRelationalExpr(PTree::Node *&exp, bool temp_args)
{
    int t;
    Token tk;
    PTree::Node *right;

    if(!rShiftExpr(exp))
	return false;

    while(t = lex->look_ahead(0),
	  (t == Token::RelOp || t == '<' || (t == '>' && !temp_args))){
	lex->get_token(tk);
	if(!rShiftExpr(right))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  shift.expr
  : additive.expr
  | shift.expr ShiftOp additive.expr
*/
bool Parser::rShiftExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *right;

    if(!rAdditiveExpr(exp))
	return false;

    while(lex->look_ahead(0) == Token::ShiftOp){
	lex->get_token(tk);
	if(!rAdditiveExpr(right))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  additive.expr
  : multiply.expr
  | additive.expr ('+' | '-') multiply.expr
*/
bool Parser::rAdditiveExpr(PTree::Node *&exp)
{
    int t;
    Token tk;
    PTree::Node *right;

    if(!rMultiplyExpr(exp))
	return false;

    while(t = lex->look_ahead(0), (t == '+' || t == '-')){
	lex->get_token(tk);
	if(!rMultiplyExpr(right))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  multiply.expr
  : pm.expr
  | multiply.expr ('*' | '/' | '%') pm.expr
*/
bool Parser::rMultiplyExpr(PTree::Node *&exp)
{
    int t;
    Token tk;
    PTree::Node *right;

    if(!rPmExpr(exp))
	return false;

    while(t = lex->look_ahead(0), (t == '*' || t == '/' || t == '%')){
	lex->get_token(tk);
	if(!rPmExpr(right))
	    return false;

	exp = new PTree::InfixExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  pm.expr	(pointer to member .*, ->*)
  : cast.expr
  | pm.expr PmOp cast.expr
*/
bool Parser::rPmExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *right;

    if(!rCastExpr(exp))
	return false;

    while(lex->look_ahead(0) == Token::PmOp){
	lex->get_token(tk);
	if(!rCastExpr(right))
	    return false;

	exp = new PTree::PmExpr(exp, PTree::list(new PTree::Atom(tk), right));
    }

    return true;
}

/*
  cast.expr
  : unary.expr
  | '(' type.name ')' cast.expr
*/
bool Parser::rCastExpr(PTree::Node *&exp)
{
    if(lex->look_ahead(0) != '(')
	return rUnaryExpr(exp);
    else{
	Token tk1, tk2;
	PTree::Node *tname;
	const char* pos = lex->save();
	lex->get_token(tk1);
	if(rTypeName(tname))
	    if(lex->get_token(tk2) == ')')
		if(rCastExpr(exp)){
		    exp = new PTree::CastExpr(new PTree::Atom(tk1),
					    PTree::list(tname, new PTree::Atom(tk2),
							exp));
		    return true;
		}

	lex->restore(pos);
	return rUnaryExpr(exp);
    }
}

/*
  type.name
  : type.specifier cast.declarator
*/
bool Parser::rTypeName(PTree::Node *&tname)
{
    Encoding type_encode;
    return rTypeName(tname, type_encode);
}

bool Parser::rTypeName(PTree::Node *&tname, Encoding& type_encode)
{
    PTree::Node *type_name, *arg;
    Encoding name_encode;

    if(!rTypeSpecifier(type_name, true, type_encode))
	return false;

    if(!rDeclarator(arg, kCastDeclarator, false, type_encode, name_encode,
		    false))
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
bool Parser::rUnaryExpr(PTree::Node *&exp)
{
    int t = lex->look_ahead(0);
    if(t == '*' || t == '&' || t == '+' || t == '-' || t == '!'
       || t == '~' || t == Token::IncOp){
	Token tk;
	PTree::Node *right;

	lex->get_token(tk);
	if(!rCastExpr(right))
	    return false;

	exp = new PTree::UnaryExpr(new PTree::Atom(tk), PTree::list(right));
	return true;
    }
    else if(t == Token::SIZEOF)
	return rSizeofExpr(exp);
    else if(t == Token::THROW)
	return rThrowExpr(exp);
    else if(isAllocateExpr(t))
	return rAllocateExpr(exp);
    else
	return rPostfixExpr(exp);
}

/*
  throw.expression
  : THROW {expression}
*/
bool Parser::rThrowExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *e;

    if(lex->get_token(tk) != Token::THROW)
	return false;

    int t = lex->look_ahead(0);
    if(t == ':' || t == ';')
	e = 0;
    else
	if(!rExpression(e))
	    return false;

    exp = new PTree::ThrowExpr(new PTree::Reserved(tk), PTree::list(e));
    return true;
}

/*
  sizeof.expr
  : SIZEOF unary.expr
  | SIZEOF '(' type.name ')'
*/
bool Parser::rSizeofExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *unary;

    if(lex->get_token(tk) != Token::SIZEOF)
	return false;

    if(lex->look_ahead(0) == '('){
	PTree::Node *tname;
	Token op, cp;

	const char* pos = lex->save();
	lex->get_token(op);
	if(rTypeName(tname))
	    if(lex->get_token(cp) == ')'){
		exp = new PTree::SizeofExpr(new PTree::Atom(tk),
					  PTree::list(new PTree::Atom(op), tname,
						      new PTree::Atom(cp)));
		return true;
	    }

	lex->restore(pos);
    }

    if(!rUnaryExpr(unary))
	return false;

    exp = new PTree::SizeofExpr(new PTree::Atom(tk), PTree::list(unary));
    return true;
}

/*
  typeid.expr
  : TYPEID unary.expr
  | TYPEID '(' type.name ')'
*/
bool Parser::rTypeidExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *unary;

    if(lex->get_token(tk) != Token::TYPEID)
	return false;

    if(lex->look_ahead(0) == '('){
	PTree::Node *tname;
	Token op, cp;

	const char* pos = lex->save();
	lex->get_token(op);
	if(rTypeName(tname))
	    if(lex->get_token(cp) == ')'){
		exp = new PTree::TypeidExpr(new PTree::Atom(tk),
					  PTree::list(new PTree::Atom(op), tname,
						      new PTree::Atom(cp)));
		return true;
	    }

	lex->restore(pos);
    }

    if(!rUnaryExpr(unary))
	return false;

    exp = new PTree::TypeidExpr(new PTree::Atom(tk), PTree::list(unary));
    return true;
}

bool Parser::isAllocateExpr(int t)
{
    if(t == Token::UserKeyword)
	return true;
    else{
	if(t == Token::Scope)
	    t = lex->look_ahead(1);

	if(t == Token::NEW || t == Token::DELETE)
	    return true;
	else
	    return false;
    }
}

/*
  allocate.expr
  : {Scope | userdef.keyword} NEW allocate.type
  | {Scope} DELETE {'[' ']'} cast.expr
*/
bool Parser::rAllocateExpr(PTree::Node *&exp)
{
    Token tk;
    PTree::Node *head = 0;

    bool ukey = false;
    int t = lex->look_ahead(0);
    if(t == Token::Scope){
	lex->get_token(tk);
	head = new PTree::Atom(tk);
    }
    else if(t == Token::UserKeyword){
	if(!rUserdefKeyword(head))
	    return false;

	ukey = true;
    }

    t = lex->get_token(tk);
    if(t == Token::DELETE){
	PTree::Node *obj;
	if(ukey)
	    return false;

	if(head == 0)
	    exp = new PTree::DeleteExpr(new PTree::Reserved(tk), 0);
	else
	    exp = new PTree::DeleteExpr(head,
				      PTree::list(new PTree::Reserved(tk)));

	if(lex->look_ahead(0) == '['){
	    lex->get_token(tk);
	    exp = PTree::snoc(exp, new PTree::Atom(tk));
	    if(lex->get_token(tk) != ']')
		return false;

	    exp = PTree::snoc(exp, new PTree::Atom(tk));
	}

	if(!rCastExpr(obj))
	    return false;

	exp = PTree::snoc(exp, obj);
	return true;
    }
    else if(t == Token::NEW){
	PTree::Node *atype;
	if(head == 0)
	    exp = new PTree::NewExpr(new PTree::Reserved(tk), 0);
	else
	    exp = new PTree::NewExpr(head, PTree::list(new PTree::Reserved(tk)));

	if(!rAllocateType(atype))
	    return false;

	exp = PTree::nconc(exp, atype);
	return true;
    }
    else
	return false;
}

/*
  userdef.keyword
  : [UserKeyword | UserKeyword5] {'(' function.arguments ')'}
*/
bool Parser::rUserdefKeyword(PTree::Node *&ukey)
{
    Token tk;

    int t = lex->get_token(tk);
    if(t != Token::UserKeyword && t != Token::UserKeyword5)
	return false;

    if(lex->look_ahead(0) != '(')
	ukey = new PTree::UserdefKeyword(new PTree::Atom(tk), 0);
    else{
	PTree::Node *args;
	Token op, cp;
	lex->get_token(op);
	if(!rFunctionArguments(args))
	    return false;

	if(lex->get_token(cp) != ')')
	    return false;

	ukey = new PTree::UserdefKeyword(new PTree::Atom(tk),
			PTree::list(new PTree::Atom(op), args, new PTree::Atom(cp)));
    }

    return true;
}

/*
  allocate.type
  : {'(' function.arguments ')'} type.specifier new.declarator
    {allocate.initializer}
  | {'(' function.arguments ')'} '(' type.name ')' {allocate.initializer}
*/
bool Parser::rAllocateType(PTree::Node *&atype)
{
    Token op, cp;
    PTree::Node *tname, *init, *exp;

    if(lex->look_ahead(0) != '(')
	atype = PTree::list(0);
    else{
	lex->get_token(op);

	const char* pos = lex->save();
	if(rTypeName(tname))
	    if(lex->get_token(cp) == ')')
		if(lex->look_ahead(0) != '('){
		    atype = PTree::list(0, PTree::list(new PTree::Atom(op), tname,
							 new PTree::Atom(cp)));
		    if(!isTypeSpecifier())
			return true;
		}
		else if(rAllocateInitializer(init)){
		    atype = PTree::list(0,
					PTree::list(new PTree::Atom(op), tname,
						    new PTree::Atom(cp)),
					init);
		    // the next token cannot be '('
		    if(lex->look_ahead(0) != '(')
			return true;
		}

	// if we reach here, we have to process '(' function.arguments ')'.
	lex->restore(pos);
	if(!rFunctionArguments(exp))
	    return false;

	if(lex->get_token(cp) != ')')
	    return false;

	atype = PTree::list(PTree::list(new PTree::Atom(op), exp, new PTree::Atom(cp)));
    }

    if(lex->look_ahead(0) == '('){
	lex->get_token(op);
	if(!rTypeName(tname))
	    return false;

	if(lex->get_token(cp) != ')')
	    return false;

	atype = PTree::snoc(atype, PTree::list(new PTree::Atom(op), tname,
					       new PTree::Atom(cp)));
    }
    else{
	PTree::Node *decl;
	Encoding type_encode;
	if(!rTypeSpecifier(tname, false, type_encode))
	    return false;

	if(!rNewDeclarator(decl, type_encode))
	    return false;

	atype = PTree::snoc(atype, PTree::list(tname, decl));
    }

    if(lex->look_ahead(0) == '('){
	if(!rAllocateInitializer(init))
	    return false;

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
bool Parser::rNewDeclarator(PTree::Node *&decl, Encoding& encode)
{
    decl = 0;
    if(lex->look_ahead(0) != '[')
	if(!optPtrOperator(decl, encode))
	    return false;

    while(lex->look_ahead(0) == '['){
	Token ob, cb;
	PTree::Node *exp;
	lex->get_token(ob);
	if(!rCommaExpression(exp))
	    return false;

	if(lex->get_token(cb) != ']')
	    return false;

	encode.Array();
	decl = PTree::nconc(decl, PTree::list(new PTree::Atom(ob), exp,
					      new PTree::Atom(cb)));
    }

    if(decl == 0)
	decl = new PTree::Declarator(encode);
    else
	decl = new PTree::Declarator(decl, encode);

    return true;
}

/*
  allocate.initializer
  : '(' {initialize.expr (',' initialize.expr)* } ')'
*/
bool Parser::rAllocateInitializer(PTree::Node *&init)
{
    Token op, cp;

    if(lex->get_token(op) != '(')
	return false;

    if(lex->look_ahead(0) == ')'){
	lex->get_token(cp);
	init = PTree::list(new PTree::Atom(op), 0, new PTree::Atom(cp));
	return true;
    }

    init = 0;
    for(;;){
	PTree::Node *exp;
	if(!rInitializeExpr(exp))
	    return false;

	init = PTree::snoc(init, exp);
	if(lex->look_ahead(0) != ',')
	    break;
	else{
	    Token tk;
	    lex->get_token(tk);
	    init = PTree::snoc(init, new PTree::Atom(tk));
	}
    }

    lex->get_token(cp);
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
bool Parser::rPostfixExpr(PTree::Node *&exp)
{
    PTree::Node *e;
    Token cp, op;
    int t, t2;

    if(!rPrimaryExpr(exp))
	return false;

    for(;;){
	switch(lex->look_ahead(0)){
	case '[' :
	    lex->get_token(op);
	    if(!rCommaExpression(e))
		return false;

	    if(lex->get_token(cp) != ']')
		return false;

	    exp = new PTree::ArrayExpr(exp, PTree::list(new PTree::Atom(op),
						      e, new PTree::Atom(cp)));
	    break;
	case '(' :
	    lex->get_token(op);
	    if(!rFunctionArguments(e))
		return false;

	    if(lex->get_token(cp) != ')')
		return false;

	    exp = new PTree::FuncallExpr(exp, PTree::list(new PTree::Atom(op),
							e, new PTree::Atom(cp)));
	    break;
	case Token::IncOp :
	    lex->get_token(op);
	    exp = new PTree::PostfixExpr(exp, PTree::list(new PTree::Atom(op)));
	    break;
	case '.' :
	case Token::ArrowOp :
	    t2 = lex->get_token(op);
	    t = lex->look_ahead(0);
	    if(t == Token::UserKeyword || t == Token::UserKeyword2 || t == Token::UserKeyword3){
		if(!rUserdefStatement(e))
		    return false;

		exp = new PTree::UserStatementExpr(exp,
						 PTree::cons(new PTree::Atom(op), e));
		break;
	    }
	    else{
		if(!rVarName(e))
		    return false;

		if(t2 == '.')
		    exp = new PTree::DotMemberExpr(exp,
						 PTree::list(new PTree::Atom(op), e));
		else
		    exp = new PTree::ArrowMemberExpr(exp,
						PTree::list(new PTree::Atom(op), e));
		break;
	    }
	default :
	    return true;
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
bool Parser::rPrimaryExpr(PTree::Node *&exp)
{
    Token tk, tk2;
    PTree::Node *exp2;
    Encoding cast_type_encode;

    switch(lex->look_ahead(0)){
    case Token::Constant :
    case Token::CharConst :
    case Token::WideCharConst :
    case Token::StringL :
    case Token::WideStringL :
	lex->get_token(tk);
	exp = new PTree::Atom(tk);
	return true;
    case Token::THIS :
	lex->get_token(tk);
	exp = new PTree::This(tk);
	return true;
    case Token::TYPEID :
	return rTypeidExpr(exp);
    case '(' :
	lex->get_token(tk);
	if(!rCommaExpression(exp2))
	    return false;

	if(lex->get_token(tk2) != ')')
	    return false;

	exp = new PTree::ParenExpr(new PTree::Atom(tk),
				 PTree::list(exp2, new PTree::Atom(tk2)));
	return true;
    default :
	if(!optIntegralTypeOrClassSpec(exp, cast_type_encode))
	    return false;

	if(exp != 0){		// if integral.or.class.spec
	    if(lex->get_token(tk) != '(')
		return false;

	    if(!rFunctionArguments(exp2))
		return false;

	    if(lex->get_token(tk2) != ')')
		return false;

	    exp = new PTree::FstyleCastExpr(cast_type_encode, exp,
					  PTree::list(new PTree::Atom(tk), exp2,
						      new PTree::Atom(tk2)));
	    return true;
	}
	else{
	    if(!rVarName(exp))
		return false;

	    if(lex->look_ahead(0) == Token::Scope){
		lex->get_token(tk);
		if(!rUserdefStatement(exp2))
		    return false;

		exp = new PTree::StaticUserStatementExpr(exp,
					PTree::cons(new PTree::Atom(tk), exp2));
	    }

	    return true;
	}
    }
}

/*
  userdef.statement
  : UserKeyword '(' function.arguments ')' compound.statement
  | UserKeyword2 '(' arg.decl.list ')' compound.statement
  | UserKeyword3 '(' expr.statement {comma.expression} ';'
			{comma.expression} ')' compound.statement
*/
bool Parser::rUserdefStatement(PTree::Node *&st)
{
    Token tk, tk2, tk3, tk4;
    PTree::Node *keyword, *exp, *body, *exp2, *exp3;
    Encoding dummy_encode;

    int t = lex->get_token(tk);
    if(lex->get_token(tk2) != '(')
	return false;

    switch(t){
    case Token::UserKeyword :
        keyword = new PTree::Reserved(tk);
	if(!rFunctionArguments(exp))
	    return false;
	goto rest;
    case Token::UserKeyword2 :
	keyword = new PTree::AtomUserKeyword2(tk);
	if(!rArgDeclList(exp, dummy_encode))
	    return false;
    rest:
	if(lex->get_token(tk3) != ')')
	    return false;

	if(!rCompoundStatement(body))
	    return false;

	st = PTree::list(keyword, new PTree::Atom(tk2), exp, new PTree::Atom(tk3),
			 body);
	return true;
    case Token::UserKeyword3 :
	if(!rExprStatement(exp))
	    return false;

	if(lex->look_ahead(0) == ';')
	    exp2 = 0;
	else
	    if(!rCommaExpression(exp2))
		return false;

	if(lex->get_token(tk3) != ';')
	    return false;

	if(lex->look_ahead(0) == ')')
	    exp3 = 0;
	else
	    if(!rCommaExpression(exp3))
		return false;

	if(lex->get_token(tk4) != ')')
	    return false;

	if(!rCompoundStatement(body))
	    return false;

	st = PTree::list(new PTree::Atom(tk), new PTree::Atom(tk2), exp, exp2,
			 new PTree::Atom(tk3), exp3, new PTree::Atom(tk4), body);
	return true;
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
bool Parser::rVarName(PTree::Node *&name)
{
    Encoding encode;

    if(rVarNameCore(name, encode)){
	if(!name->is_atom())
	    name = new PTree::Name(name, encode);

	return true;
    }
    else
	return false;
}

bool Parser::rVarNameCore(PTree::Node *&name, Encoding& encode)
{
    Token tk;
    int length = 0;

    if(lex->look_ahead(0) == Token::Scope){
	lex->get_token(tk);
	name = PTree::list(new PTree::Atom(tk));
	encode.GlobalScope();
	++length;
    }
    else
	name = 0;

    for(;;){
	int t = lex->get_token(tk);
	if(t == Token::TEMPLATE) {
	    // Skip template token, next will be identifier
	    t = lex->get_token(tk);
	}
	if(t == Token::Identifier){
	    PTree::Node *n = new PTree::Identifier(tk);
	    if(isTemplateArgs()){
		PTree::Node *args;
		Encoding args_encode;
		if(!rTemplateArgs(args, args_encode))
		    return false;

		encode.Template(n, args_encode);
		++length;
		n = PTree::list(n, args);
	    }
	    else{
		encode.SimpleName(n);
		++length;
	    }

	    if(moreVarName()){
		lex->get_token(tk);
		name = PTree::nconc(name, PTree::list(n, new PTree::Atom(tk)));
	    }
	    else{
		if(name == 0)
		    name = n;
		else
		    name = PTree::snoc(name, n);

		if(length > 1)
		    encode.Qualified(length);

		return true;
	    }
	}
	else if(t == '~'){
	    Token tk2;
	    if(lex->look_ahead(0) != Token::Identifier)
		return false;

	    lex->get_token(tk2);
	    PTree::Node *class_name = new PTree::Atom(tk2);
	    PTree::Node *dt = PTree::list(new PTree::Atom(tk), class_name);
	    if(name == 0)
		name = dt;
	    else
		name = PTree::snoc(name, dt);

	    encode.Destructor(class_name);
	    if(length > 0)
		encode.Qualified(length + 1);

	    return true;
	}
	else if(t == Token::OPERATOR){
	    PTree::Node *op;
	    if(!rOperatorName(op, encode))
		return false;

	    PTree::Node *opf = PTree::list(new PTree::Reserved(tk), op);
	    if(name == 0)
		name = opf;
	    else
		name = PTree::snoc(name, opf);

	    if(length > 0)
		encode.Qualified(length + 1);

	    return true;
	}
	else
	    return false;
    }
}

bool Parser::moreVarName()
{
    if(lex->look_ahead(0) == Token::Scope){
	int t = lex->look_ahead(1);
	if(t == Token::Identifier || t == '~' || t == Token::OPERATOR || t == Token::TEMPLATE)
	    return true;
    }

    return false;
}

/*
  template.args : '<' any* '>'

  template.args must be followed by '(', '::', ';', or ','
*/
bool Parser::isTemplateArgs()
{
    int i = 0;
    int t = lex->look_ahead(i++);
    if(t == '<'){
	int n = 1;
	while(n > 0){
	    int u = lex->look_ahead(i++);
	    if(u == '<')
		++n;
	    else if(u == '>')
		--n;
	    else if(u == '('){
		int m = 1;
		while(m > 0){
		    int v = lex->look_ahead(i++);
		    if(v == '(')
			++m;
		    else if(v == ')')
			--m;
		    else if(v == '\0' || v == ';' || v == '}')
			return false;
		}
	    }
	    else if(u == '\0' || u == ';' || u == '}')
		return false;
	}

	t = lex->look_ahead(i);
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
bool Parser::rCondition(PTree::Node *&exp)
{
    Encoding type_encode;

    // Do declarator first, otherwise "T*foo = blah" gets matched as a
    // multiplication infix expression inside an assignment expression!
    const char *save = lex->save();
    do {
	PTree::Node *storage_s, *cv_q, *cv_q2, *integral, *head, *decl;

	if (!optStorageSpec(storage_s))
	    break;

	if (storage_s == 0)
	    head = 0;
	else
	    head = storage_s;
	
	if (!optCvQualify(cv_q)
		|| !optIntegralTypeOrClassSpec(integral, type_encode))
	    break;

	if (integral != 0) {
	    // Integral Declaration
	    // Find const after type
	    if (!optCvQualify(cv_q2))
		break;
	    // Make type ptree with pre and post const ptrees
	    if (cv_q != 0)
		if (cv_q2 == 0)
		    integral = PTree::snoc(cv_q, integral);
		else
		    integral = PTree::nconc(cv_q,
			    PTree::cons(integral, cv_q2));
	    else if (cv_q2 != 0)
		integral = PTree::cons(integral, cv_q2);
	    // Store type of CV's
	    type_encode.CvQualify(cv_q, cv_q2);
	    // Parse declarator
	    if (!rDeclaratorWithInit(decl, type_encode, true, false))
		break;
	    // *must* be end of condition, condition is in a () pair
	    if (lex->look_ahead(0) != ')')
		break;
	    exp = new PTree::Declaration(head,
		    PTree::list(integral, decl));
	} else {
	    // Other Declaration
	    PTree::Node *type_name;
	    // Find name of type
	    if (!rName(type_name, type_encode))
		break;
	    // Find const after type
	    if (!optCvQualify(cv_q2))
		break;
	    // Make type ptree with pre and post const ptrees
	    if (cv_q != 0)
		if (cv_q2 == 0)
		    type_name = PTree::snoc(cv_q, type_name);
		else
		    type_name = PTree::nconc(cv_q,
			    PTree::cons(type_name, cv_q2));
	    else if (cv_q2 != 0)
		type_name = PTree::cons(type_name, cv_q2);
	    // Store type of CV's
	    type_encode.CvQualify(cv_q, cv_q2);
	    // Parse declarator
	    if (!rDeclaratorWithInit(decl, type_encode, true, false))
		break;
	    // *must* be end of condition, condition is in a () pair
	    if (lex->look_ahead(0) != ')')
		break;
	    exp = new PTree::Declaration(head,
		    PTree::list(type_name, decl));
	}
	return true;
    } while(false);

    // Must be a comma expression
    lex->restore(save);
    return rCommaExpression(exp);
}

/*
  function.body  : compound.statement
*/
bool Parser::rFunctionBody(PTree::Node *&body)
{
    return rCompoundStatement(body);
}

/*
  compound.statement
  : '{' (statement)* '}'
*/
bool Parser::rCompoundStatement(PTree::Node *&body)
{
    Token ob, cb;
    PTree::Node *ob_comments, *cb_comments;

    if(lex->get_token(ob) != '{')
	return false;

    ob_comments = wrap_comments(lex->get_comments());
    PTree::Node *sts = 0;
    while(lex->look_ahead(0) != '}'){
	PTree::Node *st;
	if(!rStatement(st)){
	    if(!SyntaxError())
		return false;	// too many errors

	    SkipTo('}');
	    lex->get_token(cb);
	    body = PTree::list(new PTree::Atom(ob), 0, new PTree::Atom(cb));
	    return true;	// error recovery
	}

	sts = PTree::snoc(sts, st);
    }

    if(lex->get_token(cb) != '}')
	return false;

    cb_comments = wrap_comments(lex->get_comments());
    body = new PTree::Block(new PTree::CommentedAtom(ob, ob_comments), sts, 
	new PTree::CommentedAtom(cb, cb_comments));
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
bool Parser::rStatement(PTree::Node *&st)
{
    Token tk1, tk2, tk3;
    PTree::Node *st2, *exp, *comments;
    int k;

    // Get the comments - if we dont make it past the switch then it is a
    // parse error anyway!
    comments = wrap_comments(lex->get_comments());

    // Whichever case we get, it must succeed
    switch(k = lex->look_ahead(0)){
    case '{' :
	if (!rCompoundStatement(st)) return false;
	break;
    case Token::USING :
	if (!rUsing(st)) return false;
	break;
    case Token::TYPEDEF :
	if (!rTypedef(st)) return false;
	break;
    case Token::IF :
	if (!rIfStatement(st)) return false;
	break;
    case Token::SWITCH :
	if (!rSwitchStatement(st)) return false;
	break;
    case Token::WHILE :
	if (!rWhileStatement(st)) return false;
	break;
    case Token::DO :
	if (!rDoStatement(st)) return false;
	break;
    case Token::FOR :
	if (!rForStatement(st)) return false;
	break;
    case Token::TRY :
	if (!rTryStatement(st)) return false;
	break;
    case Token::BREAK :
    case Token::CONTINUE :
	lex->get_token(tk1);
	if(lex->get_token(tk2) != ';')
	    return false;

	if(k == Token::BREAK)
	    st = new PTree::BreakStatement(new PTree::Reserved(tk1),
					 PTree::list(new PTree::Atom(tk2)));
	else
            st = new PTree::ContinueStatement(new PTree::Reserved(tk1),
					    PTree::list(new PTree::Atom(tk2)));
	break;
    case Token::RETURN :
	lex->get_token(tk1);
	if(lex->look_ahead(0) == ';'){
	    lex->get_token(tk2);
	    st = new PTree::ReturnStatement(new PTree::Reserved(tk1),
					  PTree::list(new PTree::Atom(tk2)));
	} else {
	    if(!rCommaExpression(exp))
		return false;

	    if(lex->get_token(tk2) != ';')
		return false;

	    st = new PTree::ReturnStatement(new PTree::Reserved(tk1),
					  PTree::list(exp, new PTree::Atom(tk2)));
	}
	break;
    case Token::GOTO :
	lex->get_token(tk1);
	if(lex->get_token(tk2) != Token::Identifier)
	    return false;

	if(lex->get_token(tk3) != ';')
	    return false;

	st = new PTree::GotoStatement(new PTree::Reserved(tk1),
				    PTree::list(new PTree::Atom(tk2), new PTree::Atom(tk3)));
	break;
    case Token::CASE :
	lex->get_token(tk1);
	if(!rExpression(exp))
	    return false;
	if(lex->get_token(tk2) != ':')
	    return false;

	if(!rStatement(st2))
	    return false;

	st = new PTree::CaseStatement(new PTree::Reserved(tk1),
				    PTree::list(exp, new PTree::Atom(tk2), st2));
	break;
    case Token::DEFAULT :
	lex->get_token(tk1);
	if(lex->get_token(tk2) != ':')
	    return false;

	if(!rStatement(st2))
	    return false;

	st = new PTree::DefaultStatement(new PTree::Reserved(tk1),
				       PTree::list(new PTree::Atom(tk2), st2));
	break;
    case Token::Identifier :
	if(lex->look_ahead(1) == ':'){	// label statement
	    lex->get_token(tk1);
	    lex->get_token(tk2);
	    if(!rStatement(st2))
		return false;

	    st = new PTree::LabelStatement(new PTree::Atom(tk1),
					 PTree::list(new PTree::Atom(tk2), st2));
	    return true;
	}
	// don't break here!
    default :
	if (!rExprStatement(st)) return false;
    }

    // No parse error, attach comment to whatever was returned
    Walker::SetLeafComments(st, comments);
    return true;
}

/*
  if.statement
  : IF '(' declaration.statement ')' statement { ELSE statement }
  : IF '(' comma.expression ')' statement { ELSE statement }
*/
bool Parser::rIfStatement(PTree::Node *&st)
{
    Token tk1, tk2, tk3, tk4;
    PTree::Node *exp, *then, *otherwise;

    if(lex->get_token(tk1) != Token::IF)
	return false;

    if(lex->get_token(tk2) != '(')
	return false;

    if(!rCondition(exp))
	return false;

    if(lex->get_token(tk3) != ')')
	return false;

    if(!rStatement(then))
	return false;

    st = new PTree::IfStatement(new PTree::Reserved(tk1),
			      PTree::list(new PTree::Atom(tk2), exp, new PTree::Atom(tk3),
					  then));
    if(lex->look_ahead(0) == Token::ELSE){
	lex->get_token(tk4);
	if(!rStatement(otherwise))
	    return false;

	st = PTree::nconc(st, PTree::list(new PTree::Atom(tk4), otherwise));
    }

    return true;
}

/*
  switch.statement
  : SWITCH '(' comma.expression ')' statement
*/
bool Parser::rSwitchStatement(PTree::Node *&st)
{
    Token tk1, tk2, tk3;
    PTree::Node *exp, *body;

    if(lex->get_token(tk1) != Token::SWITCH)
	return false;

    if(lex->get_token(tk2) != '(')
	return false;

    if(!rCondition(exp))
	return false;

    if(lex->get_token(tk3) != ')')
	return false;

    if(!rStatement(body))
	return false;

    st = new PTree::SwitchStatement(new PTree::Reserved(tk1),
				  PTree::list(new PTree::Atom(tk2), exp,
					      new PTree::Atom(tk3), body));
    return true;
}

/*
  while.statement
  : WHILE '(' comma.expression ')' statement
*/
bool Parser::rWhileStatement(PTree::Node *&st)
{
    Token tk1, tk2, tk3;
    PTree::Node *exp, *body;

    if(lex->get_token(tk1) != Token::WHILE)
	return false;

    if(lex->get_token(tk2) != '(')
	return false;

    if(!rCommaExpression(exp))
	return false;

    if(lex->get_token(tk3) != ')')
	return false;

    if(!rStatement(body))
	return false;

    st = new PTree::WhileStatement(new PTree::Reserved(tk1),
				 PTree::list(new PTree::Atom(tk2), exp,
					      new PTree::Atom(tk3), body));
    return true;
}

/*
  do.statement
  : DO statement WHILE '(' comma.expression ')' ';'
*/
bool Parser::rDoStatement(PTree::Node *&st)
{
    Token tk0, tk1, tk2, tk3, tk4;
    PTree::Node *exp, *body;

    if(lex->get_token(tk0) != Token::DO)
	return false;

    if(!rStatement(body))
	return false;

    if(lex->get_token(tk1) != Token::WHILE)
	return false;

    if(lex->get_token(tk2) != '(')
	return false;

    if(!rCommaExpression(exp))
	return false;

    if(lex->get_token(tk3) != ')')
	return false;

    if(lex->get_token(tk4) != ';')
	return false;

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
bool Parser::rForStatement(PTree::Node *&st)
{
    Token tk1, tk2, tk3, tk4;
    PTree::Node *exp1, *exp2, *exp3, *body;

    if(lex->get_token(tk1) != Token::FOR)
	return false;

    if(lex->get_token(tk2) != '(')
	return false;

    if(!rExprStatement(exp1))
	return false;

    if(lex->look_ahead(0) == ';')
	exp2 = 0;
    else
	if(!rCommaExpression(exp2))
	    return false;

    if(lex->get_token(tk3) != ';')
	return false;

    if(lex->look_ahead(0) == ')')
	exp3 = 0;
    else
	if(!rCommaExpression(exp3))
	    return false;

    if(lex->get_token(tk4) != ')')
	return false;

    if(!rStatement(body))
	return false;


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
bool Parser::rTryStatement(PTree::Node *&st)
{
    Token tk, op, cp;
    PTree::Node *body, *handler;

    if(lex->get_token(tk) != Token::TRY)
	return false;

    if(!rCompoundStatement(body))
	return false;

    st = new PTree::TryStatement(new PTree::Reserved(tk), PTree::list(body));

    do{
	if(lex->get_token(tk) != Token::CATCH)
	    return false;

	if(lex->get_token(op) != '(')
	    return false;

	if(lex->look_ahead(0) == Token::Ellipsis){
	    lex->get_token(cp);
	    handler = new PTree::Atom(cp);
	}
	else{
	    Encoding encode;
	    if(!rArgDeclaration(handler, encode))
		return false;
	}

	if(lex->get_token(cp) != ')')
	    return false;

	if(!rCompoundStatement(body))
	    return false;

	st = PTree::snoc(st, PTree::list(new PTree::Reserved(tk),
					 new PTree::Atom(op), handler, new PTree::Atom(cp),
					 body));
    } while(lex->look_ahead(0) == Token::CATCH);
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
bool Parser::rExprStatement(PTree::Node *&st)
{
    Token tk;

    if(lex->look_ahead(0) == ';'){
	lex->get_token(tk);
	st = new PTree::ExprStatement(0, PTree::list(new PTree::Atom(tk)));
	return true;
    }
    else{
        const char* pos = lex->save();
	if(rDeclarationStatement(st))
	    return true;
	else{
	    PTree::Node *exp;
	    lex->restore(pos);
	    if(!rCommaExpression(exp))
		return false;

	    if(exp->IsA(Token::ntUserStatementExpr, Token::ntStaticUserStatementExpr)){
		st = exp;
		return true;
	    }

	    if(lex->get_token(tk) != ';')
		return false;

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
bool Parser::rDeclarationStatement(PTree::Node *&statement)
{
    PTree::Node *storage_s, *cv_q, *integral;
    Encoding type_encode;

    if(!optStorageSpec(storage_s)
       || !optCvQualify(cv_q)
       || !optIntegralTypeOrClassSpec(integral, type_encode))
	return false;

    PTree::Node *head = 0;
    if(storage_s != 0)
	head = PTree::snoc(head, storage_s);

    if(integral != 0)
	return rIntegralDeclStatement(statement, type_encode, integral,
				      cv_q, head);
    else{
	type_encode.Clear();
	int t = lex->look_ahead(0);
	if(cv_q != 0 && ((t == Token::Identifier && lex->look_ahead(1) == '=')
			   || t == '*'))
	    return rConstDeclaration(statement, type_encode, head, cv_q);
	else
	    return rOtherDeclStatement(statement, type_encode, cv_q, head);
    }
}

/*
  integral.decl.statement
  : decl.head integral.or.class.spec {cv.qualify} {declarators} ';'
*/
bool Parser::rIntegralDeclStatement(PTree::Node *&statement, Encoding& type_encode,
				    PTree::Node *integral, PTree::Node *cv_q, PTree::Node *head)
{
    PTree::Node *cv_q2, *decl;
    Token tk;

    if(!optCvQualify(cv_q2))
	return false;

    if(cv_q != 0)
	if(cv_q2 == 0)
	    integral = PTree::snoc(cv_q, integral);
	else
	    integral = PTree::nconc(cv_q, PTree::cons(integral, cv_q2));
    else if(cv_q2 != 0)
	integral = PTree::cons(integral, cv_q2);

    type_encode.CvQualify(cv_q, cv_q2);
    if(lex->look_ahead(0) == ';'){
	lex->get_token(tk);
	statement = new PTree::Declaration(head, PTree::list(integral,
									new PTree::Atom(tk)));
	return true;
    }
    else{
	if(!rDeclarators(decl, type_encode, false, true))
	    return false;

	if(lex->get_token(tk) != ';')
	    return false;
	    
	statement = new PTree::Declaration(head, PTree::list(integral, decl,
									new PTree::Atom(tk)));
	return true;
    }
}

/*
   other.decl.statement
   :decl.head name {cv.qualify} declarators ';'
*/
bool Parser::rOtherDeclStatement(PTree::Node *&statement, Encoding& type_encode,
				 PTree::Node *cv_q, PTree::Node *head)
{
    PTree::Node *type_name, *cv_q2, *decl;
    Token tk;

    if(!rName(type_name, type_encode))
	return false;

    if(!optCvQualify(cv_q2))
	return false;

    if(cv_q != 0)
	if(cv_q2 == 0)
	    type_name = PTree::snoc(cv_q, type_name);
	else
	    type_name = PTree::nconc(cv_q, PTree::cons(type_name, cv_q2));
    else if(cv_q2 != 0)
	type_name = PTree::cons(type_name, cv_q2);

    type_encode.CvQualify(cv_q, cv_q2);
    if(!rDeclarators(decl, type_encode, false, true))
	return false;

    if(lex->get_token(tk) != ';')
	return false;

    statement = new PTree::Declaration(head, PTree::list(type_name, decl,
						       new PTree::Atom(tk)));
    return true;
}

bool Parser::MaybeTypeNameOrClassTemplate(Token&)
{
    return true;
}

void Parser::SkipTo(int token)
{
    Token tk;

    for(;;){
	int t = lex->look_ahead(0);
	if(t == token || t == '\0')
	    break;
	else
	    lex->get_token(tk);
    }
}

#ifdef TEST

#include "buffer.h"
#include "walker.h"

main()
{
    ProgramFromStdin	prog;
    Lex			lex(&prog);
    Parser		parse(&lex);
    Walker		w(&parse);
    PTree::Node *		def;

    while(parse.rProgram(def)){
	def->print(std::cout);
	w.Translate(def);
    }

    std::cerr << parse.NumOfErrors() << " errors\n";
}
#endif
