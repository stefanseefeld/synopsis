//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Python_TypedList_hh
#define _Synopsis_Python_TypedList_hh

#include <Synopsis/Python/Object.hh>

namespace Synopsis
{
namespace Python
{

//. A typed list replace some modifiers by a 'more typed' variant
//. This is really just provided to allow more expressive statements
template <typename T>
class TypedList : public List
{
public:
  TypedList(size_t i = 0) : List(i) {}
  TypedList(Object o) : List(o) {} // should we typecheck here all items ?

  TypedList(const T &);
  TypedList(const T &, const T &);
  TypedList(const T &, const T &, const T &);
  TypedList(const T &, const T &, const T &, const T &);

  void set(int i, const T &s) { List::set(i, s);}
  T get(int i) const { return narrow<T>(List::get(i));}
  void append(const T &s) { List::append(s);}
  void insert(int i, const T &s) { List::insert(i, s);}
};

template <typename T>
inline TypedList<T>::TypedList(const T &t1)
{
  append(t1);
}

template <typename T>
inline TypedList<T>::TypedList(const T &t1, const T &t2)
{
  append(t1);
  append(t2);
}

template <typename T>
inline TypedList<T>::TypedList(const T &t1, const T &t2, const T &t3)
{
  append(t1);
  append(t2);
  append(t3);
}

template <typename T>
inline TypedList<T>::TypedList(const T &t1, const T &t2, const T &t3, const T &t4)
{
  append(t1);
  append(t2);
  append(t3);
  append(t4);
}

}
}

#endif
