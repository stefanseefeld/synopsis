//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "PTree/Node.hh"
#include "PTree/List.hh"
#include "Lexer.hh"
#include "Walker.hh"
#include "TypeInfo.hh"
#include "Encoding.hh"
#include "Buffer.hh"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <stdexcept>

#if defined(_MSC_VER) || defined(IRIX_CC) || defined(__GLIBC__)
#include <stdlib.h>		// for exit()
#endif

using namespace PTree;

bool Node::show_encoded = false;

const char *Node::begin() const
{
  if (IsLeaf()) return GetPosition();
  else
  {
    for (const Node *p = this; p; p = p->Cdr())
    {
      const char *b = p->Car() ? p->Car()->begin() : 0;
      if (b) return b;
    }
    return 0;
  }
}

const char *Node::end() const
{
  if (IsLeaf()) return GetPosition() + GetLength();
  else
  {
    int n = Length();
    while(n > 0)
    {
      const char *e = Nth(--n)->end();
      if (e) return e;
    }    
    return 0;
  }
}

std::string Node::string() const
{
  std::ostringstream oss;
  write(oss);
  return oss.str();
};

void Node::print(std::ostream &os) const
{
  print(os, 0, 0);
  os.put('\n');
}

// error messages

void MopErrorMessage(char* where, char* msg)
{
    std::cerr << "MOP error: in " << where << ", " << msg << '\n';
    exit(1);
}

void MopErrorMessage2(char* msg1, char* msg2)
{
    std::cerr << "MOP error: " << msg1 << msg2 << '\n';
    exit(1);
}

void MopWarningMessage(char* where, char* msg)
{
    std::cerr << "MOP warning: in " << where << ", " << msg << '\n';
}

void MopWarningMessage2(char* msg1, char* msg2)
{
    std::cerr << "MOP warning: " << msg1 << msg2 << '\n';
}

void MopMoreWarningMessage(char* msg1, char* msg2)
{
    std::cerr << "             " << msg1;
    if(msg2 != 0)
	std::cerr << msg2;

    std::cerr << '\n';
}

// class Node

int Node::Write(std::ostream& s)
{
    if(this == 0)
	return 0;
    else
	return Write(s, 0);
}

bool Node::Eq(char c)
{
  if(this == 0) return false;
  else
    return(IsLeaf() && GetLength() == 1 && *GetPosition() == c);
}

bool Node::Eq(char* str)
{
    if(this == 0)
	return false;
    else if(IsLeaf()){
	char* p = GetPosition();
	int n = GetLength();
	int i;
	for(i = 0; i < n; ++i)
	    if(p[i] != str[i] || str[i] == '\0')
		return false;

	return bool(str[i] == '\0');
    }
    else
	return false;
}

bool Node::Eq(char* str, int len)
{
    if(this != 0 && IsLeaf()){
	char* p = GetPosition();
	int n = GetLength();
	if(n == len){
	    int i;
	    for(i = 0; i < n; ++i)
		if(p[i] != str[i])
		    return false;

	    return true;
	}
    }

    return false;
}

Node *Node::Ca_ar()		// compute Caa..ar
{
    Node *p = this;
    while(p != 0 && !p->IsLeaf())
	p = p->Car();

    return p;
}

int Node::What()
{
  return Token::BadToken;
}

bool Node::IsA(int kind)
{
    if(this == 0)
	return false;
    else
	return bool(What() == kind);
}

bool Node::IsA(int kind1, int kind2)
{
    if(this == 0)
	return false;
    else{
	int k = What();
	return bool(k == kind1 || k == kind2);
    }
}

bool Node::IsA(int kind1, int kind2, int kind3)
{
    if(this == 0)
	return false;
    else{
	int k = What();
	return bool(k == kind1 || k == kind2 || k == kind3);
    }
}

Node *Node::Translate(Walker* w)
{
    return w->TranslatePtree(this);
}

void Node::Typeof(Walker* w, TypeInfo& t)
{
    w->TypeofPtree(this, t);
}

char* Node::GetEncodedType() const
{
    return 0;
}

char* Node::GetEncodedName() const
{
    return 0;
}


// static members

bool Node::Eq(Node *p, char c)
{
    return p->Eq(c);
}

