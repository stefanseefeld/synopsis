//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree/Display.hh>
#include <typeinfo>

using namespace PTree;

Display::Display(std::ostream &os, bool encoded, bool typeinfo)
  : my_os(os),
    my_indent(0),
    my_depth(0),
    my_encoded(encoded),
    my_typeinfo(typeinfo)
{
}

void Display::display(Node *n)
{
  if (n) n->accept(this);
  else my_os << "nil";
  my_os.put('\n');
}

void Display::visit(Atom *a)
{
  if (my_typeinfo) my_os << typeid(*a).name() << ':';
  const char *p = a->position();
  size_t n = a->length();

  // Recall that [, ], and @ are special characters.

  if(n < 1) return;
  else if(n == 1 && *p == '@')
  {
    my_os << "\\@";
    return;
  }

  char c = *p++;
  if(c == '[' || c == ']') my_os << '\\' << c; // [ and ] at the beginning are escaped.
  else my_os << c;
  while(--n > 0) my_os << *p++;
}

void Display::visit(List *l) 
{
  if (my_typeinfo) my_os << typeid(*l).name() << ':';
  if(too_deep())
  {
    my_os << " ** too many nestings ** ";
    return;
  }
  Node *rest = l;
  my_os << '[';
  while(rest != 0)
  {
    if(rest->is_atom())
    {
      my_os << "@ ";
      ++my_depth;
      rest->accept(this);
      --my_depth;
      rest = 0;
    }
    else
    {
      Node *head = rest->car();
      if(head == 0) my_os << "nil";
      else
      {
	++my_depth;
	head->accept(this);
	--my_depth;
      }
      rest = rest->cdr();
      if(rest != 0) my_os << ' ';
    }
  }
  my_os << ']';
}

void Display::visit(DupAtom *a)
{
  if (my_typeinfo) my_os << typeid(*a).name() << ':';
  const char *pos = a->position();
  size_t length = a->length();

  if(length == 1 && *pos == '@')
  {
    my_os << "\\@";
    return;
  }

  my_os << '`';
  for(size_t i = 0; i < length; ++i)
    if(pos[i] == '[' || pos[i] == ']') my_os << '\\' << pos[i];
    else my_os << pos[i];
  my_os << '`';
}

void Display::visit(Brace *l)
{
  if (my_typeinfo) my_os << typeid(*l).name() << ':';
  if(too_deep())
  {
    my_os << " ** too many nestings ** ";
    return;
  }
  ++my_indent;
  my_os << "[{";
  Node *body = second(l);
  if(!body)
  {
    newline();
    my_os << "nil";
  }
  else
    while(body)
    {
      newline();
      if(body->is_atom())
      {
	my_os << "@ ";
	++my_depth;
	body->accept(this);
	--my_depth;
      }
      else
      {
	Node *head = body->car();
	if(!head) my_os << "nil";
	else
	{
	  ++my_depth;
	  head->accept(this);
	  --my_depth;
	}
      }
      body = body->cdr();
    }
  --my_indent;
  newline();
  my_os << "}]";
}

void Display::newline()
{
  my_os.put('\n');
  for(size_t i = 0; i != my_indent; ++i) my_os.put(' ');
}

bool Display::too_deep()
{
  return my_depth >= 32;
}

void Display::print_encoded(List *l)
{
  if (my_encoded)
  {
    Encoding type = l->encoded_type();
    if(!type.empty())
    {
      my_os << '#' << type;
    }
    Encoding name = l->encoded_name();
    if(!name.empty())
    {
      my_os << '@' << name;
    }
  }
  visit(static_cast<List *>(l));
}
