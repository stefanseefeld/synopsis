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


PTree::Node *parse(Buffer &buffer)
{
  Lexer lexer(&buffer);
  SymbolLookup::Table symbols;
  Parser parser(lexer, symbols);
  PTree::Node *node = parser.parse();
  const Parser::ErrorList &errors = parser.errors();

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

  // The PTree module uses garbage collection so just ignore memory management,
  // at least for now.
  bpl::def("parse", parse, bpl::return_value_policy<bpl::reference_existing_object>());
}
