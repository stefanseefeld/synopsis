#include <iostream>
#include <Synopsis/Interpreter.hh>
#include <Synopsis/Callable.hh>
#include <Synopsis/Module.hh>
#include "Guard.hh"
#include <string>
#include <cstdio>
#include <iostream>

using namespace Synopsis;

void print(Dict &d)
{
  List keys = d.keys();
  for (int i = 0; i != keys.size(); ++i)
  {
     Object key = keys.get(i);
     std::cout << Object::narrow<const char *>(key) << ' '
               << Object::narrow<const char *>(d.get(key).str()) << std::endl;
  }
}

void test1()
{
  std::cout << "=== test1 ===" << std::endl;
  Interpreter interp;
  Module module("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_string("print 'hello world'", Interpreter::FILE,
                                  global, local);
}

void test2()
{
  std::cout << "=== test2 ===" << std::endl;
  Interpreter interp;
  Module module("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_file("script.py", Interpreter::FILE,
                                global, local);
  print(global);
  print(local);  
}

void test3()
{
  std::cout << "=== test3 ===" << std::endl;
  Interpreter interp;
  Module module("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_file("Command.py", Interpreter::FILE,
                                global, local);
  if (!retn) PyErr_Print();
  Object o = local.get("Command");
  if (!o) { throw std::runtime_error("missing object 'Command' in 'Command.py'");}
  Callable type = local.get("Command");
  Tuple args("first", "second", "third");
  Dict kwds;
  kwds.set("input", "foo.h");
  kwds.set("output", "foo.i");
  type.call();
  type.call(args);
  type.call(args, kwds);
}

int main(int, char **)
{  
  try
  {
    test1();
    test2();
    test3();
  }
  catch (const std::exception &e)
  {
    std::cout << "Error : " << e.what() << std::endl;
  }
}