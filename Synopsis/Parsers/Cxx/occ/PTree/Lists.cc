//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <cstring>
#include <iostream>
#include "Lexer.hh"
#include "PTree/Lists.hh"
#include "PTree/operations.hh"
#include "Encoding.hh"
#include "Walker.hh"

using namespace PTree;

void Brace::print(std::ostream &os, size_t indent, size_t depth) const
{
  if(too_deep(os, depth)) return;

  size_t indent2 = indent + 1;
  os << "[{";
  const Node *body = second(this);
  if(!body)
  {
    print_indent(os, indent2);
    os << "nil";
  }
  else
    while(body)
    {
      print_indent(os, indent2);
      if(body->is_atom())
      {
	os << "@ ";
	body->print(os, indent + 1, depth + 1);
      }
      else
      {
	const Node *head = body->car();
	if(!head) os << "nil";
	else head->print(os, indent + 1, depth + 1);
      }
      body = body->cdr();
    }
  print_indent(os, indent);
  os << "}]";
}

int Brace::Write(std::ostream& out, int indent)
{
    int n = 0;

    out << '{';
    Node *p = cadr(this);
    while(p != 0){
	if(p->is_atom())
	  throw std::runtime_error("Brace::Write(): non list");
	else{
	    print_indent(out, indent + 1);
	    ++n;
	    Node *q = p->car();
	    p = p->cdr();
	    if(q != 0)
		n += q->Write(out, indent + 1);
	}
    }

    print_indent(out, indent);
    ++n;
    out << '}';
    return n;
}

Node *Brace::Translate(Walker* w)
{
  return w->TranslateBrace(this);
}

Node *Block::Translate(Walker* w)
{
  return w->TranslateBlock(this);
}

Node *ClassBody::Translate(Walker* w)
{
  return w->TranslateClassBody(this, 0, 0);
}

int Typedef::What()
{
  return Token::ntTypedef;
}

Node *Typedef::Translate(Walker* w)
{
  return w->TranslateTypedef(this);
}

int TemplateDecl::What()
{
  return Token::ntTemplateDecl;
}

Node *TemplateDecl::Translate(Walker* w)
{
  return w->TranslateTemplateDecl(this);
}

int TemplateInstantiation::What()
{
  return Token::ntTemplateInstantiation;
}

Node *TemplateInstantiation::Translate(Walker* w)
{
  return w->TranslateTemplateInstantiation(this);
}

int ExternTemplate::What()
{
  return Token::ntExternTemplate;
}

Node *ExternTemplate::Translate(Walker* w)
{
  return w->TranslateExternTemplate(this);
}

int MetaclassDecl::What()
{
  return Token::ntMetaclassDecl;
}

Node *MetaclassDecl::Translate(Walker* w)
{
    return w->TranslateMetaclassDecl(this);
}

int LinkageSpec::What()
{
  return Token::ntLinkageSpec;
}

Node *LinkageSpec::Translate(Walker* w)
{
  return w->TranslateLinkageSpec(this);
}

int NamespaceSpec::What()
{
  return Token::ntNamespaceSpec;
}

Node *NamespaceSpec::Translate(Walker* w)
{
  return w->TranslateNamespaceSpec(this);
}

int NamespaceAlias::What()
{
  return Token::ntNamespaceAlias;
}

Node *NamespaceAlias::Translate(Walker* w)
{
  return w->TranslateNamespaceAlias(this);
}

int Using::What()
{
  return Token::ntUsing;
}

Node *Using::Translate(Walker* w)
{
  return w->TranslateUsing(this);
}

int Declaration::What()
{
  return Token::ntDeclaration;
}

Node *Declaration::Translate(Walker* w)
{
  return w->TranslateDeclaration(this);
}

Declarator::Declarator(Node *list, Encoding &t, Encoding &n, Node *dname)
  : List(list->car(), list->cdr()),
    my_type(t.Get()),
    my_name(n.Get()),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Encoding &t, Encoding &n, Node *dname)
  : List(0, 0),
    my_type(t.Get()),
    my_name(n.Get()),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Node *p, Node *q, Encoding &t, Encoding &n, Node *dname)
  : List(p, q),
    my_type(t.Get()),
    my_name(n.Get()),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Node *list, Encoding &t)
  : List(list->car(), list->cdr()),
    my_type(t.Get()),
    my_name(0),
    my_declared_name(0),
    my_comments(0)
{
}

Declarator::Declarator(Encoding &t)
  : List(0, 0),
    my_type(t.Get()),
    my_name(0),
    my_declared_name(0),
    my_comments(0)
{
}

Declarator::Declarator(Declarator *decl, Node *p, Node *q)
  : List(p, q),
    my_type(decl->my_type),
    my_name(decl->my_name),
    my_declared_name(decl->my_declared_name),
    my_comments(0)
{
}

void Declarator::print(std::ostream &os, size_t indent, size_t depth) const
{
  print_encoded(os, indent, depth);
}

int Declarator::What()
{
  return Token::ntDeclarator;
}

const char *Declarator::encoded_type() const
{
  return my_type;
}

const char *Declarator::encoded_name() const
{
  return my_name;
}

Name::Name(Node *p, Encoding &e)
  : List(p->car(), p->cdr()),
    my_name(e.Get())
{
}

void Name::print(std::ostream &os, size_t indent, size_t depth) const
{
  print_encoded(os, indent, depth);
}

