#include <Synopsis/Interpreter.hh>
#include <Synopsis/AST/ASTModule.hh>
#include <Synopsis/AST/SourceFile.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test2()
{
  Interpreter interp;
  Module module("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_file("SourceFile.py", Interpreter::FILE,
                                global, local);
  Callable type = local.get("SourceFile");
  
}

void test1()
{
  ASTModule module = Synopsis::ASTModule();
  SourceFile sf = module.create_source_file("filename", "/long/filename", "C++");
  std::cout << "created source file "
	    << sf.name() << ' '
	    << sf.long_name() << ' '
	    << sf.is_main() << std::endl;
}

int main(int, char **)
{  
  try
  {
    test2();
    test1();
  }
  catch (const Interpreter::Exception &)
  {
    PyErr_Print();
  }
  catch (const std::exception &e)
  {
    std::cout << "Error : " << e.what() << std::endl;
  }
}

