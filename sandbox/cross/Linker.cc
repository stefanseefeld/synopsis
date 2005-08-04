#include "Linker.hh"

using Synopsis::Trace;
using Synopsis::Buffer;
using Synopsis::Lexer;
using Synopsis::SymbolFactory;
using Synopsis::Parser;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

Linker::Linker(Synopsis::Buffer &buffer,
	       Synopsis::SymbolTable::Scope *scope,
	       std::ostream &os)
  : ST::Walker(scope),
    my_buffer(buffer),
    my_os(os)
{
}

void Linker::link(PT::Node *node)
{
  node->accept(this);
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
  std::string begin = "<a href=\"dummy\" title=\"dummy\">";
  for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
    *i += 0x80;
  std::string end = "</a>";
  for (std::string::iterator i = end.begin(); i != end.end(); ++i)
    *i += 0x80;
  my_buffer.replace(node->position(), node->position(),
		    begin.data(), begin.size());
  my_buffer.replace(node->position() + node->length(),
		    node->position() + node->length(),
		    end.data(), end.size());
}

void Linker::visit(PT::Name *node)
{
  std::string begin = "<a href=\"dummy\" title=\"dummy\">";
  for (std::string::iterator i = begin.begin(); i != begin.end(); ++i)
    *i += 0x80;
  std::string end = "</a>";
  for (std::string::iterator i = end.begin(); i != end.end(); ++i)
    *i += 0x80;
  my_buffer.replace(node->position(), node->position(),
		    begin.data(), begin.size());
  my_buffer.replace(node->position() + node->length(),
		    node->position() + node->length(),
		    end.data(), end.size());
}

void Linker::visit(PT::ClassSpec *node)
{
  PT::Node *ident = PT::second(node);
  if (ident) ident->accept(this);
  traverse_body(node);
}

