//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "AST.hh"
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

bool Ptree::show_encoded = false;

const char *Ptree::begin() const
{
  if (IsLeaf()) return GetPosition();
  else
  {
    for (const Ptree *p = this; p; p = p->Cdr())
    {
      const char *b = p->Car() ? p->Car()->begin() : 0;
      if (b) return b;
    }
    return 0;
  }
}

const char *Ptree::end() const
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

std::string Ptree::string() const
{
  std::ostringstream oss;
  write(oss);
  return oss.str();
};

void Ptree::print(std::ostream &os) const
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

// class Ptree

int Ptree::Write(std::ostream& s)
{
    if(this == 0)
	return 0;
    else
	return Write(s, 0);
}

bool Ptree::Eq(char c)
{
  if(this == 0) return false;
  else
    return(IsLeaf() && GetLength() == 1 && *GetPosition() == c);
}

bool Ptree::Eq(char* str)
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

bool Ptree::Eq(char* str, int len)
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

Ptree* Ptree::Ca_ar()		// compute Caa..ar
{
    Ptree* p = this;
    while(p != 0 && !p->IsLeaf())
	p = p->Car();

    return p;
}

int Ptree::What()
{
  return Token::BadToken;
}

bool Ptree::IsA(int kind)
{
    if(this == 0)
	return false;
    else
	return bool(What() == kind);
}

bool Ptree::IsA(int kind1, int kind2)
{
    if(this == 0)
	return false;
    else{
	int k = What();
	return bool(k == kind1 || k == kind2);
    }
}

bool Ptree::IsA(int kind1, int kind2, int kind3)
{
    if(this == 0)
	return false;
    else{
	int k = What();
	return bool(k == kind1 || k == kind2 || k == kind3);
    }
}

Ptree* Ptree::Translate(Walker* w)
{
    return w->TranslatePtree(this);
}

void Ptree::Typeof(Walker* w, TypeInfo& t)
{
    w->TypeofPtree(this, t);
}

char* Ptree::GetEncodedType() const
{
    return 0;
}

char* Ptree::GetEncodedName() const
{
    return 0;
}


// static members

bool Ptree::Eq(Ptree* p, char c)
{
    return p->Eq(c);
}

bool Ptree::Eq(Ptree* p, char* str)
{
    return p->Eq(str);
}

bool Ptree::Eq(Ptree* p, char* str, int len)
{
    return p->Eq(str, len);
}

bool Ptree::Eq(Ptree* p, Ptree* q)
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
bool Ptree::Equiv(Ptree* p, Ptree* q)
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

bool Ptree::Equal(Ptree* p, Ptree* q)
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

const Ptree *Ptree::Last(const Ptree *p)	// return the last cons cell.
{
  const Ptree *next;
  if(!p) return 0;

  while((next = p->Cdr())) p = next;
  return p;
}

Ptree* Ptree::Last(Ptree* p)	// return the last cons cell.
{
    Ptree* next;
    if(p == 0)
	return 0;

    while((next = p->Cdr()) != 0)
	p = next;

    return p;
}

const Ptree *Ptree::First(const Ptree *p)
{
  return p ? p->Car() : 0;
}

Ptree* Ptree::First(Ptree* p)
{
    if(p != 0)
	return p->Car();
    else
	return p;
}

const Ptree *Ptree::Rest(const Ptree *p)
{
  return p ? p->Cdr() : 0;
}

Ptree* Ptree::Rest(Ptree* p)
{
    if(p != 0)
	return p->Cdr();
    else
	return p;
}

const Ptree *Ptree::Second(const Ptree *p)
{
  if(p)
  {
    p = p->Cdr();
    if(p) return p->Car();
  }
  
  return 0;
}

Ptree* Ptree::Second(Ptree* p)
{
    if(p != 0){
	p = p->Cdr();
	if(p != 0)
	    return p->Car();
    }

    return p;
}

const Ptree *Ptree::Third(const Ptree *p)
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

Ptree* Ptree::Third(Ptree* p)
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
const Ptree *Ptree::Nth(const Ptree *p, int n)
{
  while(p && n-- > 0) p = p->Cdr();
  return p ? p->Car() : 0;
}

Ptree* Ptree::Nth(Ptree* p, int n)
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
int Ptree::Length(const Ptree* p)
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

Ptree* Ptree::ListTail(Ptree* p, int k)
{
    while(p != 0 && k-- > 0)
	p = p->Cdr();

    return p;
}

Ptree* Ptree::Cons(Ptree* p, Ptree* q)
{
    return new NonLeaf(p, q);
}

