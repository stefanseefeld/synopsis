//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef IRGenerator_hh_
#define IRGenerator_hh_

#include <boost/python.hpp>
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#include <boost/wave/preprocessing_hooks.hpp>
#include <stack>
#include <Synopsis/Trace.hh>
#include <Support/path.hh>

using namespace Synopsis;
namespace wave = boost::wave;
namespace bpl = boost::python;

class IRGenerator : public wave::context_policies::default_preprocessing_hooks
{
  typedef wave::context_policies::default_preprocessing_hooks base;
public:

  typedef wave::cpplexer::lex_token<> Token;

  typedef wave::context<std::string::iterator,
                        wave::cpplexer::lex_iterator<Token>,
                        wave::iteration_context_policies::load_file_to_string,
                        IRGenerator> Context;

  // FIXME: We can't use the following two typedefs since this triggers a compile-timer
  //        error due to the circular dependency between Context and IRGenerator (which GCC
  //        considers to be an 'incomplete type' until some later time during the parse).
//   typedef Context::lexer_type Iterator;
//   typedef Context::token_sequence_type Container;

  typedef std::list<Token, boost::fast_pool_allocator<Token> > Container;

  IRGenerator(std::string const &language,
              std::string const &filename,
              std::string const &base_path, bool primary_file_only,
              bpl::object ir, bool v, bool d)
    : language_(language),
      qname_module_(bpl::import("Synopsis.QualifiedName")),
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

//   bool may_skip_whitespace (Token &, bool &) { return false;}

  template <typename IteratorT>
  bool expanding_function_like_macro(Context const &ctx, Token const &macrodef, 
                                     std::vector<Token> const &formal_args, 
                                     Container const &definition, Token const &macrocall, 
                                     std::vector<Container> const &arguments,
                                     IteratorT const &seqstart,
                                     IteratorT const &seqend);
  bool expanding_object_like_macro(Context const& ctx, Token const &macrodef, 
                                   Container const &definition, Token const &macrocall);

  void expanded_macro(Context const &ctx, Container const &result);

  void rescanned_macro(Context const &ctx, Container const &result);

  bool found_include_directive(Context const &ctx, std::string const &filename, 
                               bool include_next);

  void opened_include_file(Context const &ctx, std::string const &relname, 
                           std::string const &absname, bool is_system_include);

  void returning_from_include_file(Context const &ctx);

  template <typename ExceptionT>
  void throw_exception(Context const &c, ExceptionT const &e);

  // interpretation of #pragma's of the form 
  // 'wave option[(value)]'
  bool interpret_pragma(Context const &ctx,
                        Container &pending, 
			Token const &option, 
			Container const &values, 
			Token const &token);

  void defined_macro(Context const &ctx,
                     Token const &name, bool is_functionlike,
		     std::vector<Token> const &parameters,
		     Container const &definition,
		     bool is_predefined);

  void undefined_macro(Context const &ctx, Token const &name);

private:
  typedef std::stack<bpl::object> FileStack;

  //. Look up the given filename in the ast, creating it if necessary.
  //. Mark the file as 'primary' if so required.
  bpl::object lookup_source_file(std::string const &filename, bool primary);

  std::string          language_;
  bpl::object          qname_module_;
  bpl::object          asg_module_;
  bpl::object          sf_module_;
  bpl::list            declarations_;
  bpl::dict            types_;
  bpl::dict            files_;
  bpl::object          file_;
  std::string          raw_filename_;
  Token::position_type position_;
  std::string          base_path_;
  FileStack            file_stack_;
  std::string          include_dir_;
  bool                 include_next_dir_;
  bool                 primary_file_only_;
  unsigned int         mask_counter_;
  unsigned int         macro_level_counter_;
  //. The name of the macro currently being expanded.
  std::string          current_macro_name_;
  //. The length of the character sequence representing the current macro call.
  unsigned int         current_macro_call_length_;
  bool                 verbose_;
  bool                 debug_;
};

