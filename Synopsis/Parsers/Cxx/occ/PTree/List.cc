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

List::List(Node *p, Node *q)
{
  data.nonleaf.child = p;
  data.nonleaf.next = q;
}

void List::write(std::ostream &os) const
{
  assert(this);
  for (const Node *p = this; p; p = p->Cdr())
  {
    const Node *data = p->Car();
    if (data) data->write(os);
    else if(p->IsLeaf()) throw std::logic_error("List::write(): not list");
  }
}

void List::print(std::ostream &os, size_t indent, size_t depth) const
{
  if(too_deep(os, depth)) return;

  const Node *rest = this;
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
      const Node *head = rest->data.nonleaf.child;
      if(head == 0) os << "nil";
      else head->print(os, indent, depth + 1);
      rest = rest->data.nonleaf.next;
      if(rest != 0) os << ' ';
    }
  }
  os << ']';
}

void List::print_encoded(std::ostream &os, size_t indent, size_t depth) const
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
    Node *head = p->Car();
    if(head != 0) n += head->Write(out, indent);

    p = p->Cdr();
    if(p == 0) break;
    else if(p->IsLeaf())
      throw std::runtime_error("List::Write(): not list");
    else out << ' ';
  }
  return n;
}