Ptree* Ptree::List(Ptree* p)
{
    return new NonLeaf(p, 0);
}

Ptree* Ptree::List()
{
    return 0;
}

Ptree* Ptree::List(Ptree* p, Ptree* q)
{
    return new NonLeaf(p, new NonLeaf(q, 0));
}

Ptree* Ptree::List(Ptree* p1, Ptree* p2, Ptree* p3)
{
    return new NonLeaf(p1, new NonLeaf(p2, new NonLeaf(p3, 0)));
}

Ptree* Ptree::List(Ptree* p1, Ptree* p2, Ptree* p3, Ptree* p4)
{
    return new NonLeaf(p1, List(p2, p3, p4));
}

Ptree* Ptree::List(Ptree* p1, Ptree* p2, Ptree* p3, Ptree* p4, Ptree* p5)
{
    return Nconc(List(p1, p2), List(p3, p4, p5));
}

Ptree* Ptree::List(Ptree* p1, Ptree* p2, Ptree* p3, Ptree* p4, Ptree* p5,
		   Ptree* p6)
{
    return Nconc(List(p1, p2, p3), List(p4, p5, p6));
}

Ptree* Ptree::List(Ptree* p1, Ptree* p2, Ptree* p3, Ptree* p4, Ptree* p5,
		   Ptree* p6, Ptree* p7)
{
    return Nconc(List(p1, p2, p3), List(p4, p5, p6, p7));
}

Ptree* Ptree::List(Ptree* p1, Ptree* p2, Ptree* p3, Ptree* p4, Ptree* p5,
		   Ptree* p6, Ptree* p7, Ptree* p8)
{
    return Nconc(List(p1, p2, p3, p4), List(p5, p6, p7, p8));
}

Ptree* Ptree::CopyList(Ptree* p)
{
    return Append(p, 0);
}

