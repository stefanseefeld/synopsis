//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree/Node.hh>
#include <PTree/Atoms.hh>
#include <PTree/TypeVisitor.hh>
#include <PTree/Encoding.hh>
#include <Lexer.hh>
#include <iostream>
#include <sstream>

using namespace PTree;

Node *Encoding::bool_t = 0;
Node *Encoding::char_t = 0;
Node *Encoding::wchar_t_t = 0;
Node *Encoding::int_t = 0;
Node *Encoding::short_t = 0;
Node *Encoding::long_t = 0;
Node *Encoding::float_t = 0;
Node *Encoding::double_t = 0;
Node *Encoding::void_t = 0;

Node *Encoding::signed_t = 0;
Node *Encoding::unsigned_t = 0;
Node *Encoding::const_t = 0;
Node *Encoding::volatile_t = 0;

Node *Encoding::operator_name = 0;
Node *Encoding::new_operator = 0;
Node *Encoding::anew_operator = 0;
Node *Encoding::delete_operator = 0;
Node *Encoding::adelete_operator = 0;

Node *Encoding::star = 0;
Node *Encoding::ampersand = 0;
Node *Encoding::comma = 0;
Node *Encoding::dots = 0;
Node *Encoding::scope = 0;
Node *Encoding::tilder = 0;
Node *Encoding::left_paren = 0;
Node *Encoding::right_paren = 0;
Node *Encoding::left_bracket = 0;
Node *Encoding::right_bracket = 0;
Node *Encoding::left_angle = 0;
Node *Encoding::right_angle = 0;

void Encoding::do_init_static()
{
  Encoding::bool_t = new PTree::AtomBOOLEAN("bool", 4);
  Encoding::char_t = new PTree::AtomCHAR("char", 4);
  Encoding::wchar_t_t = new PTree::AtomCHAR("wchar_t", 7);
  Encoding::int_t = new PTree::AtomINT("int", 3);
  Encoding::short_t = new PTree::AtomSHORT("short", 5);
  Encoding::long_t = new PTree::AtomLONG("long", 4);
  Encoding::float_t = new PTree::AtomFLOAT("float", 5);
  Encoding::double_t = new PTree::AtomDOUBLE("double", 6);
  Encoding::void_t = new PTree::AtomVOID("void", 4);

  Encoding::signed_t = new PTree::AtomSIGNED("signed", 6);
  Encoding::unsigned_t = new PTree::AtomUNSIGNED("unsigned", 8);
  Encoding::const_t = new PTree::AtomCONST("const", 5);
  Encoding::volatile_t = new PTree::AtomVOLATILE("volatile", 8);
  
  Encoding::operator_name = new PTree::Reserved("operator", 8);
  Encoding::new_operator = new PTree::Reserved("new", 3);
  Encoding::anew_operator = new PTree::Reserved("new[]", 5);
  Encoding::delete_operator = new PTree::Reserved("delete", 6);
  Encoding::adelete_operator = new PTree::Reserved("delete[]", 8);
  
  Encoding::star = new PTree::Atom("*", 1);
  Encoding::ampersand = new PTree::Atom("&", 1);
  Encoding::comma = new PTree::Atom(",", 1);
  Encoding::dots = new PTree::Atom("...", 3);
  Encoding::scope = new PTree::Atom("::", 2);
  Encoding::tilder = new PTree::Atom("~", 1);
  Encoding::left_paren = new PTree::Atom("(", 1);
  Encoding::right_paren = new PTree::Atom(")", 1);
  Encoding::left_bracket = new PTree::Atom("[", 1);
  Encoding::right_bracket = new PTree::Atom("]", 1);
  Encoding::left_angle = new PTree::Atom("<", 1);
  Encoding::right_angle = new PTree::Atom(">", 1);
}

const char *Encoding::copy() const
{
  return strcpy(new (GC) char[my_buffer.size() + 1], (const char *)my_buffer.c_str());
}

void Encoding::cv_qualify(const PTree::Node *cv1, const PTree::Node *cv2)
{
  bool c = false, v = false;
  if(cv1 && !cv1->is_atom())
    while(cv1)
    {
      int kind = PTree::type_of(cv1->car());
      cv1 = cv1->cdr();
      if(kind == Token::CONST) c = true;
      else if(kind == Token::VOLATILE) v = true;
    }

  if(cv2 && !cv2->is_atom())
    while(cv2)
    {
      int kind = PTree::type_of(cv2->car());
      cv2 = cv2->cdr();
      if(kind == Token::CONST) c = true;
      else if(kind == Token::VOLATILE) v = true;
    }

  if(v) prepend('V');
  if(c) prepend('C');
}

void Encoding::global_scope()
{
  append(0x80);
}

// simple_name() is also used for operator names

void Encoding::simple_name(const PTree::Node *id)
{
  append_with_length(id->position(), id->length());
}