template <typename IteratorT>
inline 
bool IRGenerator::expanding_function_like_macro(Context const &ctx,
                                                Token const &macrodef, 
                                                std::vector<Token> const &formal_args, 
                                                Container const &definition,
                                                Token const &macrocall, 
                                                std::vector<Container> const &arguments,
                                                IteratorT const &seqstart,
                                                IteratorT const &seqend)
{
  Trace trace("IRGenerator::expand_function_like_macro", Trace::TRANSLATION);
  ++macro_level_counter_;
  if (!mask_counter_)
  {
    position_ = macrocall.get_position();
    Token::string_type tmp = macrocall.get_value();
    current_macro_name_.assign(tmp.begin(), tmp.end());
    // We are looking for the column after the closing right paren,
    // which is the last token in the sequence.
    unsigned int end = seqend->get_position().get_column() + 1;
    current_macro_call_length_ = end - position_.get_column();
  }
  return false;
}

inline 
bool IRGenerator::expanding_object_like_macro(Context const& ctx,
                                              Token const &macrodef, 
                                              Container const &definition,
                                              Token const &macrocall)
{
  Trace trace("IRGenerator::expand_object_like_macro", Trace::TRANSLATION);
  ++macro_level_counter_;
  if (!mask_counter_)
  {  
    position_ = macrocall.get_position();
    Token::string_type tmp = macrocall.get_value();
    current_macro_name_.assign(tmp.begin(), tmp.end());
    current_macro_call_length_ = current_macro_name_.size();
  }
  return false;
}

inline
void IRGenerator::expanded_macro(Context const &ctx,
                                 Container const &result)
{
  Trace trace("IRGenerator::expand_macro", Trace::TRANSLATION);
}

inline 
void IRGenerator::rescanned_macro(Context const &ctx,
                                  Container const &result)
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
    unsigned int begin = result.front().get_position().get_column() - 1;
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
    bpl::list line = bpl::extract<bpl::list>(mmap.get(position_.get_line(), bpl::list()));
    line.append(sf_module_.attr("MacroCall")(current_macro_name_,
                                             begin, end,
                                             current_macro_call_length_ - length));
    mmap[position_.get_line()] = line;
  }
}

inline
bool IRGenerator::found_include_directive(Context const &ctx,
                                          std::string const &filename, 
                                          bool next)
{
  Trace trace("IRGenerator::found_include_directive", Trace::TRANSLATION);
  trace << filename;
    
  include_dir_ = filename;
  include_next_dir_ = next;
  return false;
}

inline
void IRGenerator::opened_include_file(Context const &ctx,
                                      std::string const &relname, 
                                      std::string const &absname,
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

inline
void IRGenerator::returning_from_include_file(Context const &ctx)
{
  Trace trace("IRGenerator::returning_from_include_file", Trace::TRANSLATION);
  if (mask_counter_ < 2) file_stack_.pop();
  // if the file was masked, decrement the counter
  if (mask_counter_) --mask_counter_;
}

template <typename ExceptionT>
inline
void IRGenerator::throw_exception(Context const &c, ExceptionT const &e)
{
  // Only throw in case of error.
  if (e.get_severity() != wave::util::severity_warning)
    base::throw_exception(c, e);
}

inline bool IRGenerator::interpret_pragma(Context const &ctx,
                                          Container &pending, 
                                          Token const &option, 
                                          Container const &values, 
                                          Token const &pragma_token)
{ return false;}

inline
void IRGenerator::defined_macro(Context const &ctx,
                                Token const &name, bool is_functionlike,
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
  
  bpl::object qname = qname_module_.attr("QualifiedCxxName")(bpl::make_tuple(macro_name));
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

inline
void IRGenerator::undefined_macro(Context const &ctx, Token const &name)
{
  Trace trace("IRGenerator::undefined_macro", Trace::TRANSLATION);
  trace << name;
}

inline
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

#endif