//   q may be a leaf
//
Ptree* Ptree::Append(Ptree* p, Ptree* q)
{
    Ptree *result, *tail;

    if(p == 0)
	if(q->IsLeaf())
	    return Cons(q, 0);
	else
	    return q;

    result = tail = Cons(p->Car(), 0);
    p = p->Cdr();
    while(p != 0){
	Ptree* cell = Cons(p->Car(), 0);
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
Ptree* Ptree::ReplaceAll(Ptree* list, Ptree* orig, Ptree* subst)
{
    if(Eq(list, orig))
	return subst;
    else if(list == 0 || list->IsLeaf())
	return list;
    else{
	PtreeArray newlist;
	bool changed = false;
	Ptree* rest = list;
	while(rest != 0){
	    Ptree* p = rest->Car();
	    Ptree* q = ReplaceAll(p, orig, subst);
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

Ptree* Ptree::Subst(Ptree* newone, Ptree* old, Ptree* tree)
{
    if(old == tree)
	return newone;
    else if(tree== 0 || tree->IsLeaf())
	return tree;
    else{
	Ptree* head = tree->Car();
	Ptree* head2 = Subst(newone, old, head);
	Ptree* tail = tree->Cdr();
	Ptree* tail2 = (tail == 0) ? tail : Subst(newone, old, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Ptree* Ptree::Subst(Ptree* newone1, Ptree* old1, Ptree* newone2, Ptree* old2,
		    Ptree* tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Ptree* head = tree->Car();
	Ptree* head2 = Subst(newone1, old1, newone2, old2, head);
	Ptree* tail = tree->Cdr();
	Ptree* tail2 = (tail == 0) ? tail
			: Subst(newone1, old1, newone2, old2, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Ptree* Ptree::Subst(Ptree* newone1, Ptree* old1, Ptree* newone2, Ptree* old2,
		    Ptree* newone3, Ptree* old3, Ptree* tree)
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
	Ptree* head = tree->Car();
	Ptree* head2 = Subst(newone1, old1, newone2, old2,
			     newone3, old3, head);
	Ptree* tail = tree->Cdr();
	Ptree* tail2 = (tail == 0) ? tail :
			Subst(newone1, old1, newone2, old2,
			      newone3, old3, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

// ShallowSubst() doesn't recursively apply substitution to a subtree.

Ptree* Ptree::ShallowSubst(Ptree* newone, Ptree* old, Ptree* tree)
{
    if(old == tree)
	return newone;
    else if(tree== 0 || tree->IsLeaf())
	return tree;
    else{
	Ptree *head, *head2;
	head = tree->Car();
	if(old == head)
	    head2 = newone;
	else
	    head2 = head;

	Ptree* tail = tree->Cdr();
	Ptree* tail2 = (tail == 0) ? tail : ShallowSubst(newone, old, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Ptree* Ptree::ShallowSubst(Ptree* newone1, Ptree* old1,
			   Ptree* newone2, Ptree* old2, Ptree* tree)
{
    if(old1 == tree)
	return newone1;
    else if(old2 == tree)
	return newone2;
    else if(tree == 0 || tree->IsLeaf())
	return tree;
    else{
	Ptree *head, *head2;
	head = tree->Car();
	if(old1 == head)
	    head2 = newone1;
	else if(old2 == head)
	    head2 = newone2;
	else
	    head2 = head;

	Ptree* tail = tree->Cdr();
	Ptree* tail2 = (tail == 0) ? tail :
			ShallowSubst(newone1, old1, newone2, old2, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Ptree* Ptree::ShallowSubst(Ptree* newone1, Ptree* old1,
			   Ptree* newone2, Ptree* old2,
			   Ptree* newone3, Ptree* old3, Ptree* tree)
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
	Ptree *head, *head2;
	head = tree->Car();
	if(old1 == head)
	    head2 = newone1;
	else if(old2 == head)
	    head2 = newone2;
	else if(old3 == head)
	    head2 = newone3;
	else
	    head2 = head;

	Ptree* tail = tree->Cdr();
	Ptree* tail2 = (tail == 0) ? tail :
			ShallowSubst(newone1, old1, newone2, old2,
				     newone3, old3, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Ptree* Ptree::ShallowSubst(Ptree* newone1, Ptree* old1,
			   Ptree* newone2, Ptree* old2,
			   Ptree* newone3, Ptree* old3,
			   Ptree* newone4, Ptree* old4, Ptree* tree)
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
	Ptree *head, *head2;
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

	Ptree* tail = tree->Cdr();
	Ptree* tail2 =  (tail == 0) ? tail :
			ShallowSubst(newone1, old1, newone2, old2,
				     newone3, old3, newone4, old4, tail);
	if(head == head2 && tail == tail2)
	    return tree;
	else
	    return Cons(head2, tail2);
    }
}

Ptree* Ptree::SubstSublist(Ptree* newsub, Ptree* oldsub, Ptree* lst)
{
    if(lst == oldsub)
	return newsub;
    else
	return Cons(lst->Car(), SubstSublist(newsub, oldsub, lst->Cdr()));
}

Ptree* Ptree::Snoc(Ptree* p, Ptree* q)
{
    return Nconc(p, Cons(q, 0));
}

/* Nconc is desctructive append */

Ptree* Ptree::Nconc(Ptree* p, Ptree* q)
{
    if(p == 0)
	return q;
    else{
	Last(p)->data.nonleaf.next = q;	// Last(p)->SetCdr(q);
	return p;
    }
}

Ptree* Ptree::Nconc(Ptree* p, Ptree* q, Ptree* r)
{
    return Nconc(p, Nconc(q, r));
}

void Ptree::print_indent(std::ostream &os, size_t indent)
{
  os.put('\n');
  for(size_t i = 0; i != indent; ++i) os.put(' ');
}

// class PtreeIter

Ptree* PtreeIter::Pop()
{
    if(ptree == 0)
	return 0;
    else{
	Ptree* p = ptree->Car();
	ptree = ptree->Cdr();
	return p;
    }
}

bool PtreeIter::Next(Ptree*& car)
{
    if(ptree == 0)
	return false;
    else{
	car = ptree->Car();
	ptree = ptree->Cdr();
	return true;
    }
}

Ptree* PtreeIter::This()
{
    if(ptree == 0)
	return 0;
    else
	return ptree->Car();
}

// class PtreeArray

PtreeArray::PtreeArray(int s)
{
    num = 0;
    if(s > 8){
	size = s;
	array = new (GC) Ptree*[s];
    }
    else{
	size = 8;
	array = default_buf;
    }
}

void PtreeArray::Append(Ptree* p)
{
    if(num >= size){
	size += 8;
	Ptree** a = new (GC) Ptree*[size];
	memmove(a, array, size_t(num * sizeof(Ptree*)));
	array = a;
    }

    array[num++] = p;
}

Ptree*& PtreeArray::Ref(uint i)
{
    if(i < num)
	return array[i];
    else{
	MopErrorMessage("PtreeArray", "out of range");
	return array[0];
    }
}

Ptree* PtreeArray::All()
{
    Ptree* lst = 0;

    for(sint i = Number() - 1; i >= 0; --i)
	lst = Ptree::Cons(Ref(i), lst);

    return lst;
}

Leaf::Leaf(char *ptr, int len)
{
  data.leaf.position = ptr;
  data.leaf.length = len;
}

Leaf::Leaf(const Token &tk)
{
  data.leaf.position = const_cast<char *>(tk.ptr);
  data.leaf.length = tk.length;
}

void Leaf::write(std::ostream &os) const
{
  assert(this);
  os.write(data.leaf.position, data.leaf.length);
}

void Leaf::print(std::ostream &os, size_t, size_t) const
{
  const char *p = data.leaf.position;
  int n = data.leaf.length;

  // Recall that [, ], and @ are special characters.

  if(n < 1) return;
  else if(n == 1 && *p == '@')
  {
    os << "\\@";
    return;
  }

  char c = *p++;
  if(c == '[' || c == ']') os << '\\' << c; // [ and ] at the beginning are escaped.
  else os << c;
  while(--n > 0) os << *p++;
}

int Leaf::Write(std::ostream& out, int indent)
{
  int n = 0;
  char* ptr = data.leaf.position;
  int len = data.leaf.length;
  while(len-- > 0)
  {
    char c = *ptr++;
    if(c == '\n')
    {
      print_indent(out, indent);
      ++n;
    }
    else out << c;
  }
  return n;
}

NonLeaf::NonLeaf(Ptree* p, Ptree* q)
{
  data.nonleaf.child = p;
  data.nonleaf.next = q;
}

void NonLeaf::write(std::ostream &os) const
{
  assert(this);
  for (const Ptree *p = this; p; p = const_cast<const Ptree *>(const_cast<Ptree *>(p)->Cdr()))
  {
    const Ptree *data = const_cast<const Ptree *>(const_cast<Ptree *>(p)->Car());
    if (data) data->write(os);
    else if(p->IsLeaf()) throw std::logic_error("NonLeaf::write(): not list");
  }
}

void NonLeaf::print(std::ostream &os, size_t indent, size_t depth) const
{
  if(too_deep(os, depth)) return;

  const Ptree *rest = this;
  os << '[';
  while(rest != 0)
  {
    if(rest->IsLeaf())
    {
      os << "@ ";
      rest->print(os, indent, depth + 1);
      rest = 0;
    }
    else
    {
      const Ptree *head = rest->data.nonleaf.child;
      if(head == 0) os << "nil";
      else head->print(os, indent, depth + 1);
      rest = rest->data.nonleaf.next;
      if(rest != 0) os << ' ';
    }
  }
  os << ']';
}

void NonLeaf::print_encoded(std::ostream &os, size_t indent, size_t depth) const
{
  if (show_encoded)
  {
    const char *encode = GetEncodedType();
    if(encode)
    {
      os << '#';
      Encoding::print(os, encode);
    }
    encode = GetEncodedName();
    if(encode)
    {
      os << '@';
      Encoding::print(os, encode);
    }
  }
  NonLeaf::print(os, indent, depth);
}

bool NonLeaf::too_deep(std::ostream& s, size_t depth) const
{
  if(depth >= 32)
  {
    s << " ** too many nestings ** ";
    return true;
  }
  else return false;
}

int NonLeaf::Write(std::ostream& out, int indent)
{
  int n = 0;
  Ptree* p = this;
  while(true)
  {
    Ptree* head = p->Car();
    if(head != 0) n += head->Write(out, indent);

    p = p->Cdr();
    if(p == 0) break;
    else if(p->IsLeaf())
    {
      MopErrorMessage("NonLeaf::Write()", "not list");
      break;
    }
    else out << ' ';
  }
  return n;
}

DupLeaf::DupLeaf(const char* str, int len) : CommentedLeaf(new (GC) char[len], len)
{
  memmove(data.leaf.position, str, len);
}

DupLeaf::DupLeaf(char* str1, int len1, char* str2, int len2)
: CommentedLeaf(new (GC) char[len1 + len2], len1 + len2)
{
  memmove(data.leaf.position, str1, len1);
  memmove(&data.leaf.position[len1], str2, len2);
}

void DupLeaf::print(std::ostream &os, size_t, size_t) const
{
  const char *pos = data.leaf.position;
  int j = data.leaf.length;

  if(j == 1 && *pos == '@')
  {
    os << "\\@";
    return;
  }

  os << '`';
  for(int i = 0; i < j; ++i)
    if(pos[i] == '[' || pos[i] == ']') os << '\\' << pos[i];
    else os << pos[i];
  os << '`';
}