bool Node::Eq(Node *p, char* str)
{
    return p->Eq(str);
}

bool Node::Eq(Node *p, char* str, int len)
{
    return p->Eq(str, len);
}

bool Node::Eq(Node *p, Node *q)
{
    if(p == q)
	return true;
    else if(p == 0 || q == 0)
	return false;
    else if(p->IsLeaf() && q->IsLeaf()){
	int plen = p->GetLength();
	int qlen = q->GetLength();
	if(plen == qlen){
	    char* pstr = p->GetPosition();
	    char* qstr = q->GetPosition();
	    while(--plen >= 0)
		if(pstr[plen] != qstr[plen])
		    return false;

	    return true;
	}
    }

    return false;
}

/*
  Equiv() returns true even if p and q are lists and all the elements
  are equal respectively.
*/
bool Node::Equiv(Node *p, Node *q)
{
    if(p == q)
	return true;
    else if(p == 0 || q == 0)
	return false;
    else if(p->IsLeaf() || q->IsLeaf())
	return Eq(p, q);
    else{
	while(p != 0 && q != 0)
	    if(p->Car() != q->Car())
		return false;
	    else{
		p = p->Cdr();
		q = q->Cdr();
	    }

	return p == 0 && q == 0;
    }
}

bool Node::Equal(Node *p, Node *q)
{
    if(p == q)
	return true;
    else if(p == 0 || q == 0)
	return false;
    else if(p->IsLeaf() || q->IsLeaf())
	return Eq(p, q);
    else
	return Equal(p->Car(), q->Car()) && Equal(p->Cdr(), q->Cdr());
}

const Node *Node::Last(const Node *p)	// return the last cons cell.
{
  const Node *next;
  if(!p) return 0;

  while((next = p->Cdr())) p = next;
  return p;
}

Node *Node::Last(Node *p)	// return the last cons cell.
{
    Node *next;
    if(p == 0)
	return 0;

    while((next = p->Cdr()) != 0)
	p = next;

    return p;
}

const Node *Node::First(const Node *p)
{
  return p ? p->Car() : 0;
}

Node *Node::First(Node *p)
{
    if(p != 0)
	return p->Car();
    else
	return p;
}

const Node *Node::Rest(const Node *p)
{
  return p ? p->Cdr() : 0;
}

Node *Node::Rest(Node *p)
{
    if(p != 0)
	return p->Cdr();
    else
	return p;
}

const Node *Node::Second(const Node *p)
{
  if(p)
  {
    p = p->Cdr();
    if(p) return p->Car();
  }
  
  return 0;
}

Node *Node::Second(Node *p)
{
    if(p != 0){
	p = p->Cdr();
	if(p != 0)
	    return p->Car();
    }

    return p;
}

const Node *Node::Third(const Node *p)
{
  if(p)
  {
    p = p->Cdr();
    if(p)
    {
      p = p->Cdr();
      if(p) return p->Car();
    }
  }

  return p;
}

Node *Node::Third(Node *p)
{
    if(p != 0){
	p = p->Cdr();
	if(p != 0){
	    p = p->Cdr();
	    if(p != 0)
		return p->Car();
	}
    }

    return p;
}

/*
  Nth(lst, 0) is equivalent to First(lst).
*/
const Node *Node::Nth(const Node *p, int n)
{
  while(p && n-- > 0) p = p->Cdr();
  return p ? p->Car() : 0;
}

Node *Node::Nth(Node *p, int n)
{
    while(p != 0 && n-- > 0)
	p = p->Cdr();

    if(p != 0)
	return p->Car();
    else
	return p;
}

/*
  Length() returns a negative number if p is not a list.
*/
int Node::Length(const Node *p)
{
    int i = 0;

    if(p != 0 && p->IsLeaf())
	return -2;	/* p is not a pair */

    while(p != 0){
	++i;
	if(p->IsLeaf())
	    return -1;	/* p is a pair, but not a list. */
	else
	    p = p->Cdr();
    }

    return i;
}

Node *Node::ListTail(Node *p, int k)
{
    while(p != 0 && k-- > 0)
	p = p->Cdr();

    return p;
}

Node *Node::Cons(Node *p, Node *q)
{
  return new PTree::List(p, q);
}