// anonymous() generates an internal name for anonymous enum and class
// declarations.

void Encoding::anonymous()
{
  static int i = 0;
  static char name[] = "`0000";
  int n = i++;
  name[1] = n / 1000 + '0';
  name[2] = (n / 100) % 10 + '0';
  name[3] = (n / 10) % 10 + '0';
  name[4] = n % 10 + '0';
  append_with_length(name, 5);
}

void Encoding::template_(const PTree::Node *name, const Encoding &args)
{
  append('T');
  simple_name(name);
  append_with_length(args);
}

void Encoding::qualified(int n)
{
  prepend(0x80 + n);
  prepend('Q');
}

void Encoding::destructor(const PTree::Node *class_name)
{
  size_t len = class_name->length();
  append((unsigned char)(0x80 + len + 1));
  append('~');
  append(class_name->position(), len);
}

void Encoding::ptr_operator(int t)
{
  if(t == '*') prepend('P');
  else prepend('R');
}

void Encoding::ptr_to_member(const Encoding &enc, int n)
{
  prepend(enc);
  if(n >= 2)
  {
    prepend((unsigned char)(0x80 + n));
    prepend('Q');
  }
  prepend('M');
}

void Encoding::cast_operator(const Encoding &type)
{
  append((unsigned char)(0x80 + type.size() + 1));
  append('@');
  append(type);
}

void Encoding::array(unsigned long s) 
{
  std::ostringstream oss;
  oss << 'A' << s << '_';
  std::string str = oss.str();
  prepend(str.c_str(), str.size());
}

Encoding Encoding::get_scope()
{
  if (!is_qualified()) return "";    // no scope
  iterator i = begin() + 2;          // strip 'Q' and <size>
  size_t length = static_cast<size_t>(*i - 0x80);
  return Encoding(i, i + length + 1);
}

Encoding Encoding::get_symbol()
{
  if (!is_qualified()) return *this; // no scope
  iterator i = begin();
  size_t size = static_cast<size_t>(*++i - 0x80);
  size_t length = static_cast<size_t>(*++i - 0x80);
  i += length + 1;                   // strip outer scope
  Encoding retn(i, end());
  if (size > 2) retn.qualified(size - 1);
  return retn;
}

Encoding Encoding::get_template_arguments()
{
  int m = my_buffer[0] - 0x80;
  size_t length = my_buffer[1] - 0x80;
  if(m <= 0)
  {
    return Encoding(my_buffer.begin() + 2, my_buffer.begin() + 2 + length);
  }
  else
  {
    return Encoding(my_buffer.begin() + 2 + m, my_buffer.begin() + 2 + m + length);
  }
}

PTree::Node *Encoding::make_name()
{
  PTree::Node *name;
  int len = my_buffer[0] - 0x80;
  if(len > 0)
    name = new PTree::Identifier((const char*)&*(my_buffer.begin() + 1), len);
  else name = 0;
  my_buffer.erase(my_buffer.begin(), my_buffer.begin() + len + 1);
  return name;
}

PTree::Node *Encoding::make_qname()
{
  int n = my_buffer[0] - 0x80;
  PTree::Node *qname = 0;
  while(n-- > 0)
  {
    PTree::Node *name = make_name();
    if(name) qname = snoc(qname, name);
    if(n > 0) qname = snoc(qname, scope);
  }
  return qname;
}

