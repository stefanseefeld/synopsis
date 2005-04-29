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

ASTTranslator::ASTTranslator(std::string const &filename,
			     std::string const &base_path, bool main_file_only,
			     AST::AST ast, bool v, bool d)
  : my_ast(ast),
    my_ast_kit("C"),
    my_raw_filename(filename),
    my_base_path(base_path),
    my_main_file_only(main_file_only),
    my_mask_counter(0),
    my_verbose(v), my_debug(d) 
{
  Trace trace("ASTTranslator::ASTTranslator", Trace::TRANSLATION);
  my_file_stack.push(lookup_source_file(my_raw_filename, true));
}

void ASTTranslator::expanding_function_like_macro(Token const &macrodef, 
						  std::vector<Token> const &formal_args, 
						  Container const &definition,
						  Token const &macrocall,
						  std::vector<Container> const &arguments) 
{
  Trace trace("ASTTranslator::expand_function_like_macro", Trace::TRANSLATION);
  if (my_mask_counter) return;
  std::cout << macrocall.get_position() << ": "
	    << macrocall.get_value() << "(";

  // argument list
  for (Container::size_type i = 0; i < arguments.size(); ++i) 
  {
    std::cout << wave::util::impl::as_string(arguments[i]);
    if (i < arguments.size()-1)
      std::cout << ", ";
  }
  std::cout << ")" << std::endl; 
}
 
void ASTTranslator::expanding_object_like_macro(Token const &macro, 
						Container const &definition,
						Token const &macrocall)
{
  Trace trace("ASTTranslator::expand_object_like_macro", Trace::TRANSLATION);
  if (my_mask_counter) return;
  std::cout << macrocall.get_position() << ": "
	    << macrocall.get_value() << std::endl;
}
 
void ASTTranslator::expanded_macro(Container const &result)
{
  Trace trace("ASTTranslator::expand_macro", Trace::TRANSLATION);
  if (my_mask_counter) return;
  std::cout << wave::util::impl::as_string(result) << std::endl;
}
 
void ASTTranslator::rescanned_macro(Container const &result)
{
  Trace trace("ASTTranslator::rescanned_macro", Trace::TRANSLATION);
  std::cout << wave::util::impl::as_string(result) << std::endl;
}

void ASTTranslator::opened_include_file(std::string const &dir, 
					std::string const &filename, 
					std::size_t include_depth,
					bool is_system_include)
{
  Trace trace("ASTTranslator::opened_include_file", Trace::TRANSLATION);
  if (my_mask_counter)
  {
    ++my_mask_counter;
    return;
  }
  AST::SourceFile sf = lookup_source_file(filename, false);

  AST::Include include = my_ast_kit.create_include(sf, filename, false, false);
  Python::List includes = my_file_stack.top().includes();
  includes.append(include);
  my_file_stack.push(sf);

  std::string abs_filename = Path(filename).abs().str();
  // Only keep the first level of includes starting from the last unmasked file.
  if (my_main_file_only || 
      (my_base_path.size() && 
       abs_filename.substr(0, my_base_path.size()) != my_base_path))
    ++my_mask_counter;
}

void ASTTranslator::returning_from_include_file()
{
  Trace trace("ASTTranslator::returning_from_include_file", Trace::TRANSLATION);
  if (my_mask_counter < 2) my_file_stack.pop();
  // if the file was masked, decrement the counter
  if (my_mask_counter) --my_mask_counter;
}

AST::SourceFile ASTTranslator::lookup_source_file(std::string const &filename,
						  bool main)
{
  Path path = Path(filename).abs();
  path.strip(my_base_path);
  std::string short_name = path.str();

  Python::Dict files = my_ast.files();
  AST::SourceFile sf = files.get(short_name);
  if (!sf)
  {
    sf = my_ast_kit.create_source_file(short_name, filename);
    files.set(short_name, sf);
  }
  if (sf && main) sf.is_main(true);
  return sf;
}
