//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Synopsis/PTree/operations.hh"

namespace Synopsis
{
namespace PTree
{

bool operator == (Node const &p, char c)
{
  return p.is_atom() && p.length() == 1 && *p.position() == c;
}

bool operator == (Node const &n, char const *str)
{
  if (!n.is_atom()) return false;
  char const *p = n.position();
  size_t l = n.length();
  size_t i = 0;
  for(; i < l; ++i)
    if(p[i] != str[i] || str[i] == '\0')
      return false;
  return str[i] == '\0';
}

bool operator == (Node const &p, Node const &q)
{
  if(!p.is_atom() || !q.is_atom()) return false;

  size_t plen = p.length();
  size_t qlen = q.length();
  if(plen == qlen)
  {
    char const *pstr = p.position();
    char const *qstr = q.position();
    while(--plen >= 0)
      if(pstr[plen] != qstr[plen]) return false;
    return true;
  }
  else return false;
}

bool equal(Node const &n, char const *str, size_t len)
{
  if(!n.is_atom()) return false;
  const char *p = n.position();
  size_t l = n.length();
  if(l == len)
  {
    for(size_t i = 0; i < l; ++i)
      if(p[i] != str[i]) return false;
    return true;
  }
  else return false;
}

bool equal(Node const *p, Node const *q)
{
  if(p == q) return true;
  else if(p == 0 || q == 0) return false;
  else if(p->is_atom() || q->is_atom()) return *p == *q;
  else return equal(p->car(), q->car()) && equal(p->cdr(), q->cdr());
}

/*
  equiv() returns true even if p and q are lists and all the elements
  are equal respectively.
*/
bool equiv(Node const *p, Node const *q)
{
  if(p == q) return true;
  else if(p == 0 || q == 0) return false;
  else if(p->is_atom() || q->is_atom()) return *p == *q;
  else
  {
    while(p != 0 && q != 0)
      if(p->car() != q->car())
	return false;
      else
      {
	p = p->cdr();
	q = q->cdr();
      }
    return p == 0 && q == 0;
  }
}

}
}