PTree::Node *Encoding::make_ptree(PTree::Node *decl)
{
  PTree::Node *cv;
  PTree::Node *typespec = 0;
  if(decl) decl = PTree::list(decl);

  while(true)
  {
    cv = 0;
    unsigned char code = pop();
    switch(code)
    {
      case 'b' :
	typespec = PTree::snoc(typespec, bool_t);
	return PTree::list(typespec, decl);
      case 'c' :
	typespec = PTree::snoc(typespec, char_t);
	return PTree::list(typespec, decl);
      case 'w' :
	typespec = PTree::snoc(typespec, wchar_t_t);
	return PTree::list(typespec, decl);
      case 'i' :
	typespec = PTree::snoc(typespec, int_t);
	return PTree::list(typespec, decl);
      case 's' :
	typespec = PTree::snoc(typespec, short_t);
	return PTree::list(typespec, decl);
      case 'l' :
	typespec = PTree::snoc(typespec, long_t);
	return PTree::list(typespec, decl);
	break;
      case 'j' :
	typespec = PTree::nconc(typespec, PTree::list(long_t, long_t));
	return PTree::list(typespec, decl);
	break;
      case 'f' :
	typespec = PTree::snoc(typespec, float_t);
	return PTree::list(typespec, decl);
	break;
      case 'd' :
	typespec = PTree::snoc(typespec, double_t);
	return PTree::list(typespec, decl);
	break;
      case 'r' :
	typespec = PTree::nconc(typespec, PTree::list(long_t, double_t));
	return PTree::list(typespec, decl);
      case 'v' :
	typespec = PTree::snoc(typespec, void_t);
	return PTree::list(typespec, decl);
      case 'e' :
	return dots;
      case '?' :
	return PTree::list(typespec, decl);
      case 'Q' :
	typespec = PTree::snoc(typespec, make_qname());
	return PTree::list(typespec, decl);
      case 'S' :
	typespec = PTree::snoc(typespec, signed_t);
	break;
      case 'U' :
	typespec = PTree::snoc(typespec, unsigned_t);
	break;
      case 'C' :
	if(my_buffer[0] == 'V')
	{
	  pop();
	  cv = PTree::list(const_t, volatile_t);
	}
	else cv = PTree::list(const_t);
	goto const_or_volatile;
      case 'V' :
	cv = PTree::list(volatile_t);
      const_or_volatile :
	switch(my_buffer[0])
	{
	  case 'M' :
	  case 'P' :
	  case 'R' :
	    decl = PTree::nconc(cv, decl);
	    break;
	  case 'F' :
	    pop();
	    goto cv_function;
	  default :
	    typespec = PTree::nconc(cv, typespec);
	    break;
	}
	break;
      case 'M' :
        {
	  PTree::Node *ptr;
	  if(my_buffer[0] == 'Q')
	  {
	    pop();
	    ptr = make_qname();
	  }
	  else ptr = make_name();
	  
	  ptr = PTree::list(ptr, scope, star);
	  decl = PTree::cons(ptr, decl);
	}
	goto pointer_or_reference;
      case 'P' :
	decl = PTree::cons(star, decl);
	goto pointer_or_reference;
      case 'R' :
	decl = PTree::cons(ampersand, decl);
      pointer_or_reference :
	if(my_buffer[0] == 'A' || my_buffer[0] == 'F')
	  decl = PTree::list(PTree::list(left_paren, decl, right_paren));
	break;
      case 'A' :
	decl = PTree::nconc(decl, PTree::list(left_bracket, right_bracket));
	break;
      case 'F' :
      cv_function :
        {
	  PTree::Node *args = 0;
	  while(my_buffer[0] != '\0')
	  {
	    if(my_buffer[0] == '_')
	    {
	      pop();
	      break;
	    }
	    else if(my_buffer[0] == 'v')
	    {
	      pop(2);
	      break;
	    }
	    if(args != 0) args = PTree::snoc(args, comma);
	    args = PTree::snoc(args, make_ptree(0));
	  }
	  decl = PTree::nconc(decl, PTree::list(left_paren, args, right_paren));
	  if(cv) decl = PTree::nconc(decl, cv);
	}
	break;
      case '\0' :
	return PTree::list(typespec, decl);
      case 'T' :
	{
	  PTree::Node *tlabel = make_name();      
	  PTree::Node *args = 0;
	  int n = pop() - 0x80;
	  const unsigned char *stop = &*my_buffer.begin() + n;
	  while(&*my_buffer.begin() < stop)
	  {
	    if(args) args = PTree::snoc(args, comma);
	    args = PTree::snoc(args, make_ptree(0));
	  }
	  tlabel = PTree::list(tlabel, PTree::list(left_angle, args, right_angle));
	  typespec = PTree::nconc(typespec, tlabel);
	  return PTree::list(typespec, decl);
	}
      case '*' :
	goto error;
      default :
	prepend(code); // 'unget'
	if(code >= 0x80)
	{
	  if(typespec == 0) typespec = make_name();
	  else typespec = PTree::snoc(typespec, make_name());
	  return PTree::list(typespec, decl);
	}
      error :
	throw std::runtime_error("Encoding::make_ptree(): sorry, cannot handle this type");
	break;
    }
  }
}

PTree::Node *Encoding::name_to_ptree()
{
  if(my_buffer.empty()) return 0;
  if(my_buffer == (const unsigned char *)"new[]")
    return PTree::list(operator_name, anew_operator);
  else if(my_buffer == (const unsigned char *)"new")
    return PTree::list(operator_name, new_operator);
  else if(my_buffer == (const unsigned char *)"delete[]")
    return PTree::list(operator_name, adelete_operator);
  else if(my_buffer == (const unsigned char *)"delete")
    return PTree::list(operator_name, delete_operator);
  else if(my_buffer[0] == '~')
  {
    PTree::Encoding encoded(my_buffer.begin() + 1, my_buffer.end());
    return PTree::list(tilder, new PTree::Identifier(encoded.copy(), encoded.size()));
  }
  else if(my_buffer[0] == '@')
  {		// cast operator
    PTree::Encoding encoded(my_buffer.begin() + 1, my_buffer.end());
    return PTree::list(operator_name, encoded.make_ptree(0));
  }
  if(is_letter(my_buffer[0])) return new PTree::Identifier(copy(), my_buffer.size());
  else return PTree::list(operator_name, new PTree::Identifier(copy(), my_buffer.size()));
}
