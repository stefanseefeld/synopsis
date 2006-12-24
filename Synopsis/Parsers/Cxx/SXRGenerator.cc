//
// Copyright (C) 2006 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "SXRGenerator.hh"
#include <Synopsis/Trace.hh>

using Synopsis::Trace;
using Synopsis::Buffer;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

namespace
{
class SymbolTypeFinder : private ST::SymbolVisitor
{
public:
  std::string type(ST::Symbol const *s) 
  {
    s->accept(this);
    return type_;
  }
private:
  virtual void visit(ST::Symbol const *) { type_ = "symbol";}
  virtual void visit(ST::VariableName const *) { type_ = "variable";}
  virtual void visit(ST::ConstName const *) { type_ = "constant";}
  virtual void visit(ST::TypeName const *) { type_ = "type";}
  virtual void visit(ST::TypedefName const *) { type_ = "typedef";}
  virtual void visit(ST::ClassName const *) { type_ = "class";}
  virtual void visit(ST::EnumName const *) { type_ = "enum";}
  virtual void visit(ST::ClassTemplateName const *) 
  { type_ = "class template";}
  virtual void visit(ST::FunctionName const *) { type_ = "function";}
  virtual void visit(ST::FunctionTemplateName const *) 
  { type_ = "function template";}
  virtual void visit(ST::NamespaceName const *) { type_ = "namespace";}
  
  std::string type_;
};

class QNameFinder : private ST::ScopeVisitor
{
public:
  void find(ST::Symbol const *symbol, std::string &qname, std::string &type)
  {
    symbol->scope()->accept(this);

    SymbolTypeFinder type_finder;
    type = type_ + type_finder.type(symbol);
    qname = qname_ + "::" + qname;
  }
private:
  virtual void visit(ST::TemplateParameterScope *) 
  {
    qname_ = "";
    type_ = "template parameter ";
  }
  virtual void visit(ST::LocalScope *)
  {
    qname_ = "";
    type_ = "local ";
  }
  virtual void visit(ST::PrototypeScope *)
  {
    qname_ = "";
    type_ = "local ";
  }
  virtual void visit(ST::FunctionScope *)
  {
    qname_ = "";
    type_ = "local ";
  }
  virtual void visit(ST::Class *scope)
  {
    ST::Scope *outer = scope->outer_scope();
    if (outer)
    {
      outer->accept(this);
      if (!qname_.empty()) qname_ += "::";
    }
    qname_ += scope->name();
    type_ = "member ";
  }
  virtual void visit(ST::Namespace *scope)
  {
    ST::Scope *outer = scope->outer_scope();
    if (outer)
    {
      outer->accept(this);
      if (!qname_.empty()) qname_ += "::";
    }
    qname_ += scope->name();
    type_ = "global ";
  }

  std::string qname_;
  std::string type_;
};

//. Encodes an incoming stream as html.
class Formatter
{
public:
  Formatter(std::ostream &os);
  void format(Synopsis::Buffer::iterator begin, Synopsis::Buffer::iterator end);

private:
  std::ostream &os_;
  bool          comment_;
  bool          ccomment_;
};

char const *comment_start = "<span class=\"comment\">";

Formatter::Formatter(std::ostream &os) 
  : os_(os),
    comment_(false),
    ccomment_(false)
{
}

void Formatter::format(Buffer::iterator begin, Buffer::iterator end)
{
  os_ << "<sxr>\n";
  os_ << "<line>";
  unsigned long column = 0;
  while (begin != end)
  {
    switch (*begin)
    {
      case '<': os_ << "&lt;"; break;
      case '>': os_ << "&gt;"; break;
      case '"': os_ << "&quot;"; break;
      case '&': os_ << "&amp;"; break;
      case '\n':
	if (ccomment_ || comment_) os_ << "</span>";
	comment_ = false;
	os_ << "</line>\n";
	os_ << "<line>";
	column = 0;
	if (ccomment_) os_ << comment_start;
	break;
      case '/':
      {
	Buffer::iterator next = begin;
	++next; // Buffer::iterator is only a forward iterator, not random access...
	if (next != end)
	  switch (*next)
	  {
	    case '/':
	      os_ << comment_start << "//";
	      begin = next;
	      comment_ = true;
	      break;
	    case '*':
	      os_ << comment_start << "/*";
	      begin = next;
	      ccomment_ = true;
	      break;
  	    default:
	      os_.put('/');
	      break;
	  }
	break;
      }
      case '*':
	os_.put('*');
	if (ccomment_)
	{
	  Buffer::iterator next = begin;
	  ++next;
	  if (next != end && *next == '/')
	    os_ << "/</span>";
	  ccomment_ = false;
	}
	break;
      default:
	if (static_cast<unsigned char>(*begin) >= 128)
	  os_.put(static_cast<unsigned char>(*begin) - 128);
	else
	  os_.put(*begin);
	break;
    }
    ++begin;
  }
  os_ << "</line>\n</sxr>\n";
}

}


