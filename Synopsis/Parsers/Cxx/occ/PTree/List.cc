//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree/List.hh>
#include <Encoding.hh>
#include <cassert>
#include <stdexcept>

using namespace PTree;

void List::write(std::ostream &os) const
{
  assert(this);
  for (const Node *p = this; p; p = p->cdr())
  {
    const Node *data = p->car();
    if (data) data->write(os);
    else if(p->is_atom()) throw std::logic_error("List::write(): not list");
  }
}

void List::print(std::ostream &os, size_t indent, size_t depth) const
{
  if(too_deep(os, depth)) return;

  const Node *rest = this;
  os << '[';
  while(rest != 0)
  {
    if(rest->is_atom())
    {
      os << "@ ";
      rest->print(os, indent, depth + 1);
      rest = 0;
    }
    else
    {
      const Node *head = rest->car();
      if(head == 0) os << "nil";
      else head->print(os, indent, depth + 1);
      rest = rest->cdr();
      if(rest != 0) os << ' ';
    }
  }
  os << ']';
}

void List::print_encoded(std::ostream &os, size_t indent, size_t depth) const
{
  if (show_encoded)
  {
    const char *encode = encoded_type();
    if(encode)
    {
      os << '#';
      Encoding::print(os, encode);
    }
    encode = encoded_name();
    if(encode)
    {
      os << '@';
      Encoding::print(os, encode);
    }
  }
  List::print(os, indent, depth);
}

bool List::too_deep(std::ostream& s, size_t depth) const
{
  if(depth >= 32)
  {
    s << " ** too many nestings ** ";
    return true;
  }
  else return false;
}

int List::Write(std::ostream& out, int indent)
{
  int n = 0;
  Node *p = this;
  while(true)
  {
    Node *head = p->car();
    if(head != 0) n += head->Write(out, indent);

    p = p->cdr();
    if(p == 0) break;
    else if(p->is_atom())
      throw std::runtime_error("List::Write(): not list");
    else out << ' ';
  }
  return n;
}
