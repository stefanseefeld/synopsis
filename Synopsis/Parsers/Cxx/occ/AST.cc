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
#include "AST.hh"
#include "Encoding.hh"
#include "Walker.hh"

// class LeafName

LeafName::LeafName(Token& t) : CommentedLeaf(t) {}

Ptree* LeafName::Translate(Walker* w)
{
    return w->TranslateVariable(this);
}

void LeafName::Typeof(Walker* w, TypeInfo& t)
{
    w->TypeofVariable(this, t);
}

int LeafThis::What()
{
  return Token::THIS;
}

Ptree* LeafThis::Translate(Walker* w)
{
    return w->TranslateThis(this);
}

void LeafThis::Typeof(Walker* w, TypeInfo& t)
{
    w->TypeofThis(this, t);
}

#define ResearvedWordImpl(w) \
  int Leaf##w::What() { return Token:: w; }

ResearvedWordImpl(AUTO)
ResearvedWordImpl(BOOLEAN)
ResearvedWordImpl(CHAR)
ResearvedWordImpl(WCHAR)
ResearvedWordImpl(CONST)
ResearvedWordImpl(DOUBLE)
ResearvedWordImpl(EXTERN)
ResearvedWordImpl(FLOAT)
ResearvedWordImpl(FRIEND)
ResearvedWordImpl(INLINE)
ResearvedWordImpl(INT)
ResearvedWordImpl(LONG)
ResearvedWordImpl(MUTABLE)
ResearvedWordImpl(NAMESPACE)
ResearvedWordImpl(PRIVATE)
ResearvedWordImpl(PROTECTED)
ResearvedWordImpl(PUBLIC)
ResearvedWordImpl(REGISTER)
ResearvedWordImpl(SHORT)
ResearvedWordImpl(SIGNED)
ResearvedWordImpl(STATIC)
ResearvedWordImpl(UNSIGNED)
ResearvedWordImpl(USING)
ResearvedWordImpl(VIRTUAL)
ResearvedWordImpl(VOID)
ResearvedWordImpl(VOLATILE)

ResearvedWordImpl(UserKeyword2)

#undef ResearvedWordImpl

// class PtreeBrace

void PtreeBrace::print(std::ostream &os, size_t indent, size_t depth) const
{
  if(too_deep(os, depth)) return;

  size_t indent2 = indent + 1;
  os << "[{";
  const Ptree *body = Ptree::Second(this);
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
	const Ptree *head = body->Car();
	if(!head) os << "nil";
	else head->print(os, indent + 1, depth + 1);
      }
      body = body->Cdr();
    }
  print_indent(os, indent);
  os << "}]";
}

