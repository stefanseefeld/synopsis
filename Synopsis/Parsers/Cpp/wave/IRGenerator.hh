//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef IRGenerator_hh_
#define IRGenerator_hh_

#define BOOST_WAVE_USE_DEPRECIATED_PREPROCESSING_HOOKS 1

#include <boost/python.hpp>
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#include <boost/wave/preprocessing_hooks.hpp>
#include <stack>
#include <Synopsis/Trace.hh>

using namespace Synopsis;
namespace wave = boost::wave;
namespace bpl = boost::python;

typedef wave::cpplexer::lex_token<> Token;

class IRGenerator : public wave::context_policies::default_preprocessing_hooks
{
  typedef wave::context_policies::default_preprocessing_hooks base;
public:
  typedef std::list<Token, boost::fast_pool_allocator<Token> > Container;

  IRGenerator(std::string const &language,
              std::string const &filename,
              std::string const &base_path, bool primary_file_only,
              bpl::object ir, bool v, bool d);

//   bool may_skip_whitespace (Token &, bool &) { return false;}

  void expanding_function_like_macro(Token const &macrodef, 
				     std::vector<Token> const &formal_args, 
				     Container const &definition,
				     Token const &macrocall,
				     std::vector<Container> const &arguments);
 
  void expanding_object_like_macro(Token const &macro, 
				   Container const &definition,
				   Token const &macrocall);
 
  void expanded_macro(Container const &result);
 
  void rescanned_macro(Container const &result);

  void found_include_directive(std::string const &filename, bool);

  void opened_include_file(std::string const &dir, 
			   std::string const &filename, 
			   std::size_t include_depth,
			   bool is_system_include);

  void returning_from_include_file();

  // interpretation of #pragma's of the form 
  // 'wave option[(value)]'
  template <typename ContextT>
  bool interpret_pragma(ContextT const &ctx, Container &pending, 
			typename ContextT::token_type const &option, 
			Container const &values, 
			typename ContextT::token_type const &pragma_token)
  { return false;}

  // macro definition hooks
  void defined_macro(Token const &name, bool is_functionlike,
		     std::vector<Token> const &parameters,
		     Container const &definition,
		     bool is_predefined);

  void undefined_macro(Token const &name);
#if BOOST_VERSION >= 103500
  template <typename ContextT, typename ExceptionT>
  void throw_exception(ContextT const &c, ExceptionT const &e)
  { base::throw_exception(c, e);}
  template <typename ContextT>
  void throw_exception(ContextT const &c, wave::macro_handling_exception const &e)
  {
    // Only throw in case of error.
    if (e.get_severity() != wave::util::severity_warning)
      base::throw_exception(c, e);
  }
#endif
private:
  typedef std::stack<bpl::object> FileStack;

  //. Look up the given filename in the ast, creating it if necessary.
  //. Mark the file as 'primary' if so required.
  bpl::object lookup_source_file(std::string const &filename, bool primary);

  std::string          language_;
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

#endif
