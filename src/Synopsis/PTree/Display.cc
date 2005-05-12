//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/PTree/Display.hh>
#include <typeinfo>

using namespace Synopsis;
using namespace PTree;

Display::Display(std::ostream &os, bool encoded)
  : my_os(os),
    my_indent(0),
    my_encoded(encoded)
{
}

void Display::display(Node const *n)
{
  if (n) const_cast<Node *>(n)->accept(this);
  else my_os << "nil";
  my_os.put('\n');
}

void Display::visit(Atom *a)
{
  char const *p = a->position();
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
  Node *rest = l;
  my_os << '[';
  while(rest != 0)
  {
    if(rest->is_atom())
    {
      my_os << "@ ";
      rest->accept(this);
      rest = 0;
    }
    else
    {
      Node *head = rest->car();
      if(head == 0) my_os << "nil";
      else
      {
	head->accept(this);
      }
      rest = rest->cdr();
      if(rest != 0) my_os << ' ';
    }
  }
  my_os << ']';
}

void Display::visit(DupAtom *a)
{
  char const *pos = a->position();
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
	body->accept(this);
      }
      else
      {
	Node *head = body->car();
	if(!head) my_os << "nil";
	else
	{
	  head->accept(this);
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

void Display::print_encoded(List *l)
{
  if (my_encoded)
  {
    Encoding const &type = l->encoded_type();
    if(!type.empty()) my_os << '#' << type;
    Encoding const &name = l->encoded_name();
    if(!name.empty()) my_os << '@' << name;
  }
  visit(static_cast<List *>(l));
}

RTTIDisplay::RTTIDisplay(std::ostream &os, bool encoded)
  : my_os(os),
    my_indent(0),
    my_encoded(encoded)
{
}

void RTTIDisplay::display(Node const *n)
{
  if (n) const_cast<Node *>(n)->accept(this);
  else my_os << "nil";
  my_os.put('\n');
}

void RTTIDisplay::visit(Atom *a)
{
  newline();
  my_os << typeid(*a).name() << ": ";
  char const *p = a->position();
  size_t n = a->length();

  if(n < 1) return;
  else if(n == 1 && *p == '@')
  {
    my_os << "\\@";
    return;
  }

  my_os << *p++;
  while(--n > 0) my_os << *p++;
}

void RTTIDisplay::visit(List *l) 
{
  newline();
  my_os << typeid(*l).name() << ": ";
  if (my_encoded)
  {
    Encoding type = l->encoded_type();
    if(!type.empty())
    {
      my_os << "type=" << type << ' ';
    }
    Encoding name = l->encoded_name();
    if(!name.empty())
    {
      my_os << "name=" << name;
    }
  }
  ++my_indent;
  Node *rest = l;
  while(rest != 0)
  {
    if(rest->is_atom())
    {
      rest->accept(this);
      rest = 0;
    }
    else
    {
      Node *head = rest->car();
      if(head == 0)
      {
	newline();
	my_os << "nil";
      }
      else
      {
	head->accept(this);
      }
      rest = rest->cdr();
    }
  }
  --my_indent;
}

void RTTIDisplay::visit(DupAtom *a)
{
  newline();
  my_os << typeid(*a).name() << ": ";
  char const *pos = a->position();
  size_t length = a->length();

  if(length == 1 && *pos == '@')
  {
    my_os << "\\@";
    return;
  }

  my_os << '`';
  for(size_t i = 0; i < length; ++i) my_os << pos[i];
  my_os << '`';
}

void RTTIDisplay::newline()
{
  my_os.put('\n');
  for(size_t i = 0; i != my_indent; ++i) my_os.put(' ');
}

