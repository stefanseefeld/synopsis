//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Trace.hh>
#include <Synopsis/PTree/Node.hh>
#include <Synopsis/PTree/Atoms.hh>
#include <Synopsis/PTree/Encoding.hh>
#include <Synopsis/Lexer.hh>
#include <iostream>
#include <sstream>

using namespace Synopsis;
using namespace PTree;

namespace
{
class Unmangler
{
public:
  typedef Encoding::iterator iterator;

  Unmangler(iterator begin, iterator end) : my_cursor(begin), my_end(end) {}

  std::string unmangle();
  std::string unmangle_name();
  std::string unmangle_qname();
  std::string unmangle_template();
  std::string unmangle_func(std::string&);

private:
  iterator my_cursor;
  iterator const my_end;
};

std::string Unmangler::unmangle_name()
{
//   Trace trace("Unmangler::unmangle_name()", Trace::PTREE);
  size_t length = *my_cursor++ - 0x80;
  std::string name(length, '\0');
  std::copy(my_cursor, my_cursor + length, name.begin());
  my_cursor += length;
  return name;
}

std::string Unmangler::unmangle()
{
  Trace trace("Unmangler::unmangle()", Trace::PTREE, Encoding(my_cursor, my_end));
  std::string premod, postmod;
  std::string name;
  std::string base;
  
  if (*my_cursor == 0x80) return "";
  while (my_cursor != my_end && !name.length() && !base.length())
  {
    int c = *my_cursor++;
    switch (c)
    {
      case 'P':
	postmod += "*";
	break;
      case 'R':
	postmod += "&";
	break;
      case 'S':
	premod += "signed ";
	break;
      case 'U':
	premod += "unsigned ";
	break;
      case 'C':
	premod += "const ";
	break;
      case 'V':
	premod += "volatile ";
	break;
      case 'A':
      {
	std::string array("[");
	while (*my_cursor != '_') array += *my_cursor++;
	array += ']';
	++my_cursor;
	postmod += array;
	break;
      }
      case '*':
	base = "*";
	break;
      case 'i':
	name = "int";
	break;
      case 'v':
	name = "void";
	break;
      case 'b':
	name = "bool";
	break;
      case 's':
	name = "short";
	break;
      case 'c':
	name = "char";
	break;
      case 'w':
	name = "wchar_t";
	break;
      case 'l':
	name = "long";
	break;
      case 'j':
	name = "long long";
	break;
      case 'f':
	name = "float";
	break;
      case 'd':
	name = "double";
	break;
      case 'r':
	name = "long double";
	break;
      case 'e':
	name = "...";
	break;
      case '?':
	return ""; //FIXME !
      case 'Q':
	base = unmangle_qname();
	break;
      case '_':
	--my_cursor;
	return ""; // end of func params
      case 'F':
	base = unmangle_func(postmod);
	break;
      case 'T':
	base = unmangle_template();
	break;
      case 'M':
	// Pointer to member. Format is same as for named types
	name = unmangle_name() + "::*";
	break;
      default:
	assert(c > 0x80);
	--my_cursor;
	name = unmangle_name();
	break;
    } // switch
  } // while
  if (!base.length() && !name.length())
    throw std::runtime_error("unmangling error");
  if (!base.length())
    base = name;
  return premod + base + postmod;
}

std::string Unmangler::unmangle_qname()
{
//   Trace trace("Unmangler::unmangle_qname()", Trace::PTREE);
  // Qualified type: first is num of scopes (at least one), each a name.
  std::string qname;
  int scopes = *my_cursor++ - 0x80;
  while (scopes--)
  {
    std::string name;
    // Only handle two things here: names and templates
    if (*my_cursor > 0x80) name = unmangle_name();
    else if (*my_cursor == 0x80) name = "";
    else if (*my_cursor == 'T')
    {
      ++my_cursor;
      name = unmangle_name();
      name += '<';
      iterator tend = my_cursor;
      tend += *my_cursor++ - 0x80;
      bool first = true;
      while (my_cursor <= tend)
      {
	if (!first) name += ',';
	else first = false;
	name += unmangle();
      }
      name += '>';
    }
    else
    {
      
      std::cerr << "Warning: Unknown type inside Q: " << *my_cursor << std::endl;
      // FIXME
      //std::cerr << "         Decoding " << my_string << std::endl;
    }
    if (qname.length()) qname += "::" + name;
    else qname = name;
  }
  return qname;
}

std::string Unmangler::unmangle_func(std::string& postmod)
{
  Trace trace("Unmangler::unmangle_func", Trace::PTREE);
  // Function ptr. Encoded same as function
  std::string premod;
  // Move * from postmod to funcptr's premod. This makes the output be
  // "void (*convert)()" instead of "void (convert)()*"
  if (postmod.size() > 0 && postmod[0] == '*')
  {
    premod += postmod[0];
    postmod.erase(postmod.begin());
  }
  std::vector<std::string> params;
  while (true)
  {
    std::string type = unmangle();
    if (type.empty()) break;
    else params.push_back(type);
  }
  ++my_cursor; // skip over '_'
  trace << "all is still well";
  std::string returnType = unmangle();
  std::string ret = returnType + "(*)(";
  if (params.size())
  {
    ret += params[0];
    for (size_t p = 1; p != params.size(); ++p) ret += "," + params[p];
  }
  ret += ")";
  return ret;
}

std::string Unmangler::unmangle_template()
{
//   Trace trace("Unmangler::unmangle_template()", Trace::PTREE);

  // Template type: Name first, then size of arg field, then arg
  // types eg: T6vector54cell <-- 5 is len of 4cell
  if (*my_cursor == 'T')
    ++my_cursor;
  std::string name = unmangle_name();
  iterator tend = my_cursor;
  tend += *my_cursor++ - 0x80;
  name += "<";
  if (my_cursor <= tend) name += unmangle();
  while (my_cursor <= tend) name += "," + unmangle();
  name += ">";
  return name;
}

}