Node *Node::List(Node *p)
{
    return new PTree::List(p, 0);
}

Node *Node::List()
{
    return 0;
}

Node *Node::List(Node *p, Node *q)
{
    return new PTree::List(p, new PTree::List(q, 0));
}

Node *Node::List(Node *p1, Node *p2, Node *p3)
{
    return new PTree::List(p1, new PTree::List(p2, new PTree::List(p3, 0)));
}

Node *Node::List(Node *p1, Node *p2, Node *p3, Node *p4)
{
    return new PTree::List(p1, List(p2, p3, p4));
}

Node *Node::List(Node *p1, Node *p2, Node *p3, Node *p4, Node *p5)
{
    return Nconc(List(p1, p2), List(p3, p4, p5));
}

Node *Node::List(Node *p1, Node *p2, Node *p3, Node *p4, Node *p5,
		   Node *p6)
{
    return Nconc(List(p1, p2, p3), List(p4, p5, p6));
}

Node *Node::List(Node *p1, Node *p2, Node *p3, Node *p4, Node *p5,
		   Node *p6, Node *p7)
{
    return Nconc(List(p1, p2, p3), List(p4, p5, p6, p7));
}

Node *Node::List(Node *p1, Node *p2, Node *p3, Node *p4, Node *p5,
		   Node *p6, Node *p7, Node *p8)
{
    return Nconc(List(p1, p2, p3, p4), List(p5, p6, p7, p8));
}

Node *Node::CopyList(Node *p)
{
    return Append(p, 0);
}

//   q may be a leaf
//
Node *Node::Append(Node *p, Node *q)
{
    Node *result, *tail;

    if(p == 0)
	if(q->IsLeaf())
	    return Cons(q, 0);
	else
	    return q;

    result = tail = Cons(p->Car(), 0);
    p = p->Cdr();
    while(p != 0){
	Node *cell = Cons(p->Car(), 0);
	tail->SetCdr(cell);
	tail = cell;
	p = p->Cdr();
    }

    if(q != 0 && q->IsLeaf())
	tail->SetCdr(Cons(q, 0));
    else
	tail->SetCdr(q);

    return result;
}

/*
  ReplaceAll() substitutes SUBST for all occurences of ORIG in LIST.
  It recursively searches LIST for ORIG.
*/
Node *Node::ReplaceAll(Node *list, Node *orig, Node *subst)
{
    if(Eq(list, orig))
	return subst;
    else if(list == 0 || list->IsLeaf())
	return list;
    else{
	Array newlist;
	bool changed = false;
	Node *rest = list;
	while(rest != 0){
	    Node *p = rest->Car();
	    Node *q = ReplaceAll(p, orig, subst);
	    newlist.Append(q);
	    if(p != q)
		changed = true;

	    rest = rest->Cdr();
	}

	if(changed)
	    return newlist.All();
	else
	    return list;
    }
}

