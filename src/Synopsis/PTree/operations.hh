//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_operations_hh_
#define Synopsis_PTree_operations_hh_

#include <Synopsis/PTree/Node.hh>
#include <cassert>
#include <string>

namespace Synopsis
{
namespace PTree
{

inline std::string string(Atom *atom) 
{
  return std::string(atom->position(), atom->length());
}

bool operator == (Node const &p, char c);
inline bool operator != (Node const &p, char c) { return !operator == (p, c);}
bool operator == (Node const &p, char const *str);
inline bool operator != (Node const &p, char const *str) { return !operator == (p, str);}
bool operator == (Node const &p, Node const &q);
inline bool operator != (Node const &p, Node const &q) { return !operator == (p, q);}
bool equal(Node const &p, char const *str, size_t len);
bool equal(Node const *p, Node const *q);
bool equiv(Node const *p, Node const *q);

//. Return the length of the list or -1 if this is an atom.
inline int length(Node const *p)
{
  if (p->is_atom()) return -1;
  size_t n = 0;
  while(p)
  {
    ++n;
    p = p->cdr();
  }
  return n;
}

//. Return the last cons cell.
inline List *last(List *p)
{
  List *next = 0;
  while((next = p->cdr())) p = next;
  return p;
}

//. Return the nth item.
inline Node *nth(List const *p, size_t n)
{
  while(p && n-- > 0) p = p->cdr();
  return p ? p->car() : 0;
}

//. Return the nth item.
//. Let's see whether this can speed things up...
template <size_t N> inline Node *nth(List const *p)
{
  return p ? nth<N - 1>(p->cdr()) : 0;
}

template <> inline Node *nth<0>(List const *p) { return p ? p->car() : 0;}

//. Return the nth cons cell of the given list.
inline List const *tail(List const *p, size_t n)
{
  while(p && n-- > 0) p = p->cdr();
  return p;
}

inline List *tail(List *p, size_t n)
{
  while(p && n-- > 0) p = p->cdr();
  return p;
}

//. Return a new list whose car is p and cdr is q.
inline List *cons(Node *p, List *q = 0) { return new List(p, q);}

List *list();
List *list(Node *);
List *list(Node *, Node *);
List *list(Node *, Node *, Node *);
List *list(Node *, Node *, Node *, Node *);
List *list(Node *, Node *, Node *, Node *, Node *);
List *list(Node *, Node *, Node *, Node *, Node *, Node *);
List *list(Node *, Node *, Node *, Node *, Node *, Node *,
	   Node *);
List *list(Node *, Node *, Node *, Node *, Node *, Node *,
	   Node *, Node *);
  /*
Node *copy(Node *);
Node *append(Node *, Node *);
Node *replace_all(Node *, Node *, Node *);
Node *subst(Node *, Node *, Node *);
Node *subst(Node *, Node *, Node *, Node *, Node *);
Node *subst(Node *, Node *, Node *, Node *,
	    Node *, Node *, Node *);
Node *shallow_subst(Node *, Node *, Node *);
Node *shallow_subst(Node *, Node *, Node *, Node *, Node *);
Node *shallow_subst(Node *, Node *, Node *, Node *,
		    Node *, Node *, Node *);
Node *shallow_subst(Node *, Node *, Node *, Node *,
		    Node *, Node *, Node *, Node *, Node *);
Node *subst_sublist(Node *, Node *, Node *);
  */

//. Concatenate the second argument to the first, returning the modified list.
//. If p is nil return q instead.
inline List *conc(List *p, List *q)
{
  if (p) last(p)->set_cdr(q);
  else p = q;
  return p;
}

//. Concatenate the second argument to the first, returning the modified list.
//. Requires p to be a non-empty list.
template <typename T>
inline T *conc(T *p, List *q)
{
  // FIXME: make the following as compile-time check.
  assert(p && !p->is_atom());
  last(p)->set_cdr(q);
  return p;
}

//. Concatenate a new cons cell containing node to list.
template <typename T>
inline T *snoc(T *list, Node *node) { return conc(list, cons(node));}

}
}

#endif
