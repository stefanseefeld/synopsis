#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/AST/TypeKit.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  AST::ASTKit kit = AST::ASTKit();
  AST::TypeKit types = AST::TypeKit();
  AST::SourceFile sf = kit.create_source_file("filename", "/long/filename", "C++");
  AST::Declaration d = kit.create_declaration(sf, 2, "C--", "foo", Tuple("bar"));
  std::cout << d << std::endl;
  std::cout << "file='" << d.file() << "\'\n"
	    << "line='" << d.line() << "\'\n"
	    << "language='" << d.language() << "\'\n"
	    << "type='" << d.type() << "\'\n"
	    << "name='" << d.name() << "\'\n"
	    << "comments='" << d.comments() << std::endl;
  std::cout << kit.create_builtin(sf, 3, "C--", "eos", Tuple("eos")) << std::endl;
  std::cout << kit.create_macro(sf, 4, "C--", Tuple("ZAP"), List(), "") << std::endl;
  std::cout << kit.create_forward(sf, 5, "C--", "forward", Tuple("flip")) << std::endl;
  std::cout << kit.create_module(sf, 6, "C--", "namespace", Tuple("flop")) << std::endl;
  AST::Class c = kit.create_class(sf, 7, "C--", "class", Tuple("faz"));
  std::cout << c << std::endl;
  AST::Type t = types.create_declared("C--", Tuple("faz"), c);
  std::cout << kit.create_typedef(sf, 7, "C--", "class", Tuple("faz"), t, false) << std::endl;
}

int main(int, char **)
{
  try
  {
    test1();
  }
  catch (const std::exception &e)
  {
    std::cout << "Error : " << e.what() << std::endl;
  }
}

