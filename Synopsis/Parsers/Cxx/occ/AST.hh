//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _AST_hh
#define _AST_hh

#include "Ptree.hh"
#include "Lexer.hh"
#include "Buffer.hh"

class Encoding;

class LeafName : public CommentedLeaf {
public:
    LeafName(Token&);
    Ptree* Translate(Walker*);
    void Typeof(Walker*, TypeInfo&);
};

class LeafReserved : public CommentedLeaf {
public:
    LeafReserved(Token& t) : CommentedLeaf(t) {}
    LeafReserved(char* str, int len) : CommentedLeaf(str, len) {}
};

class LeafThis : public LeafReserved {
public:
    LeafThis(Token& t) : LeafReserved(t) {}
    int What();
    Ptree* Translate(Walker*);
    void Typeof(Walker*, TypeInfo&);
};

#define ResearvedWordDecl(w) \
class Leaf##w : public LeafReserved { \
public: \
    Leaf##w(Token& t) : LeafReserved(t) {} \
    Leaf##w(char* str, int len) : LeafReserved(str, len) {} \
    int What(); \
}

ResearvedWordDecl(AUTO);
ResearvedWordDecl(BOOLEAN);
ResearvedWordDecl(CHAR);
ResearvedWordDecl(WCHAR);
ResearvedWordDecl(CONST);
ResearvedWordDecl(DOUBLE);
ResearvedWordDecl(EXTERN);
ResearvedWordDecl(FLOAT);
ResearvedWordDecl(FRIEND);
ResearvedWordDecl(INLINE);
ResearvedWordDecl(INT);
ResearvedWordDecl(LONG);
ResearvedWordDecl(MUTABLE);
ResearvedWordDecl(NAMESPACE);
ResearvedWordDecl(PRIVATE);
ResearvedWordDecl(PROTECTED);
ResearvedWordDecl(PUBLIC);
ResearvedWordDecl(REGISTER);
ResearvedWordDecl(SHORT);
ResearvedWordDecl(SIGNED);
ResearvedWordDecl(STATIC);
ResearvedWordDecl(UNSIGNED);
ResearvedWordDecl(USING);
ResearvedWordDecl(VIRTUAL);
ResearvedWordDecl(VOID);
ResearvedWordDecl(VOLATILE);

ResearvedWordDecl(UserKeyword2);

#undef ResearvedWordDecl

class PtreeBrace : public NonLeaf {
public:
    PtreeBrace(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    PtreeBrace(Ptree* ob, Ptree* body, Ptree* cb)
	: NonLeaf(ob, Ptree::List(body, cb)) {}

  virtual void print(std::ostream &, size_t, size_t) const;

    int Write(std::ostream&, int);

    Ptree* Translate(Walker*);
};

class PtreeBlock : public PtreeBrace {
public:
    PtreeBlock(Ptree* p, Ptree* q) : PtreeBrace(p, q) {}
    PtreeBlock(Ptree* ob, Ptree* bdy, Ptree* cb) : PtreeBrace(ob, bdy, cb) {}

    Ptree* Translate(Walker*);
};

class PtreeClassBody : public PtreeBrace {
public:
    PtreeClassBody(Ptree* p, Ptree* q) : PtreeBrace(p, q) {}
    PtreeClassBody(Ptree* ob, Ptree* bdy, Ptree* cb)
						: PtreeBrace(ob, bdy, cb) {}

