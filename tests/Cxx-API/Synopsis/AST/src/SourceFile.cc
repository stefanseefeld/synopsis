#include <Synopsis/Interpreter.hh>
#include <Synopsis/AST/ASTKit.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

char SEP = '/';

void test2()
{
  std::string scripts = __FILE__;
  scripts = scripts.substr(0, scripts.rfind(SEP, scripts.rfind(SEP) - 1) + 1);
  scripts += "scripts";
  scripts += SEP;
  
  Interpreter interp;
  Module module("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_file(scripts + "SourceFile.py", Interpreter::FILE,
				global, local);
  Object type = local.get("SourceFile");
  
}

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

