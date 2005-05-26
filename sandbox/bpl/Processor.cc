//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <boost/python.hpp>
#include <boost/python/make_constructor.hpp>
#include <fstream>

namespace bpl = boost::python;
using namespace Synopsis;

namespace
{

Buffer *construct_buffer(char const *filename)
{
  std::ifstream ifs(filename);
  return new Buffer(ifs.rdbuf(), filename);
}

void write_buffer(Buffer const &buffer,
		  char const *output, std::string const &filename)
{
  std::ofstream ofs(output);
  buffer.write(ofs, filename);
}

//. insert the given string after the given node
void insert_buffer(Buffer &buffer, PTree::Node *node, std::string const &string)
{
  char *str = static_cast<char *>(memmove(new (GC) char[string.length() + 1],
					  string.data(), string.length()));
  str[string.length()] = '\0';
  buffer.replace(node->end(), node->end(), str, string.length());
}

//. replace the given node by the given string
void replace_buffer(Buffer &buffer, PTree::Node *node, std::string const &string)
{
  char *str = static_cast<char *>(memmove(new (GC) char[string.length() + 1],
					  string.data(), string.length()));
  str[string.length()] = '\0';
  buffer.replace(node->begin(), node->end(), str, string.length());
}

Parser *construct_parser(Lexer &lexer, SymbolLookup::Table &symbols, int rule_set)
{
  return new Parser(lexer, symbols, rule_set);
}

PTree::Node *parse(Parser *parser)
{
  PTree::Node *node = parser->parse();
  const Parser::ErrorList &errors = parser->errors();

  for (Parser::ErrorList::const_iterator i = errors.begin();
       i != errors.end();
       ++i)
    (*i)->write(std::cerr);
  return node;
}
}

BOOST_PYTHON_MODULE(Processor)
{
  bpl::class_<Buffer> buffer("Buffer", bpl::no_init);
  buffer.def("__init__", bpl::make_constructor(construct_buffer));
  buffer.def("write", write_buffer);
  buffer.def("insert", insert_buffer);
  buffer.def("replace", replace_buffer);

  bpl::enum_<Lexer::TokenSet> token_set("TokenSet");
  token_set.value("C", Lexer::C);
  token_set.value("CXX", Lexer::CXX);
  token_set.value("GCC", Lexer::GCC);
  token_set.value("MSVC", Lexer::MSVC);

  bpl::class_<Lexer> lexer("Lexer", bpl::init<Buffer *, int>());

  bpl::enum_<Parser::RuleSet> rule_set("RuleSet");
  rule_set.value("CXX", Parser::CXX);
  rule_set.value("MSVC", Parser::MSVC);

  bpl::class_<Parser> parser("Parser", bpl::no_init);
  // keep the symbol table hidden as it will be exported in its own module later
  parser.def("__init__", bpl::make_constructor(construct_parser));
  // The PTree module uses garbage collection so just ignore memory management,
  // at least for now.
  parser.def("parse", parse, bpl::return_value_policy<bpl::reference_existing_object>());
}