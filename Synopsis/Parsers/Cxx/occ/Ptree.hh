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

#include <iosfwd>
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
  std::string string() const;

    void Display();
    void Display2(std::ostream&);
    virtual void Print(std::ostream&, int, int) = 0;
    int Write(std::ostream&);
    virtual int Write(std::ostream&, int) = 0;
    void PrintIndent(std::ostream&, int);

    char* GetPosition() { return data.leaf.position; }
    int GetLength() { return data.leaf.length; }

    Ptree* Car() { return data.nonleaf.child; }
    Ptree* Cdr() { return data.nonleaf.next; }
    Ptree* Cadr() { return Cdr()->Car(); }
    Ptree* Cddr() { return Cdr()->Cdr(); }
    Ptree* Ca_ar();
    void SetCar(Ptree* p) { data.nonleaf.child = p; }
    void SetCdr(Ptree* p) { data.nonleaf.next = p; }

    char* LeftMost();
    char* RightMost();

    virtual int What();
    bool IsA(int);
    bool IsA(int, int);
    bool IsA(int, int, int);

    virtual Ptree* Translate(Walker*);
    virtual void Typeof(Walker*, TypeInfo&);

    virtual char* GetEncodedType();
    virtual char* GetEncodedName();

    Ptree* Last() { return Last(this); }
    Ptree* First() { return First(this); }
    Ptree* Rest() { return Rest(this); }
    Ptree* Second() { return Second(this); }
    Ptree* Third() { return Third(this); }
    Ptree* Nth(int n) { return Nth(this, n); }
    int Length() { return Length(this); }
    Ptree* ListTail(int n) { return ListTail(this, n); }

// static members

    static bool Eq(Ptree*, char);
    static bool Eq(Ptree*, char*);
    static bool Eq(Ptree*, char*, int);
    static bool Eq(Ptree*, Ptree*);
    static bool Equiv(Ptree*, Ptree*);
    static bool Equal(Ptree*, Ptree*);
    static Ptree* Last(Ptree*);
    static Ptree* First(Ptree*);
    static Ptree* Rest(Ptree*);
    static Ptree* Second(Ptree*);
    static Ptree* Third(Ptree*);
    static Ptree* Nth(Ptree*, int);
    static int Length(Ptree*);
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
    // if this is true, Print() shows an encoded type and name.
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

inline std::ostream& operator << (std::ostream& s, Ptree* p)
{
    p->Write(s);
    return s;
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
  Leaf(Token &);
  bool IsLeaf() const { return true;}
  
  virtual void write(std::ostream &) const;

  void Print(std::ostream&, int, int);
  int Write(std::ostream&, int);
};

class NonLeaf : public Ptree 
{
public:
  NonLeaf(Ptree*, Ptree*);
  bool IsLeaf() const { return false;}

  virtual void write(std::ostream &) const;

  void Print(std::ostream&, int, int);
  int Write(std::ostream&, int);
  void PrintWithEncodeds(std::ostream&, int, int);
  
protected:
  bool TooDeep(std::ostream&, int);
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

  void Print(std::ostream &, int, int);
};

// error messages

void OCXXMOP MopErrorMessage(char* method_name, char* msg);
void OCXXMOP MopErrorMessage2(char* msg1, char* msg2);
void OCXXMOP MopWarningMessage(char* method_name, char* msg);
void OCXXMOP MopWarningMessage2(char* msg1, char* msg2);
void OCXXMOP MopMoreWarningMessage(char* msg1, char* msg2 = 0);

#endif
