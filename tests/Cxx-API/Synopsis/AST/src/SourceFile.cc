#include <Synopsis/Python/Interpreter.hh>
#include <Synopsis/AST/ASTKit.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  AST::ASTKit kit = AST::ASTKit();
  AST::SourceFile sf = kit.create_source_file("filename", "/long/filename", "C++");
  sf.is_main(true);
  std::cout << "created source file "
	    << sf.name() << ' '
	    << sf.long_name() << ' '
	    << sf.is_main() << std::endl;
  AST::MacroCall mc = kit.create_macro_call("FOO", 1, 2, 3);
  Python::Dict mmap = sf.macro_calls();
  Python::List line = mmap.get(0, Python::List());
  line.append(mc);
  mmap.set(0, line);
  std::cout << sf.macro_calls().type() << std::endl;
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