Node *Node::Subst(Node *newone, Node *old, Node *tree)
{
    if(old == tree)
	return newone;
    else if(tree== 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head = tree->Car();
	Node *head2 = Subst(newone, old, head);
	Node *tail = tree->Cdr();
	Node *tail2 = (tail == 0) ? tail : Subst(newone, old, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Node *Node::Subst(Node *newone1, Node *old1, Node *newone2, Node *old2,
		    Node *tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head = tree->Car();
	Node *head2 = Subst(newone1, old1, newone2, old2, head);
	Node *tail = tree->Cdr();
	Node *tail2 = (tail == 0) ? tail
			: Subst(newone1, old1, newone2, old2, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Node *Node::Subst(Node *newone1, Node *old1, Node *newone2, Node *old2,
		    Node *newone3, Node *old3, Node *tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(old3 == tree)
	return newone3;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head = tree->Car();
	Node *head2 = Subst(newone1, old1, newone2, old2,
			     newone3, old3, head);
	Node *tail = tree->Cdr();
	Node *tail2 = (tail == 0) ? tail :
			Subst(newone1, old1, newone2, old2,
			      newone3, old3, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

// ShallowSubst() doesn't recursively apply substitution to a subtree.

Node *Node::ShallowSubst(Node *newone, Node *old, Node *tree)
{
    if(old == tree)
	return newone;
    else if(tree== 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head, *head2;
	head = tree->Car();
	if(old == head)
	    head2 = newone;
	else
	    head2 = head;

	Node *tail = tree->Cdr();
	Node *tail2 = (tail == 0) ? tail : ShallowSubst(newone, old, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Node *Node::ShallowSubst(Node *newone1, Node *old1,
			   Node *newone2, Node *old2, Node *tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head, *head2;
	head = tree->Car();
	if(old1 == head)
	    head2 = newone1;
	else if(old2 == head)
	    head2 = newone2;
	else
	    head2 = head;

	Node *tail = tree->Cdr();
	Node *tail2 = (tail == 0) ? tail :
			ShallowSubst(newone1, old1, newone2, old2, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Node *Node::ShallowSubst(Node *newone1, Node *old1,
			   Node *newone2, Node *old2,
			   Node *newone3, Node *old3, Node *tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(old3 == tree)
	return newone3;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head, *head2;
	head = tree->Car();
	if(old1 == head)
	    head2 = newone1;
	else if(old2 == head)
	    head2 = newone2;
	else if(old3 == head)
	    head2 = newone3;
	else
	    head2 = head;

	Node *tail = tree->Cdr();
	Node *tail2 = (tail == 0) ? tail :
			ShallowSubst(newone1, old1, newone2, old2,
				     newone3, old3, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Node *Node::ShallowSubst(Node *newone1, Node *old1,
			   Node *newone2, Node *old2,
			   Node *newone3, Node *old3,
			   Node *newone4, Node *old4, Node *tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(old3 == tree)
	return newone3;
    else if(old4 == tree)
	return newone4;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Node *head, *head2;
	head = tree->Car();
	if(old1 == head)
	    head2 = newone1;
	else if(old2 == head)
	    head2 = newone2;
	else if(old3 == head)
	    head2 = newone3;
	else if(old4 == head)
	    head2 = newone4;
	else
	    head2 = head;

	Node *tail = tree->Cdr();
	Node *tail2 =  (tail == 0) ? tail :
			ShallowSubst(newone1, old1, newone2, old2,
				     newone3, old3, newone4, old4, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Node *Node::SubstSublist(Node *newsub, Node *oldsub, Node *lst)
{
    if(lst == oldsub)
	return newsub;
    else
	return Cons(lst->Car(), SubstSublist(newsub, oldsub, lst->Cdr()));
}

Node *Node::Snoc(Node *p, Node *q)
{
    return Nconc(p, Cons(q, 0));
}

/* Nconc is desctructive append */

Node *Node::Nconc(Node *p, Node *q)
{
    if(p == 0)
	return q;
    else{
	Last(p)->data.nonleaf.next = q;	// Last(p)->SetCdr(q);
	return p;
    }
}

Node *Node::Nconc(Node *p, Node *q, Node *r)
{
    return Nconc(p, Nconc(q, r));
}

void Node::print_indent(std::ostream &os, size_t indent)
{
  os.put('\n');
  for(size_t i = 0; i != indent; ++i) os.put(' ');
}

Node *Iterator::Pop()
{
  if(!ptree) return 0;
  else
  {
    Node *p = ptree->Car();
    ptree = ptree->Cdr();
    return p;
  }
}

bool Iterator::Next(Node *& car)
{
  if(!ptree) return false;
  else
  {
    car = ptree->Car();
    ptree = ptree->Cdr();
    return true;
  }
}

Array::Array(int s)
{
  num = 0;
  if(s > 8)
  {
    size = s;
    array = new (GC) Node *[s];
  }
  else
  {
    size = 8;
    array = default_buf;
  }
}

void Array::Append(Node *p)
{
  if(num >= size)
  {
    size += 8;
    Node ** a = new (GC) Node *[size];
    memmove(a, array, size_t(num * sizeof(Node *)));
    array = a;
  }
  array[num++] = p;
}

Node *&Array::Ref(uint i)
{
  if(i < num) return array[i];
  else throw std::range_error("Array: out of range");
}

Node *Array::All()
{
  Node *lst = 0;

  for(sint i = Number() - 1; i >= 0; --i)
    lst = Node::Cons(Ref(i), lst);

  return lst;
}

