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
#include "Encoding.hh"
#include "Walker.hh"

using namespace PTree;

void Brace::print(std::ostream &os, size_t indent, size_t depth) const
{
  if(too_deep(os, depth)) return;

  size_t indent2 = indent + 1;
  os << "[{";
  const Node *body = Node::Second(this);
  if(!body)
  {
    print_indent(os, indent2);
    os << "nil";
  }
  else
    while(body)
    {
      print_indent(os, indent2);
      if(body->IsLeaf())
      {
	os << "@ ";
	body->print(os, indent + 1, depth + 1);
      }
      else
      {
	const Node *head = body->Car();
	if(!head) os << "nil";
	else head->print(os, indent + 1, depth + 1);
      }
      body = body->Cdr();
    }
  print_indent(os, indent);
  os << "}]";
}

int Brace::Write(std::ostream& out, int indent)
{
    int n = 0;

    out << '{';
    Node *p = this->Cadr();
    while(p != 0){
	if(p->IsLeaf())
	  throw std::runtime_error("Brace::Write(): non list");
	else{
	    print_indent(out, indent + 1);
	    ++n;
	    Node *q = p->Car();
	    p = p->Cdr();
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
  : List(list->Car(), list->Cdr()),
    type(t.Get()),
    name(n.Get()),
    declared_name(dname),
    comments(0)
{
}

Declarator::Declarator(Encoding &t, Encoding &n, Node *dname)
  : List(0, 0),
    type(t.Get()),
    name(n.Get()),
    declared_name(dname),
    comments(0)
{
}

Declarator::Declarator(Node *p, Node *q, Encoding &t, Encoding &n, Node *dname)
  : List(p, q),
    type(t.Get()),
    name(n.Get()),
    declared_name(dname),
    comments(0)
{
}

Declarator::Declarator(Node *list, Encoding &t)
  : List(list->Car(), list->Cdr()),
    type(t.Get()),
    name(0),
    declared_name(0),
    comments(0)
{
}

Declarator::Declarator(Encoding &t)
  : List(0, 0),
    type(t.Get()),
    name(0),
    declared_name(0),
    comments(0)
{
}

Declarator::Declarator(Declarator *decl, Node *p, Node *q)
  : List(p, q),
    type(decl->type),
    name(decl->name),
    declared_name(decl->declared_name),
    comments(0)
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

char *Declarator::GetEncodedType() const
{
  return type;
}

char *Declarator::GetEncodedName() const
{
  return name;
}

Name::Name(Node *p, Encoding &e)
  : List(p->Car(), p->Cdr()),
    name(e.Get())
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

char *Name::GetEncodedName() const
{
  return name;
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
    type(e.Get())
{
}

FstyleCastExpr::FstyleCastExpr(char *e, Node *p, Node *q)
  : List(p, q),
    type(e)
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

char *FstyleCastExpr::GetEncodedType() const
{
  return type;
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
    encoded_name(0),
    comments(c)
{
}

ClassSpec::ClassSpec(Node *car, Node *cdr, Node *c, char *encode)
  : List(car, cdr),
    encoded_name(encode),
    comments(c)
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

char *ClassSpec::GetEncodedName() const
{
  return encoded_name;
}

Node *ClassSpec::GetComments()
{
  return comments;
}

EnumSpec::EnumSpec(Node *head)
  : List(head, 0),    
    encoded_name(0)
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

char *EnumSpec::GetEncodedName() const
{
  return encoded_name;
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
