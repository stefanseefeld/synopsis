// $Id: TypedList.hh,v 1.1 2004/01/25 21:21:54 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_TypedList_hh
#define _Synopsis_TypedList_hh

#include <Synopsis/Object.hh>

namespace Synopsis
{

//. A typed list replace some modifiers by a 'more typed' variant
//. This is really just provided to allow more expressive statements
template <typename T>
class TypedList : public List
{
public:
  TypedList(size_t i = 0) : List(i) {}
  TypedList(Object o) : List(o) {} // should we typecheck here all items ?
  void set(int i, const T &s) { List::set(i, s);}
  void append(const T &s) { List::append(s);}
  void insert(int i, const T &s) { List::insert(i, s);}
};

}

#endif
