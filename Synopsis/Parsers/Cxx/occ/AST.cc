/*
  Copyright (C) 1997-2000 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

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
    return THIS;
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
int Leaf##w::What() { return w; }

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

void PtreeBrace::Print(std::ostream& s, int indent, int depth)
{
    if(TooDeep(s, depth))
	return;

    int indent2 = indent + 1;
    s << "[{";
    Ptree* body = Ptree::Second(this);
    if(body == 0){
	PrintIndent(s, indent2);
	s << "nil";
    }
    else
	while(body != 0){
	    PrintIndent(s, indent2);
	    if(body->IsLeaf()){
		s << "@ ";
		body->Print(s, indent + 1, depth + 1);
	    }
	    else{
		Ptree* head = body->Car();
		if(head == 0)
		    s << "nil";
		else
		    head->Print(s, indent + 1, depth + 1);
	    }

	    body = body->Cdr();
	}

    PrintIndent(s, indent);
    s << "}]";
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
	    PrintIndent(out, indent + 1);
	    ++n;
	    Ptree* q = p->Car();
	    p = p->Cdr();
	    if(q != 0)
		n += q->Write(out, indent + 1);
	}
    }

    PrintIndent(out, indent);
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
    return ntTypedef;
}

Ptree* PtreeTypedef::Translate(Walker* w)
{
    return w->TranslateTypedef(this);
}

// class PtreeTemplateDecl

int PtreeTemplateDecl::What()
{
    return ntTemplateDecl;
}

Ptree* PtreeTemplateDecl::Translate(Walker* w)
{
    return w->TranslateTemplateDecl(this);
}

// class PtreeTemplateInstantiation

int PtreeTemplateInstantiation::What()
{
    return ntTemplateInstantiation;
}

Ptree* PtreeTemplateInstantiation::Translate(Walker* w)
{
    return w->TranslateTemplateInstantiation(this);
}

// class PtreeExternTemplate

int PtreeExternTemplate::What()
{
    return ntExternTemplate;
}

Ptree* PtreeExternTemplate::Translate(Walker* w)
{
    return w->TranslateExternTemplate(this);
}

// class PtreeMetaclassDecl

int PtreeMetaclassDecl::What()
{
    return ntMetaclassDecl;
}

Ptree* PtreeMetaclassDecl::Translate(Walker* w)
{
    return w->TranslateMetaclassDecl(this);
}

// class PtreeLinkageSpec

int PtreeLinkageSpec::What()
{
    return ntLinkageSpec;
}

Ptree* PtreeLinkageSpec::Translate(Walker* w)
{
    return w->TranslateLinkageSpec(this);
}

// class PtreeNamespaceSpec

int PtreeNamespaceSpec::What()
{
    return ntNamespaceSpec;
}

Ptree* PtreeNamespaceSpec::Translate(Walker* w)
{
    return w->TranslateNamespaceSpec(this);
}

// class PtreeNamespaceAlias

int PtreeNamespaceAlias::What()
{
    return ntNamespaceAlias;
}

Ptree* PtreeNamespaceAlias::Translate(Walker* w)
{
    return w->TranslateNamespaceAlias(this);
}

// class PtreeUsing

int PtreeUsing::What()
{
    return ntUsing;
}

Ptree* PtreeUsing::Translate(Walker* w)
{
    return w->TranslateUsing(this);
}

// class PtreeDeclaration

int PtreeDeclaration::What()
{
    return ntDeclaration;
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

int PtreeDeclarator::What()
{
    return ntDeclarator;
}

char* PtreeDeclarator::GetEncodedType()
{
    return type;
}

char* PtreeDeclarator::GetEncodedName()
{
    return name;
}

void PtreeDeclarator::Print(std::ostream& s, int i, int d)
{
    if(show_encoded)
	PrintWithEncodeds(s, i, d);
    else
	NonLeaf::Print(s, i, d);
}


// class PtreeName

PtreeName::PtreeName(Ptree* p, Encoding& e)
: NonLeaf(p->Car(), p->Cdr())
{
    name = e.Get();
}

int PtreeName::What()
{
    return ntName;
}

char* PtreeName::GetEncodedName()
{
    return name;
}

void PtreeName::Print(std::ostream& s, int i, int d)
{
    if(show_encoded)
	PrintWithEncodeds(s, i, d);
    else
	NonLeaf::Print(s, i, d);
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

int PtreeFstyleCastExpr::What()
{
    return ntFstyleCast;
}

char* PtreeFstyleCastExpr::GetEncodedType()
{
    return type;
}

void PtreeFstyleCastExpr::Print(std::ostream& s, int i, int d)
{
    if(show_encoded)
	PrintWithEncodeds(s, i, d);
    else
	NonLeaf::Print(s, i, d);
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
    return ntClassSpec;
}

Ptree* PtreeClassSpec::Translate(Walker* w)
{
    return w->TranslateClassSpec(this);
}

char* PtreeClassSpec::GetEncodedName()
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
    return ntEnumSpec;
}

Ptree* PtreeEnumSpec::Translate(Walker* w)
{
    return w->TranslateEnumSpec(this);
}

char* PtreeEnumSpec::GetEncodedName()
{
    return encoded_name;
}

PtreeAccessSpec::PtreeAccessSpec(Ptree* p, Ptree* q)
: NonLeaf(p, q)
{
}

int PtreeAccessSpec::What()
{
    return ntAccessSpec;
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
    return ntAccessDecl;
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
    return ntUserAccessSpec;
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
    return ntUserdefKeyword;
}

#define PtreeStatementImpl(s)\
int Ptree##s##Statement::What() { return nt##s##Statement; }\
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
    return ntExprStatement;
}

Ptree* PtreeExprStatement::Translate(Walker* w)
{
    return w->TranslateExprStatement(this);
}

#define PtreeExprImpl(n)\
int Ptree##n##Expr::What() { return nt##n##Expr; }\
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