int PtreeBrace::Write(std::ostream& out, int indent)
{
    int n = 0;

    out << '{';
    Ptree* p = this->Cadr();
    while(p != 0){
	if(p->IsLeaf()){
	    MopErrorMessage("PtreeBrace::Write()", "non list");
	    break;
	}
	else{
	    print_indent(out, indent + 1);
	    ++n;
	    Ptree* q = p->Car();
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

Ptree* PtreeBrace::Translate(Walker* w)
{
    return w->TranslateBrace(this);
}

// class PtreeBlock

Ptree* PtreeBlock::Translate(Walker* w)
{
    return w->TranslateBlock(this);
}

// class PtreeClassBody

Ptree* PtreeClassBody::Translate(Walker* w)
{
    return w->TranslateClassBody(this, 0, 0);
}

// class PtreeTypedef

int PtreeTypedef::What()
{
  return Token::ntTypedef;
}

Ptree* PtreeTypedef::Translate(Walker* w)
{
    return w->TranslateTypedef(this);
}

// class PtreeTemplateDecl

int PtreeTemplateDecl::What()
{
    return Token::ntTemplateDecl;
}

Ptree* PtreeTemplateDecl::Translate(Walker* w)
{
    return w->TranslateTemplateDecl(this);
}

// class PtreeTemplateInstantiation

int PtreeTemplateInstantiation::What()
{
    return Token::ntTemplateInstantiation;
}

Ptree* PtreeTemplateInstantiation::Translate(Walker* w)
{
    return w->TranslateTemplateInstantiation(this);
}

// class PtreeExternTemplate

int PtreeExternTemplate::What()
{
    return Token::ntExternTemplate;
}

Ptree* PtreeExternTemplate::Translate(Walker* w)
{
    return w->TranslateExternTemplate(this);
}

// class PtreeMetaclassDecl

int PtreeMetaclassDecl::What()
{
    return Token::ntMetaclassDecl;
}

Ptree* PtreeMetaclassDecl::Translate(Walker* w)
{
    return w->TranslateMetaclassDecl(this);
}

// class PtreeLinkageSpec

int PtreeLinkageSpec::What()
{
    return Token::ntLinkageSpec;
}

Ptree* PtreeLinkageSpec::Translate(Walker* w)
{
    return w->TranslateLinkageSpec(this);
}

// class PtreeNamespaceSpec

int PtreeNamespaceSpec::What()
{
    return Token::ntNamespaceSpec;
}

Ptree* PtreeNamespaceSpec::Translate(Walker* w)
{
    return w->TranslateNamespaceSpec(this);
}

// class PtreeNamespaceAlias

int PtreeNamespaceAlias::What()
{
    return Token::ntNamespaceAlias;
}

Ptree* PtreeNamespaceAlias::Translate(Walker* w)
{
    return w->TranslateNamespaceAlias(this);
}

// class PtreeUsing

int PtreeUsing::What()
{
    return Token::ntUsing;
}

Ptree* PtreeUsing::Translate(Walker* w)
{
    return w->TranslateUsing(this);
}

// class PtreeDeclaration

int PtreeDeclaration::What()
{
    return Token::ntDeclaration;
}

Ptree* PtreeDeclaration::Translate(Walker* w)
{
    return w->TranslateDeclaration(this);
}

// class PtreeDeclarator

PtreeDeclarator::PtreeDeclarator(Ptree* list, Encoding& t, Encoding& n,
				 Ptree* dname)
: NonLeaf(list->Car(), list->Cdr())
{
    type = t.Get();
    name = n.Get();
    declared_name = dname;
    comments = 0;
}

PtreeDeclarator::PtreeDeclarator(Encoding& t, Encoding& n, Ptree* dname)
: NonLeaf(0, 0)
{
    type = t.Get();
    name = n.Get();
    declared_name = dname;
    comments = 0;
}

PtreeDeclarator::PtreeDeclarator(Ptree* p, Ptree* q,
				 Encoding& t, Encoding& n, Ptree* dname)
: NonLeaf(p, q)
{
    type = t.Get();
    name = n.Get();
    declared_name = dname;
    comments = 0;
}

PtreeDeclarator::PtreeDeclarator(Ptree* list, Encoding& t)
: NonLeaf(list->Car(), list->Cdr())
{
    type = t.Get();
    name = 0;
    declared_name = 0;
    comments = 0;
}

PtreeDeclarator::PtreeDeclarator(Encoding& t)
: NonLeaf(0, 0)
{
    type = t.Get();
    name = 0;
    declared_name = 0;
    comments = 0;
}

PtreeDeclarator::PtreeDeclarator(PtreeDeclarator* decl, Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
    type = decl->type;
    name = decl->name;
    declared_name = decl->declared_name;
    comments = 0;
}

void PtreeDeclarator::print(std::ostream &os, size_t indent, size_t depth) const
{
  print_encoded(os, indent, depth);
}

int PtreeDeclarator::What()
{
    return Token::ntDeclarator;
}

char* PtreeDeclarator::GetEncodedType() const
{
    return type;
}

char* PtreeDeclarator::GetEncodedName() const
{
    return name;
}

// class PtreeName

PtreeName::PtreeName(Ptree* p, Encoding& e)
: NonLeaf(p->Car(), p->Cdr())
{
    name = e.Get();
}

void PtreeName::print(std::ostream &os, size_t indent, size_t depth) const
{
  print_encoded(os, indent, depth);
}

int PtreeName::What()
{
    return Token::ntName;
}

char* PtreeName::GetEncodedName() const
{
    return name;
}

Ptree* PtreeName::Translate(Walker* w)
{
    return w->TranslateVariable(this);
}

void PtreeName::Typeof(Walker* w, TypeInfo& t)
{
    w->TypeofVariable(this, t);
}

// class PtreeFstyleCastExpr

PtreeFstyleCastExpr::PtreeFstyleCastExpr(Encoding& e, Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
    type = e.Get();
}

PtreeFstyleCastExpr::PtreeFstyleCastExpr(char* e, Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
    type = e;
}

void PtreeFstyleCastExpr::print(std::ostream &os, size_t indent, size_t depth) const
{
  print_encoded(os, indent, depth);
}

int PtreeFstyleCastExpr::What()
{
    return Token::ntFstyleCast;
}

char* PtreeFstyleCastExpr::GetEncodedType() const
{
    return type;
}

Ptree* PtreeFstyleCastExpr::Translate(Walker* w)
{
    return w->TranslateFstyleCast(this);
}

void PtreeFstyleCastExpr::Typeof(Walker* w, TypeInfo& t)
{
    w->TypeofFstyleCast(this, t);
}

// class PtreeClassSpec

PtreeClassSpec::PtreeClassSpec(Ptree* p, Ptree* q, Ptree* c)
: NonLeaf(p, q)
{
    encoded_name = 0;
    comments = c;
}

PtreeClassSpec::PtreeClassSpec(Ptree* car, Ptree* cdr,
		Ptree* c, char* encode) : NonLeaf(car, cdr)
{
    encoded_name = encode;
    comments = c;
}

int PtreeClassSpec::What()
{
    return Token::ntClassSpec;
}

Ptree* PtreeClassSpec::Translate(Walker* w)
{
    return w->TranslateClassSpec(this);
}

char* PtreeClassSpec::GetEncodedName() const
{
    return encoded_name;
}

Ptree* PtreeClassSpec::GetComments()
{
    return comments;
}

// class PtreeEnumSpec

PtreeEnumSpec::PtreeEnumSpec(Ptree* head)
: NonLeaf(head, 0)
{
    encoded_name = 0;
}

int PtreeEnumSpec::What()
{
    return Token::ntEnumSpec;
}

Ptree* PtreeEnumSpec::Translate(Walker* w)
{
    return w->TranslateEnumSpec(this);
}

char* PtreeEnumSpec::GetEncodedName() const
{
    return encoded_name;
}

PtreeAccessSpec::PtreeAccessSpec(Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
}

int PtreeAccessSpec::What()
{
    return Token::ntAccessSpec;
}

Ptree* PtreeAccessSpec::Translate(Walker* w)
{
    return w->TranslateAccessSpec(this);
}

PtreeAccessDecl::PtreeAccessDecl(Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
}

int PtreeAccessDecl::What()
{
    return Token::ntAccessDecl;
}

Ptree* PtreeAccessDecl::Translate(Walker* w)
{
    return w->TranslateAccessDecl(this);
}

PtreeUserAccessSpec::PtreeUserAccessSpec(Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
}

int PtreeUserAccessSpec::What()
{
    return Token::ntUserAccessSpec;
}

Ptree* PtreeUserAccessSpec::Translate(Walker* w)
{
    return w->TranslateUserAccessSpec(this);
}

PtreeUserdefKeyword::PtreeUserdefKeyword(Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
}

int PtreeUserdefKeyword::What()
{
    return Token::ntUserdefKeyword;
}

#define PtreeStatementImpl(s)\
int Ptree##s##Statement::What() { return Token::nt##s##Statement; }\
Ptree* Ptree##s##Statement::Translate(Walker* w) {\
    return w->Translate##s(this); }

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

int PtreeExprStatement::What()
{
    return Token::ntExprStatement;
}

Ptree* PtreeExprStatement::Translate(Walker* w)
{
    return w->TranslateExprStatement(this);
}

#define PtreeExprImpl(n)\
int Ptree##n##Expr::What() { return Token::nt##n##Expr; }\
Ptree* Ptree##n##Expr::Translate(Walker* w) {\
    return w->Translate##n(this); }\
void Ptree##n##Expr::Typeof(Walker* w, TypeInfo& t) {\
    w->Typeof##n(this, t); }

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