void Encoding::cv_qualify(bool c, bool v)
{
  if (v) prepend('V');
  if (c) prepend('C');
}

void Encoding::global_scope()
{
  append('Q');
  append(0x80 + 1);
  append(0x80);
}

void Encoding::simple_name(Atom const *id)
{
  append_with_length(id->position(), id->length());
  if (is_qualified()) ++my_buffer.at(1);
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

void Encoding::template_(Atom const *name, Encoding const &args)
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

Encoding::iterator Encoding::advance(iterator i)
{
  while (true)
  {
    unsigned int c = *i++;
    switch(c)
    {
      // modifiers
      case 'P':
      case 'Q': // not a qualifier, but skippable anyways.
      case 'R':
      case 'S':
      case 'U':
      case 'C':
      case 'V': break;

      // built-in types
      case 'b':
      case 'c':
      case 'w':
      case 'i':
      case 's':
      case 'l':
      case 'j':
      case 'f':
      case 'd':
      case 'r':
      case 'v':
      case 'e':
      case '?': return i;
	
      case 'A':
	while (*i != '_') ++i;
	return i + 1;

      case 'T':
	i += *i - 0x80 + 1; // skip template name
	i += *i - 0x80 + 1; // skip template parameters
	return i;

      case 'F':
	while (*i != '_') ++i; // skip parameter list
	break;                 // stay in the loop for the return type

      default:
	assert(c >= 0x80);
	i += c - 0x80; // skip name
	return i;
    }
  }
}

Encoding::iterator Encoding::end_of_scope() const
{
  if (!is_qualified()) return end(); // no scope
  
  iterator i = begin() + 2;                 // skip 'Q' and <size>
  if (*i >= 0x80) return i + *i - 0x80 + 1; // simple name
  if (*i == 'T')                            // template
  {
    i += *(i+1) - 0x80 + 2;                 // skip 'T' and simple name
    i += *i - 0x80 + 1;                     // skip template parameters
    return i;
  }
  // never get here
  std::ostringstream oss;
  oss << "internal error in qualified name encoding " << my_buffer;
  throw std::domain_error(oss.str());
}

Encoding::iterator Encoding::end_name(iterator i)
{
  // Assume 'i' points to the beginning of a simple name or a template.
  // Return the one-past-end of it.
  if (*i >= 0x80) return i + *i - 0x80 + 1;
  else if (*i == 'T')
  {
    i += *++i - 0x80 + 1; // skip name
    i += *++i - 0x80 + 1; // skip parameters
    return i;
  }
  assert(0);
}

Encoding Encoding::get_scope() const
{
  if (!is_qualified()) return "";    // no scope
  return Encoding(begin() + 2, end_of_scope());
}

Encoding Encoding::get_symbol() const
{
  if (!is_qualified()) return *this; // no scope
  iterator i = begin() + 1;
  size_t size = static_cast<size_t>(*i - 0x80);
  Encoding retn(end_of_scope(), end());
  if (size > 2) retn.qualified(size - 1);
  return retn;
}

Encoding Encoding::get_template_name() const
{
  assert(is_template_id());
  iterator begin = my_buffer.begin() + 1;
  return Encoding(begin, begin + *begin - 0x80 + 1);
}

std::string Encoding::unmangled() const
{
  if (empty()) return "";
  Unmangler unmangler(begin(), end());
  return unmangler.unmangle();
}

Encoding Encoding::get_template_arguments() const
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

