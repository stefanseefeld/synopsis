//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Node_hh
#define _PTree_Node_hh

#include <ostream>
#include <iterator>
#include "types.h"

class Walker;
class TypeInfo;
class Token;
class Encoding;

namespace PTree
{

class Node : public LightObject 
{
public:
    virtual bool IsLeaf() const = 0;
    bool Eq(char);
    bool Eq(char*);
    bool Eq(char*, int);
    bool Eq(Node *p) { return Eq(this, p); }

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

    const Node *Car() const { return data.nonleaf.child; }
    Node *Car() { return data.nonleaf.child; }
    const Node *Cdr() const { return data.nonleaf.next; }
    Node *Cdr() { return data.nonleaf.next; }
    Node *Cadr() { return Cdr()->Car(); }
    Node *Cddr() { return Cdr()->Cdr(); }
    Node *Ca_ar();
    void SetCar(Node *p) { data.nonleaf.child = p; }
    void SetCdr(Node *p) { data.nonleaf.next = p; }

    virtual int What();
    bool IsA(int);
    bool IsA(int, int);
    bool IsA(int, int, int);

    virtual Node *Translate(Walker*);
    virtual void Typeof(Walker*, TypeInfo&);

    virtual char* GetEncodedType() const;
    virtual char* GetEncodedName() const;

    const Node *Last() const { return Last(this); }
    Node *Last() { return Last(this); }
    const Node *First() const { return First(this); }
    Node *First() { return First(this); }
    const Node *Rest() const { return Rest(this); }
    Node *Rest() { return Rest(this); }
    const Node *Second() const { return Second(this); }
    Node *Second() { return Second(this); }
    const Node *Third() const { return Third(this); }
    Node *Third() { return Third(this); }
    const Node *Nth(int n) const { return Nth(this, n); }
    Node *Nth(int n) { return Nth(this, n); }
    int Length() const { return Length(this); }
    Node *ListTail(int n) { return ListTail(this, n); }

// static members

    static bool Eq(Node *, char);
    static bool Eq(Node *, char*);
    static bool Eq(Node *, char*, int);
    static bool Eq(Node *, Node *);
    static bool Equiv(Node *, Node *);
    static bool Equal(Node *, Node *);
    static const Node *Last(const Node *);
    static Node *Last(Node *);
    static const Node *First(const Node *);
    static Node *First(Node *);
    static const Node *Rest(const Node *);
    static Node *Rest(Node *);
    static const Node *Second(const Node *);
    static Node *Second(Node *);
    static const Node *Third(const Node *);
    static Node *Third(Node *);
    static const Node *Nth(const Node *, int);
    static Node *Nth(Node *, int);
    static int Length(const Node *);
    static Node *ListTail(Node *, int);

    static Node *Cons(Node *, Node *);
    static Node *List();
    static Node *List(Node *);
    static Node *List(Node *, Node *);
    static Node *List(Node *, Node *, Node *);
    static Node *List(Node *, Node *, Node *, Node *);
    static Node *List(Node *, Node *, Node *, Node *, Node *);
    static Node *List(Node *, Node *, Node *, Node *, Node *, Node *);
    static Node *List(Node *, Node *, Node *, Node *, Node *, Node *,
		       Node *);
    static Node *List(Node *, Node *, Node *, Node *, Node *, Node *,
		       Node *, Node *);
    static Node *CopyList(Node *);
    static Node *Append(Node *, Node *);
    static Node *ReplaceAll(Node *, Node *, Node *);
    static Node *Subst(Node *, Node *, Node *);
    static Node *Subst(Node *, Node *, Node *, Node *, Node *);
    static Node *Subst(Node *, Node *, Node *, Node *,
			Node *, Node *, Node *);
    static Node *ShallowSubst(Node *, Node *, Node *);
    static Node *ShallowSubst(Node *, Node *, Node *, Node *, Node *);
    static Node *ShallowSubst(Node *, Node *, Node *, Node *,
			       Node *, Node *, Node *);
    static Node *ShallowSubst(Node *, Node *, Node *, Node *,
			       Node *, Node *, Node *, Node *, Node *);
    static Node *SubstSublist(Node *, Node *, Node *);

    /* they cause side-effect */
    static Node *Snoc(Node *, Node *);
    static Node *Nconc(Node *, Node *);
    static Node *Nconc(Node *, Node *, Node *);

  static void print_indent(std::ostream &, size_t);

// in pattern.cc
public:
    static bool Match(Node *, char*, ...);
    static Node *Make(const char* pat, ...);
    static Node *MakeStatement(const char* pat, ...);
    static Node *GenSym();

    static Node *qMake(char*);
    static Node *qMakeStatement(char*);

    bool Reify(unsigned int&);
    bool Reify(char*&);

    static char* IntegerToString(sint, int&);	// not documented

private:
    static char* MatchPat(Node *, char*);
    static char* MatchList(Node *, char*);
    static char* MatchWord(Node *, char*);

public:
    // if this is true, print() shows an encoded type and name.
    static bool show_encoded;

protected:
    union {
	struct {
	    Node *child;
	    Node *next;
	} nonleaf;
	struct {
	    char* position;
	    int  length;
	} leaf;
    }data;

    friend class List;
};

inline std::ostream &operator << (std::ostream &os, Node *p)
{
  // FIXME: this doesn't take into account any Ptree / AST modifications
  //        i.e. it just refers to the unmodified buffer
  //        Once we allow modifications to the tree we need to look up
  //        overlapping buffer replacements and inject them into the stream
  //        as appropriate
  std::copy(p->begin(), p->end(), std::ostream_iterator<char>(os));
  return os;
}

class Iterator : public LightObject 
{
public:
  Iterator(Node *p) { ptree = p; }
  Node *operator () () { return Pop(); }
  Node *Pop();
  bool Next(Node *&);
  void Reset(Node *p) { ptree = p;}

  Node *operator * () { return This();}
  Node *operator ++ () { Pop(); return This();}
  Node *operator ++ (int) { return Pop();}	// postfix
  Node *This() { return ptree ? ptree->Car() : 0;}
  bool Empty() { return ptree == 0;}

private:
  Node *ptree;
};

class Array : public LightObject 
{
public:
  Array(int = 8);
  uint Number() { return num; }
  Node *& operator [] (uint index) { return Ref(index); }
  Node *& Ref(uint index);
  void Append(Node *);
  void Clear() { num = 0; }
  Node *All();

private:
  uint num, size;
  Node ** array;
  Node *default_buf[8];
};

//. PtreeHead is used to implement Ptree::qMake()
//. The implementation is in pattern.cc
class Head : public LightObject 
{
public:
  Head() { ptree = 0; }
  operator Node *() { return ptree; }
  Head& operator + (Node *);
  Head& operator + (const char*);
  Head& operator + (char*);
  Head& operator + (char);
  Head& operator + (int);

private:
  Node *Append(Node *, Node *);
  Node *Append(Node *, char*, int);

  Node *ptree;
};

}

#endif
