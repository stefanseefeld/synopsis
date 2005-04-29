//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/Trace.hh>
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/preprocessing_hooks.hpp>
#include <stack>

using namespace Synopsis;
namespace wave = boost::wave;

typedef wave::cpplexer::lex_token<> Token;

class ASTTranslator : public wave::context_policies::default_preprocessing_hooks
{
public:
  typedef std::list<Token, boost::fast_pool_allocator<Token> > Container;

  ASTTranslator(std::string const &filename,
		std::string const &base_path, bool main_file_only,
		AST::AST a, bool v, bool d);

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
  {}

  // macro definition hooks
  template <typename ParametersT, typename DefinitionT>
  void defined_macro(Token const &name, bool is_functionlike,
		     ParametersT const &parameters,
		     DefinitionT const &definition,
		     bool is_predefined)
  {}

  template <typename StringT>
  void undefined_macro(StringT const &name)
  {}

private:
  typedef std::stack<AST::SourceFile> FileStack;

  //. Look up the given filename in the ast, creating it if necessary.
  //. Mark the file as 'main' if so required.
  AST::SourceFile lookup_source_file(std::string const &filename, bool main);

  AST::AST            my_ast;
  AST::ASTKit         my_ast_kit;
  AST::SourceFile     my_file;
  std::string         my_raw_filename;
  std::string         my_base_path;
  FileStack           my_file_stack;
  bool                my_main_file_only;
  unsigned int        my_mask_counter;
  bool                my_verbose;
  bool                my_debug;
};
