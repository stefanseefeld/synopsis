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
  sf.is_main(true);
  std::cout << "created source file "
	    << sf.name() << ' '
	    << sf.long_name() << ' '
	    << sf.is_main() << std::endl;

  MacroCall mc = module.create_macro_call("FOO", 1, 2, 3);
  Dict mmap = sf.macro_calls();
  List line = mmap.get(0, List());
  line.append(mc);
  mmap.set(0, line);
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

