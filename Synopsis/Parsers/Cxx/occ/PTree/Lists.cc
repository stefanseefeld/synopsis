//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "PTree/Lists.hh"
#include "PTree/operations.hh"
#include "Encoding.hh"
#include "Walker.hh"
#include <iostream>

using namespace PTree;

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

Node *Typedef::Translate(Walker* w)
{
  return w->TranslateTypedef(this);
}

Node *TemplateDecl::Translate(Walker* w)
{
  return w->TranslateTemplateDecl(this);
}

Node *TemplateInstantiation::Translate(Walker* w)
{
  return w->TranslateTemplateInstantiation(this);
}

Node *ExternTemplate::Translate(Walker* w)
{
  return w->TranslateExternTemplate(this);
}

Node *MetaclassDecl::Translate(Walker* w)
{
    return w->TranslateMetaclassDecl(this);
}

Node *LinkageSpec::Translate(Walker* w)
{
  return w->TranslateLinkageSpec(this);
}

Node *NamespaceSpec::Translate(Walker* w)
{
  return w->TranslateNamespaceSpec(this);
}

Node *NamespaceAlias::Translate(Walker* w)
{
  return w->TranslateNamespaceAlias(this);
}

Node *Using::Translate(Walker* w)
{
  return w->TranslateUsing(this);
}

Node *Declaration::Translate(Walker* w)
{
  return w->TranslateDeclaration(this);
}

Declarator::Declarator(Node *list, Encoding &t, Encoding &n, Node *dname)
  : List(list->car(), list->cdr()),
    my_type(t),
    my_name(n),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Encoding &t, Encoding &n, Node *dname)
  : List(0, 0),
    my_type(t),
    my_name(n),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Node *p, Node *q, Encoding &t, Encoding &n, Node *dname)
  : List(p, q),
    my_type(t),
    my_name(n),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Node *list, Encoding &t)
  : List(list->car(), list->cdr()),
    my_type(t),
    my_declared_name(0),
    my_comments(0)
{
}

Declarator::Declarator(Encoding &t)
  : List(0, 0),
    my_type(t),
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

Name::Name(Node *p, const Encoding &name)
  : List(p->car(), p->cdr()),
    my_name(name)
{
}

Node *Name::Translate(Walker *w)
{
  return w->TranslateVariable(this);
}

void Name::Typeof(Walker *w, TypeInfo &t)
{
  w->TypeofVariable(this, t);
}

FstyleCastExpr::FstyleCastExpr(const Encoding &type, Node *p, Node *q)
  : List(p, q),
    my_type(type)
{
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
    my_comments(c)
{
}

ClassSpec::ClassSpec(const Encoding &name, Node *car, Node *cdr, Node *c)
  : List(car, cdr),
    my_name(name),
    my_comments(c)
{
}

Node *ClassSpec::Translate(Walker* w)
{
  return w->TranslateClassSpec(this);
}

EnumSpec::EnumSpec(Node *head)
  : List(head, 0)
{
}

Node *EnumSpec::Translate(Walker* w)
{
  return w->TranslateEnumSpec(this);
}

AccessSpec::AccessSpec(Node *p, Node *q)
  : List(p, q)
{
}

Node *AccessSpec::Translate(Walker* w)
{
  return w->TranslateAccessSpec(this);
}

AccessDecl::AccessDecl(Node *p, Node *q)
  : List(p, q)
{
}

Node *AccessDecl::Translate(Walker* w)
{
  return w->TranslateAccessDecl(this);
}

UserAccessSpec::UserAccessSpec(Node *p, Node *q)
  : List(p, q)
{
}

Node *UserAccessSpec::Translate(Walker* w)
{
  return w->TranslateUserAccessSpec(this);
}

UserdefKeyword::UserdefKeyword(Node *p, Node *q)
  : List(p, q)
{
}

#define PtreeStatementImpl(s)\
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

Node *ExprStatement::Translate(Walker* w)
{
  return w->TranslateExprStatement(this);
}

#define PtreeExprImpl(n)\
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
