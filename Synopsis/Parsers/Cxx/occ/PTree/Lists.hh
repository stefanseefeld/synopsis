//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Lists_hh
#define _PTree_Lists_hh

#include "PTree/operations.hh"
#include "PTree/Encoding.hh"

namespace PTree
{

class Brace : public List 
{
public:
  Brace(Node *p, Node *q) : List(p, q) {}
  Brace(Node *ob, Node *body, Node *cb) : List(ob, list(body, cb)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Node *Translate(Walker*);
};

class Block : public Brace 
{
public:
  Block(Node *p, Node *q) : Brace(p, q) {}
  Block(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Node *Translate(Walker*);
};

class ClassBody : public Brace 
{
public:
  ClassBody(Node *p, Node *q) : Brace(p, q) {}
  ClassBody(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Node *Translate(Walker*);
};

class Typedef : public List
{
public:
  Typedef(Node *p) : List(p, 0) {}
  Typedef(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class TemplateDecl : public List
{
public:
  TemplateDecl(Node *p, Node *q) : List(p, q) {}
  TemplateDecl(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class TemplateInstantiation : public List
{
public:
  TemplateInstantiation(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class ExternTemplate : public List
{
public:
  ExternTemplate(Node *p, Node *q) : List(p, q) {}
  ExternTemplate(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class MetaclassDecl : public List
{
public:
  MetaclassDecl(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class LinkageSpec : public List
{
public:
  LinkageSpec(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class NamespaceSpec : public List
{
public:
  NamespaceSpec(Node *p, Node *q) : List(p, q), my_comments(0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
  
  Node *get_comments() { return my_comments;}
  void set_comments(Node *c) { my_comments = c;}

private:
  Node *my_comments;
};

class NamespaceAlias : public List
{
public:
  NamespaceAlias(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class Using : public List 
{
public:
  Using(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class Declaration : public List
{
public:
  Declaration(Node *p, Node *q) : List(p, q), my_comments(0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);

  Node *get_comments() { return my_comments;}
  void set_comments(Node *c) { my_comments = c;}

private:
  Node *my_comments;
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
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_type() const { return my_type;}
  Encoding encoded_name() const { return my_name;}
  void set_encoded_type(const Encoding &t) { my_type = t;}
  Node *name() { return my_declared_name;}
  Node *get_comments() { return my_comments;}
  void set_comments(Node *c) { my_comments = c;}
private:
  Encoding my_type;
  Encoding my_name;
  Node    *my_declared_name;
  Node    *my_comments;
};

class Name : public List
{
public:
  Name(Node *, const Encoding &);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_name() const { return my_name;}
  Node *Translate(Walker*);
  void Typeof(Walker*, TypeInfo&);
private:
  Encoding my_name;
};

class FstyleCastExpr : public List
{
public:
  FstyleCastExpr(const Encoding &, Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_type() const { return my_type;}
  Node *Translate(Walker*);
  void Typeof(Walker*, TypeInfo&);
private:
  Encoding my_type;
};

class ClassSpec : public List
{
public:
  ClassSpec(Node *, Node *, Node *);
  ClassSpec(const Encoding &, Node *, Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
  Encoding encoded_name() const { return my_name;}
  void set_encoded_name(const Encoding &n) { my_name = n;}
  Node *get_comments() { return my_comments;}
private:
  Encoding my_name;
  Node    *my_comments;
};

class EnumSpec : public List
{
public:
  EnumSpec(Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
  Encoding encoded_name() const { return my_name;}
  void set_encoded_name(const Encoding &n) { my_name = n;}
private:
  Encoding my_name;
};

class AccessSpec : public List
{
public:
  AccessSpec(Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class AccessDecl : public List
{
public:
  AccessDecl(Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class UserAccessSpec : public List
{
public:
  UserAccessSpec(Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *Translate(Walker*);
};

class UserdefKeyword : public List
{
public:
  UserdefKeyword(Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

#define PtreeStatementDecl(s)\
class s##Statement : public List {\
public:\
  s##Statement(Node *p, Node *q) : List(p, q) {}	\
  virtual void accept(Visitor *visitor) { visitor->visit(this);}\
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
  virtual void accept(Visitor *visitor) { visitor->visit(this);} \
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
