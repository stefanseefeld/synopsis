//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Lists_hh
#define _PTree_Lists_hh

#include "PTree/List.hh"

class Encoding;

namespace PTree
{

class Brace : public List 
{
public:
  Brace(Node *p, Node *q) : List(p, q) {}
  Brace(Node *ob, Node *body, Node *cb) : List(ob, Node::List(body, cb)) {}

  virtual void print(std::ostream &, size_t, size_t) const;

  int Write(std::ostream&, int);

  Node *Translate(Walker*);
};

class Block : public Brace 
{
public:
  Block(Node *p, Node *q) : Brace(p, q) {}
  Block(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}

  Node *Translate(Walker*);
};

class ClassBody : public Brace 
{
public:
  ClassBody(Node *p, Node *q) : Brace(p, q) {}
  ClassBody(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}

  Node *Translate(Walker*);
};

class Typedef : public List
{
public:
  Typedef(Node *p) : List(p, 0) {}
  Typedef(Node *p, Node *q) : List(p, q) {}
  int What();
  Node *Translate(Walker*);
};

class TemplateDecl : public List
{
public:
  TemplateDecl(Node *p, Node *q) : List(p, q) {}
  TemplateDecl(Node *p) : List(p, 0) {}
  int What();
  Node *Translate(Walker*);
};

class TemplateInstantiation : public List
{
public:
  TemplateInstantiation(Node *p) : List(p, 0) {}
  int What();
  Node *Translate(Walker*);
};

class ExternTemplate : public List
{
public:
  ExternTemplate(Node *p, Node *q) : List(p, q) {}
  ExternTemplate(Node *p) : List(p, 0) {}
  int What();
  Node *Translate(Walker*);
};

class MetaclassDecl : public List
{
public:
  MetaclassDecl(Node *p, Node *q) : List(p, q) {}
  int What();
  Node *Translate(Walker*);
};

class LinkageSpec : public List
{
public:
  LinkageSpec(Node *p, Node *q) : List(p, q) {}
  int What();
  Node *Translate(Walker*);
};

class NamespaceSpec : public List
{
public:
  NamespaceSpec(Node *p, Node *q) : List(p, q), comments(0) {}
  int What();
  Node *Translate(Walker*);
  
  Node *GetComments() { return comments; }
  void SetComments(Node *c) { comments = c; }

private:
  Node *comments;
};

class NamespaceAlias : public List
{
public:
  NamespaceAlias(Node *p, Node *q) : List(p, q) {}
  int What();
  Node *Translate(Walker*);
};

class Using : public List 
{
public:
  Using(Node *p) : List(p, 0) {}
  int What();
  Node *Translate(Walker*);
};

class Declaration : public List
{
public:
  Declaration(Node *p, Node *q) : List(p, q), comments(0) {}
  int What();
  Node *Translate(Walker*);

  Node *GetComments() { return comments; }
  void SetComments(Node *c) { comments = c; }

private:
  Node *comments;
};

class Declarator : public List
{
public:
  Declarator(Node *, Encoding&, Encoding&, Node *);
  Declarator(Encoding&, Encoding&, Node *);
  Declarator(Node *, Node *, Encoding&, Encoding&, Node *);
  Declarator(Node *, Encoding&);
  Declarator(Encoding&);
  Declarator(Declarator*, Node *, Node *);

  virtual void print(std::ostream &, size_t, size_t) const;

  int What();
  char* GetEncodedType() const;
  char* GetEncodedName() const;
  void SetEncodedType(char* t) { type = t; }
  Node *Name() { return declared_name; }

  Node *GetComments() { return comments; }
  void SetComments(Node *c) { comments = c; }

private:
  char* type;
  char* name;
  Node *declared_name;
  Node *comments;
};

class Name : public List
{
public:
  Name(Node *, Encoding&);

  void print(std::ostream &, size_t, size_t) const;

  int What();
  char* GetEncodedName() const;
  Node *Translate(Walker*);
  void Typeof(Walker*, TypeInfo&);

private:
  char *name;
};

class FstyleCastExpr : public List
{
public:
  FstyleCastExpr(Encoding&, Node *, Node *);
  FstyleCastExpr(char*, Node *, Node *);

  void print(std::ostream &, size_t, size_t) const;

  int What();
  char* GetEncodedType() const;
  Node *Translate(Walker*);
  void Typeof(Walker*, TypeInfo&);

private:
  char* type;
};

class ClassSpec : public List
{
public:
  ClassSpec(Node *, Node *, Node *);
  ClassSpec(Node *, Node *, Node *, char*);
  int What();
  Node *Translate(Walker*);
  char* GetEncodedName() const;
  Node *GetComments();

private:
  char *encoded_name;
  Node *comments;

friend class Parser;
};

class EnumSpec : public List
{
public:
  EnumSpec(Node *);
  int What();
  Node *Translate(Walker*);
  char* GetEncodedName() const;

private:
  char *encoded_name;

friend class Parser;
};

class AccessSpec : public List
{
public:
  AccessSpec(Node *, Node *);
  int What();
  Node *Translate(Walker*);
};

class AccessDecl : public List
{
public:
  AccessDecl(Node *, Node *);
  int What();
  Node *Translate(Walker*);
};

class UserAccessSpec : public List
{
public:
  UserAccessSpec(Node *, Node *);
  int What();
  Node *Translate(Walker*);
};

class UserdefKeyword : public List
{
public:
  UserdefKeyword(Node *, Node *);
  int What();
};

#define PtreeStatementDecl(s)\
class s##Statement : public List {\
public:\
  s##Statement(Node *p, Node *q) : List(p, q) {}	\
  int What();						\
  Node *Translate(Walker*);				\
}

PtreeStatementDecl(If);
PtreeStatementDecl(Switch);
PtreeStatementDecl(While);
PtreeStatementDecl(Do);
PtreeStatementDecl(For);
PtreeStatementDecl(Try);
PtreeStatementDecl(Break);
PtreeStatementDecl(Continue);
PtreeStatementDecl(Return);
PtreeStatementDecl(Goto);
PtreeStatementDecl(Case);
PtreeStatementDecl(Default);
PtreeStatementDecl(Label);
PtreeStatementDecl(Expr);

#undef PtreeStatementDecl

#define PtreeExprDecl(n)\
class n##Expr : public List {\
public:\
  n##Expr(Node *p, Node *q) : List(p, q) {}	\
  int What();					\
  Node *Translate(Walker*);			\
  void Typeof(Walker*, TypeInfo&);		\
}

PtreeExprDecl(Comma);
PtreeExprDecl(Assign);
PtreeExprDecl(Cond);
PtreeExprDecl(Infix);
PtreeExprDecl(Pm);
PtreeExprDecl(Cast);
PtreeExprDecl(Unary);
PtreeExprDecl(Throw);
PtreeExprDecl(Sizeof);
PtreeExprDecl(Typeid);
PtreeExprDecl(Typeof);
PtreeExprDecl(New);
PtreeExprDecl(Delete);
PtreeExprDecl(Array);
PtreeExprDecl(Funcall);
PtreeExprDecl(Postfix);
PtreeExprDecl(UserStatement);
PtreeExprDecl(DotMember);
PtreeExprDecl(ArrowMember);
PtreeExprDecl(Paren);
PtreeExprDecl(StaticUserStatement);

#undef PtreeExprDecl

}

#endif
