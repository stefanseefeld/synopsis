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
#include <Synopsis/PTree/Atoms.hh>

namespace Synopsis
{
namespace PTree
{

class Name : public List
{
public:
  Name(Node *, const Encoding &);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_name() const { return my_name;}
private:
  Encoding my_name;
};

class Brace : public List 
{
public:
  Brace(Node *p, List *q) : List(p, q) {}
  Brace(Node *ob, Node *body, Node *cb) : List(ob, list(body, cb)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Block : public Brace 
{
public:
  Block(Node *p, List *q) : Brace(p, q) {}
  Block(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class ClassBody : public Brace 
{
public:
  ClassBody(Node *p, List *q) : Brace(p, q) {}
  ClassBody(Node *ob, Node *bdy, Node *cb) : Brace(ob, bdy, cb) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class DeclSpec : public List
{
public:
  enum StorageClass { UNDEF, AUTO, REGISTER, STATIC, EXTERN, MUTABLE};
  enum Flags { NONE = 0x0, FRIEND = 0x1, TYPEDEF = 0x2};

  DeclSpec(List *, Encoding const &, StorageClass,
	   unsigned int flags, bool decl, bool def);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Encoding const &type() const { return my_type;}
  StorageClass storage_class() const { return my_storage_class;}
  bool is_friend() const { return my_is_friend;}
  bool is_typedef() const { return my_is_typedef;}
  bool declares_class_or_enum() const { return my_declares_class_or_enum;}
  bool defines_class_or_enum() const { return my_defines_class_or_enum;}
private:
  Encoding     my_type;
  StorageClass my_storage_class : 3;
  bool         my_is_friend : 1;
  bool         my_is_typedef : 1;
  bool         my_declares_class_or_enum : 1;
  bool         my_defines_class_or_enum : 1;
};

class Declarator : public List
{
public:
  Declarator(List *, Encoding const&, Encoding const&, List *);

  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_type() const { return my_type;}
  Encoding encoded_name() const { return my_name;}
  void set_encoded_type(const Encoding &t) { my_type = t;}
  Node *initializer();
  List *get_comments() { return my_comments;}
  void set_comments(List *c) { my_comments = c;}
private:
  Encoding my_type;
  Encoding my_name;
  List    *my_comments;
};

class Declaration : public List
{
public:
  Declaration(Node *p, List *q) : List(p, q), my_comments(0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  List *get_comments() { return my_comments;}
  void set_comments(List *c) { my_comments = c;}

private:
  List *my_comments;
};

class SimpleDeclaration : public Declaration
{
public:
  SimpleDeclaration(Node *p, List *q) : Declaration(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  DeclSpec *decl_specifier_seq() { return static_cast<DeclSpec *>(nth<0>(this));}
  List *declarators() { return static_cast<List *>(nth<1>(this));}
};

class LinkageSpec : public Declaration
{
public:
  LinkageSpec(Node *p, List *q) : Declaration(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class TemplateParameterList : public List
{
public:
  TemplateParameterList() : List(0, 0) {}
};

class TemplateDeclaration : public Declaration
{
public:
  TemplateDeclaration(Node *p, List *q) : Declaration(p, q) {}
  TemplateDeclaration(Node *p) : Declaration(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class TemplateInstantiation : public Declaration
{
public:
  TemplateInstantiation(Keyword *s, Atom *t,
			DeclSpec *spec, Declarator *decl, Atom *semic)
    : Declaration(s, list(t, spec, decl, semic)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  DeclSpec *decl_specifier_seq() { return static_cast<DeclSpec *>(nth<2>(this));}
  Declarator *declarator() { return static_cast<Declarator *>(nth<3>(this));}
};

class ExternTemplate : public List
{
public:
  ExternTemplate(Node *p, List *q) : List(p, q) {}
  ExternTemplate(Node *p) : List(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class NamespaceSpec : public Declaration
{
public:
  NamespaceSpec(Node *p, List *q) : Declaration(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}  
};

class UsingDirective : public Declaration
{
public:
  UsingDirective(Node *p) : Declaration(p, 0) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class UsingDeclaration : public Declaration
{
public:
  UsingDeclaration(Kwd::Using *u, Kwd::Typename *t, Name *id)
    : Declaration(u, list(t, id)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Name *name() const { return static_cast<Name *>(nth<2>(this));}
};

class NamespaceAlias : public Declaration
{
public:
  NamespaceAlias(Node *p, List *q) : Declaration(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class FunctionDefinition : public Declaration
{
public:
  FunctionDefinition(DeclSpec *p, List *q) : Declaration(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  DeclSpec *decl_specifier_seq() const { return static_cast<DeclSpec *>(nth<0>(this));}
  Declarator *declarator() const { return static_cast<Declarator *>(nth<1>(this));}
};

class ElaboratedTypeSpec : public List
{
public:
  ElaboratedTypeSpec(Keyword *type, Node *id) : List(type, cons(id)) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Keyword *type() const { return static_cast<Keyword *>(nth<0>(this));}
  Node *name() const { return nth<1>(this);}
};

class ParameterDeclaration : public List 
{
public:
  ParameterDeclaration(DeclSpec *spec, Declarator *decl)
    : List(spec, cons(decl)) {}
  ParameterDeclaration(DeclSpec *spec, Declarator *decl,
		       Atom *equal, Node *init)
    : List(spec, list(decl, equal, init))
  {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  DeclSpec *decl_specifier_seq() const { return static_cast<DeclSpec *>(nth<0>(this));}
  Declarator *declarator() const { return static_cast<Declarator *>(nth<1>(this));}
  Node *initializer() { return PTree::length(this) > 2 ? last(this) : 0;}
};

class FstyleCastExpr : public List
{
public:
  FstyleCastExpr(const Encoding &, Node *, List *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_type() const { return my_type;}
private:
  Encoding my_type;
};

//. class-key name [opt] base-clause [opt] { member-specification [opt] }
class ClassSpec : public List
{
public:
  ClassSpec(const Encoding &, Node *, List *, List *);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Encoding encoded_name() const { return my_name;}
  List *get_comments() { return my_comments;}

  Keyword *key() const { return static_cast<Keyword *>(nth<0>(this));}
  Node *name() const { return nth<1>(this);}
  //. : base-specifier-list
  List *base_clause() const { return static_cast<List *>(nth<2>(this));}
  //. class-body
  ClassBody *body() const { return static_cast<ClassBody *>(nth<3>(this));};

private:
  Encoding my_name;
  List    *my_comments;
};

//. enum identifier [opt] { enumerator-list [opt] }
class EnumSpec : public List
{
public:
  EnumSpec(Encoding const &e, Kwd::Enum *head, Identifier *n)
    : List(head, cons(n)), my_name(e) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Atom *name() const { return static_cast<Atom *>(nth<1>(this));}
  List *enumerators() const { return static_cast<List *>(nth<3>(this));}

  //. For named enums this will be the same as 'name' above.
  //. For unnamed enums it will contain an id that is unique in the
  //. current translation unit. It is needed for enumerators to be
  //. able to name their type, even if nobody else can.
  Encoding encoded_name() const { return my_name;}

private:
  Encoding my_name;
};

class TypeParameter : public List 
{
public:
  TypeParameter(Node *p, List *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class AccessSpec : public List
{
public:
  AccessSpec(Node *p, List *q, Node *c) : List(p, q), my_comments(c) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Node *get_comments() { return my_comments;}
private:
  Node *my_comments;
};

class AccessDecl : public List
{
public:
  AccessDecl(Node *p, List *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

template <typename T>
class StatementT : public List
{
public:
  StatementT(Node *p, List *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(static_cast<T *>(this));}
};

class IfStatement : public StatementT<IfStatement> 
{
public:
  IfStatement(Node *p, List *q) : StatementT<IfStatement>(p, q) {} 
};

class SwitchStatement : public StatementT<SwitchStatement> 
{
public:
  SwitchStatement(Node *p, List *q) : StatementT<SwitchStatement>(p, q) {} 
};

class WhileStatement : public StatementT<WhileStatement> 
{
public:
  WhileStatement(Node *p, List *q) : StatementT<WhileStatement>(p, q) {} 
};

class DoStatement : public StatementT<DoStatement> 
{
public:
  DoStatement(Node *p, List *q) : StatementT<DoStatement>(p, q) {} 
};

class ForStatement : public StatementT<ForStatement> 
{
public:
  ForStatement(Node *p, List *q) : StatementT<ForStatement>(p, q) {} 
};

class TryStatement : public StatementT<TryStatement> 
{
public:
  TryStatement(Node *p, List *q) : StatementT<TryStatement>(p, q) {} 
};

class BreakStatement : public StatementT<BreakStatement> 
{
public:
  BreakStatement(Node *p, List *q) : StatementT<BreakStatement>(p, q) {} 
};

class ContinueStatement : public StatementT<ContinueStatement> 
{
public:
  ContinueStatement(Node *p, List *q) : StatementT<ContinueStatement>(p, q) {} 
};

class ReturnStatement : public StatementT<ReturnStatement> 
{
public:
  ReturnStatement(Node *p, List *q) : StatementT<ReturnStatement>(p, q) {} 
};

class GotoStatement : public StatementT<GotoStatement> 
{
public:
  GotoStatement(Node *p, List *q) : StatementT<GotoStatement>(p, q) {} 
};

class CaseStatement : public StatementT<CaseStatement> 
{
public:
  CaseStatement(Node *p, List *q) : StatementT<CaseStatement>(p, q) {} 
};

class DefaultStatement : public StatementT<DefaultStatement> 
{
public:
  DefaultStatement(Node *p, List *q) : StatementT<DefaultStatement>(p, q) {} 
};

class LabelStatement : public StatementT<LabelStatement> 
{
public:
  LabelStatement(Node *p, List *q) : StatementT<LabelStatement>(p, q) {} 
};

class ExprStatement : public StatementT<ExprStatement> 
{
public:
  ExprStatement(Node *p, List *q) : StatementT<ExprStatement>(p, q) {} 
};

class Expression : public List
{
public:
  Expression(Node *p, List *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

template <typename T>
class ExpressionT : public List
{
public:
  ExpressionT(Node *p, List *q) : List(p, q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(static_cast<T *>(this));}
};

class AssignExpr : public ExpressionT<AssignExpr> 
{
public:
  AssignExpr(Node *p, List *q) : ExpressionT<AssignExpr>(p, q) {} 
};

class CondExpr : public ExpressionT<CondExpr> 
{
public:
  CondExpr(Node *p, List *q) : ExpressionT<CondExpr>(p, q) {} 
};

class InfixExpr : public ExpressionT<InfixExpr> 
{
public:
  InfixExpr(Node *p, List *q) : ExpressionT<InfixExpr>(p, q) {} 
};

class PmExpr : public ExpressionT<PmExpr> 
{
public:
  PmExpr(Node *p, List *q) : ExpressionT<PmExpr>(p, q) {} 
};

class CastExpr : public ExpressionT<CastExpr> 
{
public:
  CastExpr(Node *p, List *q) : ExpressionT<CastExpr>(p, q) {} 
};

class UnaryExpr : public ExpressionT<UnaryExpr> 
{
public:
  UnaryExpr(Node *p, List *q) : ExpressionT<UnaryExpr>(p, q) {} 
};

class ThrowExpr : public ExpressionT<ThrowExpr> 
{
public:
  ThrowExpr(Node *p, List *q) : ExpressionT<ThrowExpr>(p, q) {} 
};

class SizeofExpr : public ExpressionT<SizeofExpr> 
{
public:
  SizeofExpr(Node *p, List *q) : ExpressionT<SizeofExpr>(p, q) {} 
};

class TypeidExpr : public ExpressionT<TypeidExpr> 
{
public:
  TypeidExpr(Node *p, List *q) : ExpressionT<TypeidExpr>(p, q) {} 
};

class TypeofExpr : public ExpressionT<TypeofExpr> 
{
public:
  TypeofExpr(Node *p, List *q) : ExpressionT<TypeofExpr>(p, q) {} 
};

class NewExpr : public ExpressionT<NewExpr> 
{
public:
  NewExpr(Node *p, List *q) : ExpressionT<NewExpr>(p, q) {} 
};

class DeleteExpr : public ExpressionT<DeleteExpr> 
{
public:
  DeleteExpr(Node *p, List *q) : ExpressionT<DeleteExpr>(p, q) {} 
};

class ArrayExpr : public ExpressionT<ArrayExpr> 
{
public:
  ArrayExpr(Node *p, List *q) : ExpressionT<ArrayExpr>(p, q) {} 
};

class FuncallExpr : public ExpressionT<FuncallExpr> 
{
public:
  FuncallExpr(Node *p, List *q) : ExpressionT<FuncallExpr>(p, q) {} 
};

class PostfixExpr : public ExpressionT<PostfixExpr> 
{
public:
  PostfixExpr(Node *p, List *q) : ExpressionT<PostfixExpr>(p, q) {} 
};

class DotMemberExpr : public ExpressionT<DotMemberExpr> 
{
public:
  DotMemberExpr(Node *p, List *q) : ExpressionT<DotMemberExpr>(p, q) {} 
};

class ArrowMemberExpr : public ExpressionT<ArrowMemberExpr> 
{
public:
  ArrowMemberExpr(Node *p, List *q) : ExpressionT<ArrowMemberExpr>(p, q) {} 
};

class ParenExpr : public ExpressionT<ParenExpr> 
{
public:
  ParenExpr(Node *p, List *q) : ExpressionT<ParenExpr>(p, q) {} 
};

//. Return the encoded name of the given node.
//. This node should be either an PT::Identifier, or a PT::Name.
Encoding name(Node const *);

}
}

#endif
