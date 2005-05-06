//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Lists_hh_
#define Synopsis_PTree_Lists_hh_

#include <Synopsis/PTree/operations.hh>
#include <Synopsis/PTree/Encoding.hh>

namespace Synopsis
{
namespace PTree
{

class Brace : public List 
{
public:
  Brace(Node *p, Node *q) : List(p, q) {}
  Brace(Node *ob, Node *body, Node *cb) : List(ob, list(body, cb)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Block : public Brace 
{
public:
  Block(Node *p, Node *q) : Brace(p, q) {}
  Block(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class ClassBody : public Brace 
{
public:
  ClassBody(Node *p, Node *q) : Brace(p, q) {}
  ClassBody(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Typedef : public List
{
public:
  Typedef(Node *p) : List(p, 0) {}
  Typedef(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class TemplateDecl : public List
{
public:
  TemplateDecl(Node *p, Node *q) : List(p, q) {}
  TemplateDecl(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class TemplateInstantiation : public List
{
public:
  TemplateInstantiation(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class ExternTemplate : public List
{
public:
  ExternTemplate(Node *p, Node *q) : List(p, q) {}
  ExternTemplate(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class MetaclassDecl : public List
{
public:
  MetaclassDecl(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class LinkageSpec : public List
{
public:
  LinkageSpec(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class NamespaceSpec : public List
{
public:
  NamespaceSpec(Node *p, Node *q) : List(p, q), my_comments(0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}  
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
};

class UsingDirective : public List 
{
public:
  UsingDirective(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Declaration : public List
{
public:
  Declaration(Node *p, Node *q) : List(p, q), my_comments(0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *get_comments() { return my_comments;}
  void set_comments(Node *c) { my_comments = c;}

private:
  Node *my_comments;
};

class ParameterDeclaration : public List 
{
public:
  ParameterDeclaration(Node *mod, Node *type, Node *decl)
    : List(mod, list(type, decl)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class UsingDeclaration : public List 
{
public:
  UsingDeclaration(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
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
private:
  Encoding my_name;
};

class FstyleCastExpr : public List
{
public:
  FstyleCastExpr(const Encoding &, Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_type() const { return my_type;}
private:
  Encoding my_type;
};

class ClassSpec : public List
{
public:
  ClassSpec(Node *, Node *, Node *);
  ClassSpec(const Encoding &, Node *, Node *, Node *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_name() const { return my_name;}
  void set_encoded_name(const Encoding &n) { my_name = n;}
  Node *get_comments() { return my_comments;}
  //. The following assumes proper C++, i.e. no OpenC++ extension.
  ClassBody const *body() const { return static_cast<ClassBody const *>(PTree::nth(this, 3));}
  ClassBody *body() { return static_cast<ClassBody *>(PTree::nth(this, 3));}
private:
  Encoding my_name;
  Node    *my_comments;
};

class EnumSpec : public List
{
public:
  EnumSpec(Node *head) : List(head, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_name() const { return my_name;}
  void set_encoded_name(const Encoding &n) { my_name = n;}
private:
  Encoding my_name;
};

class AccessSpec : public List
{
public:
  AccessSpec(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class AccessDecl : public List
{
public:
  AccessDecl(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class UserAccessSpec : public List
{
public:
  UserAccessSpec(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class UserdefKeyword : public List
{
public:
  UserdefKeyword(Node *p, Node *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

#define PtreeStatementDecl(s)\
class s##Statement : public List {\
public:\
  s##Statement(Node *p, Node *q) : List(p, q) {}	\
  virtual void accept(Visitor *visitor) { visitor->visit(this);}\
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
}

#endif
