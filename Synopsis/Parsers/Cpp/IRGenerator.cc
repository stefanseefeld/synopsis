//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "IRGenerator.hh"
#include <Synopsis/Trace.hh>
#include <Support/path.hh>

namespace boost { namespace python {

    inline long hash(object const& obj)
    {
        long result = PyObject_Hash(obj.ptr());
        if (PyErr_Occurred()) throw_error_already_set();
        return result;
    }

}} // namespace boost::python



IRGenerator::IRGenerator(std::string const &language,
                         std::string const &filename,
                         std::string const &base_path, bool primary_file_only,
                         bpl::object ir, bool v, bool d)
  : language_(language),
    asg_module_(bpl::import("Synopsis.ASG")),
    sf_module_(bpl::import("Synopsis.SourceFile")),
    declarations_(bpl::extract<bpl::list>(ir.attr("declarations"))()),
    types_(bpl::extract<bpl::dict>(ir.attr("types"))()),
    files_(bpl::extract<bpl::dict>(ir.attr("files"))()),
    raw_filename_(filename),
    base_path_(base_path),
    primary_file_only_(primary_file_only),
    mask_counter_(0),
    macro_level_counter_(0),
    verbose_(v),
    debug_(d)
{
  Trace trace("IRGenerator::IRGenerator", Trace::TRANSLATION);
  file_stack_.push(lookup_source_file(raw_filename_, true));
}

void IRGenerator::expanding_function_like_macro(Token const &macrodef, 
                                                std::vector<Token> const &formal_args, 
                                                Container const &definition,
                                                Token const &macrocall,
                                                std::vector<Container> const &arguments) 
{
  Trace trace("IRGenerator::expand_function_like_macro", Trace::TRANSLATION);
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
 
void IRGenerator::expanding_object_like_macro(Token const &macro, 
                                              Container const &definition,
                                              Token const &macrocall)
{
  Trace trace("IRGenerator::expand_object_like_macro", Trace::TRANSLATION);
  ++macro_level_counter_;
  if (mask_counter_) return;

  position_ = macrocall.get_position();
  Token::string_type tmp = macrocall.get_value();
  current_macro_name_.assign(tmp.begin(), tmp.end());
  current_macro_call_length_ = current_macro_name_.size();
}
 
void IRGenerator::expanded_macro(Container const &result)
{
  Trace trace("IRGenerator::expand_macro", Trace::TRANSLATION);
}
 
void IRGenerator::rescanned_macro(Container const &result)
{
  Trace trace("IRGenerator::rescanned_macro", Trace::TRANSLATION);
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
    bpl::dict mmap = bpl::extract<bpl::dict>(file_stack_.top().attr("macro_calls"));
    bpl::list line = bpl::extract<bpl::list>(mmap.get(position_.get_line() - 1, bpl::list()));
    line.append(sf_module_.attr("MacroCall")(current_macro_name_,
                                             begin, end,
                                             current_macro_call_length_ - length));
  }
}

void IRGenerator::found_include_directive(std::string const &filename, bool next)
{
  Trace trace("IRGenerator::found_include_directive", Trace::TRANSLATION);
  trace << filename;

  include_dir_ = filename;
  include_next_dir_ = next;
}

void IRGenerator::opened_include_file(std::string const &relname, 
                                      std::string const &absname, 
                                      std::size_t include_depth,
                                      bool is_system_include)
{
  Trace trace("IRGenerator::opened_include_file", Trace::TRANSLATION);
  trace << absname;
  if (mask_counter_)
  {
    ++mask_counter_;
    return;
  }
  bpl::object source_file = lookup_source_file(relname, false);

  bpl::object include = sf_module_.attr("Include")(source_file,
                                                   include_dir_,
                                                   false,
                                                   include_next_dir_);
  bpl::list includes = bpl::extract<bpl::list>(file_stack_.top().attr("includes"));
  includes.append(include);
  file_stack_.push(source_file);

  std::string abs_filename = make_full_path(relname);
  // Only keep the first level of includes starting from the last unmasked file.
  if (primary_file_only_ || !matches_path(abs_filename, base_path_))
    ++mask_counter_;
  else
    source_file.attr("annotations")["primary"] = true;
}

void IRGenerator::returning_from_include_file()
{
  Trace trace("IRGenerator::returning_from_include_file", Trace::TRANSLATION);
  if (mask_counter_ < 2) file_stack_.pop();
  // if the file was masked, decrement the counter
  if (mask_counter_) --mask_counter_;
}

void IRGenerator::defined_macro(Token const &name, bool is_functionlike,
                                std::vector<Token> const &parameters,
                                Container const &definition,
                                bool is_predefined)
{
  Trace trace("IRGenerator::defined_macro", Trace::TRANSLATION);
  if (mask_counter_ || is_predefined) return;
  
  Token::string_type const &m = name.get_value();
  std::string macro_name(m.begin(), m.end());
  Token::position_type position = name.get_position();

  std::string text;
  {
    Token::string_type tmp = wave::util::impl::as_string(definition);
    text = std::string(tmp.begin(), tmp.end());
  }
  bpl::list params;
  for (std::vector<Token>::const_iterator i = parameters.begin();
       i != parameters.end(); ++i) 
  {
    Token::string_type const &tmp = i->get_value();
    params.append(std::string(tmp.begin(), tmp.end()));
  }

  bpl::tuple qname = bpl::make_tuple(macro_name);
  bpl::object macro = asg_module_.attr("Macro")(file_stack_.top(),
                                                position.get_line(),
                                                "macro",
                                                qname,
                                                params,
                                                text);
  bpl::object declared = asg_module_.attr("Declared")(language_, qname, macro);
  declarations_.append(macro);
  types_[qname] = declared;
}

void IRGenerator::undefined_macro(Token const &name)
{
  Trace trace("IRGenerator::undefined_macro", Trace::TRANSLATION);
  trace << name;
}

bpl::object IRGenerator::lookup_source_file(std::string const &filename,
                                            bool primary)
{
  Trace trace("IRGenerator::lookup_source_file", Trace::TRANSLATION);
  trace << filename << ' ' << primary;

  std::string long_name = make_full_path(filename);
  std::string short_name = make_short_path(filename, base_path_);
  bpl::object source_file = files_.get(short_name);
  if (!source_file)
  {
    source_file = sf_module_.attr("SourceFile")(short_name, long_name, language_);
    files_[short_name] = source_file;
  }
  if (source_file && primary)
  {
    bpl::dict annotations = bpl::extract<bpl::dict>(source_file.attr("annotations"));
    annotations["primary"] = true;
  }
  return source_file;
}
