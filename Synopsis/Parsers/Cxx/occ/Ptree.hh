//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _Ptree_hh
#define _Ptree_hh

#include <ostream>
#include <iterator>
#include "types.h"

class Walker;
class TypeInfo;
class Token;
class Encoding;

class OCXXMOP Ptree : public LightObject 
{
public:
    virtual bool IsLeaf() const = 0;
    bool Eq(char);
    bool Eq(char*);
    bool Eq(char*, int);
    bool Eq(Ptree* p) { return Eq(this, p); }

  //. write the part of the source code this node
  //. references to the given output stream
  virtual void write(std::ostream &) const = 0;
  //. return the start address of this Ptree in the buffer
  const char *begin() const;
  //. return the one-past-the-end address of this Ptree in the buffer
  const char *end() const;
  //. return a copy of the region of the buffer this ptree represents
  std::string string() const;
  //. provide an annotated view of the ptree, for debugging purposes
  void print(std::ostream &) const;
  virtual void print(std::ostream &, size_t indent, size_t depth) const = 0;

    int Write(std::ostream&);
    virtual int Write(std::ostream&, int) = 0;

    char* GetPosition() const { return data.leaf.position; }
    int GetLength() const { return data.leaf.length; }

    const Ptree* Car() const { return data.nonleaf.child; }
    Ptree* Car() { return data.nonleaf.child; }
    const Ptree* Cdr() const { return data.nonleaf.next; }
    Ptree* Cdr() { return data.nonleaf.next; }
    Ptree* Cadr() { return Cdr()->Car(); }
    Ptree* Cddr() { return Cdr()->Cdr(); }
    Ptree* Ca_ar();
    void SetCar(Ptree* p) { data.nonleaf.child = p; }
    void SetCdr(Ptree* p) { data.nonleaf.next = p; }

    virtual int What();
    bool IsA(int);
    bool IsA(int, int);
    bool IsA(int, int, int);

    virtual Ptree* Translate(Walker*);
    virtual void Typeof(Walker*, TypeInfo&);

    virtual char* GetEncodedType() const;
    virtual char* GetEncodedName() const;

    const Ptree* Last() const { return Last(this); }
    Ptree* Last() { return Last(this); }
    const Ptree* First() const { return First(this); }
    Ptree* First() { return First(this); }
    const Ptree* Rest() const { return Rest(this); }
    Ptree* Rest() { return Rest(this); }
    const Ptree* Second() const { return Second(this); }
    Ptree* Second() { return Second(this); }
    const Ptree* Third() const { return Third(this); }
    Ptree* Third() { return Third(this); }
    const Ptree* Nth(int n) const { return Nth(this, n); }
    Ptree* Nth(int n) { return Nth(this, n); }
    int Length() const { return Length(this); }
    Ptree* ListTail(int n) { return ListTail(this, n); }

// static members

    static bool Eq(Ptree*, char);
    static bool Eq(Ptree*, char*);
    static bool Eq(Ptree*, char*, int);
    static bool Eq(Ptree*, Ptree*);
    static bool Equiv(Ptree*, Ptree*);
    static bool Equal(Ptree*, Ptree*);
    static const Ptree* Last(const Ptree*);
    static Ptree* Last(Ptree*);
    static const Ptree* First(const Ptree*);
    static Ptree* First(Ptree*);
    static const Ptree* Rest(const Ptree*);
    static Ptree* Rest(Ptree*);
    static const Ptree* Second(const Ptree*);
    static Ptree* Second(Ptree*);
    static const Ptree* Third(const Ptree*);
    static Ptree* Third(Ptree*);
    static const Ptree* Nth(const Ptree*, int);
    static Ptree* Nth(Ptree*, int);
    static int Length(const Ptree*);
    static Ptree* ListTail(Ptree*, int);

    static Ptree* Cons(Ptree*, Ptree*);
    static Ptree* List();
    static Ptree* List(Ptree*);
    static Ptree* List(Ptree*, Ptree*);
    static Ptree* List(Ptree*, Ptree*, Ptree*);
    static Ptree* List(Ptree*, Ptree*, Ptree*, Ptree*);
    static Ptree* List(Ptree*, Ptree*, Ptree*, Ptree*, Ptree*);
    static Ptree* List(Ptree*, Ptree*, Ptree*, Ptree*, Ptree*, Ptree*);
    static Ptree* List(Ptree*, Ptree*, Ptree*, Ptree*, Ptree*, Ptree*,
		       Ptree*);
    static Ptree* List(Ptree*, Ptree*, Ptree*, Ptree*, Ptree*, Ptree*,
		       Ptree*, Ptree*);
    static Ptree* CopyList(Ptree*);
    static Ptree* Append(Ptree*, Ptree*);
    static Ptree* ReplaceAll(Ptree*, Ptree*, Ptree*);
    static Ptree* Subst(Ptree*, Ptree*, Ptree*);
    static Ptree* Subst(Ptree*, Ptree*, Ptree*, Ptree*, Ptree*);
    static Ptree* Subst(Ptree*, Ptree*, Ptree*, Ptree*,
			Ptree*, Ptree*, Ptree*);
    static Ptree* ShallowSubst(Ptree*, Ptree*, Ptree*);
    static Ptree* ShallowSubst(Ptree*, Ptree*, Ptree*, Ptree*, Ptree*);
    static Ptree* ShallowSubst(Ptree*, Ptree*, Ptree*, Ptree*,
			       Ptree*, Ptree*, Ptree*);
    static Ptree* ShallowSubst(Ptree*, Ptree*, Ptree*, Ptree*,
			       Ptree*, Ptree*, Ptree*, Ptree*, Ptree*);
    static Ptree* SubstSublist(Ptree*, Ptree*, Ptree*);

