//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASTTranslator.hh"
#include <Synopsis/Trace.hh>
#include <Support/path.hh>

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
    macro_level_counter_(0),
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
  ++macro_level_counter_;
  if (mask_counter_) return;

  position_ = macrocall.get_position();
  Token::string_type tmp = macrocall.get_value();
  current_macro_name_.assign(tmp.begin(), tmp.end());
  current_macro_call_length_ =
    arguments.back().back().get_position().get_column() +
    // hack to take into account the following closing paren
    arguments.back().back().get_value().size() + 1 -
    position_.get_column();
}
 
void ASTTranslator::expanding_object_like_macro(Token const &macro, 
						Container const &definition,
						Token const &macrocall)
{
  Trace trace("ASTTranslator::expand_object_like_macro", Trace::TRANSLATION);
  ++macro_level_counter_;
  if (mask_counter_) return;

  position_ = macrocall.get_position();
  Token::string_type tmp = macrocall.get_value();
  current_macro_name_.assign(tmp.begin(), tmp.end());
  current_macro_call_length_ = current_macro_name_.size();
}
 
void ASTTranslator::expanded_macro(Container const &result)
{
  Trace trace("ASTTranslator::expand_macro", Trace::TRANSLATION);
}
 
void ASTTranslator::rescanned_macro(Container const &result)
{
  Trace trace("ASTTranslator::rescanned_macro", Trace::TRANSLATION);
  if (!--macro_level_counter_)
  {
    // All (potentially recursive) scanning is finished at this point, so we
    // can create the MacroCall object.
    //
    // The positions of the tokens in the result vector are the ones from
    // the macro definition, so we only extract the expanded length, and
    // then calculate the new positions in the (yet to be written) preprocessed
    // file.
    unsigned int begin = result.front().get_position().get_column();
    unsigned int end = 
      result.back().get_position().get_column() + 
      result.back().get_value().size();
    unsigned int length = end - begin;
    // We assume the expansion happens at the point where the macro call occured,
    // i.e. any whitespace or comments are unaltered.
    // Wave starts to count columns at index 1, synopsis at 0.
    begin = position_.get_column() - 1;
    end = begin + length;
    Python::Dict mmap = file_stack_.top().macro_calls();
    Python::List line = mmap.get(position_.get_line() - 1, Python::List());
    line.append(sf_kit_.create_macro_call(current_macro_name_,
					  begin, end,
					  current_macro_call_length_ - length));
    mmap.set(position_.get_line() - 1, line);
  }
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

  std::string abs_filename = make_full_path(relname);
  // Only keep the first level of includes starting from the last unmasked file.
  if (primary_file_only_ || !matches_path(abs_filename, base_path_))
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

  std::string long_name = make_full_path(filename);
  std::string short_name = make_short_path(filename, base_path_);

  Python::Dict files = ast_.files();
  AST::SourceFile sf = files.get(short_name);
  if (!sf)
  {
    sf = sf_kit_.create_source_file(short_name, long_name);
    files.set(short_name, sf);
  }
  if (sf && primary) sf.set_primary(true);
  return sf;
}
