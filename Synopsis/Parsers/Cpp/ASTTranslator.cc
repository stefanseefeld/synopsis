//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASTTranslator.hh"
#include <Synopsis/Trace.hh>
#include <Support/Path.hh>

using namespace Synopsis;
namespace wave = boost::wave;

ASTTranslator::ASTTranslator(std::string const &language,
			     std::string const &filename,
			     std::string const &base_path, bool primary_file_only,
			     AST::AST ast, bool v, bool d)
  : ast_(ast),
    ast_kit_(),
    sf_kit_(language),
    type_kit_(language),
    raw_filename_(filename),
    base_path_(base_path),
    primary_file_only_(primary_file_only),
    mask_counter_(0),
    verbose_(v),
    debug_(d)
{
  Trace trace("ASTTranslator::ASTTranslator", Trace::TRANSLATION);
  file_stack_.push(lookup_source_file(raw_filename_, true));
}

void ASTTranslator::expanding_function_like_macro(Token const &macrodef, 
						  std::vector<Token> const &formal_args, 
						  Container const &definition,
						  Token const &macrocall,
						  std::vector<Container> const &arguments) 
{
  Trace trace("ASTTranslator::expand_function_like_macro", Trace::TRANSLATION);
  if (mask_counter_) return;

  Token::position_type position = macrocall.get_position();
  std::string name;
  {
    Token::string_type tmp = macrocall.get_value();
    name = std::string(tmp.begin(), tmp.end());
  }
  // argument list
//   for (Container::size_type i = 0; i < arguments.size(); ++i) 
//   {
//     std::cout << wave::util::impl::as_string(arguments[i]);
//     if (i < arguments.size()-1)
//       std::cout << ", ";
//   }
  Python::Dict mmap = file_stack_.top().macro_calls();
  Python::List line = mmap.get(position.get_line(), Python::List());
  // TODO: compute correct positional parameters for the macro replacement:
  //       * start: start position of the replacement in the output stream.
  //       * end: end position of the replacement in the output stream.
  //       * diff: offset by which to shift back the following tokens to
  //               match the input stream.
  line.append(sf_kit_.create_macro_call(name, 0, 0, 0));//start, end, diff));
  mmap.set(position.get_line(), line);
}
 
void ASTTranslator::expanding_object_like_macro(Token const &macro, 
						Container const &definition,
						Token const &macrocall)
{
  Trace trace("ASTTranslator::expand_object_like_macro", Trace::TRANSLATION);
  if (mask_counter_) return;

  Token::position_type position = macrocall.get_position();
  std::string name;
  {
    Token::string_type tmp = macrocall.get_value();
    name = std::string(tmp.begin(), tmp.end());
  }
  Python::Dict mmap = file_stack_.top().macro_calls();
  Python::List line = mmap.get(position.get_line(), Python::List());
  line.append(sf_kit_.create_macro_call(name, 0, 0, 0));//start, end, diff));
  mmap.set(position.get_line(), line);
}
 
void ASTTranslator::expanded_macro(Container const &result)
{
  Trace trace("ASTTranslator::expand_macro", Trace::TRANSLATION);
  if (mask_counter_) return;
//   std::cout << wave::util::impl::as_string(result) << std::endl;
}
 
void ASTTranslator::rescanned_macro(Container const &result)
{
  Trace trace("ASTTranslator::rescanned_macro", Trace::TRANSLATION);
//   std::cout << wave::util::impl::as_string(result) << std::endl;
}

void ASTTranslator::found_include_directive(std::string const &filename, bool next)
{
  Trace trace("ASTTranslator::found_include_directive", Trace::TRANSLATION);
  trace << filename;

  include_dir_ = filename;
  include_next_dir_ = next;
}

void ASTTranslator::opened_include_file(std::string const &relname, 
					std::string const &absname, 
					std::size_t include_depth,
					bool is_system_include)
{
  Trace trace("ASTTranslator::opened_include_file", Trace::TRANSLATION);
  trace << absname;
  if (mask_counter_)
  {
    ++mask_counter_;
    return;
  }
  AST::SourceFile sf = lookup_source_file(relname, false);

  AST::Include include = sf_kit_.create_include(sf, include_dir_,
						false, include_next_dir_);
  Python::List includes = file_stack_.top().includes();
  includes.append(include);
  file_stack_.push(sf);

  // The following confusing naming indicates something needs
  // to be cleared up !
  std::string abs_filename = Path(relname).abs().str();
  // Only keep the first level of includes starting from the last unmasked file.
  if (primary_file_only_ || 
      (base_path_.size() && 
       abs_filename.substr(0, base_path_.size()) != base_path_))
    ++mask_counter_;
}

void ASTTranslator::returning_from_include_file()
{
  Trace trace("ASTTranslator::returning_from_include_file", Trace::TRANSLATION);
  if (mask_counter_ < 2) file_stack_.pop();
  // if the file was masked, decrement the counter
  if (mask_counter_) --mask_counter_;
}

void ASTTranslator::defined_macro(Token const &name, bool is_functionlike,
				  std::vector<Token> const &parameters,
				  Container const &definition,
				  bool is_predefined)
{
  Trace trace("ASTTranslator::defined_macro", Trace::TRANSLATION);
  trace << name;
  if (mask_counter_ || is_predefined) return;
  
  Token::string_type const &m = name.get_value();
  std::string macro_name(m.begin(), m.end());
  Token::position_type position = name.get_position();

  std::string text;
  {
    Token::string_type tmp = wave::util::impl::as_string(definition);
    text = std::string(tmp.begin(), tmp.end());
  }
  Python::List params;
  for (std::vector<Token>::const_iterator i = parameters.begin();
       i != parameters.end(); ++i) 
  {
    Token::string_type const &tmp = i->get_value();
    params.append(std::string(tmp.begin(), tmp.end()));
  }

  AST::ScopedName qname(macro_name);
  AST::Macro macro = ast_kit_.create_macro(file_stack_.top(),
					   position.get_line(),
					   qname,
					   params, text);
  AST::Declared declared = type_kit_.create_declared(qname, macro);
  Python::List declarations = ast_.declarations();
  declarations.append(macro);

  // FIXME: the 'types' attribute is not (yet) a dict type
  // so we have to do the call conversions manually...
  Python::Object types = ast_.types();
  types.attr("__setitem__")(Python::Tuple(qname, declared));
}

void ASTTranslator::undefined_macro(Token const &name)
{
  Trace trace("ASTTranslator::undefined_macro", Trace::TRANSLATION);
  trace << name;
}

AST::SourceFile ASTTranslator::lookup_source_file(std::string const &filename,
						  bool primary)
{
  Trace trace("ASTTranslator::lookup_source_file", Trace::TRANSLATION);
  trace << filename << ' ' << primary;
  Path path = Path(filename).abs();
  path.strip(base_path_);
  std::string short_name = path.str();

  Python::Dict files = ast_.files();
  AST::SourceFile sf = files.get(short_name);
  if (!sf)
  {
    sf = sf_kit_.create_source_file(short_name, filename);
    files.set(short_name, sf);
  }
  if (sf && primary) sf.set_primary(true);
  return sf;
}