int Name::What()
{
  return Token::ntName;
}

const char *Name::encoded_name() const
{
  return my_name;
}

Node *Name::Translate(Walker *w)
{
  return w->TranslateVariable(this);
}

void Name::Typeof(Walker *w, TypeInfo &t)
{
  w->TypeofVariable(this, t);
}

FstyleCastExpr::FstyleCastExpr(Encoding &e, Node *p, Node *q)
  : List(p, q),
    my_type(e.Get())
{
}

FstyleCastExpr::FstyleCastExpr(const char *e, Node *p, Node *q)
  : List(p, q),
    my_type(e)
{
}

void FstyleCastExpr::print(std::ostream &os, size_t indent, size_t depth) const
{
  print_encoded(os, indent, depth);
}

int FstyleCastExpr::What()
{
  return Token::ntFstyleCast;
}

const char *FstyleCastExpr::encoded_type() const
{
  return my_type;
}

Node *FstyleCastExpr::Translate(Walker* w)
{
  return w->TranslateFstyleCast(this);
}

void FstyleCastExpr::Typeof(Walker *w, TypeInfo &t)
{
  w->TypeofFstyleCast(this, t);
}

ClassSpec::ClassSpec(Node *p, Node *q, Node *c)
  : List(p, q),
    my_name(0),
    my_comments(c)
{
}

ClassSpec::ClassSpec(Node *car, Node *cdr, Node *c, const char *encode)
  : List(car, cdr),
    my_name(encode),
    my_comments(c)
{
}

int ClassSpec::What()
{
  return Token::ntClassSpec;
}

Node *ClassSpec::Translate(Walker* w)
{
  return w->TranslateClassSpec(this);
}

const char *ClassSpec::encoded_name() const
{
  return my_name;
}

Node *ClassSpec::GetComments()
{
  return my_comments;
}

EnumSpec::EnumSpec(Node *head)
  : List(head, 0),    
    my_name(0)
{
}

int EnumSpec::What()
{
  return Token::ntEnumSpec;
}

Node *EnumSpec::Translate(Walker* w)
{
  return w->TranslateEnumSpec(this);
}

const char *EnumSpec::encoded_name() const
{
  return my_name;
}

AccessSpec::AccessSpec(Node *p, Node *q)
  : List(p, q)
{
}

int AccessSpec::What()
{
  return Token::ntAccessSpec;
}

Node *AccessSpec::Translate(Walker* w)
{
  return w->TranslateAccessSpec(this);
}

AccessDecl::AccessDecl(Node *p, Node *q)
  : List(p, q)
{
}

int AccessDecl::What()
{
  return Token::ntAccessDecl;
}

Node *AccessDecl::Translate(Walker* w)
{
  return w->TranslateAccessDecl(this);
}

UserAccessSpec::UserAccessSpec(Node *p, Node *q)
  : List(p, q)
{
}

int UserAccessSpec::What()
{
  return Token::ntUserAccessSpec;
}

Node *UserAccessSpec::Translate(Walker* w)
{
  return w->TranslateUserAccessSpec(this);
}

UserdefKeyword::UserdefKeyword(Node *p, Node *q)
  : List(p, q)
{
}

int UserdefKeyword::What()
{
  return Token::ntUserdefKeyword;
}

#define PtreeStatementImpl(s)\
int s##Statement::What() { return Token::nt##s##Statement; }\
Node *s##Statement::Translate(Walker* w) { return w->Translate##s(this); }

PtreeStatementImpl(If)
PtreeStatementImpl(Switch)
PtreeStatementImpl(While)
PtreeStatementImpl(Do)
PtreeStatementImpl(For)
PtreeStatementImpl(Try)
PtreeStatementImpl(Break)
PtreeStatementImpl(Continue)
PtreeStatementImpl(Return)
PtreeStatementImpl(Goto)
PtreeStatementImpl(Case)
PtreeStatementImpl(Default)
PtreeStatementImpl(Label)

#undef PtreeStatementImpl

int ExprStatement::What()
{
  return Token::ntExprStatement;
}

Node *ExprStatement::Translate(Walker* w)
{
  return w->TranslateExprStatement(this);
}

#define PtreeExprImpl(n)\
int n##Expr::What() { return Token::nt##n##Expr; }\
Node *n##Expr::Translate(Walker* w) { return w->Translate##n(this); } \
void n##Expr::Typeof(Walker* w, TypeInfo& t) { w->Typeof##n(this, t);}

PtreeExprImpl(Comma)
PtreeExprImpl(Assign)
PtreeExprImpl(Cond)
PtreeExprImpl(Infix)
PtreeExprImpl(Pm)
PtreeExprImpl(Cast)
PtreeExprImpl(Unary)
PtreeExprImpl(Throw)
PtreeExprImpl(Sizeof)
PtreeExprImpl(Typeid)
PtreeExprImpl(Typeof)
PtreeExprImpl(New)
PtreeExprImpl(Delete)
PtreeExprImpl(Array)
PtreeExprImpl(Funcall)
PtreeExprImpl(Postfix)
PtreeExprImpl(UserStatement)
PtreeExprImpl(DotMember)
PtreeExprImpl(ArrowMember)
PtreeExprImpl(Paren)
PtreeExprImpl(StaticUserStatement)

#undef PtreeExprImpl
