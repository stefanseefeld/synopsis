#include <iostream>
#include <Synopsis/Python/Interpreter.hh>
#include <Synopsis/Python/Module.hh>
#include <string>
#include <cstdio>
#include <iostream>
#include "Guard.hh"

using namespace Synopsis::Python;

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
  Module module = Module::import("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_string("print 'hello world'", Interpreter::FILE,
                                  global, local);
}

void test2()
{
  std::cout << "=== test2 ===" << std::endl;
  Interpreter interp;
  Module path = Module::import("os.path");
  std::string dir = Object::narrow<std::string>(path.attr("dirname")(Tuple(__FILE__)));
  std::string script = Object::narrow<std::string>(path.attr("join")(Tuple(dir, "greeting.py")));

  Module module = Module::import("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_file(script, Interpreter::FILE,
                                global, local);
}

void test3()
{
  std::cout << "=== test3 ===" << std::endl;
  Interpreter interp;
  Module path = Module::import("os.path");
  std::string dir = Object::narrow<std::string>(path.attr("dirname")(Tuple(__FILE__)));
  std::string script = Object::narrow<std::string>(path.attr("join")(Tuple(dir, "Command.py")));

  Module module = Module::import("__main__");
  Dict global = module.dict();
  Dict local;
  Object retn = interp.run_file(script, Interpreter::FILE,
                                global, local);
  Object type = local.get("Command");
  Tuple args("first", "second", "third");
  Dict kwds;
  kwds.set("input", "foo.h");
  kwds.set("output", "foo.i");
  retn = type();
  retn = type(args);
  retn = type(args, kwds);
  Object method = retn.attr("execute");
  method();
}

int main(int, char **)
{  
  try
  {
    test1();
    test2();
    test3();
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
