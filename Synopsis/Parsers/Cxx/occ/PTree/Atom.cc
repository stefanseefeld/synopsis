//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Lexer.hh>
#include <PTree/Atom.hh>
#include <cassert>

using namespace PTree;

Atom::Atom(const char *ptr, size_t len)
{
  data.leaf.position = const_cast<char *>(ptr);
  data.leaf.length = len;
}

Atom::Atom(const Token &tk)
{
  data.leaf.position = const_cast<char *>(tk.ptr);
  data.leaf.length = tk.length;
}

void Atom::write(std::ostream &os) const
{
  assert(this);
  os.write(data.leaf.position, data.leaf.length);
}

void Atom::print(std::ostream &os, size_t, size_t) const
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

int Atom::Write(std::ostream& out, int indent)
{
  int n = 0;
  const char *ptr = data.leaf.position;
  size_t len = data.leaf.length;
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
