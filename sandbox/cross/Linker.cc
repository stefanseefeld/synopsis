//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "Linker.hh"

using Synopsis::Trace;
using Synopsis::Buffer;
using Synopsis::Lexer;
using Synopsis::SymbolFactory;
using Synopsis::Parser;
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
    return my_type;
  }
private:
  virtual void visit(ST::Symbol const *) { my_type = "symbol";}
  virtual void visit(ST::VariableName const *) { my_type = "variable";}
  virtual void visit(ST::ConstName const *) { my_type = "constant";}
  virtual void visit(ST::TypeName const *) { my_type = "type";}
  virtual void visit(ST::TypedefName const *) { my_type = "typedef";}
  virtual void visit(ST::ClassName const *) { my_type = "class";}
  virtual void visit(ST::EnumName const *) { my_type = "enum";}
  virtual void visit(ST::ClassTemplateName const *) 
  { my_type = "class template";}
  virtual void visit(ST::FunctionName const *) { my_type = "function";}
  virtual void visit(ST::FunctionTemplateName const *) 
  { my_type = "function template";}
  virtual void visit(ST::NamespaceName const *) { my_type = "namespace";}
  
  std::string my_type;
};

class QNameFinder : private ST::ScopeVisitor
{
public:
  void find(ST::Symbol const *symbol, std::string &qname, std::string &type)
  {
    symbol->scope()->accept(this);

    SymbolTypeFinder type_finder;
    type = my_type + type_finder.type(symbol);
    qname = my_qname + "::" + qname;
  }
private:
  virtual void visit(ST::TemplateParameterScope *) 
  {
    my_qname = "";
    my_type = "template parameter ";
  }
  virtual void visit(ST::LocalScope *)
  {
    my_qname = "";
    my_type = "local ";
  }
  virtual void visit(ST::PrototypeScope *)
  {
    my_qname = "";
    my_type = "local ";
  }
  virtual void visit(ST::FunctionScope *)
  {
    my_qname = "";
    my_type = "local ";
  }
  virtual void visit(ST::Class *scope)
  {
    ST::Scope *outer = scope->outer_scope();
    if (outer)
    {
      outer->accept(this);
      if (!my_qname.empty()) my_qname += "::";
    }
    my_qname += scope->name();
    my_type = "member ";
  }
  virtual void visit(ST::Namespace *scope)
  {
    ST::Scope *outer = scope->outer_scope();
    if (outer)
    {
      outer->accept(this);
      if (!my_qname.empty()) my_qname += "::";
    }
    my_qname += scope->name();
    my_type = "global ";
  }

  std::string my_qname;
  std::string my_type;
};

}


Linker::Linker(Buffer &buffer,
	       ST::Scope *scope,
	       std::ostream &os)
  : ST::Walker(scope),
    my_buffer(buffer),
    my_os(os)
{
}

void Linker::process(PT::Node *node)
{
  node->accept(this);
}

void Linker::link(PT::Node *node,
		  PT::Encoding const &name)
{
  ST::SymbolSet symbols = lookup(name, ST::Scope::DEFAULT);
  if (symbols.empty())
  {
    std::string filename;
    unsigned long lineno = my_buffer.origin(node->position(), filename);
    std::cerr << "Warning: '" << name.unmangled() 
	      << "' unknown at " << filename << ':' << lineno << std::endl;
  }
  else if (symbols.size() > 1)
  {
    std::string filename;
    unsigned long lineno = my_buffer.origin(node->position(), filename);
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
    my_buffer.replace(node->begin(), node->begin(),
		      begin.data(), begin.size());
    my_buffer.replace(node->end(), node->end(),
		      end.data(), end.size());
  }
}

void Linker::visit(PT::Literal *node)
{
  std::string begin = "<span class=\"literal\">";
  for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
    *i += 0x80;
  std::string end = "</span>";
  for (std::string::iterator i = end.begin(); i != end.end(); ++i)
    *i += 0x80;
  my_buffer.replace(node->position(), node->position(),
		    begin.data(), begin.size());
  my_buffer.replace(node->position() + node->length(),
		    node->position() + node->length(),
		    end.data(), end.size());
}

void Linker::visit(PT::Keyword *node)
{
  std::string begin = "<span class=\"keyword\">";
  for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
    *i += 0x80;
  std::string end = "</span>";
  for (std::string::iterator i = end.begin(); i != end.end(); ++i)
    *i += 0x80;
  my_buffer.replace(node->position(), node->position(),
		    begin.data(), begin.size());
  my_buffer.replace(node->position() + node->length(),
		    node->position() + node->length(),
		    end.data(), end.size());
}

void Linker::visit(PT::Identifier *node)
{
  link(node, PT::Encoding::simple_name(node));
}

void Linker::visit(PT::Name *node)
{
  link(node, node->encoded_name());
}

void Linker::visit(PT::ClassSpec *node)
{
  PT::Node *ident = PT::second(node);
  if (ident) ident->accept(this);
  traverse_body(node);
}

