//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef Synopsis_Trace_hh_
#define Synopsis_Trace_hh_

#include <iostream>
#include <string>

namespace Synopsis
{

class Trace
{
public:
  struct Entry
  {
    Entry(bool e);
    ~Entry();

    template <typename T> Entry const &operator<<(T const &t) const;

    bool enabled;
  };
  friend struct Entry;
  Trace(const std::string &s) : my_scope(s)
  {
    if (!my_debug) return;
    std::cout << indent() << "entering " << my_scope << std::endl;
    ++my_level;
  }
  ~Trace()
  {
    if (!my_debug) return;
    --my_level;
    std::cout << indent() << "leaving " << my_scope << std::endl;
  }

  template <typename T>
  Entry const &operator<<(T const &t) const;

  static void enable_debug() { my_debug = true;}

private:
  static std::string indent() { return std::string(my_level, ' ');}

  static bool   my_debug;
  static size_t my_level;
  std::string   my_scope;
};

inline Trace::Entry::Entry(bool e) 
  : enabled(e)
{
  if (enabled)
    std::cout << Trace::indent();
}
inline Trace::Entry::~Entry() 
{
  if (enabled)
    std::cout << std::endl;
}

template <typename T> 
inline Trace::Entry const &Trace::Entry::operator<<(T const &t) const
{
  if (enabled)
    std::cout << t;
  return *this;
}

template <typename T>
inline Trace::Entry const &Trace::operator<<(T const &t) const
{
  Entry entry(my_debug);
  return entry << t;
}

}

#endif
