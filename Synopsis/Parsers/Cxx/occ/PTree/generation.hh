//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_generation_hh
#define _PTree_generation_hh

#include <PTree/Node.hh>

namespace PTree
{
//. Head is used to implement qmake()
class Head : public LightObject 
{
public:
  Head() : ptree(0) {}
  operator Node *() { return ptree;}
  Head &operator + (Node *);
  Head &operator + (const char*);
  Head &operator + (char);
  Head &operator + (int);

private:
  Node *append(Node *, Node *);
  Node *append(Node *, const char *, size_t);

  Node *ptree;
};

bool reify(Node *, unsigned int&);
bool reify(Node *, const char*&);

bool match(Node *, const char *, ...);
Node *make(const char *pat, ...);
Node *gen_sym();

Node *qmake(const char *);
}

#endif