SXRGenerator::SXRGenerator(Buffer &buffer, ST::Scope *scope, std::ostream &os)
  : ST::Walker(scope),
    buffer_(buffer),
    os_(os)
{
}

void SXRGenerator::process(PT::Node *node)
{
  node->accept(this);
  Formatter formatter(os_);
  formatter.format(buffer_.begin(), buffer_.end());
}

void SXRGenerator::link(PT::Node *node, PT::Encoding const &name)
{
  ST::SymbolSet symbols = lookup(name, ST::Scope::DEFAULT);
  if (symbols.empty())
  {
    std::string filename;
    unsigned long lineno = buffer_.origin(node->position(), filename);
    std::cerr << "Warning: '" << name.unmangled() 
	      << "' unknown at " << filename << ':' << lineno << std::endl;
  }
  else if (symbols.size() > 1)
  {
    std::string filename;
    unsigned long lineno = buffer_.origin(node->position(), filename);
    std::cerr << "Warning: '" << name.unmangled() 
	      << "' ambiguous at " << filename << ':' << lineno << std::endl;
  }
  else
  {
    std::string qname = name.unmangled();
    std::string type;
    QNameFinder finder;
    finder.find(*symbols.begin(), qname, type);
    if (qname.substr(0, 8) == "<global>") qname = qname.substr(8);
    std::string title = type + " " + qname;
    std::string begin = "<a href=\"dummy\" title=\"" + title + "\">";
    for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
      *i += 0x80;
    std::string end = "</a>";
    for (std::string::iterator i = end.begin(); i != end.end(); ++i)
      *i += 0x80;
    buffer_.replace(node->begin(), node->begin(), begin.data(), begin.size());
    buffer_.replace(node->end(), node->end(), end.data(), end.size());
  }
}

void SXRGenerator::visit(PT::Literal *node)
{
  if (buffer_.is_replaced(node->position())) return;
  std::string begin = "<span class=\"literal\">";
  for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
    *i += 0x80;
  std::string end = "</span>";
  for (std::string::iterator i = end.begin(); i != end.end(); ++i)
    *i += 0x80;
  buffer_.replace(node->position(), node->position(),
		  begin.data(), begin.size());
  buffer_.replace(node->position() + node->length(),
		  node->position() + node->length(),
		  end.data(), end.size());
}

void SXRGenerator::visit(PT::Keyword *node)
{
  if (buffer_.is_replaced(node->position())) return;
  std::string begin = "<span class=\"keyword\">";
  for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
    *i += 0x80;
  std::string end = "</span>";
  for (std::string::iterator i = end.begin(); i != end.end(); ++i)
    *i += 0x80;
  buffer_.replace(node->position(), node->position(),
		  begin.data(), begin.size());
  buffer_.replace(node->position() + node->length(),
		  node->position() + node->length(),
		  end.data(), end.size());
}

void SXRGenerator::visit(PT::Identifier *node)
{
  if (buffer_.is_replaced(node->position())) return;
  link(node, PT::Encoding(node));
}

void SXRGenerator::visit(PT::Name *node)
{
  if (buffer_.is_replaced(node->begin())) return;
  link(node, node->encoded_name());
}

void SXRGenerator::visit(PT::ClassSpec *node)
{
  if (buffer_.is_replaced(node->begin())) return;
  visit(node->key());
  PT::Node *ident = node->name();
  if (ident) ident->accept(this);
  traverse_body(node);
}

