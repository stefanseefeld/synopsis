//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/AST/TypeKit.hh>
#include <Synopsis/Trace.hh>

#define BOOST_WAVE_USE_DEPRECIATED_PREPROCESSING_HOOKS 1

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#include <boost/wave/preprocessing_hooks.hpp>
#include <stack>

using namespace Synopsis;
namespace wave = boost::wave;

typedef wave::cpplexer::lex_token<> Token;

class ASTTranslator : public wave::context_policies::default_preprocessing_hooks
{
public:
  typedef std::list<Token, boost::fast_pool_allocator<Token> > Container;

  ASTTranslator(std::string const &language,
		std::string const &filename,
		std::string const &base_path, bool primary_file_only,
		AST::AST a, bool v, bool d);

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

private:
  typedef std::stack<AST::SourceFile> FileStack;

  //. Look up the given filename in the ast, creating it if necessary.
  //. Mark the file as 'primary' if so required.
  AST::SourceFile lookup_source_file(std::string const &filename, bool primary);

  AST::AST             ast_;
  AST::ASTKit          ast_kit_;
  AST::SourceFileKit   sf_kit_;
  AST::TypeKit         type_kit_;
  AST::SourceFile      file_;
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