    Ptree* Translate(Walker*);
};

class PtreeTypedef : public NonLeaf {
public:
    PtreeTypedef(Ptree* p) : NonLeaf(p, 0) {}
    PtreeTypedef(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeTemplateDecl : public NonLeaf {
public:
    PtreeTemplateDecl(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    PtreeTemplateDecl(Ptree* p) : NonLeaf(p, 0) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeTemplateInstantiation : public NonLeaf {
public:
    PtreeTemplateInstantiation(Ptree* p) : NonLeaf(p, 0) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeExternTemplate : public NonLeaf {
public:
    PtreeExternTemplate(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    PtreeExternTemplate(Ptree* p) : NonLeaf(p, 0) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeMetaclassDecl : public NonLeaf {
public:
    PtreeMetaclassDecl(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeLinkageSpec : public NonLeaf {
public:
    PtreeLinkageSpec(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeNamespaceSpec : public NonLeaf {
public:
    PtreeNamespaceSpec(Ptree* p, Ptree* q) : NonLeaf(p, q) { comments = 0;}
    int What();
    Ptree* Translate(Walker*);

    Ptree* GetComments() { return comments; }
    void SetComments(Ptree* c) { comments = c; }

private:
    Ptree* comments;
};

class PtreeNamespaceAlias : public NonLeaf {
public:
    PtreeNamespaceAlias(Ptree* p, Ptree* q) : NonLeaf(p, q) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeUsing : public NonLeaf {
public:
    PtreeUsing(Ptree* p) : NonLeaf(p, 0) {}
    int What();
    Ptree* Translate(Walker*);
};

class PtreeDeclaration : public NonLeaf {
public:
    PtreeDeclaration(Ptree* p, Ptree* q) : NonLeaf(p, q) { comments = 0;}
    int What();
    Ptree* Translate(Walker*);

    Ptree* GetComments() { return comments; }
    void SetComments(Ptree* c) { comments = c; }

private:
    Ptree* comments;
};

class PtreeDeclarator : public NonLeaf {
public:
    PtreeDeclarator(Ptree*, Encoding&, Encoding&, Ptree*);
    PtreeDeclarator(Encoding&, Encoding&, Ptree*);
    PtreeDeclarator(Ptree*, Ptree*, Encoding&, Encoding&, Ptree*);
    PtreeDeclarator(Ptree*, Encoding&);
    PtreeDeclarator(Encoding&);
    PtreeDeclarator(PtreeDeclarator*, Ptree*, Ptree*);

  virtual void print(std::ostream &, size_t, size_t) const;

    int What();
    char* GetEncodedType() const;
    char* GetEncodedName() const;
    void SetEncodedType(char* t) { type = t; }
    Ptree* Name() { return declared_name; }

    Ptree* GetComments() { return comments; }
    void SetComments(Ptree* c) { comments = c; }

private:
    char* type;
    char* name;
    Ptree* declared_name;
    Ptree* comments;
};

class PtreeName : public NonLeaf {
public:
    PtreeName(Ptree*, Encoding&);

  void print(std::ostream &, size_t, size_t) const;

    int What();
    char* GetEncodedName() const;
    Ptree* Translate(Walker*);
    void Typeof(Walker*, TypeInfo&);

private:
    char* name;
};

class PtreeFstyleCastExpr : public NonLeaf {
public:
    PtreeFstyleCastExpr(Encoding&, Ptree*, Ptree*);
    PtreeFstyleCastExpr(char*, Ptree*, Ptree*);

  void print(std::ostream &, size_t, size_t) const;

    int What();
    char* GetEncodedType() const;
    Ptree* Translate(Walker*);
    void Typeof(Walker*, TypeInfo&);

private:
    char* type;
};

class PtreeClassSpec : public NonLeaf {
public:
    PtreeClassSpec(Ptree*, Ptree*, Ptree*);
    PtreeClassSpec(Ptree*, Ptree*, Ptree*, char*);
    int What();
    Ptree* Translate(Walker*);
    char* GetEncodedName() const;
    Ptree* GetComments();

private:
    char* encoded_name;
    Ptree* comments;

friend class Parser;
};

class PtreeEnumSpec : public NonLeaf {
public:
    PtreeEnumSpec(Ptree*);
    int What();
    Ptree* Translate(Walker*);
    char* GetEncodedName() const;

private:
    char* encoded_name;

friend class Parser;
};

class PtreeAccessSpec : public NonLeaf {
public:
    PtreeAccessSpec(Ptree*, Ptree*);
    int What();
    Ptree* Translate(Walker*);
};

class PtreeAccessDecl : public NonLeaf {
public:
    PtreeAccessDecl(Ptree*, Ptree*);
    int What();
    Ptree* Translate(Walker*);
};

class PtreeUserAccessSpec : public NonLeaf {
public:
    PtreeUserAccessSpec(Ptree*, Ptree*);
    int What();
    Ptree* Translate(Walker*);
};

class PtreeUserdefKeyword : public NonLeaf {
public:
    PtreeUserdefKeyword(Ptree*, Ptree*);
    int What();
};

#define PtreeStatementDecl(s)\
class Ptree##s##Statement : public NonLeaf {\
public:\
    Ptree##s##Statement(Ptree* p, Ptree* q) : NonLeaf(p, q) {}\
    int What();\
    Ptree* Translate(Walker*);\
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
class Ptree##n##Expr : public NonLeaf {\
public:\
    Ptree##n##Expr(Ptree* p, Ptree* q) : NonLeaf(p, q) {}\
    int What();\
    Ptree* Translate(Walker*);\
    void Typeof(Walker*, TypeInfo&);\
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

#endif