    /* they cause side-effect */
    static Ptree* Snoc(Ptree*, Ptree*);
    static Ptree* Nconc(Ptree*, Ptree*);
    static Ptree* Nconc(Ptree*, Ptree*, Ptree*);

  static void print_indent(std::ostream &, size_t);

// in pattern.cc
public:
    static bool Match(Ptree*, char*, ...);
    static Ptree* Make(const char* pat, ...);
    static Ptree* MakeStatement(const char* pat, ...);
    static Ptree* GenSym();

    static Ptree* qMake(char*);
    static Ptree* qMakeStatement(char*);

    bool Reify(unsigned int&);
    bool Reify(char*&);

    static char* IntegerToString(sint, int&);	// not documented

private:
    static char* MatchPat(Ptree*, char*);
    static char* MatchList(Ptree*, char*);
    static char* MatchWord(Ptree*, char*);

public:
    // if this is true, print() shows an encoded type and name.
    static bool show_encoded;

protected:
    union {
	struct {
	    Ptree* child;
	    Ptree* next;
	} nonleaf;
	struct {
	    char* position;
	    int  length;
	} leaf;
    }data;

    friend class NonLeaf;
};

inline std::ostream &operator << (std::ostream &os, Ptree *p)
{
  // FIXME: this doesn't take into account any Ptree / AST modifications
  //        i.e. it just refers to the unmodified buffer
  //        Once we allow modifications to the tree we need to look up
  //        overlapping buffer replacements and inject them into the stream
  //        as appropriate
  std::copy(p->begin(), p->end(), std::ostream_iterator<char>(os));
  return os;
}

// in ptree.cc

class OCXXMOP PtreeIter : public LightObject {
public:
    PtreeIter(Ptree* p) { ptree = p; }
    Ptree* operator () () { return Pop(); }
    Ptree* Pop();
    bool Next(Ptree*&);
    void Reset(Ptree* p) { ptree = p; }

    Ptree* operator * () { return This(); }
    Ptree* operator ++ () { Pop(); return This(); }
    Ptree* operator ++ (int) { return Pop(); }	// postfix
    Ptree* This();
    bool Empty() { return bool(ptree == 0); }

private:
    Ptree* ptree;
};

class OCXXMOP PtreeArray : public LightObject {
public:
    PtreeArray(int = 8);
    uint Number() { return num; }
    Ptree*& operator [] (uint index) { return Ref(index); }
    Ptree*& Ref(uint index);
    void Append(Ptree*);
    void Clear() { num = 0; }
    Ptree* All();

private:
    uint num, size;
    Ptree** array;
    Ptree* default_buf[8];
};

// PtreeHead is used to implement Ptree::qMake()
// The implementation is in pattern.cc

class OCXXMOP PtreeHead : public LightObject {
public:
    PtreeHead() { ptree = 0; }
    operator Ptree* () { return ptree; }
    PtreeHead& operator + (Ptree*);
    PtreeHead& operator + (const char*);
    PtreeHead& operator + (char*);
    PtreeHead& operator + (char);
    PtreeHead& operator + (int);

private:
    Ptree* Append(Ptree*, Ptree*);
    Ptree* Append(Ptree*, char*, int);

private:
    Ptree* ptree;
};

class Leaf : public Ptree 
{
public:
  Leaf(char *, int);
  Leaf(const Token &);
  bool IsLeaf() const { return true;}
  
  virtual void write(std::ostream &) const;
  virtual void print(std::ostream &, size_t, size_t) const;

  int Write(std::ostream&, int);
};

class NonLeaf : public Ptree 
{
public:
  NonLeaf(Ptree*, Ptree*);
  bool IsLeaf() const { return false;}

  virtual void write(std::ostream &) const;
  virtual void print(std::ostream &, size_t, size_t) const;

  int Write(std::ostream&, int);  
protected:
  bool too_deep(std::ostream&, size_t) const;
  void print_encoded(std::ostream &, size_t, size_t) const;
};

class CommentedLeaf : public Leaf 
{
public:
  CommentedLeaf(Token &tk, Ptree *c = 0) : Leaf(tk) { comments = c;}
  CommentedLeaf(char *p, int l, Ptree *c = 0) : Leaf(p, l) { comments = c;}
  Ptree *GetComments() { return comments;}
  void SetComments(Ptree *c) { comments = c;}
private:
  Ptree *comments;
};

// class DupLeaf is used by Ptree::Make() and QuoteClass (qMake()).
// The string given to the constructors are duplicated.

class DupLeaf : public CommentedLeaf 
{
public:
  DupLeaf(const char *, int);
  DupLeaf(char *, int, char *, int);

  virtual void print(std::ostream &, size_t, size_t) const;
};

// error messages

void OCXXMOP MopErrorMessage(char* method_name, char* msg);
void OCXXMOP MopErrorMessage2(char* msg1, char* msg2);
void OCXXMOP MopWarningMessage(char* method_name, char* msg);
void OCXXMOP MopWarningMessage2(char* msg1, char* msg2);
void OCXXMOP MopMoreWarningMessage(char* msg1, char* msg2 = 0);

#endif
